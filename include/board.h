#ifndef BOARD_H
#define BOARD_H

#include <array>

constexpr int BOARD_SIZE = 8;

enum class PieceType {
    None,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum class Color {
    None,
    White,
    Black
};

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::None;
};

struct Board {
    std::array<std::array<Piece, BOARD_SIZE>, BOARD_SIZE> squares{};
    Color side_to_move = Color::White;
};

struct Move {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    PieceType promotion = PieceType::None;
};

#endif pwd

