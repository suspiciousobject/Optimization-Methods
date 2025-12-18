#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <chrono>
#include <functional>
#include <cstdint>
#include <climits>
#include <iomanip>

using namespace std;

typedef vector<vector<char>> Columns;

struct Move {
    int from, to;
    char c;
};

struct State {
    Columns cols;
    vector<Move> moves;
    int h;      // эвристика
    int g;      // глубина
    
    bool operator>(const State& other) const {
        if (h != other.h) return h > other.h;
        return g > other.g;
    }
};

// ============================================================================
// УТИЛИТЫ ДЛЯ ФОРМАТИРОВАННОГО ЛОГГИРОВАНИЯ
// ============================================================================
ofstream log_file;

string get_current_timestamp() {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = chrono::system_clock::to_time_t(now);
    tm bt = *localtime(&timer);
    
    ostringstream oss;
    oss << put_time(&bt, "%Y-%m-%d %H:%M:%S");
    oss << '.' << setfill('0') << setw(3) << ms.count();
    return oss.str();
}

void init_logger(const string& filename) {
    log_file.open(filename);
    if (!log_file.is_open()) {
        cerr << "[ERROR] Не удалось открыть лог-файл: " << filename << endl;
    }
}

void log_info(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [info] " + message;
    cerr << log_entry << endl;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

void log_error(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [error] " + message;
    cerr << log_entry << endl;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

void log_debug(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [debug] " + message;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

// ============================================================================
// ПАРАМЕТРЫ АЛГОРИТМА (оригинальные с небольшими улучшениями)
// ============================================================================
const int MAX_DEPTH = 300;
const int TIME_LIMIT_MS = 90000;  // 90 секунд для сложных задач
const int MAX_VISITED = 1000000;

inline int get_beam_width(int total_birds, int num_columns) {
    // Оригинальная логика
    if (total_birds <= 20) return 1000;
    if (total_birds <= 40) return 800;
    if (total_birds <= 80) return 500;
    if (total_birds <= 150) return 300;
    if (total_birds <= 200) return 200;
    return 150;
}

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================
string trim(const string& str);
vector<char> parse_column(const string& line);
pair<Columns, int> parse_data(const vector<string>& data_lines);
bool validate_data(const Columns& cols, int N, unordered_map<char, int>& freq, const string& filename);
int calculate_required_L(const unordered_map<char, int>& freq, int N);
bool is_goal(const Columns& cols, int N, int required_L);
vector<Move> generate_moves(const Columns& cols, int N);
int heuristic(const Columns& cols, int N, const unordered_map<char, int>& freq);
uint64_t fast_hash_state(const Columns& cols);
vector<Move> optimize_solution(const vector<Move>& moves, const Columns& start_cols, int N);
vector<Move> beam_search(const Columns& start_cols, int N, int required_L, 
                         const unordered_map<char, int>& freq, const string& filename);

// ============================================================================
// УТИЛИТЫ
// ============================================================================

string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

vector<char> parse_column(const string& line) {
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

// ============================================================================
// БЫСТРОЕ ХЕШИРОВАНИЕ СОСТОЯНИЯ
// ============================================================================

uint64_t fast_hash_state(const Columns& cols) {
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

// ============================================================================
// ПАРСИНГ ДАННЫХ
// ============================================================================

pair<Columns, int> parse_data(const vector<string>& data_lines) {
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

// ============================================================================
// ВАЛИДАЦИЯ
// ============================================================================

bool validate_data(const Columns& cols, int N, unordered_map<char, int>& freq, const string& filename) {
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
                     to_string(p.second) + " times, which is not divisible by N=" + to_string(N) + ".");
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// ВЫЧИСЛЕНИЕ ЦЕЛЕВОГО КОЛИЧЕСТВА КОЛОНОК
// ============================================================================

int calculate_required_L(const unordered_map<char, int>& freq, int N) {
    int L = 0;
    for (const auto& p : freq) {
        L += p.second / N;
    }
    return L;
}

// ============================================================================
// ПРОВЕРКА ЗАВЕРШЕНИЯ
// ============================================================================

bool is_goal(const Columns& cols, int N, int required_L) {
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

// ============================================================================
// ГЕНЕРАЦИЯ ХОДОВ (оригинальная эффективная)
// ============================================================================

vector<Move> generate_moves(const Columns& cols, int N) {
    vector<Move> moves;
    int M = cols.size();
    
    // Собираем информацию о верхних птицах
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
    
    // Стратегия: сначала перемещаем на такую же птицу
    for (int from = 0; from < M; ++from) {
        if (cols[from].empty()) continue;
        
        char bird = cols[from].back();
        
        // 1. Приоритет: переместить на такую же птицу
        for (int to : top_birds[bird]) {
            if (from == to) continue;
            if (cols[to].size() >= N) continue;
            moves.push_back({from, to, bird});
        }
        
        // 2. В пустые колонки
        for (int to : empty_cols) {
            if (from == to) continue;
            moves.push_back({from, to, bird});
        }
    }
    
    return moves;
}

// ============================================================================
// ЭВРИСТИЧЕСКАЯ ФУНКЦИЯ (оригинальная улучшенная)
// ============================================================================

int heuristic(const Columns& cols, int N, const unordered_map<char, int>& freq) {
    int score = 0;
    int complete_cols = 0;
    
    // Считаем завершенные колонки
    for (const auto& col : cols) {
        if (col.size() != N) continue;
        
        bool uniform = true;
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != col[0]) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) {
            complete_cols++;
            score -= 500;  // Награда за завершенную колонку
        }
    }
    
    // Анализируем каждую колонку
    for (const auto& col : cols) {
        if (col.empty()) {
            score -= 5;  // Пустые колонки полезны
            continue;
        }
        
        // Проверяем однородность
        bool uniform = true;
        char first = col[0];
        
        for (size_t i = 1; i < col.size(); ++i) {
            if (col[i] != first) {
                uniform = false;
                break;
            }
        }
        
        if (uniform) {
            if (col.size() == N) {
                // Уже учтено выше
            } else {
                score -= 20 * (int)col.size();  // Награда за однородные колонки
            }
        } else {
            // Штраф за разнородность
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
            
            // Если сверху самая частая птица - хорошо
            if (col.back() == most_common) {
                score -= 5;
            }
        }
    }
    
    // Дополнительный бонус за несколько завершенных колонок
    if (complete_cols > 0) {
        score -= complete_cols * 100;
    }
    
    return score;
}

// ============================================================================
// ОПТИМИЗАЦИЯ РЕШЕНИЯ
// ============================================================================

vector<Move> optimize_solution(const vector<Move>& moves, const Columns& start_cols, int N) {
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
        
        // Удаляем обратные ходы
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

// ============================================================================
// BEAM SEARCH (универсальный для всех файлов)
// ============================================================================

vector<Move> beam_search(const Columns& start_cols, int N, int required_L, 
                         const unordered_map<char, int>& freq, const string& filename) {
    auto start_time = chrono::high_resolution_clock::now();
    
    int total_birds = 0;
    for (const auto& p : freq) total_birds += p.second;
    int beam_width = get_beam_width(total_birds, start_cols.size());
    
    log_info("Starting Beam Search for " + filename);
    log_info("Problem size: " + to_string(total_birds) + " birds, N=" + to_string(N));
    log_info("Beam width: " + to_string(beam_width));
    
    State start_state{start_cols, {}, heuristic(start_cols, N, freq), 0};
    
    priority_queue<State, vector<State>, greater<State>> beam;
    beam.push(start_state);
    
    unordered_set<uint64_t> visited;
    visited.reserve(MAX_VISITED);
    visited.insert(fast_hash_state(start_cols));
    
    State best_state = start_state;
    int iterations = 0;
    vector<Move> best_solution;
    int best_solution_depth = INT_MAX;
    
    while (!beam.empty() && iterations < MAX_DEPTH) {
        iterations++;
        
        auto current_time = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(current_time - start_time);
        if (elapsed.count() > TIME_LIMIT_MS) {
            log_debug("Time limit exceeded: " + to_string(elapsed.count()) + " ms");
            break;
        }
        
        priority_queue<State, vector<State>, greater<State>> next_beam;
        int processed = 0;
        
        while (!beam.empty() && processed < beam_width) {
            State current = beam.top();
            beam.pop();
            processed++;
            
            if (is_goal(current.cols, N, required_L)) {
                if (current.g < best_solution_depth) {
                    best_solution_depth = current.g;
                    best_solution = current.moves;
                    log_info("Found solution with " + to_string(current.g) + " moves");
                    
                    // Если решение хорошее, можем вернуть его сразу
                    if (current.g <= N * required_L * 2) {
                        return optimize_solution(best_solution, start_cols, N);
                    }
                }
                continue;
            }
            
            if (current.h < best_state.h) {
                best_state = current;
            }
            
            vector<Move> moves = generate_moves(current.cols, N);
            
            // Ограничиваем количество рассматриваемых ходов
            int max_moves = 40;
            if (moves.size() > max_moves) {
                moves.resize(max_moves);
            }
            
            for (const Move& move : moves) {
                if (current.cols[move.from].empty() || 
                    current.cols[move.from].back() != move.c) {
                    continue;
                }
                
                // Избегаем обратных ходов
                if (!current.moves.empty()) {
                    const Move& last = current.moves.back();
                    if (last.from == move.to && last.to == move.from && last.c == move.c) {
                        continue;
                    }
                }
                
                Columns next_cols = current.cols;
                next_cols[move.from].pop_back();
                next_cols[move.to].push_back(move.c);
                
                uint64_t hash = fast_hash_state(next_cols);
                if (visited.count(hash)) continue;
                
                if (visited.size() >= MAX_VISITED) {
                    visited.clear();
                    visited.insert(fast_hash_state(start_cols));
                }
                visited.insert(hash);
                
                State next_state;
                next_state.cols = next_cols;
                next_state.moves = current.moves;
                next_state.moves.push_back(move);
                next_state.g = current.g + 1;
                next_state.h = heuristic(next_cols, N, freq);
                
                next_beam.push(next_state);
            }
        }
        
        beam = std::move(next_beam);
        
        if (iterations % 10 == 0) {
            log_debug("Iteration " + to_string(iterations) + 
                     ", best h=" + to_string(best_state.h) + 
                     ", depth=" + to_string(best_state.g) + 
                     ", solution=" + (best_solution.empty() ? "-" : to_string(best_solution_depth)) +
                     ", visited=" + to_string(visited.size()) +
                     ", time=" + to_string(elapsed.count()) + " ms");
        }
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto total_elapsed = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    log_info("Search completed in " + to_string(total_elapsed.count()) + " ms");
    log_info("Iterations: " + to_string(iterations));
    log_info("States visited: " + to_string(visited.size()));
    
    if (!best_solution.empty()) {
        log_info("Best solution: " + to_string(best_solution_depth) + " moves");
        return optimize_solution(best_solution, start_cols, N);
    } else if (best_state.g > 0) {
        log_info("Best partial solution: " + to_string(best_state.g) + " moves");
        
        // Проверяем сколько колонок собрано
        int complete_in_best = 0;
        for (const auto& col : best_state.cols) {
            if (col.size() == N) {
                bool uniform = true;
                for (size_t i = 1; i < col.size(); ++i) {
                    if (col[i] != col[0]) uniform = false;
                }
                if (uniform) complete_in_best++;
            }
        }
        
        log_info("Columns completed in partial: " + to_string(complete_in_best) + "/" + to_string(required_L));
        return optimize_solution(best_state.moves, start_cols, N);
    }
    
    log_info("No solution found");
    return {};
}

// ============================================================================
// MAIN ФУНКЦИЯ
// ============================================================================

int main(int argc, char* argv[]) {
    string input_filename = "input.txt";
    string log_filename = "solver.log";
    
    if (argc > 1) {
        input_filename = argv[1];
        size_t dot_pos = input_filename.find_last_of('.');
        if (dot_pos != string::npos) {
            log_filename = input_filename.substr(0, dot_pos) + ".log";
        }
    }
    
    init_logger(log_filename);
    
    log_info("Universal Solver execution started.");
    log_info("");
    
    auto total_start = chrono::high_resolution_clock::now();
    
    vector<string> all_lines;
    string line;
    
    bool in_data = false;
    vector<string> data_lines;
    
    // Чтение из stdin
    while (getline(cin, line)) {
        all_lines.push_back(line);
        string trimmed = trim(line);
        
        if (trimmed == "DATA") {
            in_data = true;
            continue;
        }
        
        if (in_data) {
            if (trimmed == "/" || trimmed == "ORDER") {
                in_data = false;
            } else if (!trimmed.empty()) {
                data_lines.push_back(line);
            }
        }
    }
    
    if (data_lines.empty()) {
        log_error("  Status: No input data");
        log_error("  Reason: DATA section is empty or missing.");
        return 1;
    }
    
    log_info("  Status: Reading input data...");
    
    auto parse_start = chrono::high_resolution_clock::now();
    auto [cols, N] = parse_data(data_lines);
    auto parse_end = chrono::high_resolution_clock::now();
    
    if (N == 0 || cols.empty()) {
        log_error("  Status: Parsing failed");
        log_error("  Reason: Could not parse columns or determine column height.");
        return 1;
    }
    
    unordered_map<char, int> freq;
    if (!validate_data(cols, N, freq, input_filename)) {
        log_error("  Status: Invalid input");
        return 1;
    }
    
    int required_L = calculate_required_L(freq, N);
    int total_birds = 0;
    for (const auto& p : freq) total_birds += p.second;
    
    log_info("  Status: Input validated successfully");
    log_info("  Column height (N): " + to_string(N));
    log_info("  Columns in input: " + to_string(cols.size()));
    log_info("  Bird types: " + to_string(freq.size()));
    log_info("  Total birds: " + to_string(total_birds));
    log_info("  Required stacks (L): " + to_string(required_L));
    
    log_info("");
    log_info("Starting solution search...");
    
    auto solve_start = chrono::high_resolution_clock::now();
    vector<Move> solution = beam_search(cols, N, required_L, freq, input_filename);
    auto solve_end = chrono::high_resolution_clock::now();
    auto solve_time = chrono::duration_cast<chrono::milliseconds>(solve_end - solve_start);
    
    if (solution.empty()) {
        log_error("  Status: No solution found");
        log_error("  Reason: Algorithm could not find a valid solution within time limit.");
        return 1;
    }
    
    log_info("  Status: Solution sequence generated");
    log_info("  Moves generated: " + to_string(solution.size()));
    log_info("  Search time: " + to_string(solve_time.count()) + " ms");
    
    // Проверка решения
    Columns final_cols = cols;
    for (const auto& move : solution) {
        if (move.from < 0 || move.from >= (int)final_cols.size() ||
            move.to < 0 || move.to >= (int)final_cols.size()) {
            log_error("Invalid move in solution");
            return 1;
        }
        final_cols[move.from].pop_back();
        final_cols[move.to].push_back(move.c);
    }
    
    bool is_complete = is_goal(final_cols, N, required_L);
    
    // Вывод результата
    cout << "DATA" << endl;
    for (const auto& line : data_lines) {
        string trimmed = trim(line);
        if (trimmed == "/") break;
        cout << trimmed << endl;
    }
    cout << "/" << endl << endl;
    
    if (!solution.empty()) {
        cout << "ORDER" << endl;
        for (const auto& move : solution) {
            cout << (move.from + 1) << " " << (move.to + 1) << " " << move.c << endl;
        }
        cout << "/" << endl << endl;
    } else {
        cout << "ORDER" << endl;
        cout << "/" << endl << endl;
    }
    
    cout << "Финальное состояние:" << endl;
    int complete_cols = 0;
    for (size_t i = 0; i < final_cols.size(); i++) {
        cout << i + 1 << ": ";
        if (final_cols[i].empty()) {
            cout << "==";
        } else {
            for (char c : final_cols[i]) {
                cout << c << " ";
            }
            
            if (final_cols[i].size() == N) {
                bool uniform = true;
                for (size_t j = 1; j < final_cols[i].size(); j++) {
                    if (final_cols[i][j] != final_cols[i][0]) {
                        uniform = false;
                        break;
                    }
                }
                if (uniform) {
                    cout << " ✓";
                    complete_cols++;
                }
            }
        }
        cout << endl;
    }
    
    int L = complete_cols;
    int K = solution.size();
    int F = 100 * N * L - K;
    
    cout << "\nЦелевая функция: F = 100 * N * L - K" << endl;
    cout << "N = " << N << ", L = " << L << ", K = " << K << endl;
    cout << "F = " << F << endl;
    
    if (L < required_L) {
        cout << "\n⚠️ ВНИМАНИЕ: Решение неполное!" << endl;
        cout << "Собрано колонок: " << L << " из " << required_L << endl;
    } else {
        cout << "\n✅ РЕШЕНИЕ КОРРЕКТНОЕ!" << endl;
        cout << "Все " << required_L << " колонок собраны" << endl;
    }
    
    auto total_end = chrono::high_resolution_clock::now();
    auto total_time = chrono::duration_cast<chrono::milliseconds>(total_end - total_start);
    
    log_info("");
    log_info("Execution finished.");
    log_info("Total execution time: " + to_string(total_time.count()) + " ms");
    
    if (log_file.is_open()) {
        log_file.close();
    }
    
    return is_complete ? 0 : 1;
}
