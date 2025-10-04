#include "utils.h"
#include "board.h"
#include <random>

// Zobrist keys
u64 zobrist_piece[13][64];
u64 zobrist_castle[16];
u64 zobrist_enpassant[64];
u64 zobrist_side;
std::mt19937_64 rng(0xC0FFEE123456ULL);

void init_zobrist() {
    for (int p = 0; p < 13; p++)
        for (int s = 0; s < 64; s++)
            zobrist_piece[p][s] = rng();
    for (int i = 0; i < 16; i++)
        zobrist_castle[i] = rng();
    for (int s = 0; s < 64; s++)
        zobrist_enpassant[s] = rng();
    zobrist_side = rng();
}

u64 compute_zobrist_key(const Board& b) {
    u64 h = 0;
    for (int p = 1; p <= 12; p++) {
        u64 bb = b.pieces[p];
        while (bb) { 
            int sq = bit_scan_forward(bb); 
            h ^= zobrist_piece[p][sq]; 
            bb &= bb - 1ULL; 
        }
    }
    if (b.enpassant_square != SQ_NONE) 
        h ^= zobrist_enpassant[b.enpassant_square];
    h ^= zobrist_castle[b.castle_rights];
    if (b.side_to_move == BLACK) 
        h ^= zobrist_side;
    return h;
}

int char_to_piece(char c) {
    switch (c) {
        case 'P': return WHITE_PAWN;
        case 'N': return WHITE_KNIGHT;
        case 'B': return WHITE_BISHOP;
        case 'R': return WHITE_ROOK;
        case 'Q': return WHITE_QUEEN;
        case 'K': return WHITE_KING;
        case 'p': return BLACK_PAWN;
        case 'n': return BLACK_KNIGHT;
        case 'b': return BLACK_BISHOP;
        case 'r': return BLACK_ROOK;
        case 'q': return BLACK_QUEEN;
        case 'k': return BLACK_KING;
        default: return EMPTY;
    }
}

char piece_to_char(int piece) {
    switch (piece) {
        case WHITE_PAWN: return 'P';
        case WHITE_KNIGHT: return 'N';
        case WHITE_BISHOP: return 'B';
        case WHITE_ROOK: return 'R';
        case WHITE_QUEEN: return 'Q';
        case WHITE_KING: return 'K';
        case BLACK_PAWN: return 'p';
        case BLACK_KNIGHT: return 'n';
        case BLACK_BISHOP: return 'b';
        case BLACK_ROOK: return 'r';
        case BLACK_QUEEN: return 'q';
        case BLACK_KING: return 'k';
        default: return '.';
    }
}