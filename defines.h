#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <algorithm>
#include <array>
#include <ranges>
#include <tuple>
#include <vector>

#include "iface_config.h"

#define MAC_ADDRESS_LEN 6
#define IP_ADDRESS_LEN 4

enum conn_elem { dst_mac = 0, src_mac, dst_ip, src_ip, dst_port, src_port, ether_type, protocol };

// TODO: struct with designated init
using connection_t = std::tuple<
    std::array<uint8_t, MAC_ADDRESS_LEN>,
    std::array<uint8_t, MAC_ADDRESS_LEN>, 
    std::array<uint8_t, IP_ADDRESS_LEN>, 
    std::array<uint8_t, IP_ADDRESS_LEN>,
    uint16_t,
    uint16_t,
    uint16_t,
    uint8_t>;

using req_res_t = struct _ {
    std::vector<uint8_t> request = {};
    std::vector<uint8_t> response = {};
    bool is_sent = false;
};

using connections_t = std::map<connection_t, req_res_t>;

#endif
