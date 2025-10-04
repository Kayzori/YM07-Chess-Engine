#ifndef BOARD_H
#define BOARD_H

#include "utils.h"
#include <string>
#include <cstring>

// Forward declaration only
struct Move;

class Board {
public:
    u64 pieces[13];
    u64 occupancies[3];
    Color side_to_move;
    int castle_rights;
    int enpassant_square;
    int halfmove_clock;
    int fullmove_number;
    u64 zobrist_key;

    Board();
    void clear();
    void set_from_fen(const std::string& fen);
    std::string to_fen() const;
    
    struct UndoInfo {
        // Store move components directly
        int from;
        int to;
        int piece;
        int captured;
        int promotion;
        bool isEnPassant;
        bool isCastle;
        
        // Board state to restore
        int castle_rights;
        int enpassant;
        int halfmove;
        u64 zobrist_key;
    };
    
    UndoInfo make_move(const Move& move);
    void undo_move(const UndoInfo& undo);
    
    bool is_square_attacked(int square, Color attacker) const;
    bool in_check(Color side) const;
    
private:
    void update_occupancies();
};

#endif