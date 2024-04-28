#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "defines.h"

#define MAX_SSID_LEN 32
#define MAX_PASSWD_LEN 64
#define BSSID_LEN 6
#define MAX_DNS_CNT 3

// TODO : check for constructor parameter validity
class Message {
  public:
    std::vector<uint8_t> m_msg;

  protected:
    Message() {} // make one with one arugment, so simple children do not need
                 // their own + get, or initializer list?? with type
    int append_string(const std::string_view str, int max_len) {
        if (str.size() > max_len) {
            return -1;
        }
        m_msg.push_back(str.size());

        copy(str.begin(), str.end(), back_inserter(m_msg));
        m_msg.resize(m_msg.size() + (max_len - str.size()), 0x00);

        return 0;
    }

    void append_uint16(uint16_t value) {
        m_msg.push_back(value & 0xFF);
        m_msg.push_back((value >> 8) & 0xFF);
    }

    void append_uint32(uint32_t value) {
        append_uint16(value & 0xFFF);
        append_uint16((value >> 16) & 0xFFF);
    }
};

class StationDisconnect : public Message {
  public:
    StationDisconnect() { m_msg = {0x49}; }
};

class LogLevel : public Message {
  public:
    LogLevel(uint8_t log_level) { m_msg = {0x82, log_level}; }
};

class ForwardingMode : public Message {
  public:
    ForwardingMode(uint8_t mode) { m_msg = {0x11, mode}; }
};

class ForwardIPBroadcasts : public Message {
  public:
    ForwardIPBroadcasts(uint8_t forward) { m_msg = {0x10, forward}; }
};

class WifiSleepMode : public Message {
  public:
    WifiSleepMode(uint8_t sleep_type) { m_msg = {0x21, sleep_type}; }
};

class WifiMode : public Message {
  public:
    WifiMode(uint8_t mode) { m_msg = {0x20, mode}; }
};

class StationDhcpClientState : public Message {
  public:
    StationDhcpClientState(uint8_t enable) { m_msg = {0x42, enable}; }
};

class WifiScanRequest : public Message {
  public:
    WifiScanRequest(const std::string_view ssid, const std::array<uint8_t, BSSID_LEN>* bssid, uint8_t channel, bool show_hidden) {
        m_msg.push_back(0x22);

        if (append_string(ssid, MAX_SSID_LEN)) {
            m_msg.clear();
            return;
        }

        if (bssid == nullptr) {
            m_msg.push_back(0x00);
            m_msg.insert(m_msg.end(), BSSID_LEN, 0x00);
        } else {
            m_msg.push_back(0xFF);
            copy(bssid->begin(), bssid->end(), back_inserter(m_msg));
        }

        m_msg.push_back(channel);
        m_msg.push_back(show_hidden);
    }
};

class StationConfig : public Message {
  public:
    StationConfig(const std::string_view ssid, const std::string_view passwd) {
        m_msg.push_back(0x40);

        if (append_string(ssid, MAX_SSID_LEN)) {
            m_msg.clear();
            return;
        }
        if (append_string(passwd, MAX_PASSWD_LEN)) {
            m_msg.clear();
            return;
        }
    }
};

// TODO: define address and netmask in a class
class StationStaticIpConfig : public Message {
  public:
    StationStaticIpConfig(uint32_t address, uint32_t netmask, uint32_t gateway, const std::array<uint32_t, MAX_DNS_CNT>& dns) {
        m_msg.push_back(0x41);
        append_uint32(address);
        append_uint32(netmask);
        append_uint32(gateway);

        for (auto&& d : dns) {
            append_uint32(d);
        }
    }
};

// TODO: write template specialization that accept only vector and string
class EchoRequest : public Message {
  private:
    template <typename T> void _echo_request(T msg) {
        m_msg.push_back(0x84);
        copy(msg.begin(), msg.end(), back_inserter(m_msg));
    }

  public:
    EchoRequest(std::string msg) { _echo_request(msg); }

    EchoRequest(std::vector<uint8_t> msg) { _echo_request(msg); }
};

class StationConnectionStatusRequest : public Message {
  public:
    StationConnectionStatusRequest() { m_msg.push_back(0x45); }
};

class WifiMacAddrRequest : public Message {
  public:
    WifiMacAddrRequest() { m_msg.push_back(0x25); }
};

class EthernetFrameSend : public Message {
  public:
    EthernetFrameSend(const std::array<uint8_t, 6>& mac_dst, const std::array<uint8_t, 6> mac_src, uint16_t ethertype,
                      std::vector<uint8_t>& payload) {
        m_msg.push_back(0x01);
        copy(mac_dst.begin(), mac_dst.end(), back_inserter(m_msg));
        copy(mac_src.begin(), mac_src.end(), back_inserter(m_msg));
        m_msg.push_back((ethertype >> 8) & 0xFF);
        m_msg.push_back(ethertype & 0xFF);
        copy(payload.begin(), payload.end(), back_inserter(m_msg));
    }

    EthernetFrameSend(std::vector<uint8_t>& payload) {
        m_msg.push_back(0x01);
        copy(payload.begin(), payload.end(), back_inserter(m_msg));
    }
};

// TODO: payload class

#endif
