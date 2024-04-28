#include "crc.h"
#include <iostream>
#include <stddef.h>
#include <stdint.h>

uint16_t crc16_mcrf4xx(uint16_t crc, std::vector<uint8_t>& data) {
    uint8_t T;
    uint8_t L;

    if (data.size() <= 0)
        return crc;

    // derived from https://gist.github.com/aurelj/270bb8af82f65fa645c1
    for (auto&& d : data) {
        crc = crc ^ d;
        L = crc ^ (crc << 4);
        T = (L << 3) | (L >> 5);
        L ^= (T & 0x07);
        T = (T & 0xF8) ^ (((T << 1) | (T >> 7)) & 0x0F) ^ (crc >> 8);
        crc = (L << 8) | T;
    }
    return crc;
}

uint16_t crc16_ip(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end) {
    auto odd = (end - begin) & 1;
    uint32_t sum{0};

    for (auto i = begin; i < (end - odd); i += 2) {
        sum += ((*i) << 8) | (*(i + 1));
    }
    sum += (*(end - 1)) * odd;

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}
