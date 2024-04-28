#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "../defines.h"
#include "arp.h"
#include "ip.h"

// TODO: Eliminate magic numbers

class EthernetFrame {

  private:
    static const uint16_t ipv4 = 0x0800;
    static const uint16_t ipv6 = 0x86DD;
    static const uint16_t vlan = 0x8100;
    static const uint16_t arp  = 0x0806;

  public:
    // recv
    EthernetFrame(std::vector<uint8_t>::const_iterator& frame_it, std::vector<uint8_t>::const_iterator& end_it,
                  connection_t& connection, connections_t& connections) {
        std::copy_n(frame_it, MAC_ADDRESS_LEN, std::get<conn_elem::dst_mac>(connection).begin());
        std::advance(frame_it, MAC_ADDRESS_LEN);
        std::copy_n(frame_it, MAC_ADDRESS_LEN, std::get<conn_elem::src_mac>(connection).begin());
        std::advance(frame_it, MAC_ADDRESS_LEN);

        std::get<conn_elem::ether_type>(connection) = (uint16_t)(*frame_it) << 8;
        std::advance(frame_it, 1);
        std::get<conn_elem::ether_type>(connection) |= *frame_it;
        std::advance(frame_it, 1);

        switch (std::get<conn_elem::ether_type>(connection)) {
        case vlan:
            break;
        case ipv4:
            IpPacket(frame_it, end_it, connection, connections);
            break;
        case arp:
            ArpPacket(frame_it, connection, connections);
            break;
        }
    }

    // send
    EthernetFrame(std::vector<uint8_t>& frame, const connection_t& connection, req_res_t& state) {
        // swap src and dst to respond to sender
        std::copy_n(std::get<conn_elem::src_mac>(connection).begin(), MAC_ADDRESS_LEN, back_inserter(frame));
        std::ranges::copy(std::array<uint8_t, MAC_ADDRESS_LEN>{MY_MAC_ADDR}, back_inserter(frame));
        frame.push_back(std::get<conn_elem::ether_type>(connection) >> 8);
        frame.push_back(std::get<conn_elem::ether_type>(connection) & 0xFF);

        switch (std::get<conn_elem::ether_type>(connection)) {
        case vlan:
            break;
        case ipv4:
            IpPacket(frame, connection, state);
            break;
        case arp:
            ArpPacket(frame, connection, state);
            break;
        }
    }
};

#endif
