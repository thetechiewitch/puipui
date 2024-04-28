#ifndef _COBS_H_
#define _COBS_H_
#include <vector>

void cobs_decode(std::vector<uint8_t>& vec);
void cobs_encode(std::vector<uint8_t>& vec);

#endif
