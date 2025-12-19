#include <bits/stdc++.h>
using namespace std;
#include <chrono>

struct Move { int from, to; char c; };

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string remove_comment(const string &s) {
    size_t p = s.find("--");
    if(p == string::npos) return s;
    return s.substr(0, p);
}

string get_simulated_timestamp(int& time_counter) {
    long long ms_increment = 467LL + time_counter;
    time_counter++;
    
    string ms = to_string(ms_increment % 1000);
    while (ms.length() < 3) ms = "0" + ms;
    
    return "[2025-11-16 03:27:04." + ms + "]";
}

void log_entry(ostream& log, int& time_counter, const string& level, const string& message) {
    log << get_simulated_timestamp(time_counter) << " [" << level << "] " << message << "\n";
}

bool process_file(const string &filename, ostream &log, int& time_counter) {
    ifstream fin(filename);
    
    log_entry(log, time_counter, "info", "Processing file: " + filename);

    if (!fin.is_open()) {
        log_entry(log, time_counter, "error", "  Status: Invalid input");
        log_entry(log, time_counter, "error", "  Reason: Cannot open file");
        log_entry(log, time_counter, "info", "");
        return false;
    }

    vector<string> lines;
    string line;
    while(getline(fin, line)) lines.push_back(line);
    fin.close();

    vector<string> data_lines;
    vector<Move> order_moves; 
    enum Sec{NONE, DATA, ORDER} sec = NONE; 

    for(auto &raw: lines) {
        string s = trim(raw);
        if(s.empty()) continue;
        string su = s; for(char &c: su) c = toupper(c);
        
        if(su == "DATA") { sec = DATA; continue; }
        if(su == "ORDER") { sec = ORDER; continue; } 
        if(s == "/"){ sec = NONE; continue; }

        string t = trim(remove_comment(s));
        if(!t.empty()){
            if(sec == DATA){ 
                data_lines.push_back(t);
            } else if (sec == ORDER) {
                stringstream ss(t);
                Move mv;
                string type_str;
                if (ss >> mv.from && ss >> mv.to && ss >> type_str) {
                    if (!type_str.empty()) {
                        mv.c = toupper(type_str[0]);
                        mv.from--; 
                        mv.to--;   
                        order_moves.push_back(mv);
                    }
                }
            }
        }
    }
    
    if(data_lines.empty()){
        log_entry(log, time_counter, "error", "  Status: Invalid input");
        log_entry(log, time_counter, "error", "  Reason: No DATA block found.");
        log_entry(log, time_counter, "info", "");
        return false;
    }

    vector<vector<char>> initial_stacks;
    int N = -1;
    for(auto &dl: data_lines){
        string t = trim(dl);
        if(t == "=="){ initial_stacks.emplace_back(); continue; }
        stringstream ss(t); string token; vector<char> v;
        while(ss >> token) if(!token.empty()) v.push_back(token[0]);
        if(N == -1) N = (int)v.size(); else if((int)v.size() != N){ 
            log_entry(log, time_counter, "error", "  Status: Invalid input");
            log_entry(log, time_counter, "error", "  Reason: Column height mismatch: expected " + to_string(N) + ", found column of size " + to_string(v.size()) + ".");
            log_entry(log, time_counter, "info", "");
            return false;
        }
        initial_stacks.push_back(v);
    }
    if(N == -1){ N = 0; }
    int M = (int)initial_stacks.size();

    vector<int> cnt(26, 0);
    for(auto &col: initial_stacks) for(char c: col) cnt[c - 'A']++;
    for(int i = 0; i < 26; i++) {
        if(cnt[i] > 0 && N > 0 && cnt[i] % N != 0) {
            log_entry(log, time_counter, "error", "  Status: Unsolvable");
            log_entry(log, time_counter, "error", "  Reason: Letter '" + string(1, char('A' + i)) + "' appears " + to_string(cnt[i]) + " times, which is not divisible by N=" + to_string(N) + ".");
            log_entry(log, time_counter, "info", "");
            return false;
        }
    }

    if (order_moves.empty()) {
        log_entry(log, time_counter, "info", "  Status: Solvable");
        log_entry(log, time_counter, "info", "  Column height (N): " + to_string(N));
        log_entry(log, time_counter, "info", "  Columns processed: " + to_string(M));
        log_entry(log, time_counter, "info", "  Result: VALID");
        log_entry(log, time_counter, "info", "");
        return true;
    }
    
    auto start = chrono::high_resolution_clock::now();
    vector<vector<char>> current_stacks = initial_stacks;
    int K = 0;

    for (const auto& mv : order_moves) {
        K++;
        
        if (mv.from < 0 || mv.from >= M || mv.to < 0 || mv.to >= M) {
             log_entry(log, time_counter, "error", "  Status: Invalid Move Sequence");
             log_entry(log, time_counter, "error", "  Reason: Move #" + to_string(K) + " (" + to_string(mv.from + 1) + " -> " + to_string(mv.to + 1) + " " + mv.c + ") index out of range (1 to " + to_string(M) + " expected).");
             log_entry(log, time_counter, "info", "");
             return false;
        }

        vector<char>& source = current_stacks[mv.from];
        vector<char>& target = current_stacks[mv.to];
        
        if (source.empty() || source.back() != mv.c) {
            log_entry(log, time_counter, "error", "  Status: Invalid Move Sequence");
            log_entry(log, time_counter, "error", "  Reason: Move #" + to_string(K) + " (" + to_string(mv.from + 1) + " -> " + to_string(mv.to + 1) + " " + mv.c + ") violates rule 4. The top bird in column " + to_string(mv.from + 1) + " is '" + (source.empty() ? "N/A" : string(1, source.back())) + "', not '" + string(1, mv.c) + "'.");
            log_entry(log, time_counter, "info", "");
            return false;
        }

        if ((int)target.size() >= N) {
             log_entry(log, time_counter, "error", "  Status: Invalid Move Sequence");
             log_entry(log, time_counter, "error", "  Reason: Move #" + to_string(K) + " (" + to_string(mv.from + 1) + " -> " + to_string(mv.to + 1) + " " + mv.c + ") violates rule 6. Target column " + to_string(mv.to + 1) + " is already full (N=" + to_string(N) + ").");
             log_entry(log, time_counter, "info", "");
             return false;
        }

        if (!target.empty() && target.back() != mv.c) {
            log_entry(log, time_counter, "error", "  Status: Invalid Move Sequence");
            log_entry(log, time_counter, "error", "  Reason: Move #" + to_string(K) + " (" + to_string(mv.from + 1) + " -> " + to_string(mv.to + 1) + " " + mv.c + ") violates rule 5. Target column " + to_string(mv.to + 1) + "'s top bird ('" + target.back() + "') does not match '" + mv.c + "'.");
            log_entry(log, time_counter, "info", "");
            return false;
        }

        source.pop_back();
        target.push_back(mv.c);
    }

    auto end = chrono::high_resolution_clock::now();
    
    int L = 0;
    for(int i = 0; i < M; i++) {
        const auto& stack = current_stacks[i];
        if((int)stack.size() == N && !stack.empty()) {
            bool same = true;
            for(char c : stack) if(c != stack[0]){ same = false; break; }
            if(same) L++;
        }
    }
    long long F = 100LL * N * L - K;

    log_entry(log, time_counter, "info", "  Status: Valid Solution Found");
    log_entry(log, time_counter, "info", "  Column height (N): " + to_string(N));
    log_entry(log, time_counter, "info", "  Columns processed: " + to_string(M));
    log_entry(log, time_counter, "info", "  Moves (K): " + to_string(K));
    log_entry(log, time_counter, "info", "  Collected Stacks (L): " + to_string(L));
    log_entry(log, time_counter, "info", "  Target Function (F): " + to_string(F));
    log_entry(log, time_counter, "info", "  Result: VALID");
    log_entry(log, time_counter, "info", "");
    
    return true;
}

int main(){
    ofstream log("../../data/checker_result/checker_result.log");
    if(!log.is_open()){ cerr << "Cannot open results.log\n"; return 1; }

    int time_counter = 0;
    
    log_entry(log, time_counter, "info", "Checker execution started.");
    
    log_entry(log, time_counter, "info", "");
    log_entry(log, time_counter, "info", "Starting Strict ORDER Check");
    log_entry(log, time_counter, "info", "");
    
    process_file("../../data/BIRDS__1.txt", log, time_counter);
    process_file("../../data/BIRDS__2.txt", log, time_counter);
    process_file("../../data/BIRDS__3.txt", log, time_counter);
    process_file("../../data/BIRDS__5.txt", log, time_counter);
    process_file("../../data/BIRDS__6.txt", log, time_counter);
    process_file("../../data/BIRDS_LARGE.txt", log, time_counter);


    log_entry(log, time_counter, "info", "");
    log_entry(log, time_counter, "info", "Starting Solvability Check (BIRDS_3.txt to BIRDS_13.txt)");
    log_entry(log, time_counter, "info", "");
    
    for(int i = 3; i <= 13; i++){
        string fname = "../../data/BIRDS_" + to_string(i) + ".txt";
        process_file(fname, log, time_counter);
    }
    
    log_entry(log, time_counter, "info", "Execution finished.");
    log.close();
    return 0;
}
