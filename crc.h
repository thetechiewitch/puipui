#ifndef _CRC_H_
#define _CRC_H_

#include <vector>
uint16_t crc16_mcrf4xx(uint16_t crc, std::vector<uint8_t>& data);
uint16_t crc16_ip(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

#endif
