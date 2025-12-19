#ifndef SOLUTION_OPTIMIZATION_HPP
#define SOLUTION_OPTIMIZATION_HPP

#include <vector>
#include "types.hpp"

using namespace std;

// ============================================================================
// ОПТИМИЗАЦИЯ РЕШЕНИЯ
// ============================================================================

inline vector<Move> optimize_solution(const vector<Move>& moves, const Columns& start_cols, int N) {
    if (moves.size() <= 1) return moves;
    
    vector<Move> optimized;
    Columns current = start_cols;
    
    for (size_t i = 0; i < moves.size(); ++i) {
        const Move& move = moves[i];
        
        if (move.from < 0 || move.from >= (int)current.size() ||
            move.to < 0 || move.to >= (int)current.size()) {
            continue;
        }
        
        if (current[move.from].empty() ||
            current[move.from].back() != move.c) {
            continue;
        }
        
        // Проверка на обратный ход
        if (!optimized.empty()) {
            const Move& last = optimized.back();
            if (last.from == move.to && last.to == move.from && last.c == move.c) {
                optimized.pop_back();
                current[last.to].pop_back();
                current[last.from].push_back(last.c);
                continue;
            }
        }
        
        current[move.from].pop_back();
        current[move.to].push_back(move.c);
        optimized.push_back(move);
    }
    
    return optimized;
}

#endif // SOLUTION_OPTIMIZATION_HPP
