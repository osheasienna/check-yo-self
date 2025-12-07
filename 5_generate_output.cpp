#include <string>
#include <fstream>
#include <iostream>

#include "include/move.h"

std::string move_to_uci(move m) {
    std::string lookup_files = "abcdefgh";

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
        if (m.promotion == QUEEN)  uci += 'q';
        if (m.promotion == ROOK)   uci += 'r';
        if (m.promotion == BISHOP) uci += 'b';
        if (m.promotion == KNIGHT) uci += 'n';  // 'n' for knight (not 'k' which is king)
    }

    return uci;
}

bool write_move_to_file(move m, std::string path) {
    // 1. Convert move to UCI string
    std::string uci = move_to_uci(m);

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
