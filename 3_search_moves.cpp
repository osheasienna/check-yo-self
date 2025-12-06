#include "board.h"
#include <array>

// static evaluation for the chess engine
// combines material value and piece-square tables (PST) to score a position
// positive score = good for White, negative score = good for Black

// namespace for local functions
// prevents other files from calling mirror_square_index() and positional_bonus() directly
// can only call evaluate_board()
namespace {

// array of base values for each piece type
// index corresponds to PieceType enum
constexpr int MATERIAL_VALUES[] = {
    0,   // None
    100, // Pawn
    320, // Knight
    330, // Bishop
    500, // Rook
    900, // Queen
    0    // King (handled via PST only for basic safety)
};

// PIECE-SQUARE TABLES
// each entry is a score for a specific square
// index corresponds to square index (0-63)

// pawns get bonuses for advancing to the centre of the board
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

// positive values in the centre encourages knights to move to the centre
// negative values in the corners discourages knights from moving to the corners
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

// bonuses for bishops to occupy central positions, diagonals
// negative values in the corners discourage bishops from moving to the corners
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

// bonuses for rooks to occupy central positions
// small penalties for being trapped on the edges
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

// encourages central squares, discourages edges
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

// bonuses for kings staying on the back rank (castled / safe)
// penalties for advancing towards the center or up the board
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

// MIRROR_SQUARE_INDEX FUNCTION
// converts square index to mirror square index
// used for the black pieces, so both can share PST as white
int mirror_square_index(int index) {
    // compute row by dividing by BOARD_SIZE
    int row = index / BOARD_SIZE;
    // compute column by taking the remainder of BOARD_SIZE
    int col = index % BOARD_SIZE;
    // compute the row reflected vertically 
    int mirrored_row = BOARD_SIZE - 1 - row;
    // convert the mirrored (row, col) back into a single index
    return mirrored_row * BOARD_SIZE + col;
}

// POSITIONAL_BONUS FUNCTION
// look up the PST value for this piece on this square, mirroring if black
// TYPE: type of the piece
// COLOR: color of the piece
// SQUARE_INDEX: index of the square
int positional_bonus(PieceType type, Color color, int square_index) {
    // if white, use the square index directly
    // if black, use the mirror square index
    int idx = (color == Color::White) ? square_index : mirror_square_index(square_index);

    // depending on the piece type, use the correct PST array
    // return the value at the index idx
    // if there is no piece or unknown type, return 0
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

// EVALUATE_BOARD FUNCTION
// BOARD: current board position
// returns the score of the board
// score is positive for white, negative for black
int evaluate_board(const Board& board) {
    // initialise score to 0, accumulate the total evaluation of the board
    int score = 0;

    // double loop over all 64 squares on the board
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            // look at the Piece on this square
            const Piece& piece = board.squares[row][col];
            // if piece.type is None, skip this square and go to the next square
            if (piece.type == PieceType::None) {
                continue;
            }

            // convert row and column to a single index
            int square_index = row * BOARD_SIZE + col;
            // look up the material value for this piece type
            int material = MATERIAL_VALUES[static_cast<int>(piece.type)];
            // get the positional bonus based on the piece type, color, and square index
            int positional = positional_bonus(piece.type, piece.color, square_index);
            // combine material and positional bonuses into a single total score
            int total = material + positional;

            // if the piece is white, add the total score to the score
            // if the piece is black, subtract the total score from the score
            if (piece.color == Color::White) {
                score += total;
            } else {
                score -= total;
            }
        }
    }

    // return the final evaluation of the board
    return score;
}

