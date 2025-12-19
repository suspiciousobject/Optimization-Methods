#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include "types.hpp"
#include "logger.hpp"

using namespace std;

// ============================================================================
// ВАЛИДАЦИЯ ВХОДНЫХ ДАННЫХ
// ============================================================================

inline bool validate_data(const Columns& cols, int N, unordered_map<char, int>& freq, 
                   const string& filename) {
    for (const auto& col : cols) {
        if (col.empty()) continue;
        if (col.size() != N) {
            log_error("Column height mismatch: expected " + to_string(N) +
                      ", found column of size " + to_string(col.size()) + ".");
            return false;
        }
        for (char bird : col) {
            freq[bird]++;
        }
    }

    for (const auto& p : freq) {
        if (p.second % N != 0) {
            log_error("Letter '" + string(1, p.first) + "' appears " +
                      to_string(p.second) + " times, which is not divisible by N=" + 
                      to_string(N) + ".");
            return false;
        }
    }
    return true;
}

inline int calculate_required_L(const unordered_map<char, int>& freq, int N) {
    int L = 0;
    for (const auto& p : freq) {
        L += p.second / N;
    }
    return L;
}

#endif // VALIDATION_HPP
