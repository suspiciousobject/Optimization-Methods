#include <bits/stdc++.h>
using namespace std;
#include <chrono>

struct Move { int from, to; char c; };

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);
}

string remove_comment(const string &s) {
    size_t p = s.find("--");
    if(p==string::npos) return s;
    return s.substr(0,p);
}

// Обработка одного файла
bool process_file(const string &filename, ostream &log) {
    ifstream fin(filename);

    vector<string> lines;
    string line;
    while(getline(fin,line)) lines.push_back(line);
    fin.close();

    vector<string> data_lines;
    enum Sec{NONE, DATA} sec=NONE;
    for(auto &raw: lines) {
        string s=trim(raw);
        if(s.empty()) continue;
        string su=s; for(char &c: su) c=toupper(c);
        if(su=="DATA"){ sec=DATA; continue; }
        if(s=="/"){ sec=NONE; continue; }
        if(sec==DATA){ string t=trim(remove_comment(s)); if(!t.empty()) data_lines.push_back(t); }
    }

    if(data_lines.empty()){ log<<filename<<": No DATA\n"; return false; }

    // Разбор колонок
    vector<vector<char>> stacks;
    int N=-1;
    for(auto &dl: data_lines){
        string t=trim(dl);
        if(t=="=="){ stacks.emplace_back(); continue; }
        stringstream ss(t); string token; vector<char> v;
        while(ss>>token) if(!token.empty()) v.push_back(token[0]);
        if(N==-1) N=(int)v.size(); else if((int)v.size()!=N){ log<<filename<<": INVALID INPUT\n"; return false; }
        stacks.push_back(v);
    }
    if(N==-1){ log<<filename<<": empty data\n"; return true; }
    int M=(int)stacks.size();

    // Алгоритм
    auto start = chrono::high_resolution_clock::now();

    vector<int> cnt(26,0);
    for(auto &col: stacks) for(char c: col) cnt[c-'A']++;
    for(int i=0;i<26;i++) if(cnt[i]>0 && cnt[i]%N!=0){ log<<filename<<": NO SOLUTION species "<<char('A'+i)<<"\n"; return false; }

    vector<int> needCols(26,0); int totalNeeded=0;
    for(int i=0;i<26;i++){ needCols[i]=cnt[i]/N; totalNeeded+=needCols[i]; }
    if(totalNeeded>M){ log<<filename<<": NO SOLUTION, not enough columns\n"; return false; }

    vector<int> targetOf(M,-1), assigned(26,0);
    for(int i=0;i<M;i++){ 
        if(stacks[i].empty()){ 
            for(int s=0;s<26;s++) if(needCols[s]>assigned[s]){ targetOf[i]=s; assigned[s]++; break; } 
        } 
    }
    for(int i=0;i<M;i++){ 
        if(targetOf[i]==-1){ 
            for(int s=0;s<26;s++) if(needCols[s]>assigned[s]){ targetOf[i]=s; assigned[s]++; break; } 
        } 
    }

    auto can_place=[&](int j,char c){
        if(j<0||j>=M||(int)stacks[j].size()>=N) return false;
        return stacks[j].empty() || stacks[j].back()==c;
    };

    // Умный выбор буферной колонки
    auto find_temp_smart=[&](char ch,const vector<int>& avoid)->int{
        vector<char> avoidFlag(M,0);
        for(int a: avoid) if(a>=0&&a<M) avoidFlag[a]=1;

        // 1. Пустые колонки
        for(int i=0;i<M;i++) if(!avoidFlag[i] && stacks[i].empty()) return i;

        // 2. Колонки с вершиной того же вида
        for(int i=0;i<M;i++) 
            if(!avoidFlag[i] && !stacks[i].empty() && stacks[i].back()==ch && (int)stacks[i].size()<N) return i;

        // 3. Колонки, не являющиеся целевыми для других видов
        for(int i=0;i<M;i++)
            if(!avoidFlag[i] && (int)stacks[i].size()<N && (targetOf[i]==-1 || targetOf[i]==(ch-'A'))) return i;

        // 4. Любая колонка с доступным местом
        for(int i=0;i<M;i++) if(!avoidFlag[i] && (int)stacks[i].size()<N) return i;

        return -1;
    };

    vector<Move> moves;

    function<bool(int,char)> uncover_top=[&](int col,char needChar)->bool{
        int maxsteps=10000000, safety=0;
        while(stacks[col].back()!=needChar){
            if(++safety>maxsteps) return false;
            char top=stacks[col].back();
            vector<int> avoid={col};
            int dest=find_temp_smart(top,avoid);
            if(dest==-1) return false;
            stacks[col].pop_back();
            stacks[dest].push_back(top);
            moves.push_back({col,dest,top});
        }
        return true;
    };

    // Основная сборка
    for(int s=0;s<26;s++){
        int need=needCols[s]; if(need==0) continue;
        vector<int> targets;
        for(int i=0;i<M;i++) if(targetOf[i]==s) targets.push_back(i);
        for(int i=0;i<M && (int)targets.size()<need; ++i) if(targetOf[i]==-1){ targetOf[i]=s; targets.push_back(i); }
        for(int tcol: targets){
            while((int)stacks[tcol].size()<N){
                char want='A'+s;
                int src=-1;
                for(int i=0;i<M;i++) if(i!=tcol && !stacks[i].empty() && stacks[i].back()==want){ src=i; break; }
                if(src!=-1){
                    if(!can_place(tcol,want)){
                        char topTarget=stacks[tcol].back();
                        int dest2=find_temp_smart(topTarget,{tcol,src});
                        if(dest2==-1){ log<<filename<<": NO SOLUTION\n"; return false; }
                        stacks[tcol].pop_back();
                        stacks[dest2].push_back(topTarget);
                        moves.push_back({tcol,dest2,topTarget});
                        continue;
                    }
                    stacks[src].pop_back();
                    stacks[tcol].push_back(want);
                    moves.push_back({src,tcol,want});
                    continue;
                }

                int col_with_want=-1;
                for(int i=0;i<M;i++){
                    if(i==tcol) continue;
                    for(int p=(int)stacks[i].size()-1;p>=0;--p) if(stacks[i][p]==want){ col_with_want=i; break; }
                    if(col_with_want!=-1) break;
                }
                if(col_with_want==-1){ log<<filename<<": INTERNAL ERROR no source for "<<want<<"\n"; return false; }
                if(!uncover_top(col_with_want,want)){ log<<filename<<": CANNOT UNCOVER "<<want<<"\n"; return false; }
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> diff=end-start;

    // Финальная сводка
    vector<vector<char>> st2;
    for(auto &dl: data_lines){
        string t=trim(dl);
        if(t=="=="){ st2.emplace_back(); continue; }
        stringstream ss(t); string token; vector<char> v;
        while(ss>>token) if(!token.empty()) v.push_back(token[0]);
        st2.push_back(v);
    }

    // Cимуляция ходов
    for(auto &mv: moves){
        st2[mv.to].push_back(mv.c);
        st2[mv.from].pop_back();
    }

    int K=(int)moves.size();
    int L=0;
    for(int i=0;i<M;i++){
        if((int)st2[i].size()==N && !st2[i].empty()){
            bool same=true;
            for(char c: st2[i]) if(c!=st2[i][0]){ same=false; break; }
            if(same) L++;
        }
    }
    long long F=100LL*N*L-K;

    log<<filename<<": moves="<<K<<", N="<<N<<", L="<<L<<", F="<<F
        <<", time="<<diff.count()<<" s\n";

    return true;
}

// Главная функция
int main(){
    ofstream log("../../data/solve/results.log");
    if(!log.is_open()){ cerr<<"Cannot open results.log\n"; return 1; }

    for(int i=3;i<=13;i++){
        string fname="../../data/BIRDS_"+to_string(i)+".txt";
        process_file(fname,log);
    }

    log.close();
    return 0;
}
