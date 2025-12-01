#include "board.h"

void make_move(Board& board, const move& m) {
    Piece piece = board.squares[m.from_row][m.from_col];
    
    // Handle promotion
    if (m.promotion != NONE) {
        switch (m.promotion) {
            case QUEEN: piece.type = PieceType::Queen; break;
            case ROOK: piece.type = PieceType::Rook; break;
            case BISHOP: piece.type = PieceType::Bishop; break;
            case KNIGHT: piece.type = PieceType::Knight; break;
            default: break;
        }
    }

    board.squares[m.from_row][m.from_col] = {PieceType::None, Color::None};
    board.squares[m.to_row][m.to_col] = piece;

    // Switch side to move
    board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
}
