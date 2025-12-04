#ifndef MOVE_H
#define MOVE_H

#include <string>
#include <fstream>
#include <iostream>

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

std::string move_to_uci(move m);
bool write_move_to_file(move m, std::string path);

#endif 