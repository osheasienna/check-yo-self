#include "board.h"
#include <array>
#include "attacks.h"

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

// Evaluation bonuses/penalties (in centipawns)
constexpr int BISHOP_PAIR_BONUS = 50;       // Two bishops are stronger than B+N
constexpr int DOUBLED_PAWN_PENALTY = 15;    // Pawns on same file are weaker
constexpr int ROOK_OPEN_FILE_BONUS = 25;    // Rook on file with no pawns
constexpr int ROOK_SEMI_OPEN_FILE_BONUS = 15; // Rook on file with only enemy pawns

// EVALUATE_BOARD FUNCTION
// BOARD: current board position
// returns the score of the board
// score is positive for white, negative for black
int evaluate_board(const Board& board) {
    // initialise score to 0, accumulate the total evaluation of the board
    int score = 0;

    // Piece counters for bishop pair detection
    int white_bishops = 0, black_bishops = 0;
    
    // Pawn counters per file for doubled pawn detection
    int white_pawns_per_file[8] = {0};
    int black_pawns_per_file[8] = {0};

    // first pass: compute non-pawn material to decide phase + count pieces
    int non_pawn_material = 0;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.type == PieceType::None) continue;
            
            if (piece.type != PieceType::Pawn) {
                non_pawn_material += MATERIAL_VALUES[static_cast<int>(piece.type)];
            }
            
            // Count bishops for bishop pair bonus
            if (piece.type == PieceType::Bishop) {
                if (piece.color == Color::White) white_bishops++;
                else black_bishops++;
            }
            
            // Count pawns per file for doubled pawn penalty
            if (piece.type == PieceType::Pawn) {
                if (piece.color == Color::White) white_pawns_per_file[col]++;
                else black_pawns_per_file[col]++;
            }
        }
    }
    bool is_endgame = (non_pawn_material < 1500);

    // second pass: evaluate material, PST, passed pawns, rook on open files
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
            int rook_file_bonus = 0;

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
            
            // Rook on open/semi-open file bonus
            if (piece.type == PieceType::Rook) {
                bool own_pawns = (piece.color == Color::White) ? 
                    (white_pawns_per_file[col] > 0) : (black_pawns_per_file[col] > 0);
                bool enemy_pawns = (piece.color == Color::White) ? 
                    (black_pawns_per_file[col] > 0) : (white_pawns_per_file[col] > 0);
                
                if (!own_pawns && !enemy_pawns) {
                    rook_file_bonus = ROOK_OPEN_FILE_BONUS;      // Open file: no pawns
                } else if (!own_pawns && enemy_pawns) {
                    rook_file_bonus = ROOK_SEMI_OPEN_FILE_BONUS; // Semi-open: only enemy pawns
                }
            }

            // combine material and positional bonuses into a single total score
            int total = material + positional + passed_bonus + rook_file_bonus;

            // if the piece is white, add the total score to the score
            // if the piece is black, subtract the total score from the score
            if (piece.color == Color::White) {
                score += total;
            } else {
                score -= total;
            }
        }
    }
    
    // Bishop pair bonus: having both bishops is an advantage
    if (white_bishops >= 2) score += BISHOP_PAIR_BONUS;
    if (black_bishops >= 2) score -= BISHOP_PAIR_BONUS;
    
    // Doubled pawn penalty: penalize files with 2+ pawns of same color
    for (int file = 0; file < 8; ++file) {
        if (white_pawns_per_file[file] > 1) {
            score -= DOUBLED_PAWN_PENALTY * (white_pawns_per_file[file] - 1);
        }
        if (black_pawns_per_file[file] > 1) {
            score += DOUBLED_PAWN_PENALTY * (black_pawns_per_file[file] - 1);
        }
    }

    // simple check-related bonuses to encourage mating ideas
    Color opponent = (board.side_to_move == Color::White) ? Color::Black : Color::White;
    if (is_in_check(board, opponent)) {
        score += (board.side_to_move == Color::White) ? 10 : -10;
    }
    if (is_in_check(board, board.side_to_move)) {
        score += (board.side_to_move == Color::White) ? -20 : 20;
    }

    // return the final evaluation of the board
    return score;
}
// Checkmate score constant - used by search to detect mate
constexpr int CHECKMATE_SCORE = 100000;

int evaluate_terminal(const Board& board, int depth) {
    // No legal moves + in check => checkmate
    // Depth-adjusted: prefer faster mates (higher depth = found earlier = better)
    if (is_in_check(board, board.side_to_move)) {
        return -(CHECKMATE_SCORE - depth);  // Faster mate = higher score magnitude
    }

    // No legal moves + not in check => stalemate
    return 0;
}

// Overload for backward compatibility (used by quiescence search at depth 0)
int evaluate_terminal(const Board& board) {
    return evaluate_terminal(board, 0);
}



