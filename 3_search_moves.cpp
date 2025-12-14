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

// king activity in endgames: encourage centralization, discourage corners
constexpr std::array<int, 64> KING_ENDGAME_TABLE = {
    // Rank 1 to Rank 8 (corners negative, center positive)
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-10,  5, 10, 10,  5,-10,-30,
    -30,  5, 20, 25, 25, 20,  5,-30,
    -30, 10, 25, 30, 30, 25, 10,-30,
    -30, 10, 25, 30, 30, 25, 10,-30,
    -30,  5, 20, 25, 25, 20,  5,-30,
    -30,-10,  5, 10, 10,  5,-10,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
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
// IS_ENDGAME: select king table based on game phase
int positional_bonus(PieceType type, Color color, int square_index, bool is_endgame) {
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
            return is_endgame ? KING_ENDGAME_TABLE[idx] : KING_TABLE[idx];
        case PieceType::None:
        default:
            return 0;
    }
}

} // namespace

// Forward declarations for helpers defined later in this file.
static bool is_attacked(const Board& board, int target_row, int target_col, Color attacker_color);
static bool is_king_in_check(const Board& board, Color color);

// EVALUATE_BOARD FUNCTION
// BOARD: current board position
// returns the score of the board
// score is positive for white, negative for black
int evaluate_board(const Board& board) {
    // initialise score to 0, accumulate the total evaluation of the board
    int score = 0;

    // first pass: compute non-pawn material to decide phase
    int non_pawn_material = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.type != PieceType::None && piece.type != PieceType::Pawn) {
                non_pawn_material += MATERIAL_VALUES[static_cast<int>(piece.type)];
            }
        }
    }
    bool is_endgame = (non_pawn_material < 1500);

    // second pass: evaluate material, PST, passed pawns
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
            int positional = positional_bonus(piece.type, piece.color, square_index, is_endgame);
            int passed_bonus = 0;

            // passed pawn detection with rank-scaled bonus
            if (piece.type == PieceType::Pawn) {
                int pawn_dir = (piece.color == Color::White) ? 1 : -1;
                int start_check_row = row + pawn_dir;
                bool is_passed = true;

                for (int check_file = col - 1; check_file <= col + 1 && is_passed; ++check_file) {
                    if (check_file < 0 || check_file >= BOARD_SIZE) {
                        continue;
                    }
                    for (int check_row = start_check_row;
                         (piece.color == Color::White) ? check_row < BOARD_SIZE : check_row >= 0;
                         check_row += pawn_dir) {
                        const Piece& ahead = board.squares[check_row][check_file];
                        if (ahead.type == PieceType::Pawn && ahead.color != piece.color) {
                            is_passed = false;
                            break;
                        }
                    }
                }

                if (is_passed) {
                    int rank = (piece.color == Color::White) ? row : (7 - row);
                    if (rank >= 7) {
                        passed_bonus = 200;
                    } else if (rank >= 6) {
                        passed_bonus = 100;
                    } else if (rank >= 5) {
                        passed_bonus = 50;
                    } else if (rank >= 4) {
                        passed_bonus = 30;
                    } else if (rank >= 3) {
                        passed_bonus = 20;
                    }
                }
            }

            // combine material and positional bonuses into a single total score
            int total = material + positional + passed_bonus;

            // if the piece is white, add the total score to the score
            // if the piece is black, subtract the total score from the score
            if (piece.color == Color::White) {
                score += total;
            } else {
                score -= total;
            }
        }
    }

    // simple check-related bonuses to encourage mating ideas
    Color opponent = (board.side_to_move == Color::White) ? Color::Black : Color::White;
    if (is_king_in_check(board, opponent)) {
        score += (board.side_to_move == Color::White) ? 10 : -10;
    }
    if (is_king_in_check(board, board.side_to_move)) {
        score += (board.side_to_move == Color::White) ? -20 : 20;
    }

    // return the final evaluation of the board
    return score;
}

// Helper: checks if a square is attacked by any piece of attacker_color
static bool is_attacked(const Board& board, int target_row, int target_col, Color attacker_color) {
    // Direction pawns attack from (depends on color)
    int pawn_dir = (attacker_color == Color::White) ? -1 : 1;

    // Pawn attacks
    for (int dc : {-1, 1}) {
        int r = target_row + pawn_dir;
        int c = target_col + dc;
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::Pawn && p.color == attacker_color) {
                return true;
            }
        }
    }

    // Knight attacks
    const int knight_offsets[8][2] = {
        { 2, 1}, { 1, 2}, {-1, 2}, {-2, 1},
        {-2,-1}, {-1,-2}, { 1,-2}, { 2,-1}
    };
    for (auto& off : knight_offsets) {
        int r = target_row + off[0];
        int c = target_col + off[1];
        if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::Knight && p.color == attacker_color) {
                return true;
            }
        }
    }

    // Sliding pieces: bishop / rook / queen
    const int directions[8][2] = {
        { 1, 0}, {-1, 0}, { 0, 1}, { 0,-1}, // rook-like
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}  // bishop-like
    };

    for (int d = 0; d < 8; ++d) {
        int dr = directions[d][0];
        int dc = directions[d][1];
        int r = target_row + dr;
        int c = target_col + dc;

        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == attacker_color) {
                    if (
                        (d < 4 && (p.type == PieceType::Rook || p.type == PieceType::Queen)) ||
                        (d >= 4 && (p.type == PieceType::Bishop || p.type == PieceType::Queen))
                    ) {
                        return true;
                    }
                }
                break;
            }
            r += dr;
            c += dc;
        }
    }

    // King attacks (adjacent squares)
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = target_row + dr;
            int c = target_col + dc;
            if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                const Piece& p = board.squares[r][c];
                if (p.type == PieceType::King && p.color == attacker_color) {
                    return true;
                }
            }
        }
    }

    return false;
}


static bool is_king_in_check(const Board& board, Color color) {
    int king_row = -1;
    int king_col = -1;

    // 1) Find the king square for the given color
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::King && p.color == color) {
                king_row = row;
                king_col = col;
                // Break out of both loops
                row = BOARD_SIZE;
                break;
            }
        }
    }

    // Safety: if no king found, treat as "not in check"
    if (king_row == -1) {
        return false;
    }

    // 2) The king is in check if its square is attacked by the opponent
    Color enemy = (color == Color::White) ? Color::Black : Color::White;

    // IMPORTANT: this assumes is_attacked(board, target_row, target_col, attacker_color)
    // means "is (target_row,target_col) attacked by attacker_color?"
    return is_attacked(board, king_row, king_col, enemy);
}

int evaluate_terminal(const Board& board) {
    // Keep mate score comfortably below your search infinities
    constexpr int CHECKMATE_SCORE = 100000;

    // No legal moves + in check => checkmate
    if (is_king_in_check(board, board.side_to_move)) {
        return -CHECKMATE_SCORE;  // losing for side to move
    }

    // No legal moves + not in check => stalemate
    return 0;
}



