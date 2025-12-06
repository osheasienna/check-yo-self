#include "board.h"

void make_move(Board& board, const move& m) {
    Piece piece = board.squares[m.from_row][m.from_col];
    Piece captured = board.squares[m.to_row][m.to_col];

    if (captured.type == PieceType::Rook)
    {
        if (captured.color == Color::White)
        {
            if (m.to_row == 0 && m.to_col == 0)
                board.white_can_castle_queenside = false;
            if (m.to_row == 0 && m.to_col == 7)
                board.white_can_castle_kingside = false;
        } else if (captured.color == Color::Black)
        {
            if (m.to_row == 7 && m.to_col == 0)
                board.black_can_castle_queenside = false;
            if(m.to_row == 7 && m.to_col == 7)
                board.black_can_castle_kingside = false;
        }
    }
    
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

    if (piece.type == PieceType::King)
    {
        int row = m.from_row;
        int delta_col = m.to_col - m.from_col;

        if (piece.color == Color::White)
        {
            board.white_can_castle_kingside = false;
            board.white_can_castle_queenside = false;
        } else 
        {
            board.black_can_castle_kingside = false;
            board.black_can_castle_queenside = false;
        }

        if (std::abs(delta_col) == 2)
        {
            if (delta_col > 0)
            {
                Piece rook = board.squares[row][7];
                board.squares[row][7] = {PieceType::None, Color::None};
                board.squares[row][5] = rook;
            } else 
            {
                Piece rook = board.squares[row][0];
                board.squares[row][0] = {PieceType::None, Color::None};
                board.squares[row][3] = rook;
            }
        }
    }

    if (piece.type == PieceType::Rook)
    {
        if (piece.color == Color::White)
        {
            if(m.from_row == 0 && m.from_col == 0)
                board.white_can_castle_queenside = false;
            if(m.from_row == 0 && m.from_col == 7)
                board.white_can_castle_kingside = false;
        } else if  (piece.color == Color::Black)
        {
            if (m.from_row == 7 && m.from_col == 0)
                board.black_can_castle_queenside = false;
            if(m.from_row == 7 && m.from_col == 7)
                board.black_can_castle_kingside = false;
        }
    }


    board.squares[m.to_row][m.to_col] = piece;

    // Switch side to move
    board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
}
