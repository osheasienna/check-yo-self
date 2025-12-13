#include "board.h"
#include <vector>

// implemented negamax with alpha-beta pruning
// generate legal moves, simulate the move, evaluate the board, choose best score 

// namespace for local functions
// prevents other files from calling negamax(), select_move(), find_best_move() directly
// can only call find_best_move()
namespace {
constexpr int NEG_INF = -1000000; // negative infinity
constexpr int POS_INF = 1000000; // positive infinity

// helper to convert score based on the current player
int evaluate_for_current_player(const Board& board) {
    // evaluate the board for the current player
    int eval = evaluate_board(board);
    // returns positive score good for side to move, negative score good for side not to move
    return (board.side_to_move == Color::White) ? eval : -eval;
}

/*
 * is_capture_move
 * --------------
 * Returns true if the move is a capture in the current position.
 *
 * A normal capture means the destination square contains an enemy piece.
 * En passant is also a capture even though the destination square is empty.
 */
bool is_capture_move(const Board& board, const move& m) {
    // Normal capture: destination square contains a piece
    const Piece& target = board.squares[m.to_row][m.to_col];
    return target.type != PieceType::None &&
           target.color != board.side_to_move;
}

/*
 * quiescence_search
 * ----------------
 * Extends the search at depth 0 by exploring ONLY capture moves.
 * This avoids the horizon effect where the engine evaluates a position
 * in the middle of a capture exchange.
 *
 * Alpha-beta logic is the same idea as negamax:
 *  - stand_pat is the static evaluation if we stop right now.
 *  - if stand_pat >= beta -> fail-high cutoff.
 *  - otherwise, we try capture moves and update alpha.
 */
int quiescence_search(Board& board, int alpha, int beta) {
    // 1) Static evaluation ("stand pat")
    int stand_pat = evaluate_for_current_player(board);

    // 2) If even standing still is too good, cutoff
    if (stand_pat >= beta) {
        return beta;
    }

    // 3) Improve alpha with stand_pat if possible
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // 4) Generate legal moves and filter to captures
    std::vector<move> moves = generate_legal_moves(board);

    // Terminal position (mate/stalemate)
    if (moves.empty()) {
        return evaluate_terminal(board);
    }

    // 5) Only search capture moves
    for (const move& m : moves) {
        if (!is_capture_move(board, m)) {
            continue;
        }

        // Make the capture (in-place), keeping Undo info
        Undo undo;
        make_move(board, m, undo);

        // Negamax recursion: flip sign and window
        int score = -quiescence_search(board, -beta, -alpha);

        // Restore board
        unmake_move(board, m, undo);

        // Alpha-beta cutoff
        if (score >= beta) {
            return beta;
        }

        // Improve alpha
        if (score > alpha) {
            alpha = score;
        }
    }

    // 6) Best score found in this quiescence node
    return alpha;
}

// NEGAMAX ALGORITHM
/* - each position has a score, positive = good for player to move, negative = bad for player to move
   - when turn changes, flip the score (whats good for the current player is bad for the opponent)

   ALGORITHM STEPS
   1. if depth = 0, return static evaluation of the position 
   2. generate all legal moves
   3. for each move:
        - apply the move to a temporary board
        - call negamax() recursively on the new position with depth - 1
        - negate the returned score to flip for the opponent
        - keep highest score among all moves
    4. use alpha-beta pruning
        - alpha is the best value for the current player so far
        - beta is the best score the opponent can achieve so far
        - if alpha >= beta, stop searching 
*/
// DEPTH: number of moves to look ahead
// ALPHA: best score for the current player
// BETA: best score for the opponent
int negamax(Board& board, int depth, int alpha, int beta) {
    // BASE CASE
    // If depth is exhausted, switch to quiescence search (captures only)
    if (depth == 0) {
        return quiescence_search(board, alpha, beta);
    }

    // Generate all legal moves
    std::vector<move> legal_moves = generate_legal_moves(board);

    // Terminal node: checkmate or stalemate
    if (legal_moves.empty()) {
        return evaluate_terminal(board);
    }

    int best_score = NEG_INF;

    // Search all moves
    for (const auto& candidate : legal_moves) {
        // Apply move in-place and remember everything needed to undo it
        Undo undo;
        make_move(board, candidate, undo);

        // Negamax recursion with flipped alpha/beta window
        int score = -negamax(board, depth - 1, -beta, -alpha);

        // Undo move to restore previous board state
        unmake_move(board, candidate, undo);

        // Update best score
        if (score > best_score) {
            best_score = score;
        }

        // Update alpha
        if (score > alpha) {
            alpha = score;
        }

        // Alpha-beta cutoff
        if (alpha >= beta) {
            break;
        }
    }

    return best_score;
}

// SELECT_MOVE FUNCTION
// BOARD: current board position
// DEPTH: number of moves to look ahead
// returns the best move for the current player
move select_move(const Board& board, int depth) {
    // generate all legal moves
    // legal moves is now a vector of all these moves
    std::vector<move> legal_moves = generate_legal_moves(board);
    // if there are no legal moves => game is over (checkmate or stalemate)
    // returns a dummy move
    if (legal_moves.empty()) {
        return move(0, 0, 0, 0);
    }

    // initialise best_move to the first move in the vector
    move best_move = legal_moves.front();
    // initialise best_score to the worst possible score
    int best_score = NEG_INF;
    // initialise alpha worst possible scores
    int alpha = NEG_INF;
    // initialise beta to the best possible score (best score for the opponent)
    int beta = POS_INF;

    // loop through each legal move
    for (const auto& candidate : legal_moves) {
        Board temp = board;
        // make a copy of the current board
        Undo undo;
        // apply candidate move to the copy
        // next_board = new position after we play candidate move
        make_move(temp, candidate, undo);
        // call the recursive negamax search on resulting position
        // next_board = child position (DEPTH - 1)
        // -alpha and -beta to flip for the opponent
        int score = -negamax(temp, depth - 1, -beta, -alpha);

        unmake_move(temp, candidate, undo);

        // if move gives a better score, update best_score
        if (score > best_score) {
            best_score = score;
            best_move = candidate;
        }
        // update alpha to the best score we are getting from the current move
        if (score > alpha) {
            alpha = score;
        }
    }

    // return the best move for the current player
    return best_move;
}

} // namespace

// FIND_BEST_MOVE FUNCTION
// BOARD: current board position
// DEPTH: number of moves to look ahead
// returns the best move for the current player
// start with first move as deault, replace if a better move is found
move find_best_move(const Board& board, int depth) {
    // CHECK if depth is valid
    // DEPTH = 0 makes engine simply evaluate the current position with no move selection
    if (depth < 1) {
        depth = 1;
    }
    // call the select_move function to get the best move
    return select_move(board, depth);
}
