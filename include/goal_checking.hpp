#ifndef GOAL_CHECKING_HPP
#define GOAL_CHECKING_HPP

#include "types.hpp"

using namespace std;

inline bool is_goal(const Columns& cols, int N, int required_L) {
    int complete = 0;
    for (const auto& col : cols) {
        if (col.size() != N) continue;
        
        char first = col[0];
        bool uniform = true;
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != first) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) {
            complete++;
            if (complete >= required_L) return true;
        }
    }
    return false;
}

#endif // GOAL_CHECKING_HPP
