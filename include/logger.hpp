#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

extern ofstream log_file;

inline string get_current_timestamp() {
    auto now = chrono::system_clock::now();
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = chrono::system_clock::to_time_t(now);
    tm bt = *localtime(&timer);
    ostringstream oss;
    oss << put_time(&bt, "%Y-%m-%d %H:%M:%S");
    oss << '.' << setfill('0') << setw(3) << ms.count();
    return oss.str();
}

inline void init_logger(const string& filename) {
    log_file.open(filename);
    if (!log_file.is_open()) {
        cerr << "[ERROR] Не удалось открыть лог-файл: " << filename << endl;
    }
}

inline void log_info(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [info] " + message;
    cerr << log_entry << endl;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

inline void log_error(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [error] " + message;
    cerr << log_entry << endl;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

inline void log_debug(const string& message) {
    string timestamp = get_current_timestamp();
    string log_entry = "[" + timestamp + "] [debug] " + message;
    if (log_file.is_open()) {
        log_file << log_entry << endl;
    }
}

#endif // LOGGER_HPP
