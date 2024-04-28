#include "cobs.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

void cobs_decode(std::vector<uint8_t>& vec) {
    bool erase = true;

    for (auto it = vec.begin(); *it;) {
        auto distance = *it;
        auto adjusted_distance = distance;

        if (erase) {
            erase = false;
            it = vec.erase(it);
            adjusted_distance = distance - 1;
        } else {
            *it = 0;
        }
        std::ranges::advance(it, adjusted_distance, vec.end());
        if (it == vec.end()) {
            std::cerr << "error decoding\n";
            break;
        }
        erase = (distance == 0xff);
    }
    vec.pop_back();
}

void cobs_encode(std::vector<uint8_t>& vec) {
    auto target_it = vec.insert(vec.begin(), 1);
    vec.insert(vec.end(), 0);
    uint8_t distance = 0;

    for (auto it = vec.begin(); it != vec.end(); it++) {
        if (distance == 0xff) {
            *target_it = distance;
            it = vec.insert(it, 1);
            target_it = it;
            distance = 0;
        }
        if (*it == 0) {
            *target_it = distance;
            target_it = it;
            distance = 0;
        }
        distance++;
    }
}
