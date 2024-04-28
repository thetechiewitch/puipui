#include "helper.h"

bool validate_crc(std::vector<uint8_t>::const_iterator begin_it, std::vector<uint8_t>::const_iterator end_it) {
    uint16_t sum = 0;
    while (begin_it != end_it) {

        uint16_t data = (uint16_t)(*begin_it) << 8;
        std::advance(begin_it, 1);
        data |= *begin_it;
        std::advance(begin_it, 1);

        sum += data;
    }
    return sum == 0xFFFF;
}
