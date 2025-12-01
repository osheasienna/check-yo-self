#ifndef MOVE_H
#define MOVE_H

#include <string>

enum promotion_piece_type {
    NONE,
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT
};

struct move {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    promotion_piece_type promotion;

    move(int from_r, int from_c, int to_r, int to_c, promotion_piece_type promo = NONE)
        : from_row(from_r), from_col(from_c), to_row(to_r), to_col(to_c), promotion(promo) {}
};

std::string move_to_uci(move m) {
    // Step 1: Convert column index to letter
    std::string lookup = "abcdefgh";
    char letter_from = lookup[m.from_col];
    char letter_to = lookup[m.to_col];

    // Step 2: Convert row index to digit (0-7 → 1-8)
    char rank_from = '1' + m.from_row;
    char rank_to = '1' + m.to_row;

    // Step 3: Build the base string
    std::string uci;
    uci += letter_from;
    uci += rank_from;
    uci += letter_to;
    uci += rank_to;

    // Step 4: Add promotion letter if needed
    if (m.promotion != NONE) {
        if (m.promotion == QUEEN)  uci += 'q';
        if (m.promotion == ROOK)   uci += 'r';
        if (m.promotion == BISHOP) uci += 'b';
        if (m.promotion == KNIGHT) uci += 'k';
    }

    return uci;
}

bool write_move_to_file(move m, std::string path) {
    // Convert move to UCI string
    std::string uci = move_to_uci(m);

    // Open file in write mode
    std::ofstream file(path);

    // Check if file opened successfully
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        return false;
    }

    // Write UCI string to file with newline
    file << uci << std::endl;

    // Close file
    file.close();

    return true;
}