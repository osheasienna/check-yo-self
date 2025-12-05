#include "board.h"

#include <vector>

namespace {

constexpr int NEG_INF = -1000000;
constexpr int POS_INF = 1000000;

int evaluate_for_current_player(const Board& board) {
    int eval = evaluate_board(board);
    return (board.side_to_move == Color::White) ? eval : -eval;
}

int negamax(Board& board, int depth, int alpha, int beta) {
    if (depth == 0) {
        return evaluate_for_current_player(board);
    }

    std::vector<move> legal_moves = generate_legal_moves(board);
    if (legal_moves.empty()) {
        return evaluate_for_current_player(board);
    }

    int best_score = NEG_INF;

    for (const auto& candidate : legal_moves) {
        Board next_board = board;
        make_move(next_board, candidate);
        int score = -negamax(next_board, depth - 1, -beta, -alpha);

        if (score > best_score) {
            best_score = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break; // Alpha-Beta cutoff
        }
    }

    return best_score;
}

move select_move(const Board& board, int depth) {
    std::vector<move> legal_moves = generate_legal_moves(board);
    if (legal_moves.empty()) {
        return move(0, 0, 0, 0);
    }

    move best_move = legal_moves.front();
    int best_score = NEG_INF;
    int alpha = NEG_INF;
    int beta = POS_INF;

    for (const auto& candidate : legal_moves) {
        Board next_board = board;
        make_move(next_board, candidate);
        int score = -negamax(next_board, depth - 1, -beta, -alpha);

        if (score > best_score) {
            best_score = score;
            best_move = candidate;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return best_move;
}

} // namespace

move find_best_move(const Board& board, int depth) {
    if (depth < 1) {
        depth = 1;
    }
    return select_move(board, depth);
}

