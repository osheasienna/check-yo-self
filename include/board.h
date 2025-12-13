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

    bool white_can_castle_kingside = true;
    bool white_can_castle_queenside = true;
    bool black_can_castle_kingside = true;
    bool black_can_castle_queenside = true;
    
    // En passant: column where a pawn just moved 2 squares forward
    // -1 means no en passant capture is possible
    // Set when a pawn moves 2 squares from its starting position
    int en_passant_col = -1;
};

struct Undo 
{
    Piece captured;
    PieceType moved_piece_type;
    bool white_can_castle_kingside;
    bool white_can_castle_queenside;
    bool black_can_castle_kingside;
    bool black_can_castle_queenside;
    Color side_to_move;
    // Save en passant state so we can restore it when undoing moves
    int en_passant_col;
};

Board make_starting_position();
void make_move(Board& board, const move& m, Undo& undo);
void make_move(Board& board, const move& m);
void unmake_move( Board& board, const move& m, const Undo& undo);
std::vector<move> generate_legal_moves(const Board& board);


int evaluate_board(const Board& board);
int evaluate_terminal(const Board& board);
move find_best_move(const Board& board, int depth);

#endif // BOARD_H
