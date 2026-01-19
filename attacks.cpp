#include "board.h"

namespace {

bool is_valid_square(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

} // anonymous namespace

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

    // Straights (Rook)
    int rook_dirs[3][2] = {{1, 0}, {-1, 0}, {0, 1}};
    for (auto& dir : rook_dirs) {
        for (int dist = 1; dist < BOARD_SIZE; ++dist) {
            int r = target_row + dir[0] * dist;
            int c = target_col + dir[1] * dist;
            if (!is_valid_square(r, c)) break;
            const Piece& p = board.squares[r][c];
            if (p.type != PieceType::None) {
                if (p.color == by_color && (p.type == PieceType::Rook)) return true;
                break; // Blocked
            }
        }
    }

//queen attacks
    int queen_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& dir : queen_dirs) {
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
    int king_row = -1, king_col = -1;

    // Find the king
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.type == PieceType::King && p.color == color) {
                king_row = row;
                king_col = col;
                row = BOARD_SIZE; // break outer loop
                break;
            }
        }
    }

    if (king_row == -1) return false; // should never happen

    Color enemy = (color == Color::White) ? Color::Black : Color::White;
    return is_attacked(board, king_row, king_col, enemy);
}