#include <string>
#include <fstream>
#include <iostream>

#include "include/move.h"

std::string move_to_uci(move m) {
    std::string lookup_files = "abcdefgh";

    // sanity checks to avoid out-of-bounds indexing and badly-formatted output
    auto in_bounds = [](int x) { return 0 <= x && x < 8; };
    if (!in_bounds(m.from_col) || !in_bounds(m.to_col) || !in_bounds(m.from_row) || !in_bounds(m.to_row)) {
        return ""; // invalid move coordinates
    }

    // 1. Convert columns (files)
    char letter_from = lookup_files[m.from_col];
    char letter_to = lookup_files[m.to_col];

    // 2. Convert rows (ranks: 0..7 → '1'..'8')
    char rank_from = '1' + m.from_row;
    char rank_to = '1' + m.to_row;

    // 3. Build base string "e2e4"
    std::string uci;
    uci += letter_from;
    uci += rank_from;
    uci += letter_to;
    uci += rank_to;

    // 4. Add promotion letter if needed
    if (m.promotion != NONE) {
        switch (m.promotion) {
            case QUEEN:  uci += 'q'; break;
            case ROOK:   uci += 'r'; break;
            case BISHOP: uci += 'b'; break;
            case KNIGHT: uci += 'n'; break; // Standard UCI format: 'n' for knight
            default: return ""; // Unknown promotion code
        }
    }

    return uci;
}

bool write_move_to_file(move m, std::string path) {
    // 1. Convert move to UCI string
    std::string uci = move_to_uci(m);
    if (uci.empty()) {
        std::cerr << "Error: Could not encode move to UCI (invalid move)." << std::endl;
        return false;
    }

    // 2. Open the output file (overwrite mode)
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return false;
    }

    // 3. Write the move
    file << uci << std::endl;

    // 4. Close the file
    file.close();

    return true;
}
