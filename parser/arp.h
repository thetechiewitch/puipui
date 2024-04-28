#ifndef _ARP_H
#define _ARP_H

#include "../defines.h"

class ArpPacket {
  private:
    static const uint8_t arp_header_len = 6;
    const uint8_t hardware_type_hi = 0;
    const uint8_t hardware_type_lo = 1;
    const uint8_t protocol_type_hi = 0x08;
    const uint8_t protocol_type_lo = 0x00;
    const uint8_t hardware_len = 6;
    const uint8_t protocol_len = 4;
    const std::array<uint8_t, arp_header_len> header = {hardware_type_hi, hardware_type_lo, protocol_type_hi, protocol_type_lo, hardware_len, protocol_len};

  public:
    // recv
    ArpPacket(std::vector<uint8_t>::const_iterator& frame_it, connection_t& connection, connections_t& connections) {
        std::advance(frame_it, arp_header_len); // skip static arp header parts

        uint16_t operation = *frame_it >> 8;
        std::advance(frame_it, 1);
        operation |= *frame_it & 0xFF;
        std::advance(frame_it, 1);

        if (operation == 1) { // is request
            std::copy_n(frame_it, MAC_ADDRESS_LEN, std::get<conn_elem::src_mac>(connection).begin());
            std::advance(frame_it, MAC_ADDRESS_LEN);
            std::copy_n(frame_it, IP_ADDRESS_LEN, std::get<conn_elem::src_ip>(connection).begin());
            std::advance(frame_it, IP_ADDRESS_LEN);
            // "THA is ignored in arp requests"
            std::advance(frame_it, MAC_ADDRESS_LEN);
            std::copy_n(frame_it, IP_ADDRESS_LEN, std::get<conn_elem::dst_ip>(connection).begin());
            std::advance(frame_it, IP_ADDRESS_LEN);

            if (std::get<conn_elem::dst_ip>(connection) == std::array<uint8_t, IP_ADDRESS_LEN>{MY_STATIC_IP}) {
                connections[connection] = {};

                std::ranges::copy(header, back_inserter(connections[connection].response));
                std::ranges::copy(std::array<uint8_t, 2>{0x00, 0x02},
                                  back_inserter(connections[connection].response)); // OPER
                // swap sender and receiver...
                std::ranges::copy(std::array<uint8_t, MAC_ADDRESS_LEN>{MY_MAC_ADDR},
                                  back_inserter(connections[connection].response)); // SHA
                std::ranges::copy(std::array<uint8_t, IP_ADDRESS_LEN>{MY_STATIC_IP},
                                  back_inserter(connections[connection].response)); // SPA
                std::ranges::copy(std::array<uint8_t, MAC_ADDRESS_LEN>{std::get<conn_elem::src_mac>(connection)},
                                  back_inserter(connections[connection].response)); // THA
                std::ranges::copy(std::get<conn_elem::src_ip>(connection),
                                  back_inserter(connections[connection].response)); // TPA
            }
        } // else ignore replies
    }

    // send
    ArpPacket(std::vector<uint8_t>& frame, const connection_t& connection, req_res_t& state) {
        std::ranges::copy(state.response, back_inserter(frame));
        state.is_sent = true;
    }
};

#endif
