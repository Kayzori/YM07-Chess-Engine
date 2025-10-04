#ifndef UCI_H
#define UCI_H

#include "board.h"
#include "search.h"
#include <string>
#include <sstream>

class UCI {
private:
    Board& board;
    Searcher& searcher;
    bool is_running = false;
    
    // Command handlers
    void handle_uci();
    void handle_isready();
    void handle_ucinewgame();
    void handle_position(std::stringstream& ss);
    void handle_go(std::stringstream& ss);
    void handle_stop();
    void handle_quit();
    void handle_setoption(std::stringstream& ss);
    void handle_debug(std::stringstream& ss);
    
    // Utility functions
    void print_board() const;
    void print_moves(const std::vector<Move>& moves) const;
    
public:
    UCI(Board& b, Searcher& s) : board(b), searcher(s) {}
    void run();
    void process_command(const std::string& command);
};

#endif