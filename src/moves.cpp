#include "moves.h"
#include "board.h"
#include <algorithm>

// Precomputed move tables
u64 knight_moves[64];
u64 king_moves[64];
u64 pawn_attacks[2][64];

void init_move_tables() {
    // Knight moves
    int knight_dx[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    int knight_dy[8] = {2, 1, -1, -2, -2, -1, 1, 2};
    
    for (int sq = 0; sq < 64; sq++) {
        int r = rank_of(sq), f = file_of(sq);
        knight_moves[sq] = 0;
        
        for (int i = 0; i < 8; i++) {
            int rr = r + knight_dy[i], ff = f + knight_dx[i];
            if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8)
                knight_moves[sq] |= 1ULL << (rr * 8 + ff);
        }
    }
    
    // King moves
    int king_dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int king_dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    
    for (int sq = 0; sq < 64; sq++) {
        int r = rank_of(sq), f = file_of(sq);
        king_moves[sq] = 0;
        
        for (int i = 0; i < 8; i++) {
            int rr = r + king_dy[i], ff = f + king_dx[i];
            if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8)
                king_moves[sq] |= 1ULL << (rr * 8 + ff);
        }
    }
    
    // Pawn attacks
    for (int sq = 0; sq < 64; sq++) {
        pawn_attacks[WHITE][sq] = 0;
        pawn_attacks[BLACK][sq] = 0;
        
        int r = rank_of(sq), f = file_of(sq);
        
        // White pawn attacks
        if (r < 7) {
            if (f > 0) pawn_attacks[WHITE][sq] |= 1ULL << (sq + 7);
            if (f < 7) pawn_attacks[WHITE][sq] |= 1ULL << (sq + 9);
        }
        
        // Black pawn attacks
        if (r > 0) {
            if (f > 0) pawn_attacks[BLACK][sq] |= 1ULL << (sq - 9);
            if (f < 7) pawn_attacks[BLACK][sq] |= 1ULL << (sq - 7);
        }
    }
}

std::string Move::to_uci() const {
    auto sq_name = [](int s) {
        std::string r;
        r.push_back('a' + file_of(s));
        r.push_back('1' + rank_of(s));
        return r;
    };
    
    std::string s = sq_name(from) + sq_name(to);
    if (promotion) {
        char pc = 'q';
        if (promotion == WHITE_ROOK || promotion == BLACK_ROOK) pc = 'r';
        else if (promotion == WHITE_KNIGHT || promotion == BLACK_KNIGHT) pc = 'n';
        else if (promotion == WHITE_BISHOP || promotion == BLACK_BISHOP) pc = 'b';
        s.push_back(pc);
    }
    return s;
}

u64 get_slider_attacks(int square, const Board& board, bool bishop, bool rook) {
    u64 attacks = 0;
    int r = rank_of(square), f = file_of(square);
    
    const int dirsR[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    const int dirsB[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    
    int dir_count = (bishop && rook) ? 8 : (bishop ? 4 : 4);
    
    for (int d = 0; d < dir_count; d++) {
        int dr, df;
        if (d < 4) {
            if (bishop) { dr = dirsB[d][0]; df = dirsB[d][1]; }
            else { dr = dirsR[d][0]; df = dirsR[d][1]; }
        } else {
            dr = dirsR[d-4][0]; df = dirsR[d-4][1];
        }
        
        int rr = r + dr, ff = f + df;
        while (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
            int s = rr * 8 + ff;
            attacks |= 1ULL << s;
            if (board.occupancies[2] & (1ULL << s)) break;
            rr += dr;
            ff += df;
        }
    }
    return attacks;
}

void MoveGenerator::generate_moves(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    generate_pawn_moves(board, moves);
    generate_knight_moves(board, moves);
    generate_bishop_moves(board, moves);
    generate_rook_moves(board, moves);
    generate_queen_moves(board, moves);
    generate_king_moves(board, moves);
    generate_castling_moves(board, moves);
}

void MoveGenerator::generate_captures(const Board& board, std::vector<Move>& moves) {
    generate_moves(board, moves);
    // Filter to only captures and promotions
    moves.erase(std::remove_if(moves.begin(), moves.end(), 
        [](const Move& m) { return !m.captured && !m.promotion; }), 
        moves.end());
}

bool MoveGenerator::is_move_legal(Board& board, const Move& move) {
    Board::UndoInfo undo = board.make_move(move);
    bool legal = !board.in_check((Color)(!board.side_to_move));
    board.undo_move(undo);
    return legal;
}

void MoveGenerator::generate_pawn_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    u64 pawns = board.pieces[stm == WHITE ? WHITE_PAWN : BLACK_PAWN];
    
    while (pawns) {
        int sq = bit_scan_forward(pawns);
        pawns &= pawns - 1;
        int r = rank_of(sq), f = file_of(sq);
        
        if (stm == WHITE) {
            // Single push
            int to = sq + 8;
            if (r < 7 && !(board.occupancies[2] & (1ULL << to))) {
                if (r == 6) { // Promotion
                    for (int promo : {WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT}) {
                        moves.push_back({sq, to, WHITE_PAWN, 0, promo, false, false});
                    }
                } else {
                    moves.push_back({sq, to, WHITE_PAWN, 0, 0, false, false});
                    // Double push from rank 2
                    if (r == 1) {
                        int to2 = sq + 16;
                        if (!(board.occupancies[2] & (1ULL << to2))) {
                            moves.push_back({sq, to2, WHITE_PAWN, 0, 0, false, false});
                        }
                    }
                }
            }
            
            // Captures
            if (f > 0) {
                int toL = sq + 7;
                if (r < 7) {
                    if (board.occupancies[BLACK] & (1ULL << toL)) {
                        int cap = 0;
                        for (int p = BLACK_PAWN; p <= BLACK_KING; p++) {
                            if (board.pieces[p] & (1ULL << toL)) {
                                cap = p;
                                break;
                            }
                        }
                        if (r == 6) {
                            for (int promo : {WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT}) {
                                moves.push_back({sq, toL, WHITE_PAWN, cap, promo, false, false});
                            }
                        } else {
                            moves.push_back({sq, toL, WHITE_PAWN, cap, 0, false, false});
                        }
                    }
                    if (board.enpassant_square == toL) {
                        moves.push_back({sq, toL, WHITE_PAWN, BLACK_PAWN, 0, true, false});
                    }
                }
            }
            
            if (f < 7) {
                int toR = sq + 9;
                if (r < 7) {
                    if (board.occupancies[BLACK] & (1ULL << toR)) {
                        int cap = 0;
                        for (int p = BLACK_PAWN; p <= BLACK_KING; p++) {
                            if (board.pieces[p] & (1ULL << toR)) {
                                cap = p;
                                break;
                            }
                        }
                        if (r == 6) {
                            for (int promo : {WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT}) {
                                moves.push_back({sq, toR, WHITE_PAWN, cap, promo, false, false});
                            }
                        } else {
                            moves.push_back({sq, toR, WHITE_PAWN, cap, 0, false, false});
                        }
                    }
                    if (board.enpassant_square == toR) {
                        moves.push_back({sq, toR, WHITE_PAWN, BLACK_PAWN, 0, true, false});
                    }
                }
            }
        } else { // BLACK pawns
            // Single push
            int to = sq - 8;
            if (r > 0 && !(board.occupancies[2] & (1ULL << to))) {
                if (r == 1) { // Promotion
                    for (int promo : {BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT}) {
                        moves.push_back({sq, to, BLACK_PAWN, 0, promo, false, false});
                    }
                } else {
                    moves.push_back({sq, to, BLACK_PAWN, 0, 0, false, false});
                    // Double push from rank 7
                    if (r == 6) {
                        int to2 = sq - 16;
                        if (!(board.occupancies[2] & (1ULL << to2))) {
                            moves.push_back({sq, to2, BLACK_PAWN, 0, 0, false, false});
                        }
                    }
                }
            }
            
            // Captures
            if (f > 0) {
                int toL = sq - 9;
                if (r > 0) {
                    if (board.occupancies[WHITE] & (1ULL << toL)) {
                        int cap = 0;
                        for (int p = WHITE_PAWN; p <= WHITE_KING; p++) {
                            if (board.pieces[p] & (1ULL << toL)) {
                                cap = p;
                                break;
                            }
                        }
                        if (r == 1) {
                            for (int promo : {BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT}) {
                                moves.push_back({sq, toL, BLACK_PAWN, cap, promo, false, false});
                            }
                        } else {
                            moves.push_back({sq, toL, BLACK_PAWN, cap, 0, false, false});
                        }
                    }
                    if (board.enpassant_square == toL) {
                        moves.push_back({sq, toL, BLACK_PAWN, WHITE_PAWN, 0, true, false});
                    }
                }
            }
            
            if (f < 7) {
                int toR = sq - 7;
                if (r > 0) {
                    if (board.occupancies[WHITE] & (1ULL << toR)) {
                        int cap = 0;
                        for (int p = WHITE_PAWN; p <= WHITE_KING; p++) {
                            if (board.pieces[p] & (1ULL << toR)) {
                                cap = p;
                                break;
                            }
                        }
                        if (r == 1) {
                            for (int promo : {BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT}) {
                                moves.push_back({sq, toR, BLACK_PAWN, cap, promo, false, false});
                            }
                        } else {
                            moves.push_back({sq, toR, BLACK_PAWN, cap, 0, false, false});
                        }
                    }
                    if (board.enpassant_square == toR) {
                        moves.push_back({sq, toR, BLACK_PAWN, WHITE_PAWN, 0, true, false});
                    }
                }
            }
        }
    }
}

void MoveGenerator::generate_knight_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    int knight_piece = (stm == WHITE) ? WHITE_KNIGHT : BLACK_KNIGHT;
    u64 knights = board.pieces[knight_piece];
    
    while (knights) {
        int sq = bit_scan_forward(knights);
        knights &= knights - 1;
        u64 targets = knight_moves[sq] & ~board.occupancies[stm];
        
        while (targets) {
            int to = bit_scan_forward(targets);
            targets &= targets - 1;
            
            int cap = 0;
            Color opp = (Color)(!stm);
            for (int p = (opp == WHITE ? WHITE_PAWN : BLACK_PAWN); 
                 p <= (opp == WHITE ? WHITE_KING : BLACK_KING); p++) {
                if (board.pieces[p] & (1ULL << to)) {
                    cap = p;
                    break;
                }
            }
            moves.push_back({sq, to, knight_piece, cap, 0, false, false});
        }
    }
}

void MoveGenerator::generate_bishop_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    int bishop_piece = (stm == WHITE) ? WHITE_BISHOP : BLACK_BISHOP;
    u64 bishops = board.pieces[bishop_piece];
    
    while (bishops) {
        int sq = bit_scan_forward(bishops);
        bishops &= bishops - 1;
        u64 attacks = get_slider_attacks(sq, board, true, false);
        u64 targets = attacks & ~board.occupancies[stm];
        
        while (targets) {
            int to = bit_scan_forward(targets);
            targets &= targets - 1;
            
            int cap = 0;
            Color opp = (Color)(!stm);
            for (int p = (opp == WHITE ? WHITE_PAWN : BLACK_PAWN); 
                 p <= (opp == WHITE ? WHITE_KING : BLACK_KING); p++) {
                if (board.pieces[p] & (1ULL << to)) {
                    cap = p;
                    break;
                }
            }
            moves.push_back({sq, to, bishop_piece, cap, 0, false, false});
        }
    }
}

void MoveGenerator::generate_rook_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    int rook_piece = (stm == WHITE) ? WHITE_ROOK : BLACK_ROOK;
    u64 rooks = board.pieces[rook_piece];
    
    while (rooks) {
        int sq = bit_scan_forward(rooks);
        rooks &= rooks - 1;
        u64 attacks = get_slider_attacks(sq, board, false, true);
        u64 targets = attacks & ~board.occupancies[stm];
        
        while (targets) {
            int to = bit_scan_forward(targets);
            targets &= targets - 1;
            
            int cap = 0;
            Color opp = (Color)(!stm);
            for (int p = (opp == WHITE ? WHITE_PAWN : BLACK_PAWN); 
                 p <= (opp == WHITE ? WHITE_KING : BLACK_KING); p++) {
                if (board.pieces[p] & (1ULL << to)) {
                    cap = p;
                    break;
                }
            }
            moves.push_back({sq, to, rook_piece, cap, 0, false, false});
        }
    }
}

void MoveGenerator::generate_queen_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    int queen_piece = (stm == WHITE) ? WHITE_QUEEN : BLACK_QUEEN;
    u64 queens = board.pieces[queen_piece];
    
    while (queens) {
        int sq = bit_scan_forward(queens);
        queens &= queens - 1;
        u64 attacks = get_slider_attacks(sq, board, true, true);
        u64 targets = attacks & ~board.occupancies[stm];
        
        while (targets) {
            int to = bit_scan_forward(targets);
            targets &= targets - 1;
            
            int cap = 0;
            Color opp = (Color)(!stm);
            for (int p = (opp == WHITE ? WHITE_PAWN : BLACK_PAWN); 
                 p <= (opp == WHITE ? WHITE_KING : BLACK_KING); p++) {
                if (board.pieces[p] & (1ULL << to)) {
                    cap = p;
                    break;
                }
            }
            moves.push_back({sq, to, queen_piece, cap, 0, false, false});
        }
    }
}

void MoveGenerator::generate_king_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    int king_piece = (stm == WHITE) ? WHITE_KING : BLACK_KING;
    u64 kings = board.pieces[king_piece];
    
    while (kings) {
        int sq = bit_scan_forward(kings);
        kings &= kings - 1;
        u64 targets = king_moves[sq] & ~board.occupancies[stm];
        
        while (targets) {
            int to = bit_scan_forward(targets);
            targets &= targets - 1;
            
            int cap = 0;
            Color opp = (Color)(!stm);
            for (int p = (opp == WHITE ? WHITE_PAWN : BLACK_PAWN); 
                 p <= (opp == WHITE ? WHITE_KING : BLACK_KING); p++) {
                if (board.pieces[p] & (1ULL << to)) {
                    cap = p;
                    break;
                }
            }
            moves.push_back({sq, to, king_piece, cap, 0, false, false});
        }
    }
}

void MoveGenerator::generate_castling_moves(const Board& board, std::vector<Move>& moves) {
    Color stm = board.side_to_move;
    
    if (stm == WHITE) {
        if ((board.castle_rights & 1) && // Kingside
            !(board.occupancies[2] & (0x60ULL)) && // Squares between
            board.pieces[WHITE_KING] & (1ULL << 60) && // King on e1
            !board.is_square_attacked(60, BLACK) &&
            !board.is_square_attacked(61, BLACK) &&
            !board.is_square_attacked(62, BLACK)) {
            moves.push_back({60, 62, WHITE_KING, 0, 0, false, true});
        }
        if ((board.castle_rights & 2) && // Queenside
            !(board.occupancies[2] & (0xEULL)) && // Squares between
            board.pieces[WHITE_KING] & (1ULL << 60) && // King on e1
            !board.is_square_attacked(60, BLACK) &&
            !board.is_square_attacked(59, BLACK) &&
            !board.is_square_attacked(58, BLACK)) {
            moves.push_back({60, 58, WHITE_KING, 0, 0, false, true});
        }
    } else {
        if ((board.castle_rights & 4) && // Kingside
            !(board.occupancies[2] & (0x6000000000000000ULL)) && // Squares between
            board.pieces[BLACK_KING] & (1ULL << 4) && // King on e8
            !board.is_square_attacked(4, WHITE) &&
            !board.is_square_attacked(5, WHITE) &&
            !board.is_square_attacked(6, WHITE)) {
            moves.push_back({4, 6, BLACK_KING, 0, 0, false, true});
        }
        if ((board.castle_rights & 8) && // Queenside
            !(board.occupancies[2] & (0xE00000000000000ULL)) && // Squares between
            board.pieces[BLACK_KING] & (1ULL << 4) && // King on e8
            !board.is_square_attacked(4, WHITE) &&
            !board.is_square_attacked(3, WHITE) &&
            !board.is_square_attacked(2, WHITE)) {
            moves.push_back({4, 2, BLACK_KING, 0, 0, false, true});
        }
    }
}

Move uci_to_move(const std::string& uci, const Board& board) {
    if (uci.length() < 4) return {-1, -1, 0, 0, 0, false, false};
    
    int from = (uci[0] - 'a') + (uci[1] - '1') * 8;
    int to = (uci[2] - 'a') + (uci[3] - '1') * 8;
    
    Move move;
    move.from = from;
    move.to = to;
    move.promotion = 0;
    move.isCastle = false;
    move.isEnPassant = false;
    move.captured = 0;
    
    // Find moving piece
    for (int p = 1; p <= 12; p++) {
        if (board.pieces[p] & (1ULL << from)) {
            move.piece = p;
            break;
        }
    }
    
    // Handle capture
    for (int p = 1; p <= 12; p++) {
        if (p != move.piece && (board.pieces[p] & (1ULL << to))) {
            move.captured = p;
            break;
        }
    }
    
    // Handle promotion
    if (uci.size() == 5) {
        char pc = uci[4];
        if (move.piece == WHITE_PAWN) {
            if (pc == 'q') move.promotion = WHITE_QUEEN;
            else if (pc == 'r') move.promotion = WHITE_ROOK;
            else if (pc == 'n') move.promotion = WHITE_KNIGHT;
            else if (pc == 'b') move.promotion = WHITE_BISHOP;
        } else if (move.piece == BLACK_PAWN) {
            if (pc == 'q') move.promotion = BLACK_QUEEN;
            else if (pc == 'r') move.promotion = BLACK_ROOK;
            else if (pc == 'n') move.promotion = BLACK_KNIGHT;
            else if (pc == 'b') move.promotion = BLACK_BISHOP;
        }
    }
    
    // Handle castling
    if ((move.piece == WHITE_KING || move.piece == BLACK_KING) && abs(from - to) == 2) {
        move.isCastle = true;
    }
    
    // Handle en passant
    if ((move.piece == WHITE_PAWN || move.piece == BLACK_PAWN) && 
        file_of(from) != file_of(to) && move.captured == 0) {
        move.isEnPassant = true;
        move.captured = (move.piece == WHITE_PAWN) ? BLACK_PAWN : WHITE_PAWN;
    }
    
    return move;
}