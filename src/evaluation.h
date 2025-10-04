#ifndef EVALUATION_H
#define EVALUATION_H

#include "board.h"

// Piece values (centipawns)
extern const int PIECE_VALUES[13];

// Piece-square tables
extern int mg_pawn_table[64];
extern int mg_knight_table[64];
extern int mg_bishop_table[64];
extern int mg_rook_table[64];
extern int mg_queen_table[64];
extern int mg_king_table[64];

extern int eg_pawn_table[64];
extern int eg_knight_table[64];
extern int eg_bishop_table[64];
extern int eg_rook_table[64];
extern int eg_queen_table[64];
extern int eg_king_table[64];

void init_evaluation_tables();

class Evaluator {
public:
    static int evaluate(const Board& board);
    static int evaluate_material(const Board& board);
    static int evaluate_positional(const Board& board);
    static int evaluate_pawn_structure(const Board& board);
    static int evaluate_king_safety(const Board& board);
    static int evaluate_mobility(const Board& board);
    
private:
    static int get_game_phase(const Board& board);
    static int interpolate(int mg_score, int eg_score, int phase);
    static u64 get_pawn_attacks(Color color, u64 pawns);
};

#endif