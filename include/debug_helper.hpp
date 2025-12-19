#ifndef DEBUG_HELPER_HPP
#define DEBUG_HELPER_HPP

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <sstream>
#include "types.hpp"
#include "logger.hpp"

using namespace std;

// ============================================================================
// МОДУЛЬ ОТЛАДКИ ДЛЯ АНАЛИЗА АЛГОРИТМА
// ============================================================================

namespace DebugHelper {

// ============================================================================
// АНАЛИЗ ТЕКУЩЕГО СОСТОЯНИЯ КОЛОНОК
// ============================================================================

struct StateAnalysis {
    int total_columns;
    int empty_columns;
    int full_columns;
    int complete_columns;       // Полные и однородные
    int partial_columns;        // Неполные колонки
    unordered_map<char, int> bird_count;       // Сколько каждой птицы
    unordered_map<char, int> complete_by_type; // Полные колонки каждого типа
    
    void analyze(const Columns& cols, int N) {
        total_columns = cols.size();
        empty_columns = 0;
        full_columns = 0;
        complete_columns = 0;
        partial_columns = 0;
        bird_count.clear();
        complete_by_type.clear();
        
        for (const auto& col : cols) {
            if (col.empty()) {
                empty_columns++;
            } else if (col.size() == N) {
                full_columns++;
                
                // Проверяем, однородна ли колонка
                bool uniform = true;
                for (size_t i = 1; i < col.size(); ++i) {
                    if (col[i] != col[0]) {
                        uniform = false;
                        break;
                    }
                }
                
                if (uniform) {
                    complete_columns++;
                    complete_by_type[col[0]]++;
                }
            } else {
                partial_columns++;
            }
            
            // Считаем птиц
            for (char c : col) {
                bird_count[c]++;
            }
        }
    }
    
    void log_analysis() {
        log_info("=== STATE ANALYSIS ===");
        log_info("Total columns: " + to_string(total_columns));
        log_info("Empty: " + to_string(empty_columns) + 
                 " | Full: " + to_string(full_columns) +
                 " | Complete: " + to_string(complete_columns));
        log_info("Partial (incomplete): " + to_string(partial_columns));
        
        log_info("--- Bird Distribution ---");
        for (const auto& [bird, count] : bird_count) {
            string complete_str = "";
            if (complete_by_type.count(bird)) {
                complete_str = " (complete: " + to_string(complete_by_type[bird]) + ")";
            }
            log_info("Bird '" + string(1, bird) + "': " + to_string(count) + complete_str);
        }
    }
    
    string get_summary() {
        return "Complete: " + to_string(complete_columns) + 
               " | Full: " + to_string(full_columns) + 
               " | Partial: " + to_string(partial_columns) + 
               " | Empty: " + to_string(empty_columns);
    }
};

// ============================================================================
// ВИЗУАЛИЗАЦИЯ МАЛЫХ ЗАДАЧ
// ============================================================================

void visualize_small_state(const Columns& cols, int N, const string& label = "") {
    if (cols.size() > 20) {
        log_info("Visualization skipped (too many columns: " + to_string(cols.size()) + ")");
        return;
    }
    
    log_info("=== VISUALIZATION: " + label + " ===");
    
    // Вывод колонок рядом для наглядности
    for (int row = 0; row < N; ++row) {
        stringstream ss;
        ss << "Row " << setw(2) << row << ": ";
        
        for (size_t col = 0; col < cols.size(); ++col) {
            if (row < (int)cols[col].size()) {
                ss << cols[col][row] << " ";
            } else {
                ss << ". ";
            }
        }
        
        log_info(ss.str());
    }
}

// ============================================================================
// ПРОВЕРКА ВАЛИДНОСТИ СОСТОЯНИЯ
// ============================================================================

struct ValidationResult {
    bool is_valid;
    vector<string> errors;
    vector<string> warnings;
    
    void log_result() {
        if (is_valid) {
            log_info("✓ Validation PASSED");
        } else {
            log_error("✗ Validation FAILED");
            for (const auto& err : errors) {
                log_error("  ERROR: " + err);
            }
        }
        
        for (const auto& warn : warnings) {
            log_error("  WARNING: " + warn);
        }
    }
};

ValidationResult validate_state(const Columns& cols, int N, const unordered_map<char, int>& required_per_bird) {
    ValidationResult result;
    result.is_valid = true;
    
    // Проверка 1: Размеры колонок
    for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i].size() > N) {
            result.is_valid = false;
            result.errors.push_back("Column " + to_string(i) + " exceeds N=" + to_string(N));
        }
    }
    
    // Проверка 2: Дубликаты птиц
    unordered_map<char, int> bird_count;
    for (const auto& col : cols) {
        for (char c : col) {
            bird_count[c]++;
        }
    }
    
    for (const auto& [bird, required] : required_per_bird) {
        int have = bird_count[bird];
        int expected = required * N;
        
        if (have > expected) {
            result.is_valid = false;
            result.errors.push_back("Bird '" + string(1, bird) + "': " + 
                                   to_string(have) + " > " + to_string(expected) + " expected");
        } else if (have < expected) {
            result.warnings.push_back("Bird '" + string(1, bird) + "': " + 
                                     to_string(have) + " < " + to_string(expected) + " expected");
        }
    }
    
    // Проверка 3: Целевое состояние
    int complete = 0;
    for (const auto& col : cols) {
        if (col.size() != N) continue;
        
        bool uniform = true;
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != col[0]) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) complete++;
    }
    
    int target_complete = 0;
    for (const auto& [bird, cnt] : required_per_bird) {
        target_complete += cnt;
    }
    
    if (complete < target_complete) {
        result.warnings.push_back("Only " + to_string(complete) + "/" + to_string(target_complete) + 
                                 " target columns completed");
    }
    
    return result;
}

// ============================================================================
// ОТСЛЕЖИВАНИЕ ПРОГРЕССА ПО ПТИЦАМ
// ============================================================================

struct BirdProgressTracker {
    unordered_map<char, int> columns_built;
    unordered_map<char, int> stuck_attempts;
    unordered_map<char, int> backtrack_count;
    
    void record_success(char bird_type) {
        columns_built[bird_type]++;
    }
    
    void record_stuck(char bird_type, int attempt) {
        stuck_attempts[bird_type] = max(stuck_attempts[bird_type], attempt);
    }
    
    void record_backtrack(char bird_type) {
        backtrack_count[bird_type]++;
    }
    
    void log_progress(const unordered_map<char, int>& required_per_bird) {
        log_info("=== BIRD PROGRESS REPORT ===");
        
        for (const auto& [bird_type, required] : required_per_bird) {
            int built = columns_built[bird_type];
            int stuck = stuck_attempts[bird_type];
            int backtracks = backtrack_count[bird_type];
            
            stringstream ss;
            ss << "Bird '" << bird_type << "': " << built << "/" << required;
            if (stuck > 0) {
                ss << " (stuck " << stuck << " times)";
            }
            if (backtracks > 0) {
                ss << " (backtracked " << backtracks << " times)";
            }
            
            if (built == required) {
                log_info("✓ " + ss.str());
            } else if (built > 0) {
                log_error("⚠ " + ss.str());
            } else {
                log_error("✗ " + ss.str());
            }
        }
    }
};

// ============================================================================
// АНАЛИЗ УЗКИХ МЕСТ
// ============================================================================

void find_bottlenecks(const Columns& cols, int N) {
    log_info("=== BOTTLENECK ANALYSIS ===");
    
    // Найти самые проблемные колонки
    vector<pair<int, int>> problem_cols;  // (col_idx, disorder_score)
    
    for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i].empty()) continue;
        
        int disorder = 0;
        unordered_map<char, int> bird_count;
        
        for (char c : cols[i]) {
            bird_count[c]++;
        }
        
        // Чем больше разных типов - тем больше беспорядок
        disorder = bird_count.size();
        
        // Чем дальше от цели - тем больше штраф
        if (cols[i].size() < N) {
            disorder += (N - cols[i].size()) * 2;
        }
        
        if (disorder > 0) {
            problem_cols.push_back({i, disorder});
        }
    }
    
    // Сортируем по проблемности
    sort(problem_cols.begin(), problem_cols.end(),
        [](const pair<int, int>& a, const pair<int, int>& b) {
            return a.second > b.second;
        });
    
    // Выводим топ-5 проблемных
    int count = 0;
    for (const auto& [col_idx, disorder] : problem_cols) {
        if (count >= 5) break;
        
        stringstream ss;
        ss << "Column " << col_idx << ": disorder=" << disorder << " | content: ";
        
        for (size_t i = 0; i < min(cols[col_idx].size(), (size_t)20); ++i) {
            ss << cols[col_idx][i];
        }
        if (cols[col_idx].size() > 20) {
            ss << "...";
        }
        
        log_error(ss.str());
        count++;
    }
}

}  // namespace DebugHelper

#endif // DEBUG_HELPER_HPP
