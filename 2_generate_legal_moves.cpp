#include "board.h"
#include <vector>

namespace {

bool is_valid_square(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// Helper to add move if square is empty or has enemy piece. Returns true if square was blocked (by friend or enemy).
bool add_move_if_valid(const Board& board, int from_row, int from_col, int to_row, int to_col, std::vector<Move>& moves) {
    if (!is_valid_square(to_row, to_col)) {
        return true;
    }

    const Piece& source_piece = board.squares[from_row][from_col];
    const Piece& target_piece = board.squares[to_row][to_col];

    if (target_piece.type == PieceType::None) {
        moves.push_back({from_row, from_col, to_row, to_col});
        return false; // Not blocked, can continue (for sliding pieces)
    } else if (target_piece.color != source_piece.color) {
        moves.push_back({from_row, from_col, to_row, to_col});
        return true; // Blocked by enemy piece (capture)
    }

    return true; // Blocked by friendly piece
}

void generate_pawn_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    const Piece& piece = board.squares[row][col];
    int direction = (piece.color == Color::White) ? 1 : -1;
    int start_row = (piece.color == Color::White) ? 1 : 6;

    // Move forward 1
    int next_row = row + direction;
    if (is_valid_square(next_row, col) && board.squares[next_row][col].type == PieceType::None) {
        moves.push_back({row, col, next_row, col});

        // Move forward 2 (only if moved forward 1)
        int two_steps_row = row + 2 * direction;
        if (row == start_row && is_valid_square(two_steps_row, col) && board.squares[two_steps_row][col].type == PieceType::None) {
            moves.push_back({row, col, two_steps_row, col});
        }
    }

    // Captures
    int capture_cols[] = {col - 1, col + 1};
    for (int next_col : capture_cols) {
        if (is_valid_square(next_row, next_col)) {
            const Piece& target = board.squares[next_row][next_col];
            if (target.type != PieceType::None && target.color != piece.color) {
                moves.push_back({row, col, next_row, next_col});
            }
        }
    }
}

void generate_knight_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    int offsets[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    for (auto& offset : offsets) {
        add_move_if_valid(board, row, col, row + offset[0], col + offset[1], moves);
    }
}

void generate_sliding_moves(const Board& board, int row, int col, const int directions[][2], int num_directions, std::vector<Move>& moves) {
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

void generate_bishop_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    static const int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    generate_sliding_moves(board, row, col, directions, 4, moves);
}

void generate_rook_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    generate_sliding_moves(board, row, col, directions, 4, moves);
}

void generate_queen_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    static const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    generate_sliding_moves(board, row, col, directions, 8, moves);
}

void generate_king_moves(const Board& board, int row, int col, std::vector<Move>& moves) {
    static const int offsets[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (auto& offset : offsets) {
        add_move_if_valid(board, row, col, row + offset[0], col + offset[1], moves);
    }
}

} 

std::vector<Move> generate_legal_moves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(50);

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            if (piece.color == board.side_to_move) {
                switch (piece.type) {
                    case PieceType::Pawn:
                        generate_pawn_moves(board, row, col, moves);
                        break;
                    case PieceType::Knight:
                        generate_knight_moves(board, row, col, moves);
                        break;
                    case PieceType::Bishop:
                        generate_bishop_moves(board, row, col, moves);
                        break;
                    case PieceType::Rook:
                        generate_rook_moves(board, row, col, moves);
                        break;
                    case PieceType::Queen:
                        generate_queen_moves(board, row, col, moves);
                        break;
                    case PieceType::King:
                        generate_king_moves(board, row, col, moves);
                        break;
                    case PieceType::None:
                    default:
                        break;
                }
            }
        }
    }

    return moves;
}

