#include "board.h"
#include <array>
#include <algorithm>  // For std::max, std::abs
#include <cstdlib>    // For std::abs (integer version)
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

// ============================================================================
// ENDGAME EVALUATION HELPERS
// ============================================================================
// In endgames, we need special evaluation terms to help the engine:
// 1. Push the enemy king towards edges/corners for mating
// 2. Bring our king closer to the enemy king
// 3. Recognize simple mating patterns (KQ vs K, KR vs K)
// ============================================================================

// Manhattan distance between two squares
int manhattan_distance(int row1, int col1, int row2, int col2) {
    return std::abs(row1 - row2) + std::abs(col1 - col2);
}

// Chebyshev distance (king distance) - max of row/col difference
int king_distance(int row1, int col1, int row2, int col2) {
    return std::max(std::abs(row1 - row2), std::abs(col1 - col2));
}

// Distance from center (0-3, 0 = center, 3 = corner)
int center_distance(int row, int col) {
    int row_dist = std::max(3 - row, row - 4);  // Distance from center rows (3,4)
    int col_dist = std::max(3 - col, col - 4);  // Distance from center cols (3,4)
    return row_dist + col_dist;
}

// Find king position for a given color
// Returns true if found, sets king_row and king_col
bool find_king(const Board& board, Color color, int& king_row, int& king_col) {
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::King && p.color == color) {
                king_row = row;
                king_col = col;
                return true;
            }
        }
    }
    return false;
}

// Evaluate endgame-specific bonuses
// Returns bonus from White's perspective (positive = good for white)
int evaluate_endgame_bonus(const Board& board, bool is_endgame, int white_material, int black_material) {
    if (!is_endgame) return 0;
    
    int bonus = 0;
    
    // Find both kings
    int white_king_row = 0, white_king_col = 0;
    int black_king_row = 0, black_king_col = 0;
    
    if (!find_king(board, Color::White, white_king_row, white_king_col)) return 0;
    if (!find_king(board, Color::Black, black_king_row, black_king_col)) return 0;
    
    // Calculate material advantage (positive = white ahead)
    int material_diff = white_material - black_material;
    
    // If we have material advantage, apply endgame bonuses
    if (material_diff > 200) {  // White is winning
        // Bonus for pushing black king to edge
        int black_center_dist = center_distance(black_king_row, black_king_col);
        bonus += black_center_dist * 10;  // Up to 60 centipawns
        
        // Bonus for king proximity (our king should approach theirs)
        int kings_dist = king_distance(white_king_row, white_king_col, black_king_row, black_king_col);
        bonus += (7 - kings_dist) * 5;  // Up to 35 centipawns for being close
        
        // Extra bonus if enemy king is in corner (easier to mate)
        bool in_corner = (black_king_row == 0 || black_king_row == 7) && 
                         (black_king_col == 0 || black_king_col == 7);
        if (in_corner) bonus += 30;
        
    } else if (material_diff < -200) {  // Black is winning
        // Bonus for pushing white king to edge
        int white_center_dist = center_distance(white_king_row, white_king_col);
        bonus -= white_center_dist * 10;
        
        // Bonus for king proximity
        int kings_dist = king_distance(white_king_row, white_king_col, black_king_row, black_king_col);
        bonus -= (7 - kings_dist) * 5;
        
        // Extra bonus if enemy king is in corner
        bool in_corner = (white_king_row == 0 || white_king_row == 7) && 
                         (white_king_col == 0 || white_king_col == 7);
        if (in_corner) bonus -= 30;
    }
    
    return bonus;
}

// ============================================================================
// KING SAFETY EVALUATION
// ============================================================================
// Evaluates king safety based on:
// 1. Pawn shield - pawns in front of the castled king
// 2. Open files near king - dangerous for rook/queen attacks
// 3. General exposure - king on center files is vulnerable in middlegame
// 4. Attack counting - squares around king under attack (NEW)
// 5. King escape squares - how many safe squares the king can flee to (NEW)
// 6. Attacker proximity - pieces near king zone by type (NEW)
// Returns bonus from White's perspective (positive = good for white)
// ============================================================================

// King safety weights - tuned for aggressive play
constexpr int PAWN_SHIELD_BONUS = 12;           // Per pawn on shield
constexpr int PAWN_SHIELD_ADVANCED_BONUS = 6;   // Pawn moved one square
constexpr int OPEN_FILE_PENALTY = 20;           // Open file near king
constexpr int SEMI_OPEN_FILE_PENALTY = 12;      // Semi-open file near king
constexpr int CENTER_KING_PENALTY = 25;         // King stuck in center
constexpr int KING_ZONE_ATTACK_PENALTY = 8;     // Per attacked square in king zone
constexpr int NO_ESCAPE_PENALTY = 30;           // King has no safe squares
constexpr int FEW_ESCAPE_PENALTY = 15;          // King has only 1-2 safe squares

// Attacker weights - how dangerous each piece type is near the king
constexpr int QUEEN_ATTACK_WEIGHT = 5;
constexpr int ROOK_ATTACK_WEIGHT = 3;
constexpr int BISHOP_ATTACK_WEIGHT = 2;
constexpr int KNIGHT_ATTACK_WEIGHT = 2;

// Count attackers of each piece type near the king zone
// Returns a danger score based on piece types attacking the king area
int count_king_zone_attackers(const Board& board, int king_row, int king_col, Color enemy_color) {
    int danger = 0;
    
    // King zone: 3x3 area around the king (or extended for more aggressive eval)
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            int r = king_row + dr;
            int c = king_col + dc;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) continue;
            
            // Check if this square in the king zone is attacked
            if (is_attacked(board, r, c, enemy_color)) {
                danger += KING_ZONE_ATTACK_PENALTY;
            }
        }
    }
    
    // Also check attackers in a wider ring (2 squares away) for major pieces
    for (int dr = -2; dr <= 2; ++dr) {
        for (int dc = -2; dc <= 2; ++dc) {
            if (std::abs(dr) <= 1 && std::abs(dc) <= 1) continue; // Skip inner zone
            int r = king_row + dr;
            int c = king_col + dc;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) continue;
            
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::None || p.color != enemy_color) continue;
            
            // Weight by piece type
            switch (p.type) {
                case PieceType::Queen:
                    danger += QUEEN_ATTACK_WEIGHT;
                    break;
                case PieceType::Rook:
                    danger += ROOK_ATTACK_WEIGHT;
                    break;
                case PieceType::Bishop:
                    danger += BISHOP_ATTACK_WEIGHT;
                    break;
                case PieceType::Knight:
                    danger += KNIGHT_ATTACK_WEIGHT;
                    break;
                default:
                    break;
            }
        }
    }
    
    return danger;
}

// Count how many safe squares the king can escape to
int count_king_escape_squares(const Board& board, int king_row, int king_col, Color king_color) {
    int safe_squares = 0;
    Color enemy = (king_color == Color::White) ? Color::Black : Color::White;
    
    // Check all 8 adjacent squares
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue; // Skip king's current square
            
            int r = king_row + dr;
            int c = king_col + dc;
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) continue;
            
            const Piece& p = board.squares[r][c];
            
            // Square must be empty or have an enemy piece (capturable)
            // AND not be attacked by enemy
            if ((p.type == PieceType::None || p.color == enemy) &&
                !is_attacked(board, r, c, enemy)) {
                safe_squares++;
            }
        }
    }
    
    return safe_squares;
}

int evaluate_king_safety(const Board& board, bool is_endgame,
                         const int* white_pawns_per_file, const int* black_pawns_per_file) {
    // King safety matters less in endgames
    if (is_endgame) return 0;
    
    int white_king_row = 0, white_king_col = 0;
    int black_king_row = 0, black_king_col = 0;
    
    if (!find_king(board, Color::White, white_king_row, white_king_col)) return 0;
    if (!find_king(board, Color::Black, black_king_row, black_king_col)) return 0;
    
    int white_safety = 0;
    int black_safety = 0;
    
    // =========================================================================
    // WHITE KING SAFETY
    // =========================================================================
    // Pawn shield bonus for castled king (king on g1/h1 or a1/b1/c1)
    if (white_king_row == 0) {
        // Kingside castled (king on f1, g1, or h1)
        if (white_king_col >= 5) {
            // Check pawns on f2, g2, h2 (row 1, cols 5,6,7)
            for (int col = 5; col <= 7 && col < BOARD_SIZE; ++col) {
                const Piece& p = board.squares[1][col];
                if (p.type == PieceType::Pawn && p.color == Color::White) {
                    white_safety += PAWN_SHIELD_BONUS;
                } else if (col < BOARD_SIZE) {
                    const Piece& p3 = board.squares[2][col];
                    if (p3.type == PieceType::Pawn && p3.color == Color::White) {
                        white_safety += PAWN_SHIELD_ADVANCED_BONUS;
                    }
                }
            }
        }
        // Queenside castled (king on a1, b1, or c1)
        else if (white_king_col <= 2) {
            for (int col = 0; col <= 2; ++col) {
                const Piece& p = board.squares[1][col];
                if (p.type == PieceType::Pawn && p.color == Color::White) {
                    white_safety += PAWN_SHIELD_BONUS;
                } else {
                    const Piece& p3 = board.squares[2][col];
                    if (p3.type == PieceType::Pawn && p3.color == Color::White) {
                        white_safety += PAWN_SHIELD_ADVANCED_BONUS;
                    }
                }
            }
        }
    }
    
    // Penalty for open files near white king
    for (int col = std::max(0, white_king_col - 1); col <= std::min(7, white_king_col + 1); ++col) {
        if (white_pawns_per_file[col] == 0 && black_pawns_per_file[col] == 0) {
            white_safety -= OPEN_FILE_PENALTY;
        } else if (white_pawns_per_file[col] == 0) {
            white_safety -= SEMI_OPEN_FILE_PENALTY;
        }
    }
    
    // Penalty for king on central files (d, e) in middlegame
    if (white_king_col >= 3 && white_king_col <= 4 && white_king_row <= 1) {
        white_safety -= CENTER_KING_PENALTY;
    }
    
    // NEW: King zone attack counting
    int white_danger = count_king_zone_attackers(board, white_king_row, white_king_col, Color::Black);
    white_safety -= white_danger;
    
    // NEW: King escape squares
    int white_escapes = count_king_escape_squares(board, white_king_row, white_king_col, Color::White);
    if (white_escapes == 0) {
        white_safety -= NO_ESCAPE_PENALTY;
    } else if (white_escapes <= 2) {
        white_safety -= FEW_ESCAPE_PENALTY;
    }
    
    // =========================================================================
    // BLACK KING SAFETY (mirror of white)
    // =========================================================================
    if (black_king_row == 7) {
        // Kingside castled (king on f8, g8, or h8)
        if (black_king_col >= 5) {
            for (int col = 5; col <= 7 && col < BOARD_SIZE; ++col) {
                const Piece& p = board.squares[6][col];
                if (p.type == PieceType::Pawn && p.color == Color::Black) {
                    black_safety += PAWN_SHIELD_BONUS;
                } else if (col < BOARD_SIZE) {
                    const Piece& p3 = board.squares[5][col];
                    if (p3.type == PieceType::Pawn && p3.color == Color::Black) {
                        black_safety += PAWN_SHIELD_ADVANCED_BONUS;
                    }
                }
            }
        }
        // Queenside castled
        else if (black_king_col <= 2) {
            for (int col = 0; col <= 2; ++col) {
                const Piece& p = board.squares[6][col];
                if (p.type == PieceType::Pawn && p.color == Color::Black) {
                    black_safety += PAWN_SHIELD_BONUS;
                } else {
                    const Piece& p3 = board.squares[5][col];
                    if (p3.type == PieceType::Pawn && p3.color == Color::Black) {
                        black_safety += PAWN_SHIELD_ADVANCED_BONUS;
                    }
                }
            }
        }
    }
    
    // Penalty for open files near black king
    for (int col = std::max(0, black_king_col - 1); col <= std::min(7, black_king_col + 1); ++col) {
        if (white_pawns_per_file[col] == 0 && black_pawns_per_file[col] == 0) {
            black_safety -= OPEN_FILE_PENALTY;
        } else if (black_pawns_per_file[col] == 0) {
            black_safety -= SEMI_OPEN_FILE_PENALTY;
        }
    }
    
    // Penalty for king on central files
    if (black_king_col >= 3 && black_king_col <= 4 && black_king_row >= 6) {
        black_safety -= CENTER_KING_PENALTY;
    }
    
    // NEW: King zone attack counting
    int black_danger = count_king_zone_attackers(board, black_king_row, black_king_col, Color::White);
    black_safety -= black_danger;
    
    // NEW: King escape squares
    int black_escapes = count_king_escape_squares(board, black_king_row, black_king_col, Color::Black);
    if (black_escapes == 0) {
        black_safety -= NO_ESCAPE_PENALTY;
    } else if (black_escapes <= 2) {
        black_safety -= FEW_ESCAPE_PENALTY;
    }
    
    // Return from white's perspective (positive = white safer)
    return white_safety - black_safety;
}

// ============================================================================
// PIECE ACTIVITY & ATTACK EVALUATION
// ============================================================================
// Evaluates how active the pieces are and if they're attacking the enemy king.
// This encourages aggressive play and piece coordination.
// ============================================================================
int evaluate_piece_activity(const Board& board, bool is_endgame) {
    int white_activity = 0;
    int black_activity = 0;
    
    // Find both kings
    int white_king_row = 0, white_king_col = 0;
    int black_king_row = 0, black_king_col = 0;
    find_king(board, Color::White, white_king_row, white_king_col);
    find_king(board, Color::Black, black_king_row, black_king_col);
    
    // Count defenders near each king (for defensive evaluation)
    int white_defenders = 0;  // White pieces near white king
    int black_defenders = 0;  // Black pieces near black king
    
    // Evaluate each piece's activity
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::None || p.type == PieceType::Pawn || p.type == PieceType::King) {
                continue;
            }
            
            int enemy_king_row = (p.color == Color::White) ? black_king_row : white_king_row;
            int enemy_king_col = (p.color == Color::White) ? black_king_col : white_king_col;
            int own_king_row = (p.color == Color::White) ? white_king_row : black_king_row;
            int own_king_col = (p.color == Color::White) ? white_king_col : black_king_col;
            
            // Distance to enemy king (closer = more dangerous)
            int dist_to_enemy_king = king_distance(row, col, enemy_king_row, enemy_king_col);
            // Distance to own king (closer = defending)
            int dist_to_own_king = king_distance(row, col, own_king_row, own_king_col);
            
            // Bonus for pieces close to enemy king (attacking potential)
            int attack_bonus = 0;
            if (!is_endgame) {
                // In middlegame, reward pieces near enemy king
                if (dist_to_enemy_king <= 2) {
                    attack_bonus = 20;  // Very close - dangerous!
                } else if (dist_to_enemy_king <= 3) {
                    attack_bonus = 10;  // Moderately close
                } else if (dist_to_enemy_king <= 4) {
                    attack_bonus = 5;   // Within striking range
                }
                
                // Extra bonus for queens and rooks near enemy king
                if (p.type == PieceType::Queen && dist_to_enemy_king <= 3) {
                    attack_bonus += 15;
                }
                if (p.type == PieceType::Rook && dist_to_enemy_king <= 2) {
                    attack_bonus += 10;
                }
            }
            
            // DEFENSE BONUS: Pieces close to own king help defend
            int defense_bonus = 0;
            if (!is_endgame && dist_to_own_king <= 2) {
                // Knights and bishops are good defenders
                if (p.type == PieceType::Knight || p.type == PieceType::Bishop) {
                    defense_bonus = 15;
                }
                // Rooks nearby are excellent defenders
                if (p.type == PieceType::Rook) {
                    defense_bonus = 20;
                }
                // Queen near king is powerful defense
                if (p.type == PieceType::Queen) {
                    defense_bonus = 10;
                }
                
                // Count defenders
                if (p.color == Color::White) {
                    white_defenders++;
                } else {
                    black_defenders++;
                }
            }
            
            // Centralization bonus (pieces in center are more active)
            int center_bonus = 0;
            int center_dist = center_distance(row, col);
            if (center_dist <= 2) {
                center_bonus = (4 - center_dist) * 3;  // Up to 12 for central squares
            }
            
            // Add to appropriate color
            if (p.color == Color::White) {
                white_activity += attack_bonus + center_bonus + defense_bonus;
            } else {
                black_activity += attack_bonus + center_bonus + defense_bonus;
            }
        }
    }
    
    // PENALTY for having very few defenders when not in endgame
    // This encourages keeping pieces near the king for defense
    if (!is_endgame) {
        if (white_defenders == 0) {
            white_activity -= 40;  // No defenders = dangerous!
        } else if (white_defenders == 1) {
            white_activity -= 15;  // Only one defender
        }
        
        if (black_defenders == 0) {
            black_activity -= 40;
        } else if (black_defenders == 1) {
            black_activity -= 15;
        }
    }
    
    return white_activity - black_activity;
}

// ============================================================================
// MOBILITY EVALUATION (simplified - count piece mobility)
// ============================================================================
// Bonus for having more legal moves available. This encourages active play.
// Note: This is expensive to compute, so we use a simplified version.
// ============================================================================
int evaluate_mobility_simple(const Board& board) {
    // Count pieces that are "developed" (off back rank for minor pieces)
    int white_developed = 0;
    int black_developed = 0;
    
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::None) continue;
            
            // Knights and bishops off back rank = developed
            if (p.type == PieceType::Knight || p.type == PieceType::Bishop) {
                if (p.color == Color::White && row > 0) {
                    white_developed += 10;
                    if (row >= 2) white_developed += 5;  // Extra for being more advanced
                } else if (p.color == Color::Black && row < 7) {
                    black_developed += 10;
                    if (row <= 5) black_developed += 5;
                }
            }
            
            // Rooks on 7th rank (2nd rank for black) = very active
            if (p.type == PieceType::Rook) {
                if (p.color == Color::White && row == 6) {
                    white_developed += 20;  // Rook on 7th rank!
                } else if (p.color == Color::Black && row == 1) {
                    black_developed += 20;  // Rook on 2nd rank!
                }
            }
            
            // Queen off starting square = developed
            if (p.type == PieceType::Queen) {
                if (p.color == Color::White && !(row == 0 && col == 3)) {
                    white_developed += 5;
                } else if (p.color == Color::Black && !(row == 7 && col == 3)) {
                    black_developed += 5;
                }
            }
        }
    }
    
    return white_developed - black_developed;
}

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
    int white_material = 0;  // Total white material (for endgame eval)
    int black_material = 0;  // Total black material (for endgame eval)
    
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.type == PieceType::None) continue;
            
            int piece_value = MATERIAL_VALUES[static_cast<int>(piece.type)];
            
            if (piece.type != PieceType::Pawn) {
                non_pawn_material += piece_value;
            }
            
            // Track material per side for endgame evaluation
            if (piece.color == Color::White) {
                white_material += piece_value;
            } else {
                black_material += piece_value;
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
                    // Significantly increased bonuses for passed pawns
                    // A pawn close to promotion is almost worth a piece!
                    if (rank >= 7) {
                        passed_bonus = 400;  // One square from promotion - very valuable!
                    } else if (rank >= 6) {
                        passed_bonus = 200;  // Two squares from promotion
                    } else if (rank >= 5) {
                        passed_bonus = 100;  // Getting dangerous
                    } else if (rank >= 4) {
                        passed_bonus = 50;   // Past the middle
                    } else if (rank >= 3) {
                        passed_bonus = 30;   // Starting to advance
                    }
                    
                    // BONUS: Path to promotion is clear (no pieces blocking)
                    bool path_clear = true;
                    for (int check_row = row + pawn_dir;
                         (piece.color == Color::White) ? check_row < BOARD_SIZE : check_row >= 0;
                         check_row += pawn_dir) {
                        if (board.squares[check_row][col].type != PieceType::None) {
                            path_clear = false;
                            break;
                        }
                    }
                    if (path_clear) {
                        passed_bonus += 50;  // Nothing blocking the pawn's path!
                    }
                    
                    // BONUS: Passed pawn is protected by another pawn
                    bool is_protected = false;
                    // Check for defending pawns diagonally behind
                    int behind_row = row - pawn_dir;
                    if (behind_row >= 0 && behind_row < BOARD_SIZE) {
                        for (int dc = -1; dc <= 1; dc += 2) {
                            int protect_col = col + dc;
                            if (protect_col >= 0 && protect_col < BOARD_SIZE) {
                                const Piece& protector = board.squares[behind_row][protect_col];
                                if (protector.type == PieceType::Pawn && protector.color == piece.color) {
                                    is_protected = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (is_protected) {
                        passed_bonus += 30;  // Protected passed pawn!
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
    
    // =========================================================================
    // CONNECTED ROOKS AND BATTERIES
    // =========================================================================
    // Connected rooks (same rank or file with no pieces between) are powerful
    // Queen-Rook batteries (aligned on same file/rank) are also very strong
    // =========================================================================
    
    // Find rooks and queens for both sides
    struct PiecePos { int row, col; };
    std::vector<PiecePos> white_rooks, black_rooks;
    PiecePos white_queen = {-1, -1}, black_queen = {-1, -1};
    
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::Rook) {
                if (p.color == Color::White) {
                    white_rooks.push_back({r, c});
                } else {
                    black_rooks.push_back({r, c});
                }
            } else if (p.type == PieceType::Queen) {
                if (p.color == Color::White) {
                    white_queen = {r, c};
                } else {
                    black_queen = {r, c};
                }
            }
        }
    }
    
    // Helper lambda to check if two pieces are connected (same row or col, no blockers)
    auto are_connected = [&](int r1, int c1, int r2, int c2) -> bool {
        if (r1 == r2) {
            // Same row - check columns between them
            int start = std::min(c1, c2) + 1;
            int end = std::max(c1, c2);
            for (int c = start; c < end; ++c) {
                if (board.squares[r1][c].type != PieceType::None) {
                    return false;
                }
            }
            return true;
        } else if (c1 == c2) {
            // Same column - check rows between them
            int start = std::min(r1, r2) + 1;
            int end = std::max(r1, r2);
            for (int r = start; r < end; ++r) {
                if (board.squares[r][c1].type != PieceType::None) {
                    return false;
                }
            }
            return true;
        }
        return false;
    };
    
    // White connected rooks
    if (white_rooks.size() >= 2) {
        if (are_connected(white_rooks[0].row, white_rooks[0].col, 
                         white_rooks[1].row, white_rooks[1].col)) {
            score += 25;  // Connected rooks bonus
        }
    }
    
    // Black connected rooks
    if (black_rooks.size() >= 2) {
        if (are_connected(black_rooks[0].row, black_rooks[0].col, 
                         black_rooks[1].row, black_rooks[1].col)) {
            score -= 25;  // Connected rooks bonus for black
        }
    }
    
    // White Queen-Rook battery
    if (white_queen.row >= 0) {
        for (const auto& rook : white_rooks) {
            if (are_connected(white_queen.row, white_queen.col, rook.row, rook.col)) {
                score += 30;  // Queen-Rook battery
                break;  // Only count one battery
            }
        }
    }
    
    // Black Queen-Rook battery
    if (black_queen.row >= 0) {
        for (const auto& rook : black_rooks) {
            if (are_connected(black_queen.row, black_queen.col, rook.row, rook.col)) {
                score -= 30;  // Queen-Rook battery for black
                break;
            }
        }
    }
    
    // Development penalty: penalize pieces still on starting squares
    // This encourages the bot to develop knights and bishops early
    // White pieces on back rank (row 0)
    if (board.squares[0][1].type == PieceType::Knight && 
        board.squares[0][1].color == Color::White) score -= 30;  // b1 knight
    if (board.squares[0][6].type == PieceType::Knight && 
        board.squares[0][6].color == Color::White) score -= 30;  // g1 knight
    if (board.squares[0][2].type == PieceType::Bishop && 
        board.squares[0][2].color == Color::White) score -= 25;  // c1 bishop
    if (board.squares[0][5].type == PieceType::Bishop && 
        board.squares[0][5].color == Color::White) score -= 25;  // f1 bishop
    
    // Black pieces on back rank (row 7) - add to score = penalty for black
    if (board.squares[7][1].type == PieceType::Knight && 
        board.squares[7][1].color == Color::Black) score += 30;  // b8 knight
    if (board.squares[7][6].type == PieceType::Knight && 
        board.squares[7][6].color == Color::Black) score += 30;  // g8 knight
    if (board.squares[7][2].type == PieceType::Bishop && 
        board.squares[7][2].color == Color::Black) score += 25;  // c8 bishop
    if (board.squares[7][5].type == PieceType::Bishop && 
        board.squares[7][5].color == Color::Black) score += 25;  // f8 bishop

    // simple check-related bonuses to encourage mating ideas
    Color opponent = (board.side_to_move == Color::White) ? Color::Black : Color::White;
    if (is_in_check(board, opponent)) {
        score += (board.side_to_move == Color::White) ? 10 : -10;
    }
    if (is_in_check(board, board.side_to_move)) {
        score += (board.side_to_move == Color::White) ? -20 : 20;
    }

    // ENDGAME BONUS: King proximity, edge pushing, mating patterns
    // This helps the engine convert winning endgames
    score += evaluate_endgame_bonus(board, is_endgame, white_material, black_material);

    // KING SAFETY: Pawn shield, open files, center exposure
    // This helps the engine protect its king in the middlegame
    score += evaluate_king_safety(board, is_endgame, white_pawns_per_file, black_pawns_per_file);

    // PIECE ACTIVITY: Reward pieces attacking enemy king and controlling center
    // This encourages aggressive, coordinated play
    score += evaluate_piece_activity(board, is_endgame);
    
    // MOBILITY: Reward developed pieces and active positions
    score += evaluate_mobility_simple(board);

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



