#include "search.h"
#include "evaluation.h"
#include <algorithm>
#include <chrono>

void TranspositionTable::store(u64 key, int depth, int value, TTFlag flag, Move best_move) {
    TTEntry entry;
    entry.key = key;
    entry.depth = depth;
    entry.value = value;
    entry.flag = flag;
    entry.best_move = best_move;
    entry.age = current_age;
    table[key] = entry;
}

bool TranspositionTable::probe(u64 key, int depth, int& value, TTFlag& flag, Move& best_move) {
    auto it = table.find(key);
    if (it == table.end()) return false;
    
    TTEntry& entry = it->second;
    if (entry.depth >= depth) {
        value = entry.value;
        flag = entry.flag;
        best_move = entry.best_move;
        return true;
    }
    return false;
}

void TranspositionTable::clear() {
    table.clear();
    current_age = 0;
}

SearchStats Searcher::search(Board& board, const SearchLimits& limits) {
    stats = SearchStats();
    stop_search = false;
    tt.set_age(tt.current_age + 1);
    
    Move best_move;
    int best_score = -1000000;
    
    for (int depth = 1; depth <= limits.depth && !stop_search; depth++) {
        stats.depth = depth;
        
        int score = alpha_beta(board, depth, -1000000, 1000000, true);
        
        if (!stop_search && score > best_score) {
            best_score = score;
            stats.score = score;
            
            int tt_value;
            TTFlag tt_flag;
            Move tt_move;
            if (tt.probe(board.zobrist_key, depth, tt_value, tt_flag, tt_move)) {
                best_move = tt_move;
                stats.best_move = tt_move;
            }
        }
        
        std::cerr << "info depth " << depth << " score cp " << score 
                  << " nodes " << stats.nodes << std::endl;
        
        if (stop_condition(limits)) break;
    }
    
    return stats;
}

int Searcher::alpha_beta(Board& board, int depth, int alpha, int beta, bool do_null) {
    stats.nodes++;
    
    if (is_repetition(board, depth)) {
        return 0;
    }
    
    int tt_value;
    TTFlag tt_flag;
    Move tt_move;
    bool tt_hit = tt.probe(board.zobrist_key, depth, tt_value, tt_flag, tt_move);
    if (tt_hit) {
        stats.tthits++;
        if (tt_flag == TT_EXACT) return tt_value;
        if (tt_flag == TT_ALPHA && tt_value <= alpha) return alpha;
        if (tt_flag == TT_BETA && tt_value >= beta) return beta;
    }
    
    if (depth <= 0) {
        return quiescence(board, alpha, beta, 0);
    }
    
    if (do_null && depth >= 3 && !board.in_check(board.side_to_move)) {
        // Create a null move
        Move null_move = {0, 0, 0, 0, 0, false, false};
        Board::UndoInfo undo = board.make_move(null_move);
        int null_score = -alpha_beta(board, depth - 3, -beta, -beta + 1, false);
        board.undo_move(undo);
        
        if (null_score >= beta) return beta;
    }
    
    std::vector<Move> moves;
    MoveGenerator::generate_moves(board, moves);
    
    if (moves.empty()) {
        if (board.in_check(board.side_to_move)) {
            return -1000000 + (100 - depth);
        } else {
            return 0;
        }
    }
    
    order_moves(moves, tt_move, depth);
    
    int best_value = -1000000;
    Move best_move = moves[0];
    int moves_searched = 0;
    
    for (const Move& move : moves) {
        Board::UndoInfo undo = board.make_move(move);
        
        if (board.in_check((Color)(!board.side_to_move))) {
            board.undo_move(undo);
            continue;
        }
        
        int score;
        if (moves_searched == 0) {
            score = -alpha_beta(board, depth - 1, -beta, -alpha, true);
        } else {
            int reduction = (depth >= 3 && moves_searched >= 4) ? 1 : 0;
            score = -alpha_beta(board, depth - 1 - reduction, -alpha - 1, -alpha, true);
            
            if (score > alpha) {
                score = -alpha_beta(board, depth - 1, -beta, -alpha, true);
            }
        }
        
        board.undo_move(undo);
        moves_searched++;
        
        if (score > best_value) {
            best_value = score;
            best_move = move;
        }
        
        if (score > alpha) {
            alpha = score;
        }
        
        if (alpha >= beta) {
            if (!move.captured) {
                killer_moves[depth][1] = killer_moves[depth][0];
                killer_moves[depth][0] = move;
            }
            break;
        }
    }
    
    TTFlag flag = TT_EXACT;
    if (best_value <= alpha) flag = TT_ALPHA;
    else if (best_value >= beta) flag = TT_BETA;
    
    tt.store(board.zobrist_key, depth, best_value, flag, best_move);
    
    return best_value;
}

int Searcher::quiescence(Board& board, int alpha, int beta, int depth) {
    stats.qnodes++;
    
    int stand_pat = Evaluator::evaluate(board);
    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;
    
    if (depth >= 8) return stand_pat;
    
    std::vector<Move> moves;
    MoveGenerator::generate_captures(board, moves);
    
    for (const Move& move : moves) {
        Board::UndoInfo undo = board.make_move(move);
        
        if (board.in_check((Color)(!board.side_to_move))) {
            board.undo_move(undo);
            continue;
        }
        
        int score = -quiescence(board, -beta, -alpha, depth + 1);
        board.undo_move(undo);
        
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    
    return alpha;
}

void Searcher::order_moves(std::vector<Move>& moves, const Move& tt_move, int depth) const {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return score_move(a, tt_move, depth) > score_move(b, tt_move, depth);
    });
}

int Searcher::score_move(const Move& move, const Move& tt_move, int depth) const {
    if (move == tt_move) return 10000;
    
    if (move.captured) {
        return 9000 + PIECE_VALUES[move.captured] - PIECE_VALUES[move.piece] / 10;
    }
    
    for (int i = 0; i < 2; i++) {
        if (move == killer_moves[depth][i]) return 8000 - i;
    }
    
    int history_score = 0; // Simplified for now
    if (move.promotion) return 7000 + PIECE_VALUES[move.promotion];
    
    return history_score;
}

bool Searcher::is_repetition(const Board& board, int ply) const {
    (void)board; (void)ply;
    return false;
}

bool Searcher::stop_condition(const SearchLimits& limits) const {
    if (stop_search) return true;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - limits.start_time);
    
    if (limits.movetime > 0 && elapsed.count() >= limits.movetime) return true;
    if (limits.nodes > 0 && stats.nodes >= limits.nodes) return true;
    
    return false;
}

u64 Searcher::perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    std::vector<Move> moves;
    MoveGenerator::generate_moves(board, moves);
    
    u64 nodes = 0;
    for (const Move& move : moves) {
        Board::UndoInfo undo = board.make_move(move);
        if (!board.in_check((Color)(!board.side_to_move))) {
            nodes += perft(board, depth - 1);
        }
        board.undo_move(undo);
    }
    return nodes;
}

u64 Searcher::divide(Board& board, int depth) {
    std::vector<Move> moves;
    MoveGenerator::generate_moves(board, moves);
    
    u64 total = 0;
    for (const Move& move : moves) {
        Board::UndoInfo undo = board.make_move(move);
        if (!board.in_check((Color)(!board.side_to_move))) {
            u64 count = (depth == 1) ? 1 : perft(board, depth - 1);
            std::cout << move.to_uci() << ": " << count << std::endl;
            total += count;
        }
        board.undo_move(undo);
    }
    std::cout << "Total: " << total << std::endl;
    return total;
}