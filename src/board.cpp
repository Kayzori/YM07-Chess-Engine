#include "board.h"
#include "moves.h"
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

Board::Board() { 
    clear(); 
}

void Board::clear() {
    for (int i = 0; i < 13; i++) {
        pieces[i] = 0ULL;
    }
    for (int i = 0; i < 3; i++) {
        occupancies[i] = 0ULL;
    }
    side_to_move = WHITE;
    castle_rights = 0;
    enpassant_square = SQ_NONE;
    halfmove_clock = 0;
    fullmove_number = 1;
    zobrist_key = 0ULL;
}

void Board::set_from_fen(const std::string& fen) {
    clear();
    std::stringstream ss(fen);
    std::string board_str;
    ss >> board_str;

    int rank = 7;
    int file = 0;

    for (char c : board_str) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            file += c - '0';
        } else {
            int piece = char_to_piece(c);
            if (piece != EMPTY) {
                int square = rank * 8 + file;
                pieces[piece] |= (1ULL << square);
            }
            file++;
        }
    }

    update_occupancies();

    std::string color;
    ss >> color;
    side_to_move = (color == "w") ? WHITE : BLACK;

    std::string castling;
    ss >> castling;
    castle_rights = 0;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castle_rights |= 1; break;
                case 'Q': castle_rights |= 2; break;
                case 'k': castle_rights |= 4; break;
                case 'q': castle_rights |= 8; break;
            }
        }
    }

    std::string ep;
    ss >> ep;
    if (ep == "-") {
        enpassant_square = SQ_NONE;
    } else if (ep.length() == 2 && ep[0] >= 'a' && ep[0] <= 'h' && ep[1] >= '1' && ep[1] <= '8') {
        int ep_file = ep[0] - 'a';
        int ep_rank = ep[1] - '1';
        enpassant_square = ep_rank * 8 + ep_file;
    } else {
        enpassant_square = SQ_NONE;
    }

    std::string halfmove, fullmove;
    if (ss >> halfmove) {
        halfmove_clock = std::stoi(halfmove);
    }
    if (ss >> fullmove) {
        fullmove_number = std::stoi(fullmove);
    }

    zobrist_key = compute_zobrist_key(*this);
}

std::string Board::to_fen() const {
    std::stringstream fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empty_count = 0;
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            char piece_char = '.';
            for (int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
                if (pieces[piece] & (1ULL << square)) {
                    piece_char = piece_to_char(piece);
                    break;
                }
            }
            if (piece_char == '.') {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen << empty_count;
                    empty_count = 0;
                }
                fen << piece_char;
            }
        }
        if (empty_count > 0) {
            fen << empty_count;
        }
        if (rank > 0) {
            fen << '/';
        }
    }

    fen << (side_to_move == WHITE ? " w " : " b ");

    if (castle_rights == 0) {
        fen << "-";
    } else {
        if (castle_rights & 1) fen << 'K';
        if (castle_rights & 2) fen << 'Q';
        if (castle_rights & 4) fen << 'k';
        if (castle_rights & 8) fen << 'q';
    }

    fen << ' ';
    if (enpassant_square == SQ_NONE) {
        fen << '-';
    } else {
        fen << char('a' + file_of(enpassant_square));
        fen << char('1' + rank_of(enpassant_square));
    }

    fen << ' ' << halfmove_clock;
    fen << ' ' << fullmove_number;

    return fen.str();
}

void Board::update_occupancies() {
    occupancies[WHITE] = 0ULL;
    occupancies[BLACK] = 0ULL;
    for (int piece = WHITE_PAWN; piece <= WHITE_KING; piece++) {
        occupancies[WHITE] |= pieces[piece];
    }
    for (int piece = BLACK_PAWN; piece <= BLACK_KING; piece++) {
        occupancies[BLACK] |= pieces[piece];
    }
    occupancies[2] = occupancies[WHITE] | occupancies[BLACK];
}

Board::UndoInfo Board::make_move(const Move& move) {
    UndoInfo undo;
    
    // Store move components directly
    undo.from = move.from;
    undo.to = move.to;
    undo.piece = move.piece;
    undo.captured = move.captured;
    undo.promotion = move.promotion;
    undo.isEnPassant = move.isEnPassant;
    undo.isCastle = move.isCastle;
    
    // Save current state
    undo.castle_rights = castle_rights;
    undo.enpassant = enpassant_square;
    undo.halfmove = halfmove_clock;
    undo.zobrist_key = zobrist_key;

    pieces[move.piece] &= ~(1ULL << move.from);

    if (move.isEnPassant) {
        int capture_square = (side_to_move == WHITE) ? move.to - 8 : move.to + 8;
        pieces[move.captured] &= ~(1ULL << capture_square);
    } else if (move.captured != EMPTY) {
        pieces[move.captured] &= ~(1ULL << move.to);
    }

    if (move.promotion != EMPTY) {
        pieces[move.promotion] |= (1ULL << move.to);
    } else {
        pieces[move.piece] |= (1ULL << move.to);
    }

    if (move.isCastle) {
        if (move.to == 62) {
            pieces[WHITE_ROOK] &= ~(1ULL << 63);
            pieces[WHITE_ROOK] |= (1ULL << 61);
        } else if (move.to == 58) {
            pieces[WHITE_ROOK] &= ~(1ULL << 56);
            pieces[WHITE_ROOK] |= (1ULL << 59);
        } else if (move.to == 6) {
            pieces[BLACK_ROOK] &= ~(1ULL << 7);
            pieces[BLACK_ROOK] |= (1ULL << 5);
        } else if (move.to == 2) {
            pieces[BLACK_ROOK] &= ~(1ULL << 0);
            pieces[BLACK_ROOK] |= (1ULL << 3);
        }
    }

    update_occupancies();

    if (move.piece == WHITE_KING) {
        castle_rights &= ~(1 | 2);
    } else if (move.piece == BLACK_KING) {
        castle_rights &= ~(4 | 8);
    }

    if (move.piece == WHITE_ROOK) {
        if (move.from == 63) castle_rights &= ~1;
        if (move.from == 56) castle_rights &= ~2;
    } else if (move.piece == BLACK_ROOK) {
        if (move.from == 7) castle_rights &= ~4;
        if (move.from == 0) castle_rights &= ~8;
    }

    if (move.captured == WHITE_ROOK) {
        if (move.to == 63) castle_rights &= ~1;
        if (move.to == 56) castle_rights &= ~2;
    } else if (move.captured == BLACK_ROOK) {
        if (move.to == 7) castle_rights &= ~4;
        if (move.to == 0) castle_rights &= ~8;
    }

    enpassant_square = SQ_NONE;
    if (move.piece == WHITE_PAWN && (move.to - move.from) == 16) {
        enpassant_square = move.from + 8;
    } else if (move.piece == BLACK_PAWN && (move.from - move.to) == 16) {
        enpassant_square = move.from - 8;
    }

    if (move.piece == WHITE_PAWN || move.piece == BLACK_PAWN || move.captured != EMPTY) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    if (side_to_move == BLACK) {
        fullmove_number++;
    }

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;

    zobrist_key = compute_zobrist_key(*this);

    return undo;
}

void Board::undo_move(const UndoInfo& undo) {
    // Switch side back first
    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;
    
    halfmove_clock = undo.halfmove;
    if (side_to_move == BLACK) {
        fullmove_number--;
    }
    castle_rights = undo.castle_rights;
    enpassant_square = undo.enpassant;

    if (undo.promotion != EMPTY) {
        pieces[undo.promotion] &= ~(1ULL << undo.to);
    } else {
        pieces[undo.piece] &= ~(1ULL << undo.to);
    }
    pieces[undo.piece] |= (1ULL << undo.from);

    if (undo.isEnPassant) {
        int capture_square = (side_to_move == WHITE) ? undo.to - 8 : undo.to + 8;
        pieces[undo.captured] |= (1ULL << capture_square);
    } else if (undo.captured != EMPTY) {
        pieces[undo.captured] |= (1ULL << undo.to);
    }

    if (undo.isCastle) {
        if (undo.to == 62) {
            pieces[WHITE_ROOK] &= ~(1ULL << 61);
            pieces[WHITE_ROOK] |= (1ULL << 63);
        } else if (undo.to == 58) {
            pieces[WHITE_ROOK] &= ~(1ULL << 59);
            pieces[WHITE_ROOK] |= (1ULL << 56);
        } else if (undo.to == 6) {
            pieces[BLACK_ROOK] &= ~(1ULL << 5);
            pieces[BLACK_ROOK] |= (1ULL << 7);
        } else if (undo.to == 2) {
            pieces[BLACK_ROOK] &= ~(1ULL << 3);
            pieces[BLACK_ROOK] |= (1ULL << 0);
        }
    }

    update_occupancies();
    zobrist_key = undo.zobrist_key;
}

bool Board::is_square_attacked(int square, Color attacker) const {
    if (attacker == WHITE) {
        if (file_of(square) > 0 && rank_of(square) < 7 && 
            (pieces[WHITE_PAWN] & (1ULL << (square + 7)))) return true;
        if (file_of(square) < 7 && rank_of(square) < 7 && 
            (pieces[WHITE_PAWN] & (1ULL << (square + 9)))) return true;
    } else {
        if (file_of(square) > 0 && rank_of(square) > 0 && 
            (pieces[BLACK_PAWN] & (1ULL << (square - 9)))) return true;
        if (file_of(square) < 7 && rank_of(square) > 0 && 
            (pieces[BLACK_PAWN] & (1ULL << (square - 7)))) return true;
    }

    u64 knight_attacks = knight_moves[square];
    u64 attacker_knights = (attacker == WHITE) ? pieces[WHITE_KNIGHT] : pieces[BLACK_KNIGHT];
    if (knight_attacks & attacker_knights) return true;

    u64 king_attacks = king_moves[square];
    u64 attacker_king = (attacker == WHITE) ? pieces[WHITE_KING] : pieces[BLACK_KING];
    if (king_attacks & attacker_king) return true;

    u64 bishop_attacks = get_slider_attacks(square, *this, true, false);
    u64 attacker_bishops_queens = (attacker == WHITE) ? 
        (pieces[WHITE_BISHOP] | pieces[WHITE_QUEEN]) : 
        (pieces[BLACK_BISHOP] | pieces[BLACK_QUEEN]);
    if (bishop_attacks & attacker_bishops_queens) return true;

    u64 rook_attacks = get_slider_attacks(square, *this, false, true);
    u64 attacker_rooks_queens = (attacker == WHITE) ? 
        (pieces[WHITE_ROOK] | pieces[WHITE_QUEEN]) : 
        (pieces[BLACK_ROOK] | pieces[BLACK_QUEEN]);
    if (rook_attacks & attacker_rooks_queens) return true;

    return false;
}

bool Board::in_check(Color side) const {
    int king_square = SQ_NONE;
    u64 kings = (side == WHITE) ? pieces[WHITE_KING] : pieces[BLACK_KING];
    if (kings != 0) {
        king_square = bit_scan_forward(kings);
    }
    if (king_square == SQ_NONE) {
        return true;
    }
    return is_square_attacked(king_square, static_cast<Color>(!side));
}