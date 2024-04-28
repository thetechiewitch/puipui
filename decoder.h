#ifndef _DECODER_H_
#define _DECODER_H_

#include <cstdint>
#include <vector>

// TODO: create define.h
#define BSSID_LEN 6
#define MAC_ADDRESS_LEN 6

#define LOG_TYPE 0x83
#define STATUS_TYPE 0x80
#define BOOT_TYPE 0x81
#define SCAN_REPLY 0x23
#define SCAN_ENTRY 0x24
#define ETH_FRAME_TYPE 0x01

constexpr auto READ_LIMIT{5};

// TODO: check message validity
// TODO: tostring method
// TODO: change names of all classes: for ex LogDecoder instead DecodeLog
class Decoder {
  public:
    Decoder() : msg_type{0} {}
    const int get_msg_type() { return msg_type; }
    virtual void decode(std::vector<uint8_t>& msg) = 0;
    virtual bool is_response_ok() { return false; }
    virtual int get_msg_read_limit() { return READ_LIMIT; }
    virtual int get_msg_read_delay() { return READ_LIMIT; }

  protected:
    Decoder(int type) : msg_type(type) {}

  private:
    const int msg_type;
};

class DecodeStatus : public Decoder {
  public:
    uint8_t status;
    DecodeStatus() : Decoder(STATUS_TYPE) {}
    DecodeStatus(std::vector<uint8_t>& msg) : DecodeStatus() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) { status = msg[0]; }

    bool is_response_ok() { return !status; }
};

class DecodeLog : public Decoder {
  public:
    uint8_t level;
    std::string m_message;
    DecodeLog() : Decoder(LOG_TYPE) {}
    DecodeLog(std::vector<uint8_t>& msg) : DecodeLog() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) {
        level = msg[0];
        std::copy(msg.begin() + 1, msg.end(), std::back_inserter(m_message));
    }
};

class DecodeWifiScanReply : public Decoder {
  public:
    uint8_t status;
    uint16_t num_entries;
    DecodeWifiScanReply() : Decoder(0x23) {}
    DecodeWifiScanReply(std::vector<uint8_t>& msg) : DecodeWifiScanReply() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) {
        status = msg[0];
        num_entries = msg[1] | (msg[2] << 8);
    }

    bool is_response_ok() { return !status; }

    int get_msg_read_limit() { return 50; }
    int get_msg_read_delay() { return 100; }
};

// TODO: add iterator
class DecodeWifiScanEntry : public Decoder {
  public:
    uint16_t index;
    std::string ssid;
    std::array<uint8_t, BSSID_LEN> bssid;
    uint8_t channel;
    uint8_t auth_mode;
    int16_t rssi;
    uint8_t is_hidden;

    DecodeWifiScanEntry() : Decoder(0x24) {}
    DecodeWifiScanEntry(std::vector<uint8_t>& msg) : DecodeWifiScanEntry() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) {
        ssid.clear();
        index = msg[0] | (msg[1] << 8);
        uint8_t ssid_len = msg[2];
        std::copy_n(msg.begin() + 3, ssid_len, std::back_inserter(ssid));
        std::copy_n(msg.begin() + 35, BSSID_LEN, bssid.begin());
        channel = msg[41];
        auth_mode = msg[42];
        rssi = msg[43] | (msg[44] << 8);
        is_hidden = msg[45];

        std::cout << "index: " << index << " ssid: " << ssid << std::endl;
    }

    bool is_response_ok() { return true; }
};

class Boot : public Decoder {
  public:
    Boot() : Decoder(BOOT_TYPE) {}
    void decode(std::vector<uint8_t>& msg) {}
};

class EchoReply : public Decoder {
  public:
    int msg_type;
    std::vector<uint8_t> m_msg;

    EchoReply() : Decoder(0x85) {}
    EchoReply(std::vector<uint8_t>& msg) : EchoReply() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) {
        m_msg = msg;
        // std::cout << msg_type << std::endl;
    }

    bool is_response_ok() { return true; }
};

class StationConnectionStatusReply : public Decoder {
  public:
    uint8_t status;

    StationConnectionStatusReply() : Decoder(0x46) {}
    StationConnectionStatusReply(std::vector<uint8_t>& msg) : StationConnectionStatusReply() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) { status = msg[0]; }
};

class WifiMacAddressReply : public Decoder {
  public:
    std::array<uint8_t, MAC_ADDRESS_LEN> mac_address;

    WifiMacAddressReply() : Decoder(0x26) {}
    WifiMacAddressReply(std::vector<uint8_t>& msg) : WifiMacAddressReply() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) { std::copy_n(msg.begin(), MAC_ADDRESS_LEN, mac_address.begin()); }
};

class EthernetFrameReceive : public Decoder {
  public:
    std::vector<uint8_t>::const_iterator frame_it;
    std::vector<uint8_t>::const_iterator end_it;

    EthernetFrameReceive() : Decoder(0x01) {}
    EthernetFrameReceive(std::vector<uint8_t>& msg) : EthernetFrameReceive() { decode(msg); }

    void decode(std::vector<uint8_t>& msg) {
        std::cout << "msg len: " << msg.size() << std::endl;
        frame_it = msg.begin();
        end_it = msg.end();
    }
};

#endif
