#include "board.h"

void make_move(Board& board, const Move& move) {
    Piece piece = board.squares[move.from_row][move.from_col];
    
    // Handle promotion
    if (move.promotion != PieceType::None) {
        piece.type = move.promotion;
    }

    board.squares[move.from_row][move.from_col] = {PieceType::None, Color::None};
    board.squares[move.to_row][move.to_col] = piece;

    // Switch side to move
    board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
}

