#include "board.h"

#include <array>

namespace {

constexpr int MATERIAL_VALUES[] = {
    0,   // None
    100, // Pawn
    320, // Knight
    330, // Bishop
    500, // Rook
    900, // Queen
    0    // King (handled via PST only for basic safety)
};

constexpr std::array<int, 64> PAWN_TABLE = {
    // Rank 1 to Rank 8 (row 0 -> row 7)
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

constexpr std::array<int, 64> KNIGHT_TABLE = {
    // Rank 1 to Rank 8
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

constexpr std::array<int, 64> BISHOP_TABLE = {
    // Rank 1 to Rank 8
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5, 10, 15, 15, 10,  5,-10,
   -10,  0, 10, 15, 15, 10,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0,  5,  0,  0,  5,  0,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

constexpr std::array<int, 64> ROOK_TABLE = {
    // Rank 1 to Rank 8
     0,  0,  5, 10, 10,  5,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

constexpr std::array<int, 64> QUEEN_TABLE = {
    // Rank 1 to Rank 8
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

constexpr std::array<int, 64> KING_TABLE = {
    // Rank 1 to Rank 8
    20, 30, 10,  0,  0, 10, 30, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
   -10,-20,-20,-20,-20,-20,-20,-10,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30
};

int mirror_square_index(int index) {
    int row = index / BOARD_SIZE;
    int col = index % BOARD_SIZE;
    int mirrored_row = BOARD_SIZE - 1 - row;
    return mirrored_row * BOARD_SIZE + col;
}

int positional_bonus(PieceType type, Color color, int square_index) {
    int idx = (color == Color::White) ? square_index : mirror_square_index(square_index);

    switch (type) {
        case PieceType::Pawn:
            return PAWN_TABLE[idx];
        case PieceType::Knight:
            return KNIGHT_TABLE[idx];
        case PieceType::Bishop:
            return BISHOP_TABLE[idx];
        case PieceType::Rook:
            return ROOK_TABLE[idx];
        case PieceType::Queen:
            return QUEEN_TABLE[idx];
        case PieceType::King:
            return KING_TABLE[idx];
        case PieceType::None:
        default:
            return 0;
    }
}

} // namespace

int evaluate_board(const Board& board) {
    int score = 0;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.type == PieceType::None) {
                continue;
            }

            int square_index = row * BOARD_SIZE + col;
            int material = MATERIAL_VALUES[static_cast<int>(piece.type)];
            int positional = positional_bonus(piece.type, piece.color, square_index);
            int total = material + positional;

            if (piece.color == Color::White) {
                score += total;
            } else {
                score -= total;
            }
        }
    }

    return score;
}

