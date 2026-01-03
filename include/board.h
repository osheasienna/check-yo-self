#ifndef BOARD_H
#define BOARD_H

#include <array>
#include <vector>
#include "move.h"

constexpr int BOARD_SIZE = 8;

/* Piecetype represents the kind of chess piece on a square*/
enum class PieceType {
    None,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

/* Colour represents the player to which a piece belongs to */

enum class Color {
    None,
    White,
    Black
};

/* a pPiece is defined by the Piectype and the player who owns it (Color)*/

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::None;
};

/* Boards represents the complete game state

it contains:
1. the grid of pieces (8 x 8)
2. which side is to move
3. castling rights
4. en passant information
*/

struct Board {
    std::array<std::array<Piece, BOARD_SIZE>, BOARD_SIZE> squares{};
    Color side_to_move = Color::White;

    bool white_can_castle_kingside = true;
    bool white_can_castle_queenside = true;
    bool black_can_castle_kingside = true;
    bool black_can_castle_queenside = true;
    
    // En passant target square (the square "passed over" by a 2-square pawn push).
    // If en_passant_row == -1, no en passant capture is available.
    int en_passant_row = -1;
    int en_passant_col = -1;
};

/* The undo structure stores what is necessary to restore the board so we don't have to make copies

it stores:
1. the piece that was captured 
2. the piece type before promotion
3. castling rights
4. whose turn it was before the move
5. en passant before the move
6. whether the move was an en passant capture
*/

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
    int en_passant_row;
    int en_passant_col;
    // True iff the move being undone was an en passant capture.
    // Needed because the captured pawn is not on the destination square.
    bool was_en_passant = false;
};

Board make_starting_position();
/* make move with undo*/
void make_move(Board& board, const move& m, Undo& undo);
/* used in places where no need to have undo*/
void make_move(Board& board, const move& m);
/*restores to the state before the make move*/
void unmake_move( Board& board, const move& m, const Undo& undo);
std::vector<move> generate_legal_moves(const Board& board);


int evaluate_board(const Board& board);
int evaluate_terminal(const Board& board);
int evaluate_terminal(const Board& board, int depth);  // Depth-adjusted for faster mates
move find_best_move(const Board& board, int depth);

#endif // BOARD_H
