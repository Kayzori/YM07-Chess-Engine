#include "board.h"
#include "moves.h"
#include "evaluation.h"
#include "search.h"
#include "uci.h"
#include "utils.h"
#include <iostream>

// Global instances
Board board;
Searcher searcher;

void init_engine() {
    init_zobrist();
    
    std::cout << "YM07 Chess Engine initialized" << std::endl;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    init_engine();
    
    // Test position
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    UCI uci(board, searcher);
    uci.run();
    
    return 0;
}