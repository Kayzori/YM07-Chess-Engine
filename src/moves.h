#ifndef MOVES_H
#define MOVES_H

#include "utils.h"
#include <vector>

// Forward declaration
class Board;

struct Move {
    int from, to;
    int piece;
    int captured;
    int promotion;
    bool isEnPassant;
    bool isCastle;
    
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && 
               piece == other.piece && promotion == other.promotion;
    }
    
    std::string to_uci() const;
};

// Precomputed move tables
extern u64 knight_moves[64];
extern u64 king_moves[64];
extern u64 pawn_attacks[2][64]; // [color][square]

void init_move_tables();
u64 get_slider_attacks(int square, const Board& board, bool bishop, bool rook);

class MoveGenerator {
public:
    static void generate_moves(const Board& board, std::vector<Move>& moves);
    static void generate_captures(const Board& board, std::vector<Move>& moves);
    static bool is_move_legal(Board& board, const Move& move);
    
private:
    static void generate_pawn_moves(const Board& board, std::vector<Move>& moves);
    static void generate_knight_moves(const Board& board, std::vector<Move>& moves);
    static void generate_bishop_moves(const Board& board, std::vector<Move>& moves);
    static void generate_rook_moves(const Board& board, std::vector<Move>& moves);
    static void generate_queen_moves(const Board& board, std::vector<Move>& moves);
    static void generate_king_moves(const Board& board, std::vector<Move>& moves);
    static void generate_castling_moves(const Board& board, std::vector<Move>& moves);
};

// UCI move conversion
Move uci_to_move(const std::string& uci, const Board& board);

#endif