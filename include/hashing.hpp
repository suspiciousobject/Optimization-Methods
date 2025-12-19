#ifndef HASHING_HPP
#define HASHING_HPP

#include <cstdint>
#include "types.hpp"

using namespace std;

inline uint64_t fast_hash_state(const Columns& cols) {
    static const uint64_t prime = 0x100000001B3ULL;
    uint64_t hash = 14695981039346656037ULL;
    
    for (const auto& col : cols) {
        for (char c : col) {
            hash ^= static_cast<uint64_t>(c);
            hash *= prime;
        }
        hash ^= 0xFF;
        hash *= prime;
    }
    return hash;
}

#endif // HASHING_HPP
