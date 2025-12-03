#ifndef BOARD_H
#define BOARD_H

#include <array>
#include <vector>
#include "move.h"

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

Board make_starting_position();
void make_move(Board& board, const Move& m);
std::vector<Move> generate_legal_moves(const Board& board);


int evaluate_board(const Board& board);

#endif // BOARD_H
