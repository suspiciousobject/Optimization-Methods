// ../src/checker.cpp

#include <bits/stdc++.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

inline std::string extractColumn(const std::string& line) {
    std::string col;
    for (char c : line) {
        if (c == ' ' || c == '\t') continue;
        if (c == '-' || c == '\r' || c == '\n') break;
        if (c >= 'A' && c <= 'Z') col += c;
    }
    return col;
}

struct ValidationResult {
    enum class Status {
        SOLVABLE,
        UNSOLVABLE,
        INVALID
    };

    Status status;
    std::string reason;
    int N = 0;
    int column_count = 0;
};

ValidationResult analyzeFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return {ValidationResult::Status::INVALID, "File could not be opened."};
    }

    std::vector<std::string> columns;
    std::string line;
    bool parsing_data = false;

    while (std::getline(file, line)) {
        if (line.find("DATA") != std::string::npos) {
            parsing_data = true;
            continue;
        }
        if (!parsing_data) continue;

        if (line.find("/") != std::string::npos || line.find("ORDER") != std::string::npos) {
            break;
        }

        if (line.find("==") != std::string::npos) {
            columns.emplace_back("");
        } else {
            std::string col = extractColumn(line);
            if (!col.empty()) {
                columns.push_back(std::move(col));
            }
        }
    }

    if (columns.empty()) {
        return {ValidationResult::Status::INVALID, "No columns found after DATA section."};
    }

    int N = 0;
    for (const auto& col : columns) {
        if (!col.empty()) {
            N = static_cast<int>(col.size());
            break;
        }
    }

    if (N <= 0) {
        return {ValidationResult::Status::INVALID, "Column height N must be positive."};
    }

    for (const auto& col : columns) {
        if (!col.empty() && static_cast<int>(col.size()) != N) {
            return {
                ValidationResult::Status::INVALID,
                "Column height mismatch: expected " + std::to_string(N) +
                ", found column of size " + std::to_string(col.size()) + "."
            };
        }
    }

    std::array<int, 26> freq{};
    for (const auto& col : columns) {
        for (char c : col) {
            if (c >= 'A' && c <= 'Z') {
                freq[c - 'A']++;
            }
        }
    }

    for (int i = 0; i < 26; ++i) {
        if (freq[i] > 0 && freq[i] % N != 0) {
            char letter = static_cast<char>('A' + i);
            return {
                ValidationResult::Status::UNSOLVABLE,
                "Letter '" + std::string(1, letter) + "' appears " + std::to_string(freq[i]) +
                " times, which is not divisible by N=" + std::to_string(N) + "."
            };
        }
    }

    return {
        ValidationResult::Status::SOLVABLE,
        "All constraints satisfied.",
        N,
        static_cast<int>(columns.size())
    };
}

int main() {
    auto logger = spdlog::basic_logger_mt("solver_checker", "../../data/checker_result/log_latest.log");
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    spdlog::set_level(spdlog::level::info);

    spdlog::info("Checker execution started.");
    spdlog::info("Evaluating solvability of input instances.");
    spdlog::info("");

    const std::vector<std::string> input_files = {
        "../../data/BIRDS_3.txt", "../../data/BIRDS_4.txt", "../../data/BIRDS_5.txt",
        "../../data/BIRDS_6.txt", "../../data/BIRDS_7.txt", "../../data/BIRDS_8.txt",
        "../../data/BIRDS_9.txt", "../../data/BIRDS_10.txt", "../../data/BIRDS_11.txt",
        "../../data/BIRDS_12.txt", "../../data/BIRDS_13.txt"
    };

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& path : input_files) {
        spdlog::info("Processing file: {}", path);

        ValidationResult result = analyzeFile(path);

        switch (result.status) {
            case ValidationResult::Status::SOLVABLE:
                spdlog::info("  Status: Solvable");
                spdlog::info("  Column height (N): {}", result.N);
                spdlog::info("  Columns processed: {}", result.column_count);
                spdlog::info("  Result: VALID");
                break;

            case ValidationResult::Status::UNSOLVABLE:
                spdlog::error("  Status: Unsolvable");
                spdlog::error("  Reason: {}", result.reason);
                break;

            case ValidationResult::Status::INVALID:
                spdlog::error("  Status: Invalid input");
                spdlog::error("  Reason: {}", result.reason);
                break;
        }

        spdlog::info("");
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();


    spdlog::info("Execution finished. Total time: {} ms", duration_ms);

    spdlog::shutdown();
    std::cout << "Log saved to ../../data/checker_result/log_latest.log\n";
    return 0;
}
