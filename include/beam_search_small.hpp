#ifndef BEAM_SEARCH_SMALL_HPP
#define BEAM_SEARCH_SMALL_HPP

#include <climits>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include "types.hpp"
#include "logger.hpp"
#include "hashing.hpp"
#include "solution_optimization.hpp"

using namespace std;

const int SMALL_MAX_DEPTH = 300;
const int SMALL_TIME_LIMIT_MS = 90000;
const int SMALL_MAX_VISITED = 1000000;

inline int get_beam_width_small(int total_birds, int num_columns) {
    if (total_birds <= 20) return 1000;
    if (total_birds <= 40) return 800;
    if (total_birds <= 80) return 500;
    if (total_birds <= 150) return 300;
    if (total_birds <= 200) return 200;
    return 150;
}

inline int heuristic_small(const Columns& cols, int N, const unordered_map<char, int>& freq) {
    int score = 0;
    int complete_cols = 0;
    
    for (const auto& col : cols) {
        if (col.size() != N) continue;
        bool uniform = true;
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != col[0]) uniform = false;
        }
        if (uniform) {
            complete_cols++;
            score -= 500;
        }
    }
    
    for (const auto& col : cols) {
        if (col.empty()) {
            score -= 5;
            continue;
        }
        
        bool uniform = true;
        char first = col[0];
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != first) uniform = false;
        }
        
        if (uniform) {
            if (col.size() == N) {
                // уже учтено
            } else {
                score -= 20 * (int)col.size();
            }
        } else {
            unordered_map<char, int> count;
            for (char c : col) count[c]++;
            
            char most_common = col[0];
            int max_count = 0;
            for (const auto& p : count) {
                if (p.second > max_count) {
                    max_count = p.second;
                    most_common = p.first;
                }
            }
            
            int wrong_birds = col.size() - max_count;
            score += wrong_birds * 10;
            if (col.back() == most_common) {
                score -= 5;
            }
        }
    }
    
    if (complete_cols > 0) {
        score -= complete_cols * 100;
    }
    
    return score;
}

inline vector<Move> generate_moves_small(const Columns& cols, int N) {
    vector<Move> moves;
    int M = cols.size();
    
    unordered_map<char, vector<int>> top_birds;
    vector<int> empty_cols;
    
    for (int i = 0; i < M; ++i) {
        if (cols[i].empty()) {
            empty_cols.push_back(i);
        } else {
            char top = cols[i].back();
            top_birds[top].push_back(i);
        }
    }
    
    for (int from = 0; from < M; ++from) {
        if (cols[from].empty()) continue;
        char bird = cols[from].back();
        
        for (int to : top_birds[bird]) {
            if (from == to) continue;
            if (cols[to].size() >= N) continue;
            moves.push_back({from, to, bird});
        }
        
        for (int to : empty_cols) {
            if (from == to) continue;
            moves.push_back({from, to, bird});
        }
    }
    
    return moves;
}

// ============================================================================
// BEAM SEARCH АЛГОРИТМ
// ============================================================================

inline vector<Move> beam_search_small(const Columns& start_cols, int N, int required_L,
                               const unordered_map<char, int>& freq, const string& filename) {
    
    auto start_time = chrono::high_resolution_clock::now();
    
    int total_birds = 0;
    for (const auto& p : freq) total_birds += p.second;
    
    int beam_width = get_beam_width_small(total_birds, start_cols.size());
    
    log_info("Starting Beam Search (small) for " + filename);
    log_info("Beam width: " + to_string(beam_width) + ", N=" + to_string(N));
    
    State start_state{start_cols, {}, heuristic_small(start_cols, N, freq), 0};
    priority_queue<State, vector<State>, greater<State>> beam;
    beam.push(start_state);
    
    unordered_set<uint64_t> visited;
    visited.reserve(SMALL_MAX_VISITED);
    visited.insert(fast_hash_state(start_cols));
    
    State best_state = start_state;
    int iterations = 0;
    vector<Move> best_solution;
    int best_solution_depth = INT_MAX;
    
    while (!beam.empty() && iterations < SMALL_MAX_DEPTH) {
        iterations++;
        
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - start_time);
        if (elapsed.count() > SMALL_TIME_LIMIT_MS) break;
        
        priority_queue<State, vector<State>, greater<State>> next_beam;
        int processed = 0;
        
        while (!beam.empty() && processed < beam_width) {
            State current = beam.top();
            beam.pop();
            processed++;
            
            // Проверка цели
            int complete = 0;
            for (const auto& col : current.cols) {
                if (col.size() == N) {
                    bool uniform = true;
                    for (size_t i = 1; i < col.size(); ++i) {
                        if (col[i] != col[0]) uniform = false;
                    }
                    if (uniform) complete++;
                }
            }
            
            if (complete >= required_L) {
                if (current.g < best_solution_depth) {
                    best_solution_depth = current.g;
                    best_solution = current.moves;
                    log_info("Found solution with " + to_string(current.g) + " moves");
                    return optimize_solution(best_solution, start_cols, N);
                }
                continue;
            }
            
            if (current.h < best_state.h) {
                best_state = current;
            }
            
            vector<Move> moves = generate_moves_small(current.cols, N);
            if (moves.size() > 40) moves.resize(40);
            
            for (const Move& move : moves) {
                if (current.cols[move.from].empty() ||
                    current.cols[move.from].back() != move.c) continue;
                
                if (!current.moves.empty()) {
                    const Move& last = current.moves.back();
                    if (last.from == move.to && last.to == move.from && last.c == move.c) 
                        continue;
                }
                
                Columns next_cols = current.cols;
                next_cols[move.from].pop_back();
                next_cols[move.to].push_back(move.c);
                
                uint64_t hash = fast_hash_state(next_cols);
                if (visited.count(hash)) continue;
                
                if (visited.size() >= SMALL_MAX_VISITED) {
                    visited.clear();
                    visited.insert(fast_hash_state(start_cols));
                }
                
                visited.insert(hash);
                
                State next_state;
                next_state.cols = next_cols;
                next_state.moves = current.moves;
                next_state.moves.push_back(move);
                next_state.g = current.g + 1;
                next_state.h = heuristic_small(next_cols, N, freq);
                
                next_beam.push(next_state);
            }
        }
        
        beam = std::move(next_beam);
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto total_elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    log_info("Beam search completed in " + to_string(total_elapsed.count()) + " ms");
    
    if (!best_solution.empty()) {
        return optimize_solution(best_solution, start_cols, N);
    } else if (best_state.g > 0) {
        log_info("Returning best partial solution");
        return optimize_solution(best_state.moves, start_cols, N);
    }
    
    return {};
}

#endif // BEAM_SEARCH_SMALL_HPP
