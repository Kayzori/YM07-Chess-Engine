#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

using u64 = uint64_t;

// Basic types
enum Color { WHITE = 0, BLACK = 1 };
enum Piece { 
    EMPTY = 0, 
    WHITE_PAWN = 1, WHITE_KNIGHT = 2, WHITE_BISHOP = 3, 
    WHITE_ROOK = 4, WHITE_QUEEN = 5, WHITE_KING = 6,
    BLACK_PAWN = 7, BLACK_KNIGHT = 8, BLACK_BISHOP = 9, 
    BLACK_ROOK = 10, BLACK_QUEEN = 11, BLACK_KING = 12 
};

// Pure C++ bitboard utilities (no compiler intrinsics)
inline int bit_scan_forward(u64 b) {
    if (b == 0) return 64;
    
    // De Bruijn multiplication method
    static const int index64[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    
    static const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
    return index64[((b & (~b + 1ULL)) * debruijn64) >> 58];
}

inline int popcount(u64 b) {
    // Brian Kernighan's algorithm
    int count = 0;
    while (b) {
        count++;
        b &= b - 1ULL;
    }
    return count;
}

inline u64 lsb(u64 b) { 
    return b & (~b + 1ULL);
}

// Square utilities
const int SQ_NONE = -1;
inline int sq_index(int rank, int file) { return rank * 8 + file; }
inline int rank_of(int sq) { return sq >> 3; }
inline int file_of(int sq) { return sq & 7; }

// Piece character conversion
int char_to_piece(char c);
char piece_to_char(int piece);

// Zobrist hashing
extern u64 zobrist_piece[13][64];
extern u64 zobrist_castle[16];
extern u64 zobrist_enpassant[64];
extern u64 zobrist_side;
void init_zobrist();
u64 compute_zobrist_key(const class Board& b);

#endif