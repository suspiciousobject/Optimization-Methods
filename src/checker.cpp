// ../src/checker.cpp

#include <bits/stdc++.h>

inline std::string parseLineToCol(const std::string& line) {
    std::string col;
    for (char c : line) {
        if (c == ' ' || c == '\t') continue;
        if (c == '-' || c == '\r' || c == '\n') break;
        if (c >= 'A' && c <= 'Z') col += c;
    }
    return col;
}

bool isSolvable(const std::vector<std::string>& cols, int N) {
    if (N <= 0) return false;
    int cnt[26] = {0};

    for (const auto& col : cols) {
        if (col.empty()) continue;
        if ((int)col.size() != N) return false;
        for (char c : col) {
            if (c >= 'A' && c <= 'Z') {
                ++cnt[c - 'A'];
            }
        }
    }

    for (int i = 0; i < 26; ++i) {
        if (cnt[i] > 0 && cnt[i] % N != 0) return false;
    }
    return true;
}

std::vector<std::string> parseFile(const std::string& filename) {
    std::ifstream f(filename, std::ios::in);
    std::vector<std::string> cols;
    std::string line;
    bool inData = false;

    while (getline(f, line)) {
        if (line.find("DATA") != std::string::npos) {
            inData = true;
            continue;
        }
        if (!inData) continue;

        if (line.find("/") != std::string::npos) break;

        if (line.find("ORDER") != std::string::npos) break;

        if (line.find("==") != std::string::npos) {
            cols.emplace_back("");
        } else {
            std::string col = parseLineToCol(line);
            if (!col.empty()) cols.emplace_back(std::move(col));
        }
    }
    return cols;
}

int getN(const std::vector<std::string>& cols) {
    for (const auto& col : cols)
        if (!col.empty()) return col.size();
    return 0;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const std::vector<std::string> files = {
        "../data/BIRDS_3.txt", "../data/BIRDS_4.txt", "../data/BIRDS_5.txt", "../data/BIRDS_6.txt", "../data/BIRDS_7.txt",
        "../data/BIRDS_8.txt", "../data/BIRDS_9.txt", "../data/BIRDS_10.txt", "../data/BIRDS_11.txt", "../data/BIRDS_12.txt",
        "../data/BIRDS_13.txt"
    };

    std::ofstream log("../data/checker_result/log_latest.txt");
    auto start = std::chrono::high_resolution_clock::now();

    for (const std::string& file : files) {
        auto cols = parseFile(file);
        if (cols.empty()) {
            log << file << ": BAD (parse error)\n";
            continue;
        }

        int N = getN(cols);
        bool ok = isSolvable(cols, N);
        log << file << ": " << (ok ? "OK" : "BAD") << '\n';
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    log << "\nProcessing time: " << ms << " ms\n";
    log.close();

    std::cout << "Анализ файлов завершён. Результат сохранён в ../data/checker_result/log_latest.txt\n";
    return 0;
}
