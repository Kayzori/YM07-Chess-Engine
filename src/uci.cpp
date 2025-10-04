#include "uci.h"
#include "evaluation.h"
#include <iostream>

void UCI::run() {
    is_running = true;
    std::cout << "YM07 Chess Engine" << std::endl;
    
    std::string line;
    while (is_running && std::getline(std::cin, line)) {
        process_command(line);
    }
}

void UCI::process_command(const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    ss >> token;
    
    if (token == "uci") {
        handle_uci();
    } else if (token == "isready") {
        handle_isready();
    } else if (token == "ucinewgame") {
        handle_ucinewgame();
    } else if (token == "position") {
        handle_position(ss);
    } else if (token == "go") {
        handle_go(ss);
    } else if (token == "stop") {
        handle_stop();
    } else if (token == "quit") {
        handle_quit();
    } else if (token == "setoption") {
        handle_setoption(ss);
    } else if (token == "debug") {
        handle_debug(ss);
    } else if (token == "print") {
        print_board();
    } else if (token == "eval") {
        std::cout << "eval: " << Evaluator::evaluate(board) << std::endl;
    } else if (!token.empty()) {
        std::cout << "Unknown command: " << token << std::endl;
    }
}

void UCI::handle_uci() {
    std::cout << "id name YM07 Chess Engine" << std::endl;
    std::cout << "id author Kayzori" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::handle_isready() {
    std::cout << "readyok" << std::endl;
}

void UCI::handle_ucinewgame() {
    searcher.clear();
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void UCI::handle_position(std::stringstream& ss) {
    std::string token;
    ss >> token;
    
    if (token == "startpos") {
        board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } else if (token == "fen") {
        std::string fen;
        ss >> fen;
        board.set_from_fen(fen);
    }
    
    // Parse moves
    else if (token == "moves") {
        while (ss >> token) {
            Move move = uci_to_move(token, board);
            if (move.from != -1)board.make_move(move);
        }
    }
}

void UCI::handle_go(std::stringstream& ss) {
    SearchLimits limits;
    limits.start_time = std::chrono::steady_clock::now();
    
    std::string token;
    while (ss >> token) {
        if (token == "depth") {
            ss >> limits.depth;
        } else if (token == "movetime") {
            ss >> limits.movetime;
        } else if (token == "nodes") {
            ss >> limits.nodes;
        } else if (token == "infinite") {
            limits.infinite = true;
        }
    }
    
    SearchStats stats = searcher.search(board, limits);
    std::cout << "bestmove " << stats.best_move.to_uci() << std::endl;
}

void UCI::handle_stop() {
    searcher.stop();
}

void UCI::handle_quit() {
    is_running = false;
}

void UCI::handle_setoption(std::stringstream& ss) {
    // Option handling not implemented
    std::string token;
    while (ss >> token) {
        // Parse options
    }
}

void UCI::handle_debug(std::stringstream& ss) {
    std::string token;
    ss >> token;
    // Debug handling
}

void UCI::print_board() const {
    for (int r = 7; r >= 0; r--) {
        std::cout << r + 1 << " ";
        for (int f = 0; f < 8; f++) {
            int sq = r * 8 + f;
            char pc = '.';
            for (int p = 1; p <= 12; p++) {
                if (board.pieces[p] & (1ULL << sq)) {
                    pc = piece_to_char(p);
                    break;
                }
            }
            std::cout << pc << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "  a b c d e f g h" << std::endl;
    std::cout << "FEN: " << board.to_fen() << std::endl;
    std::cout << "Side: " << (board.side_to_move == WHITE ? "white" : "black") << std::endl;
}

void UCI::print_moves(const std::vector<Move>& moves) const {
    for (const auto& move : moves) {
        std::cout << move.to_uci() << " ";
    }
    std::cout << std::endl;
}