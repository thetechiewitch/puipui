#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "cobs.h"
#include "crc.h"
#include "decoder.h"
#include "defines.h"
#include "message.h"
#include "serial.h"

#include "parser/ethernet.h"

#include "wifilogin.h"

#define STATE_MSG 0
#define STATE_DECODER 1

using state_t = struct {
    Message* msg;
    Decoder* dec;
};

int esp_reset() {

    if (serial_CTSbit(0)) {
        return -1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (serial_CTSbit(1)) {
        return -1;
    }
    return 0;
}

int msg_write(const Message* msg) {

    std::vector<uint8_t> frame(msg->m_msg);
    int total_bytes_written = 0;

    std::cout << "frame: " << std::hex;
    for (auto&& byte : msg->m_msg) {
        std::cout << (uint32_t)byte << " ";
    }
    std::cout << std::dec << std::endl;

    uint16_t crc = crc16_mcrf4xx(0xFFFF, frame);
    frame.push_back(crc & 0xFF);
    frame.push_back((crc >> 8) & 0xFF);

    cobs_encode(frame);

    // TODO: handle cases where bytes_written is zero or negative
    while (total_bytes_written < frame.size()) {
        int bytes_written = serial_write(frame, total_bytes_written);
        if (bytes_written <= 0) {
            return -1;
        } else {
            total_bytes_written += bytes_written;
        }
    }

    return 0;
}

// TODO: IMPORTANT: OS serial read buffer length is fixed at 4096 chars.
//                  Meaning, if our processing is too slow, this buffer could
//                  overflow.
std::vector<uint8_t> msg_read(int limit = 50, int delay = 100) {

    static std::vector<uint8_t> partial_msg{};
    std::vector<uint8_t> msg(partial_msg);

    while (true) {
        auto msg_end_it = find(msg.begin(), msg.end(), 0);
        if (msg_end_it == std::end(msg)) {

            int bytes_read = serial_read(msg);
            if (bytes_read <= 0) {
                if (bytes_read < 0 || --limit == 0) {
                    std::cout << "message clear \n";
                    partial_msg.clear();
                    return {};
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                continue;
            }
        } else {
            partial_msg.clear();
            if (msg_end_it + 1 != std::end(msg)) {
                std::copy(msg_end_it + 1, msg.end(), back_inserter(partial_msg));
                msg.resize(std::distance(msg.begin(), msg_end_it) + 1);
            }
            break;
        }
    }
    cobs_decode(msg);

    if (crc16_mcrf4xx(0xFFFF, msg) != 0) {
        std::cout << "crc kaputt \n";
        return {};
    }

    // delete crc from msg
    msg.pop_back();
    msg.pop_back();

    std::vector<uint8_t> msg_out;
    std::copy(msg.begin(), msg.end(), back_inserter(msg_out));
    return msg_out;
}

int setup(std::vector<state_t>& states) {
    bool do_write = true;
    uint16_t scan_entries = 0;

    for (int state = 0; state < states.size();) {
        std::cout << std::endl;

        if (do_write) {
            msg_write(states[state].msg);
        }
        do_write = true;

        Decoder* dec = states[state].dec;
        if (dec == nullptr) {
            state += 1;
            continue;
        }

        // TODO: IMPORTANT msg_read(50, 100) for WifiScanReply wait
        std::vector<uint8_t> response = msg_read(dec->get_msg_read_limit(), dec->get_msg_read_delay());
        if (response.empty()) {
            std::cout << "response empty" << std::endl;
            return -1;
        }

        uint8_t msg_type = response[0];
        response.erase(response.begin());

        switch (msg_type) {
        case ETH_FRAME_TYPE:
            std::cout << "ethernet frame ignored" << std::endl;
            do_write = false;
            continue;

        case LOG_TYPE:
            std::cout << "log: \"" << DecodeLog(response).message << "\"" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            do_write = false;
            continue;

        case BOOT_TYPE:
            return -2;

        default:
            do_write = false;

            if (msg_type == dec->get_msg_type()) {
                std::cout << "Decoder: " << typeid(*dec).name() << std::endl;

                dec->decode(response);
                if (dec->is_response_ok()) {
                    if (msg_type == SCAN_REPLY) {
                        scan_entries = ((DecodeWifiScanReply*)dec)->num_entries;
                        state += 1;
                    } else if (msg_type == SCAN_ENTRY) {
                        std::cout << "scan entry: " << scan_entries << std::endl;
                        if (--scan_entries == 0) {
                            std::cout << "scan entries done" << std::endl;
                            state += 1;
                            do_write = true;
                        }
                    } else { // all other msg_types
                        state += 1;
                        do_write = true;
                    }
                } else {
                    return -3;
                }
            } else if (msg_type == STATUS_TYPE) {
                DecodeStatus status(response);
                if (!status.is_response_ok()) {
                    std::cout << "error status: " << (uint32_t)status.status << std::endl;
                    return -4;
                } else {
                    std::cout << "ok status: " << (uint32_t)status.status << std::endl;
                }
            } else {
                return -5;
            }
            break;
        }
    }
    return 0;
}

int run_mode() {
    std::cout << "run_mode\n";
    std::cout << std::hex;

    std::map<connection_t, req_res_t> connections;
    while (true) {
        std::vector<uint8_t> frame = msg_read();
        if (frame.empty()) {
            std::cout << "frame empty" << std::endl;
            continue;
        }

        uint8_t msg_type = frame[0];
        frame.erase(frame.begin());

        switch (msg_type) {
        case ETH_FRAME_TYPE: {
            connection_t connection;

            auto efr = EthernetFrameReceive(frame);
            EthernetFrame(efr.frame_it, efr.end_it, connection, connections);

            for (auto& [key, value] : connections) {
                if (!value.response.empty()) {

                    std::vector<uint8_t> frame;
                    EthernetFrame(frame, key, value);

                    auto efs = EthernetFrameSend(frame);
                    msg_write(&efs);
                }
            }
            std::erase_if(connections, [](const auto& item) { return item.second.is_sent; });
            break;
        }

        case LOG_TYPE:
            std::cout << "log: \"" << DecodeLog(frame).message << "\"" << std::endl;
            break;

        default:
            return -1;
        }
    }
    return 0;
}


std::vector<state_t> states;

// TODO: erase Message type from message after switch case !!!!!
int main(int argc, char** argv) {
    std::string port = "/dev/tty.usbserial-A800H8A4";

    serial_init(port);

    // TODO: Echoreply add argument to constructor to compare against actual
    // received
    // TODO: same for decodestatus
    if (argc > 1) {
        StationDisconnect sd;
        msg_write(&sd);

        while (true) {
            std::vector<uint8_t> resp = msg_read();

            std::cout << "msg_read: " << std::hex;
            for (auto&& byte : resp) {
                std::cout << " " << (uint32_t)byte;
            }
            std::cout << std::dec << std::endl;
        }
        serial_close();
        return 0;
    }
    EchoRequest req("hello foo");
    EchoReply repl;
    LogLevel log_level(0x10);
    DecodeStatus decode_status;
    ForwardingMode forwarding_mode(2);
    WifiSleepMode wifi_sleep_mode(0);
    WifiMode wifi_mode(1);
    WifiScanRequest wifi_scan_req(WIFI_NAME, nullptr, 0, false);
    DecodeWifiScanReply decode_wifi_scan_repl;
    DecodeWifiScanEntry decode_wifi_scan_entry;
    StationConfig station_config(WIFI_NAME, WIFI_PASSWORD);

    states.push_back({&req, &repl});
    states.push_back({&log_level, &decode_status});
    states.push_back({&forwarding_mode, &decode_status});
    states.push_back({&wifi_sleep_mode, &decode_status});
    states.push_back({&wifi_mode, &decode_status});
    // states.push_back({&wifi_scan_req, &decode_wifi_scan_repl});
    // states.push_back({nullptr, &decode_wifi_scan_entry});
    // states.push_back({&req, &repl});
    states.push_back({&station_config, &decode_status});

    std::cout << "setup: " << setup(states) << std::endl;

    // WifiMacAddrRequest mac;
    // msg_write(&mac);
    // auto response = msg_read();
    //    response.erase(response.begin());
    // WifiMacAddressReply wmar(response);
    //
    // for(auto&& mac: wmar.mac_address)
    //     std::cout << (uint32_t)mac << ":";
    // std::cout << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(10));

    run_mode();

    serial_close();
    return 0;
}

