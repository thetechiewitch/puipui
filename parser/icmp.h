#ifndef _ICMP_H
#define _ICMP_H

#include "../defines.h"
#include "helper.h"

class IcmpPacket {

  public:
    // receive
    IcmpPacket(std::vector<uint8_t>::const_iterator& frame_it, std::vector<uint8_t>::const_iterator& end_it,
               connection_t& connection, connections_t& connections) {
        if (*frame_it != 8) {
            return;
        }
        if (auto const a = *std::next(frame_it); a != 0) {
            return;
        }
        if (auto const a = validate_crc(frame_it, end_it); a) {
            return;
        }
        std::advance(frame_it, 4);
        std::copy(frame_it, end_it, std::back_inserter(connections[connection].response));
    }

    // send
    IcmpPacket(std::vector<uint8_t>& frame, const connection_t& connection, req_res_t& state) {
        const auto icmp_start_index = frame.size();
        frame.push_back(0);
        frame.push_back(0);
        auto crc_index = frame.size();
        frame.push_back(0);
        frame.push_back(0);
        std::ranges::copy(state.response, back_inserter(frame));
        auto crc = crc16_ip(frame.begin() + icmp_start_index, frame.end());

        frame[crc_index] = crc >> 8;
        frame[crc_index + 1] = crc & 0xFF;

        state.is_sent = true;
    }
};

#endif
