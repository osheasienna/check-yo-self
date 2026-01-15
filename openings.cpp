#include "openings.h"
#include <unordered_map>

// Opening book: maps move history (comma-separated) to best reply
// Format: "e2e4,e7e5,g1f3" -> next move to play
static const std::unordered_map<std::string, std::string> OPENING_BOOK = {
    // =========================================
    // WHITE OPENINGS (we play first)
    // =========================================
    
    // Starting position: play e4
    {"", "e2e4"},
    
    // After 1.e4 e5 - Italian/Scotch setup
    {"e2e4,e7e5", "g1f3"},                           // 2.Nf3
    {"e2e4,e7e5,g1f3,b8c6", "f1c4"},                 // 3.Bc4 (Italian)
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5", "c2c3"},      // 4.c3 (prep d4)
    {"e2e4,e7e5,g1f3,b8c6,f1c4,g8f6", "d2d3"},      // 4.d3 (solid)
    {"e2e4,e7e5,g1f3,g8f6", "b1c3"},                 // Petrov: 3.Nc3
    
    // After 1.e4 c5 - Open Sicilian
    {"e2e4,c7c5", "g1f3"},                           // 2.Nf3
    {"e2e4,c7c5,g1f3,d7d6", "d2d4"},                 // 3.d4
    {"e2e4,c7c5,g1f3,b8c6", "d2d4"},                 // 3.d4
    {"e2e4,c7c5,g1f3,e7e6", "d2d4"},                 // 3.d4
    
    // After 1.e4 e6 - French Defense
    {"e2e4,e7e6", "d2d4"},                           // 2.d4
    {"e2e4,e7e6,d2d4,d7d5", "b1c3"},                 // 3.Nc3
    
    // After 1.e4 c6 - Caro-Kann
    {"e2e4,c7c6", "d2d4"},                           // 2.d4
    {"e2e4,c7c6,d2d4,d7d5", "b1c3"},                 // 3.Nc3
    
    // After 1.e4 d5 - Scandinavian
    {"e2e4,d7d5", "e4d5"},                           // 2.exd5
    
    // =========================================
    // BLACK OPENINGS (opponent plays first)
    // =========================================
    
    // Against 1.e4 - play e5 (classical)
    {"e2e4", "e7e5"},
    {"e2e4,g1f3", "b8c6"},                           // 2...Nc6
    {"e2e4,f1c4", "g8f6"},                           // 2...Nf6 vs Bishop's Opening
    {"e2e4,d2d4", "e7e5"},                           // vs 2.d4?! play e5
    {"e2e4,b1c3", "g8f6"},                           // vs 2.Nc3 play Nf6
    
    // Against 1.e4 e5 2.Nf3 - continue development
    {"e2e4,e7e5,g1f3", "b8c6"},                      // 2...Nc6
    {"e2e4,e7e5,g1f3,b8c6,f1c4", "f8c5"},           // 3...Bc5 (Italian)
    {"e2e4,e7e5,g1f3,b8c6,f1b5", "a7a6"},           // 3...a6 (Morphy Defense)
    {"e2e4,e7e5,g1f3,b8c6,d2d4", "e5d4"},           // 3...exd4 (Scotch)
    
    // Against 1.d4 - King's Indian setup
    {"d2d4", "g8f6"},                                // 1...Nf6
    {"d2d4,g8f6,c2c4", "g7g6"},                      // 2...g6
    {"d2d4,g8f6,c2c4,g7g6,b1c3", "f8g7"},           // 3...Bg7
    {"d2d4,g8f6,g1f3", "g7g6"},                      // vs 2.Nf3, play g6
    {"d2d4,g8f6,c1f4", "g7g6"},                      // vs London, play g6
    
    // Against 1.c4 - English
    {"c2c4", "e7e5"},                                // 1...e5 (Reversed Sicilian)
    
    // Against 1.Nf3 - flexible
    {"g1f3", "d7d5"},                                // 1...d5
    
    // =========================================
    // COMMON CONTINUATIONS
    // =========================================
    
    // Italian Game continuations
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3", "g8f6"}, // 4...Nf6
    {"e2e4,e7e5,g1f3,b8c6,f1c4,f8c5,c2c3,g8f6,d2d4", "e5d4"}, // 5...exd4
    
    // Sicilian continuations  
    {"e2e4,c7c5,g1f3,d7d6,d2d4,c5d4", "f3d4"},      // 4.Nxd4
    {"e2e4,c7c5,g1f3,b8c6,d2d4,c5d4", "f3d4"},      // 4.Nxd4
};

// Helper: convert move history vector to lookup key
static std::string history_to_key(const std::vector<std::string>& move_history) {
    std::string key;
    for (size_t i = 0; i < move_history.size(); ++i) {
        if (i > 0) key += ",";
        key += move_history[i];
    }
    return key;
}

// Helper: parse UCI move string to move struct
static move parse_uci_move(const std::string& uci) {
    if (uci.length() < 4) {
        return move(0, 0, 0, 0);  // Invalid
    }
    
    int from_col = uci[0] - 'a';
    int from_row = uci[1] - '1';
    int to_col = uci[2] - 'a';
    int to_row = uci[3] - '1';
    
    promotion_piece_type promo = NONE;
    if (uci.length() >= 5) {
        switch (uci[4]) {
            case 'q': promo = QUEEN; break;
            case 'r': promo = ROOK; break;
            case 'b': promo = BISHOP; break;
            case 'n': promo = KNIGHT; break;
        }
    }
    
    return move(from_row, from_col, to_row, to_col, promo);
}

move get_book_move(const std::vector<std::string>& move_history) {
    // Don't use book after move 12 (24 half-moves)
    if (move_history.size() > 24) {
        return move(0, 0, 0, 0);
    }
    
    std::string key = history_to_key(move_history);
    
    auto it = OPENING_BOOK.find(key);
    if (it != OPENING_BOOK.end()) {
        return parse_uci_move(it->second);
    }
    
    // Not in book
    return move(0, 0, 0, 0);
}

bool is_opening_phase(int move_count) {
    // Opening is roughly the first 10-12 moves (20-24 half-moves)
    return move_count < 20;
}