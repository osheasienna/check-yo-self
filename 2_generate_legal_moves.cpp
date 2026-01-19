#include "board.h"
#include <vector>
#include "attacks.h"

namespace {

bool is_valid_square(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
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
    // An en passant opportunity exists iff a target square is set
    if (board.en_passant_row != -1 && board.en_passant_col != -1) {
        // Our pawn must land diagonally on the en passant target square
        int en_passant_rank = (piece.color == Color::White) ? 4 : 3;

        if (row == en_passant_rank && next_row == board.en_passant_row) {
            for (int capture_col : capture_cols) {
                if (capture_col == board.en_passant_col) {
                    // The captured pawn sits on the same row as the capturing pawn
                    const Piece& enemy_pawn = board.squares[row][board.en_passant_col];

                    if (enemy_pawn.type == PieceType::Pawn &&
                        enemy_pawn.color != piece.color) {
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
    static const int directions[3][2] = {{1, 0}, {-1, 0}, {0, 1}};
    generate_sliding_moves(board, row, col, directions, 3, moves);
}

void generate_queen_moves(const Board& board, int row, int col, std::vector<move>& moves) {
    static const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    generate_sliding_moves(board, row, col, directions, 8, moves);
}

void generate_castling_moves(Board& board, int row, int col, std::vector<move>& moves)
{
    const Piece& king = board.squares[row][col];
    if (king.type != PieceType::King) return;

    Color color = king.color;
    int back_rank = (color == Color::White) ? 0 : 7;

    // King must be on starting square e-file
    if (row != back_rank || col != 4) return;

    Color enemy = (color == Color::White) ? Color::Black : Color::White;

    // Can't castle out of check
    if (is_attacked(board, back_rank, 4, enemy)) return;

    // Kingside
    bool can_kingside = (color == Color::White)
        ? board.white_can_castle_kingside
        : board.black_can_castle_kingside;

    if (can_kingside) {
        // Squares between king and rook must be empty: f, g
        if (board.squares[back_rank][5].type == PieceType::None &&
            board.squares[back_rank][6].type == PieceType::None) {

            // Rook must be on h-file
            const Piece& rook = board.squares[back_rank][7];
            if (rook.type == PieceType::Rook && rook.color == color) {

                // Can't pass through or land on attacked squares
                if (!is_attacked(board, back_rank, 5, enemy) &&
                    !is_attacked(board, back_rank, 6, enemy)) {

                    moves.emplace_back(back_rank, 4, back_rank, 6, NONE);
                }
            }
        }
    }

    // Queenside
    bool can_queenside = (color == Color::White)
        ? board.white_can_castle_queenside
        : board.black_can_castle_queenside;

    if (can_queenside) {
        // Squares between king and rook must be empty: d, c, b
        if (board.squares[back_rank][3].type == PieceType::None &&
            board.squares[back_rank][2].type == PieceType::None &&
            board.squares[back_rank][1].type == PieceType::None) {

            // Rook must be on a-file
            const Piece& rook = board.squares[back_rank][0];
            if (rook.type == PieceType::Rook && rook.color == color) {

                // Can't pass through or land on attacked squares
                if (!is_attacked(board, back_rank, 3, enemy) &&
                    !is_attacked(board, back_rank, 2, enemy)) {

                    moves.emplace_back(back_rank, 4, back_rank, 2, NONE);
                }
            }
        }
    }
}



void generate_king_moves(Board& board, int row, int col, std::vector<move>& moves) {
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

std::vector<move> generate_legal_moves(const Board& board_in) {
    Board board = board_in;

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
        Undo u;
        Color us = board.side_to_move;
        make_move(board, m, u);

        if (!is_in_check(board, us)) {
            moves.push_back(m);
        }

        unmake_move(board, m , u);
    }

    return moves;
}
