#include "board.h"

// Builds the standard chess starting position.
Board make_starting_position() {
    Board board;

   
    for (int col = 0; col < BOARD_SIZE; ++col) {
        board.squares[1][col] = {PieceType::Pawn, Color::White};
    }

    for (int col = 0; col < BOARD_SIZE; ++col) {
        board.squares[6][col] = {PieceType::Pawn, Color::Black};
    }

    auto place_back_rank = [&](int row, Color color) {
        board.squares[row][0] = {PieceType::Rook,   color};
        board.squares[row][1] = {PieceType::Knight, color};
        board.squares[row][2] = {PieceType::Bishop, color};
        board.squares[row][3] = {PieceType::Queen,  color};
        board.squares[row][4] = {PieceType::King,   color};
        board.squares[row][5] = {PieceType::Bishop, color};
        board.squares[row][6] = {PieceType::Knight, color};
        board.squares[row][7] = {PieceType::Rook,   color};
    };
     // Place back ranks: White on row 0, Black on row 7
    place_back_rank(0, Color::White);
    place_back_rank(7, Color::Black);

    // White moves first
    board.side_to_move = Color::White;
    // All castling rights are initially available
    board.white_can_castle_kingside = true;
    board.white_can_castle_queenside = true;
    board.black_can_castle_kingside = true;
    board.black_can_castle_queenside = true;
    
    return board;
}
