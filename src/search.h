#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "moves.h"
#include <unordered_map>
#include <chrono>

struct SearchLimits {
    int depth = 6;
    int movetime = 0;
    int nodes = 0;
    bool infinite = false;
    std::chrono::steady_clock::time_point start_time;
};

struct SearchStats {
    int nodes = 0;
    int qnodes = 0;
    int tthits = 0;
    int depth = 0;
    int score = 0;
    Move best_move;
};

enum TTFlag { TT_EXACT, TT_ALPHA, TT_BETA };

struct TTEntry {
    u64 key;
    int depth;
    int value;
    TTFlag flag;
    Move best_move;
    int age;
};

class TranspositionTable {
private:
    std::unordered_map<u64, TTEntry> table;
    
public:
    int current_age = 0;
    void store(u64 key, int depth, int value, TTFlag flag, Move best_move);
    bool probe(u64 key, int depth, int& value, TTFlag& flag, Move& best_move);
    void clear();
    void set_age(int age) { current_age = age; }
    int size() const { return table.size(); }
};

class Searcher {
private:
    TranspositionTable tt;
    SearchStats stats;
    int history[2][64][64];
    Move killer_moves[100][2];
    bool stop_search = false;
    
    int quiescence(Board& board, int alpha, int beta, int depth);
    int alpha_beta(Board& board, int depth, int alpha, int beta, bool do_null);
    
    int score_move(const Move& move, const Move& tt_move, int depth) const;
    void order_moves(std::vector<Move>& moves, const Move& tt_move, int depth) const;
    
    bool is_repetition(const Board& board, int ply) const;
    bool stop_condition(const SearchLimits& limits) const;
    
public:
    SearchStats search(Board& board, const SearchLimits& limits);
    void stop() { stop_search = true; }
    void clear() { tt.clear(); stats = SearchStats(); }
    
    u64 perft(Board& board, int depth);
    u64 divide(Board& board, int depth);
};

#endif