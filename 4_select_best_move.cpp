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
    // if DEPTH = 0, stop searching and return evaluation of the board
    if (depth == 0) {
        return evaluate_for_current_player(board);
    }

    // RECURSIVE CASE
    // generate all legal moves
    std::vector<move> legal_moves = generate_legal_moves(board);
    // if no legal moves => either in checkmate or stalemate
    // doesn't check these states independently, just evaluates the board as is
    // TO DO: treat checkmate and stalemate states separately
    if (legal_moves.empty()) {
        return evaluate_for_current_player(board);
    }

    // start with worst possible score for current player
    int best_score = NEG_INF;

    // loop through each legal move that player can make
    for (const auto& candidate : legal_moves) {
        // make a copy of the current board
        Board next_board = board;
        // apply the move to the copy
        // new_board = new poisition after the move is applied
        make_move(next_board, candidate);
        // search the position resulting from playing this current move, but one level deeper
        // (DEPTH - 1)
        // -negamax because the opponent is now the player to make the move
        // -beta and -alpha to flip for the opponent
        int score = -negamax(next_board, depth - 1, -beta, -alpha);

        // if move gives a better score, update best_score
        if (score > best_score) {
            best_score = score;
        }
        // // update alpha to the best score we are getting from the current move
        if (score > alpha) {
            alpha = score;
        }
        // stop checking further moves
        if (alpha >= beta) {
            break; // Alpha-Beta cutoff
        }
    }
    // return the best score for the current player
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
        // make a copy of the current board
        Board next_board = board;
        // apply candidate move to the copy
        // next_board = new position after we play candidate move
        make_move(next_board, candidate);
        // call the recursive negamax search on resulting position
        // next_board = child position (DEPTH - 1)
        // -alpha and -beta to flip for the opponent
        int score = -negamax(next_board, depth - 1, -beta, -alpha);

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
