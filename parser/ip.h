#ifndef _IP_H
#define _IP_H

#include "../crc.h"
#include "../defines.h"
#include "helper.h"
#include "icmp.h"

#define IP_V_IHL_LEN 1
#define IP_DSCP_ECN_LEN 1
#define IP_TOTL_LEN 2
#define IP_ID_LEN 2
#define IP_FL_FRAGO_LEN 2
#define IP_TTL_LEN 1
#define IP_PROTO_LEN 1
#define IP_HEAD_CRC_LEN 2

#define VERSION(v_ihl) (v_ihl >> 4)
#define IHL(v_ihl) (v_ihl & 0x0F)
#define OPTIONS(ihl) ((ihl - 5) * 4)
#define FRAG_OFFSET(fl_fo) (fl_fo & 0x1FFF)
#define FLAG_BIT0_RESERVED(fl_fo) (fl_fo & 1)
#define FLAG_BIT1_DF(fl_fo) (fl_fo & 2)
#define FLAG_BIT2_MF(fl_fo) (fl_fo & 4)

// TODO: Fragmented IP
// TODO: Enable ECN

class IpPacket {
  private:
    const uint8_t version_ihl = 0x45;
    const uint8_t dscp_ecn = 0x0;
    uint16_t total_len;

    static const uint8_t tcp = 0x06;
    static const uint8_t udp = 0x11;
    static const uint8_t icmp = 0x01;

  public:
    // recv
    IpPacket(std::vector<uint8_t>::const_iterator& frame_it, std::vector<uint8_t>::const_iterator& end_it,
             connection_t& connection, connections_t& connections) {
        auto begin_it = frame_it;

        if (VERSION(*frame_it) != 4) {
            std::cout << "version is wrong\n";
            return;
        }

        uint8_t options_len = OPTIONS(IHL(*frame_it));
        std::advance(frame_it, IP_V_IHL_LEN);
        std::advance(frame_it, IP_DSCP_ECN_LEN); // TOS
        total_len = (uint16_t)(*frame_it) << 8;
        std::advance(frame_it, 1);
        total_len |= *frame_it;
        if (total_len != std::distance(begin_it, end_it)) {
            std::cout << "total_len is wrong\n";
            return;
        }
        std::advance(frame_it, 1);
        // ignore id
        std::advance(frame_it, IP_ID_LEN);
        std::advance(frame_it, IP_FL_FRAGO_LEN);

        // TTL 0
        if (*frame_it == 0) {
            std::cout << "ttl is wrong\n";
            return;
        }

        std::advance(frame_it, IP_TTL_LEN);
        uint8_t protocol = *frame_it;
        // save type of protocol in connection
        std::get<conn_elem::protocol>(connection) = protocol;

        std::advance(frame_it, IP_PROTO_LEN);
        std::advance(frame_it, IP_HEAD_CRC_LEN);

        std::copy_n(frame_it, IP_ADDRESS_LEN, std::get<conn_elem::src_ip>(connection).begin());
        std::advance(frame_it, IP_ADDRESS_LEN);
        std::copy_n(frame_it, IP_ADDRESS_LEN, std::get<conn_elem::dst_ip>(connection).begin());
        std::advance(frame_it, IP_ADDRESS_LEN);

        std::advance(frame_it, options_len);

        if (auto const a = validate_crc(begin_it, frame_it); a) {
            std::cout << a << ":  crc is wrong\n";
            return;
        }

        switch (protocol) {
        case tcp:
            break;
        case udp:
            break;
        case icmp:
            IcmpPacket(frame_it, end_it, connection, connections);
            break;
        }
    }

    // send
    IpPacket(std::vector<uint8_t>& frame, const connection_t& connection, req_res_t& state) {
        size_t frame_length = frame.size();

        frame.push_back(version_ihl);
        // ecn disabled
        frame.push_back(0);
        // total length
        auto total_len_index = frame.size();
        frame.push_back(0);
        frame.push_back(0);
        // identification
        frame.push_back(0);
        frame.push_back(0);
        // flag/fragment offset
        frame.push_back(0);
        frame.push_back(0);
        // TTL
        frame.push_back(64);
        // protocol
        frame.push_back(std::get<conn_elem::protocol>(connection));
        // crc
        auto crc_index = frame.size();
        frame.push_back(0);
        frame.push_back(0);

        std::ranges::copy(std::array<uint8_t, IP_ADDRESS_LEN>{MY_STATIC_IP}, back_inserter(frame));
        std::copy_n(std::get<conn_elem::src_ip>(connection).begin(), IP_ADDRESS_LEN, back_inserter(frame));

        size_t ip_header_end_index = frame.size();

        switch (std::get<conn_elem::protocol>(connection)) {
        case icmp:
            IcmpPacket(frame, connection, state);
            break;
        case tcp:
            break;
        case udp:
            break;
        }

        // total length stelle finden und setzen und wert setzen
        frame[total_len_index] = (frame.size() - frame_length) >> 8;
        frame[total_len_index + 1] = (frame.size() - frame_length) & 0xFF;

        auto crc = crc16_ip(frame.begin() + frame_length, frame.begin() + ip_header_end_index);

        frame[crc_index] = crc >> 8;
        frame[crc_index + 1] = crc & 0xFF;
    }
};

#endif
