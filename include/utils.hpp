#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include "types.hpp"

using namespace std;

// ============================================================================
// УТИЛИТЫ ПАРСИНГА И РАБОТЫ СО СТРОКАМИ
// ============================================================================

inline string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

inline vector<char> parse_column(const string& line) {
    vector<char> birds;
    stringstream ss(line);
    char bird;
    while (ss >> bird) {
        if (bird >= 'A' && bird <= 'Z') {
            birds.push_back(bird);
        }
    }
    return birds;
}

inline pair<Columns, int> parse_data(const vector<string>& data_lines) {
    Columns cols;
    int N = 0;
    for (const auto& line : data_lines) {
        string trimmed = trim(line);
        if (trimmed.empty() || trimmed == "/") continue;
        if (trimmed == "==") {
            cols.push_back({});
            continue;
        }
        vector<char> col = parse_column(trimmed);
        if (!col.empty()) {
            cols.push_back(col);
            if (N == 0) N = col.size();
        }
    }
    return {cols, N};
}

#endif // UTILS_HPP
