#include "board.h"
#include <vector>

namespace {

bool is_valid_square(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// Check if a specific square is attacked by pieces of a given color
bool is_attacked(const Board& board, int target_row, int target_col, Color by_color) {
    // 1. Pawn attacks
    // If by_color is White: pawns attack from row-1 (they're below, attacking upward)
    // If by_color is Black: pawns attack from row+1 (they're above, attacking downward)
    int pawn_dir = (by_color == Color::White) ? -1 : 1;
    int attack_cols[] = {target_col - 1, target_col + 1};
    for (int col : attack_cols) {
        int row = target_row + pawn_dir;
        if (is_valid_square(row, col)) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::Pawn && p.color == by_color) return true;
        }
    }

    // 2. Knight attacks
    int knight_offsets[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
    for (auto& offset : knight_offsets) {
        int r = target_row + offset[0];
        int c = target_col + offset[1];
        if (is_valid_square(r, c)) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::Knight && p.color == by_color) return true;
        }
    }

    // 3. Sliding pieces (Bishop, Rook, Queen)
    // Diagonals (Bishop, Queen)
    int diag_dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& dir : diag_dirs) {
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int r = target_row + dir[0] * dist;
            int c = target_col + dir[1] * dist;
            if (!is_valid_square(r, c)) break;
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == by_color && (p.type == PieceType::Bishop || p.type == PieceType::Queen)) return true;
                break; // Blocked
            }
        }
    }

    // Straights (Rook, Queen)
    int straight_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& dir : straight_dirs) {
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int r = target_row + dir[0] * dist;
            int c = target_col + dir[1] * dist;
            if (!is_valid_square(r, c)) break;
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == by_color && (p.type == PieceType::Rook || p.type == PieceType::Queen)) return true;
                break; // Blocked
            }
        }
    }

    // 4. King attacks (adjacent enemy king)
    int king_offsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& offset : king_offsets) {
        int r = target_row + offset[0];
        int c = target_col + offset[1];
        if (is_valid_square(r, c)) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::King && p.color == by_color) return true;
        }
    }

    return false;
}

bool is_in_check(const Board& board, Color color) {
    // Find the king
    int king_row = -1, king_col = -1;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (board.squares[row][col].type == PieceType::King && board.squares[row][col].color == color) {
                king_row = row;
                king_col = col;
                break;
            }
        }
        if (king_row != -1) break;
    }

    if (king_row == -1) return false; // Should not happen

    // Check for attacks from enemy pieces
    Color enemy_color = (color == Color::White) ? Color::Black : Color::White;

    // 1. Pawn attacks
    // White pawns capture at row+1 (from their perspective), so if I am White, enemy Black pawns are at row+1.
    // If I am Black, enemy White pawns are at row-1.
    int pawn_dir = (enemy_color == Color::White) ? -1 : 1; 
    // Wait: White pawns move +1. So if enemy is White, they are at king_row-1 attacking king_row.
    // If enemy is Black (move -1), they are at king_row+1 attacking king_row.
    
    int attack_cols[] = {king_col - 1, king_col + 1};
    for (int col : attack_cols) {
        int row = king_row + pawn_dir; 
        if (is_valid_square(row, col)) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::Pawn && p.color == enemy_color) return true;
        }
    }

    // 2. Knight attacks
    int knight_offsets[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};
    for (auto& offset : knight_offsets) {
        int r = king_row + offset[0];
        int c = king_col + offset[1];
        if (is_valid_square(r, c)) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::Knight && p.color == enemy_color) return true;
        }
    }

    // 3. Sliding pieces (Bishop, Rook, Queen)
    // Diagonals (Bishop, Queen)
    int diag_dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& dir : diag_dirs) {
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int r = king_row + dir[0] * dist;
            int c = king_col + dir[1] * dist;
            if (!is_valid_square(r, c)) break;
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == enemy_color && (p.type == PieceType::Bishop || p.type == PieceType::Queen)) return true;
                break; // Blocked
            }
        }
    }

    // Straights (Rook, Queen)
    int straight_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& dir : straight_dirs) {
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int r = king_row + dir[0] * dist;
            int c = king_col + dir[1] * dist;
            if (!is_valid_square(r, c)) break;
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == enemy_color && (p.type == PieceType::Rook || p.type == PieceType::Queen)) return true;
                break; // Blocked
            }
        }
    }

    // 4. King attacks (adjacent enemy king)
    int king_offsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (auto& offset : king_offsets) {
        int r = king_row + offset[0];
        int c = king_col + offset[1];
        if (is_valid_square(r, c)) {
            const Piece& p = board.squares[r][c];
            if (p.type == PieceType::King && p.color == enemy_color) return true;
        }
    }

    return false;
}

// Helper to add move if square is empty or has enemy piece. Returns true if square was blocked (by friend or enemy).
bool add_move_if_valid(const Board& board, int from_row, int from_col, int to_row, int to_col, std::vector<move>& moves) {
    if (!is_valid_square(to_row, to_col)) {
        return true;
    }

    const Piece& source_piece = board.squares[from_row][from_col];
    const Piece& target_piece = board.squares[to_row][to_col];

    if (target_piece.type == PieceType::None) {
        moves.emplace_back(from_row, from_col, to_row, to_col);
        return false; // Not blocked, can continue (for sliding pieces)
    } else if (target_piece.color != source_piece.color) {
        moves.emplace_back(from_row, from_col, to_row, to_col);
        return true; // Blocked by enemy piece (capture)
    }

    return true; // Blocked by friendly piece
}

//pawn pormotion
void pawn_move_promotion(int from_row, int from_col, int to_row, int to_col, Color color, std::vector<move>& moves)
{
    bool promote_rank = (color == Color::White && to_row == BOARD_SIZE -1) ||
                        (color == Color::Black && to_row == 0);


    if (!promote_rank){
        moves.emplace_back(from_row, from_col, to_row, to_col);
        return;
    }

    moves.emplace_back(from_row, from_col, to_row, to_col, QUEEN);
    moves.emplace_back(from_row, from_col, to_row, to_col, ROOK);
    moves.emplace_back(from_row, from_col, to_row, to_col, BISHOP);
    moves.emplace_back(from_row, from_col, to_row, to_col, KNIGHT);

}

void generate_pawn_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    const Piece& piece = board.squares[row][col];
    int direction = (piece.color == Color::White) ? 1 : -1;
    int start_row = (piece.color == Color::White) ? 1 : 6;

    // Move forward 1
    int next_row = row + direction;
    if (is_valid_square(next_row, col) && board.squares[next_row][col].type == PieceType::None) {
        pawn_move_promotion(row, col, next_row, col, piece.color, moves);

        // Move forward 2 (only if moved forward 1)
        int two_steps_row = row + 2 * direction;
        if (row == start_row && is_valid_square(two_steps_row, col) && board.squares[two_steps_row][col].type == PieceType::None) {
            moves.emplace_back(row, col, two_steps_row, col);
        }
    }

    // Normal captures (diagonal captures of enemy pieces)
    int capture_cols[] = {col - 1, col + 1};
    for (int next_col : capture_cols) {
        if (is_valid_square(next_row, next_col)) {
            const Piece& target = board.squares[next_row][next_col];
            if (target.type != PieceType::None && target.color != piece.color) {
                pawn_move_promotion(row, col, next_row, next_col,piece.color, moves);
            }
        }
    }

    // En passant capture: capture a pawn that just moved 2 squares forward
    // Check if an en passant opportunity exists
    if (board.en_passant_col != -1) {
        // Determine the row where en passant captures can happen
        // White pawns on row 4 can capture en passant, Black pawns on row 3 can capture
        int en_passant_row = (piece.color == Color::White) ? 4 : 3;
        
        // Check if our pawn is on the correct row for en passant
        if (row == en_passant_row) {
            // Check if the en passant column is adjacent to our pawn (left or right)
            for (int capture_col : capture_cols) {
                if (capture_col == board.en_passant_col) {
                    // The en passant column is adjacent to our pawn
                    // Verify there's an enemy pawn in the en passant position
                    // The enemy pawn is on the same row as our pawn, in the en passant column
                    const Piece& enemy_pawn = board.squares[row][board.en_passant_col];
                    
                    if (enemy_pawn.type == PieceType::Pawn && enemy_pawn.color != piece.color) {
                        // Valid en passant capture! Add the move
                        // The pawn moves diagonally forward to capture the enemy pawn
                        moves.emplace_back(row, col, next_row, capture_col);
                    }
                }
            }
        }
    }
}

void generate_knight_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    int offsets[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    for (auto& offset : offsets) {
        add_move_if_valid(board, row, col, row + offset[0], col + offset[1], moves);
    }
}

void generate_sliding_moves(const Board& board, int row, int col, const int directions[][2], int num_directions, std::vector<move>& moves) {
    for (int i = 0; i < num_directions; ++i) {
        int d_row = directions[i][0];
        int d_col = directions[i][1];
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int to_row = row + d_row * dist;
            int to_col = col + d_col * dist;
            if (add_move_if_valid(board, row, col, to_row, to_col, moves)) {
                break; // Blocked
            }
        }
    }
}

void generate_bishop_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    static const int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    generate_sliding_moves(board, row, col, directions, 4, moves);
}

void generate_rook_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    generate_sliding_moves(board, row, col, directions, 4, moves);
}

void generate_queen_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    static const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    generate_sliding_moves(board, row, col, directions, 8, moves);
}

void generate_castling_moves(const Board& board, int row, int col, std::vector<move>& moves)
{
    const Piece& king = board.squares[row][col];
    if (king.type != PieceType::King) return;

    Color color = king.color;
    int back_rank = (color == Color::White) ? 0 : 7;

    if (row != back_rank || col != 4) return;

    bool can_kingside = (color == Color::White ? board.white_can_castle_kingside : board.black_can_castle_kingside);

    if (can_kingside )
    {
        if (board.squares[back_rank][5].type == PieceType::None && board.squares[back_rank][6].type == PieceType::None)
        {
            const Piece& rook = board.squares[back_rank][7];
            if(rook.type == PieceType::Rook && rook.color == color)
            {
                Board temp = board;

                make_move(temp, move(back_rank, 4, back_rank, 5, NONE));

                if(!is_in_check(temp, color))
                {
                    Board temp2 = temp;
                    make_move(temp2, move(back_rank, 5, back_rank, 6, NONE));

                    if (!is_in_check(temp2, color))
                    {
                        moves.emplace_back(back_rank, 4, back_rank, 6, NONE);
                    }
                }
            }
        }
    }

    bool can_queenside = (color == Color:: White ? board.white_can_castle_queenside : board.black_can_castle_queenside);

    if (can_queenside)
    {
        if (board.squares[back_rank][3].type == PieceType::None && board.squares[back_rank][2].type == PieceType::None &&
        board.squares[back_rank][1].type == PieceType::None)
        {
            const Piece& rook = board.squares[back_rank][0];
            if (rook.type == PieceType::Rook && rook.color == color)
            {
                Board temp = board;

                make_move(temp, move(back_rank, 4, back_rank, 3, NONE));

                if(!is_in_check(temp, color))
                {
                    Board temp2 = temp;
                    make_move(temp2, move(back_rank, 3, back_rank, 2, NONE));

                    if (!is_in_check(temp2,color))
                    {
                        moves.emplace_back(back_rank, 4, back_rank, 2, NONE);

                    } 
                }
            }

        }
    }


}


void generate_king_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    static const int offsets[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (auto& offset : offsets) {
        add_move_if_valid(board, row, col, row + offset[0], col + offset[1], moves);
    }

    generate_castling_moves(board, row, col, moves);
}

} 

std::vector<move> generate_legal_moves(const Board& board) {
    std::vector<move> moves;
    moves.reserve(50);
    std::vector<move> pseudo_legal_moves;
    pseudo_legal_moves.reserve(50);

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.color == board.side_to_move) {
                switch (piece.type) {
                    case PieceType::Pawn:
                        generate_pawn_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::Knight:
                        generate_knight_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::Bishop:
                        generate_bishop_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::Rook:
                        generate_rook_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::Queen:
                        generate_queen_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::King:
                        generate_king_moves(board, row, col, pseudo_legal_moves);
                        break;
                    case PieceType::None:
                    default:
                        break;
                }
            }
        }
    }

    // Filter moves that leave the king in check
    for (const auto& m : pseudo_legal_moves) {
        Board temp_board = board;
        make_move(temp_board, m);
        if (!is_in_check(temp_board, board.side_to_move)) {
            moves.push_back(m);
        }
    }

    return moves;
}
