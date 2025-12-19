#ifndef GREEDY_LARGE_HPP
#define GREEDY_LARGE_HPP

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include "types.hpp"
#include "logger.hpp"

using namespace std;

vector<Move> greedy_huge_solver(Columns& cols, int N, const unordered_map<char, int>& required_per_bird) {
    auto start_time = chrono::high_resolution_clock::now();
    
    vector<Move> moves;
    int M = cols.size();
    
    log_info("Starting Improved Greedy Algorithm (Total Cleanup Strategy)");
    log_info("Strategy: Ensure target column is 100% pure before locking");

    vector<pair<char, int>> bird_list(required_per_bird.begin(), required_per_bird.end());
    sort(bird_list.begin(), bird_list.end(), [](auto& a, auto& b) {
        return a.second > b.second;
    });

    unordered_set<int> completed_cols;

    for (const auto& [bird_type, columns_needed] : bird_list) {
        if (columns_needed == 0) continue;

        log_info("Processing " + string(1, bird_type) + ": " + to_string(columns_needed) + " columns needed");

        for (int col_idx = 0; col_idx < columns_needed; col_idx++) {
            int target_col = -1;

            // 1. Выбор лучшего кандидата (где больше всего нужных птиц и нет чужих внизу)
            int best_score = -1;
            for (int i = 0; i < M; i++) {
                if (completed_cols.count(i)) continue;
                
                int count = 0;
                bool has_trash_below = false;
                for (int j = 0; j < (int)cols[i].size(); j++) {
                    if (cols[i][j] == bird_type) count++;
                    else {
                        // Если нашли чужую птицу, а выше есть нужные — это плохой кандидат, 
                        // но его можно очистить.
                        has_trash_below = true;
                    }
                }
                
                int score = count * 10 - (has_trash_below ? 50 : 0);
                if (score > best_score) {
                    best_score = score;
                    target_col = i;
                }
            }

            if (target_col == -1) {
                log_error("No available column for " + string(1, bird_type));
                return moves;
            }

            // 2. ГЛУБОКАЯ ОЧИСТКА: Удаляем всё, что мешает однородности
            bool needs_cleanup = true;
            while (needs_cleanup) {
                needs_cleanup = false;
                if (cols[target_col].empty()) break;

                // Проверяем, есть ли в столбце хоть одна чужая птица
                bool has_wrong = false;
                for (char c : cols[target_col]) {
                    if (c != bird_type) {
                        has_wrong = true;
                        break;
                    }
                }

                if (has_wrong) {
                    char to_move = cols[target_col].back();
                    bool moved = false;

                    // Пытаемся переместить верхнюю птицу (даже если она нужная, но под ней мусор)
                    for (int phase = 0; phase < 2; phase++) {
                        for (int to = 0; to < M && !moved; to++) {
                            if (to == target_col || completed_cols.count(to) || (int)cols[to].size() >= N) continue;
                            
                            // Phase 0: к своим или в пустой, Phase 1: в любой доступный
                            if (phase == 0 && (cols[to].empty() || cols[to].back() == to_move)) {
                                moves.push_back({target_col, to, to_move});
                                cols[target_col].pop_back();
                                cols[to].push_back(to_move);
                                moved = true;
                            } else if (phase == 1) {
                                moves.push_back({target_col, to, to_move});
                                cols[target_col].pop_back();
                                cols[to].push_back(to_move);
                                moved = true;
                            }
                        }
                    }
                    if (!moved) {
                        log_error("CRITICAL: Stuck while clearing target column " + to_string(target_col));
                        return moves;
                    }
                    needs_cleanup = true; // Продолжаем, пока есть хоть одна чужая птица
                }
            }

            // 3. ЗАПОЛНЕНИЕ: Ищем нужных птиц по всей доске
            while ((int)cols[target_col].size() < N) {
                int best_from = -1;
                int min_depth = 10000;

                for (int from = 0; from < M; from++) {
                    if (from == target_col || completed_cols.count(from) || cols[from].empty()) continue;
                    
                    for (int d = (int)cols[from].size() - 1; d >= 0; d--) {
                        if (cols[from][d] == bird_type) {
                            int depth = (int)cols[from].size() - 1 - d;
                            if (depth < min_depth) {
                                min_depth = depth;
                                best_from = from;
                            }
                            break; 
                        }
                    }
                }

                if (best_from == -1) {
                    log_error("Cannot find any more birds of type " + string(1, bird_type));
                    return moves;
                }

                // Достаем нужную птицу из best_from
                while (cols[best_from].back() != bird_type) {
                    char blocking = cols[best_from].back();
                    bool moved_blocking = false;
                    for (int to = 0; to < M && !moved_blocking; to++) {
                        if (to == target_col || to == best_from || completed_cols.count(to) || (int)cols[to].size() >= N) continue;
                        moves.push_back({best_from, to, blocking});
                        cols[best_from].pop_back();
                        cols[to].push_back(blocking);
                        moved_blocking = true;
                    }
                    if (!moved_blocking) return moves;
                }

                // Переносим целевую птицу
                moves.push_back({best_from, target_col, bird_type});
                cols[best_from].pop_back();
                cols[target_col].push_back(bird_type);
            }

            // 4. ФИНАЛЬНАЯ ПРОВЕРКА И БЛОКИРОВКА
            bool final_check = true;
            for (char c : cols[target_col]) {
                if (c != bird_type) final_check = false;
            }

            if (final_check && (int)cols[target_col].size() == N) {
                completed_cols.insert(target_col);
            } else {
                log_error("Failed to verify column " + to_string(target_col));
                return moves;
            }
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto total_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    log_info("Greedy huge solver finished. Time: " + to_string(total_time.count()) + "ms");
    
    return moves;
}

#endif // GREEDY_LARGE_HPP
