#include "evaluation.h"
#include "moves.h"

// Piece values (centipawns)
const int PIECE_VALUES[13] = {
    0, 
    100, 320, 330, 500, 900, 20000,  // white pieces
    -100, -320, -330, -500, -900, -20000  // black pieces
};

// Middle-game piece-square tables
int mg_pawn_table[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0
};

int mg_knight_table[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

int mg_bishop_table[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

int mg_rook_table[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   0,   0,   5,   5,   0,   0,   0
};

int mg_queen_table[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
     -5,   0,   5,   5,   5,   5,   0,  -5,
      0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20
};

int mg_king_table[64] = {
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
     20,  20,   0,   0,   0,   0,  20,  20,
     20,  30,  10,   0,   0,  10,  30,  20
};

// End-game tables (simplified - same as MG for now)
int eg_pawn_table[64];
int eg_knight_table[64];
int eg_bishop_table[64];
int eg_rook_table[64];
int eg_queen_table[64];
int eg_king_table[64];

void init_evaluation_tables() {
    // Initialize endgame tables (for now, same as middle game)
    for (int i = 0; i < 64; i++) {
        eg_pawn_table[i] = mg_pawn_table[i];
        eg_knight_table[i] = mg_knight_table[i];
        eg_bishop_table[i] = mg_bishop_table[i];
        eg_rook_table[i] = mg_rook_table[i];
        eg_queen_table[i] = mg_queen_table[i];
        eg_king_table[i] = mg_king_table[i];
    }
}

int Evaluator::evaluate(const Board& board) {
    int score = evaluate_material(board) + evaluate_positional(board);
    return (board.side_to_move == WHITE) ? score : -score;
}

int Evaluator::evaluate_material(const Board& board) {
    int score = 0;
    for (int p = 1; p <= 12; p++) {
        u64 bb = board.pieces[p];
        score += PIECE_VALUES[p] * popcount(bb);
    }
    return score;
}

int Evaluator::evaluate_positional(const Board& board) {
    int mg_score = 0, eg_score = 0;
    
    // Piece-square tables
    for (int p = 1; p <= 12; p++) {
        u64 bb = board.pieces[p];
        bool is_white = p <= 6;
        
        while (bb) {
            int sq = bit_scan_forward(bb);
            bb &= bb - 1;
            
            // Flip square for black pieces
            int eval_sq = is_white ? sq : 63 - sq;
            
            switch (p) {
                case WHITE_PAWN: case BLACK_PAWN:
                    mg_score += mg_pawn_table[eval_sq];
                    eg_score += eg_pawn_table[eval_sq];
                    break;
                case WHITE_KNIGHT: case BLACK_KNIGHT:
                    mg_score += mg_knight_table[eval_sq];
                    eg_score += eg_knight_table[eval_sq];
                    break;
                case WHITE_BISHOP: case BLACK_BISHOP:
                    mg_score += mg_bishop_table[eval_sq];
                    eg_score += eg_bishop_table[eval_sq];
                    break;
                case WHITE_ROOK: case BLACK_ROOK:
                    mg_score += mg_rook_table[eval_sq];
                    eg_score += eg_rook_table[eval_sq];
                    break;
                case WHITE_QUEEN: case BLACK_QUEEN:
                    mg_score += mg_queen_table[eval_sq];
                    eg_score += eg_queen_table[eval_sq];
                    break;
                case WHITE_KING: case BLACK_KING:
                    mg_score += mg_king_table[eval_sq];
                    eg_score += eg_king_table[eval_sq];
                    break;
            }
        }
    }
    
    int phase = get_game_phase(board);
    return interpolate(mg_score, eg_score, phase);
}

int Evaluator::get_game_phase(const Board& board) {
    // Simple game phase calculation based on material
    int phase = 0;
    phase += popcount(board.pieces[WHITE_QUEEN] | board.pieces[BLACK_QUEEN]) * 4;
    phase += popcount(board.pieces[WHITE_ROOK] | board.pieces[BLACK_ROOK]) * 2;
    phase += popcount(board.pieces[WHITE_BISHOP] | board.pieces[BLACK_BISHOP] | 
                     board.pieces[WHITE_KNIGHT] | board.pieces[BLACK_KNIGHT]);
    
    // Normalize to 0-256 range
    phase = std::min(24, phase) * 256 / 24;
    return phase;
}

int Evaluator::interpolate(int mg_score, int eg_score, int phase) {
    return (mg_score * phase + eg_score * (256 - phase)) / 256;
}

// Simplified versions for now
int Evaluator::evaluate_pawn_structure(const Board& board) { return 0; }
int Evaluator::evaluate_king_safety(const Board& board) { return 0; }
int Evaluator::evaluate_mobility(const Board& board) { return 0; }