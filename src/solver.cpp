#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>

#include "../include/types.hpp"
#include "../include/logger.hpp"
#include "../include/utils.hpp"
#include "../include/validation.hpp"
#include "../include/goal_checking.hpp"
#include "../include/solution_optimization.hpp"
#include "../include/algorithm_selection.hpp"

using namespace std;

// Глобальная переменная для логирования
ofstream log_file;

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
    log_info("Files 3-7: Beam Search | BIRDS_11: Greedy Algorithm");
    log_info("");
    
    auto total_start = chrono::high_resolution_clock::now();
    
    // Чтение входных данных со стандартного входа
    vector<string> all_lines;
    string line;
    bool in_data = false;
    vector<string> data_lines;
    
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
        log_error(" Status: No input data");
        log_error(" Reason: DATA section is empty or missing.");
        return 1;
    }
    
    log_info(" Status: Reading input data...");
    
    auto [cols, N] = parse_data(data_lines);
    
    if (N == 0 || cols.empty()) {
        log_error(" Status: Parsing failed");
        log_error(" Reason: Could not parse columns or determine column height.");
        return 1;
    }
    
    unordered_map<char, int> freq;
    if (!validate_data(cols, N, freq, input_filename)) {
        log_error(" Status: Invalid input");
        return 1;
    }
    
    // Вычисляем required_per_bird — сколько колонок на каждый тип
    unordered_map<char, int> required_per_bird;
    for (const auto& p : freq) {
        required_per_bird[p.first] = p.second / N;
    }
    
    int required_L = 0;
    for (const auto& p : required_per_bird) required_L += p.second;
    
    int total_birds = 0;
    for (const auto& p : freq) total_birds += p.second;
    
    log_info(" Status: Input validated successfully");
    log_info(" Column height (N): " + to_string(N));
    log_info(" Columns in input: " + to_string(cols.size()));
    log_info(" Bird types: " + to_string(freq.size()));
    log_info(" Total birds: " + to_string(total_birds));
    log_info(" Required stacks (L): " + to_string(required_L));
    log_info("");
    
    log_info("Selecting algorithm based on problem size...");
    
    auto solve_start = chrono::high_resolution_clock::now();
    vector<Move> solution = select_and_run_algorithm(cols, N, required_per_bird, input_filename);
    auto solve_end = chrono::high_resolution_clock::now();
    auto solve_time = chrono::duration_cast<chrono::milliseconds>(solve_end - solve_start);
    
    if (solution.empty()) {
        log_error(" Status: No solution found");
        log_error(" Reason: Algorithm could not find a valid solution.");
        return 1;
    }
    
    // Оптимизация решения
    // solution = optimize_solution(solution, cols, N);
    
    log_info(" Status: Solution sequence generated");
    log_info(" Moves generated: " + to_string(solution.size()));
    log_info(" Search time: " + to_string(solve_time.count()) + " ms");
    
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
    
    // Подсчёт завершённых колонок и вывод результата
    int complete_cols = 0;
    for (size_t i = 0; i < final_cols.size(); i++) {
        if (final_cols[i].size() == N) {
            bool uniform = true;
            for (size_t j = 1; j < final_cols[i].size(); j++) {
                if (final_cols[i][j] != final_cols[i][0]) {
                    uniform = false;
                    break;
                }
            }
            if (uniform) complete_cols++;
        }
    }
    
    cout << "Финальное состояние:" << endl;
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
                if (uniform) cout << " ✓";
            }
        }
        cout << endl;
    }
    
    int L = complete_cols;
    int K = solution.size();
    long long F = 100LL * N * L - K;
    
    cout << "\nЦелевая функция: F = 100 * N * L - K" << endl;
    cout << "N = " << N << ", L = " << L << ", K = " << K << endl;
    cout << "F = " << F << endl;
    
    if (L < required_L) {
        cout << "\nВНИМАНИЕ: Решение неполное!" << endl;
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
    log_info("Complete columns: " + to_string(L) + "/" + to_string(required_L));
    log_info("Solution status: " + string(is_complete ? "COMPLETE" : "PARTIAL"));
    
    if (log_file.is_open()) {
        log_file.close();
    }
    
    return is_complete ? 0 : 1;
}
