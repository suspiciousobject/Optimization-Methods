#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>

using namespace std;

typedef vector<vector<char>> Columns;

struct Move {
    int from, to;
    char c;
};

struct State {
    Columns cols;
    vector<Move> moves;
    int h; // эвристика
    int g; // глубина
    
    bool operator>(const State& other) const {
        if (h != other.h) return h > other.h;
        return g > other.g;
    }
};

#endif // TYPES_HPP
