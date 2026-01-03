#include "board.h"
#include <vector>
#include <iostream>
#include <cstdlib>
#include <algorithm>  // For std::sort to order moves

// implemented negamax with alpha-beta pruning
// generate legal moves, simulate the move, evaluate the board, choose best score 

// namespace for local functions
// prevents other files from calling negamax(), select_move(), find_best_move() directly
// can only call find_best_move()
namespace {
constexpr int NEG_INF = -1000000; // negative infinity
constexpr int POS_INF = 1000000; // positive infinity
constexpr int MAX_QS_DEPTH = 8;  // Limit quiescence to 8 plies of captures

// DEBUG: verify make_move/unmake_move restore the board perfectly.
// Enable by compiling with -DDEBUG_UNDO
static bool boards_equal(const Board& a, const Board& b) {
    if (a.side_to_move != b.side_to_move) return false;

    if (a.white_can_castle_kingside  != b.white_can_castle_kingside)  return false;
    if (a.white_can_castle_queenside != b.white_can_castle_queenside) return false;
    if (a.black_can_castle_kingside  != b.black_can_castle_kingside) return false;
    if (a.black_can_castle_queenside != b.black_can_castle_queenside) return false;

    if (a.en_passant_row != b.en_passant_row) return false;
    if (a.en_passant_col != b.en_passant_col) return false;

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (a.squares[r][c].type  != b.squares[r][c].type)  return false;
            if (a.squares[r][c].color != b.squares[r][c].color) return false;
        }
    }
    return true;
}

#ifdef DEBUG_UNDO
static void debug_abort_board_mismatch(const move& m) {
    std::cerr << "ERROR: Board mismatch after unmake_move for move: "
              << "(" << m.from_row << "," << m.from_col << ")->(" << m.to_row << "," << m.to_col << ")";
    if (m.promotion != NONE) {
        std::cerr << " promo=" << static_cast<int>(m.promotion);
    }
    std::cerr << std::endl;
    std::abort();
}
#endif


// Material values for MVV-LVA move ordering
// These values match the ones in 3_search_moves.cpp
// Used to calculate: 10 * value_of_victim - value_of_attacker
constexpr int MATERIAL_VALUES[] = {
    0,   // None
    100, // Pawn
    320, // Knight
    330, // Bishop
    500, // Rook
    900, // Queen
    0    // King (not used in captures)
};

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
    const Piece& moving_piece = board.squares[m.from_row][m.from_col];
    const Piece& target = board.squares[m.to_row][m.to_col];

    // Normal capture: destination square contains an enemy piece
    if (target.type != PieceType::None && target.color != board.side_to_move) {
        return true;
    }

    // En passant capture: pawn moves diagonally onto the en passant target square and destination is empty
    if (moving_piece.type == PieceType::Pawn &&
        m.from_col != m.to_col &&
        target.type == PieceType::None &&
        board.en_passant_row == m.to_row &&
        board.en_passant_col == m.to_col) {
        return true;
    }

    return false;
}

/*
 * calculate_move_score
 * --------------------
 * Assigns a score to a move for move ordering using MVV-LVA heuristic.
 * Higher scores = better moves (will be searched first).
 * 
 * MVV-LVA: Most Valuable Victim - Least Valuable Aggressor
 * - Captures: 10 * value_of_victim - value_of_attacker
 *   Example: PxQ (Pawn takes Queen) = 10*900 - 100 = 8900
 *   Example: RxN (Rook takes Knight) = 10*320 - 500 = 2700
 * - Promotions: High score (+900)
 * - Quiet moves: 0
 */
 int calculate_move_score(const Board& board, const move& m) {
    // Check if this is a promotion move
    // Promotions are very valuable, give them high priority
    if (m.promotion != NONE) {
        return 900;  // High score for promotions
    }
    
    // Get the piece that is moving (the aggressor)
    const Piece& moving_piece = board.squares[m.from_row][m.from_col];
    int attacker_value = MATERIAL_VALUES[static_cast<int>(moving_piece.type)];
    
    // Check if this is a normal capture (destination square has an enemy piece)
    const Piece& target = board.squares[m.to_row][m.to_col];
    if (target.type != PieceType::None && target.color != board.side_to_move) {
        // This is a capture! Calculate MVV-LVA score
        int victim_value = MATERIAL_VALUES[static_cast<int>(target.type)];
        // MVV-LVA formula: 10 * victim - attacker
        // Higher victim value = better, lower attacker value = better
        return 10 * victim_value - attacker_value;
    }
    
    // Check for en passant capture
    // En passant: pawn moves diagonally, destination is empty, but en passant is possible
    if (moving_piece.type == PieceType::Pawn &&
        m.from_col != m.to_col &&
        target.type == PieceType::None &&
        board.en_passant_row == m.to_row &&
        board.en_passant_col == m.to_col) {  // En passant target square matches
        
        // En passant captures a pawn (value 100)
        // Attacker is also a pawn (value 100)
        int victim_value = MATERIAL_VALUES[static_cast<int>(PieceType::Pawn)];
        return 10 * victim_value - attacker_value;  // 10 * 100 - 100 = 900
    }
    
    // Quiet move (no capture, no promotion)
    // These moves get the lowest priority
    return 0;
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
 *
 * qs_depth limits how deep we search to prevent explosion in tactical positions.
 */
int quiescence_search(Board& board, int alpha, int beta, int qs_depth) {
    // 1) Static evaluation ("stand pat")
    int stand_pat = evaluate_for_current_player(board);

    // 1b) If quiescence depth exhausted, return static evaluation
    if (qs_depth <= 0) {
        return stand_pat;
    }

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

#ifdef DEBUG_UNDO
        const Board before = board;
#endif
        // Make the capture (in-place), keeping Undo info
        Undo undo;
        make_move(board, m, undo);

        // Negamax recursion: flip sign and window, decrement depth
        int score = -quiescence_search(board, -beta, -alpha, qs_depth - 1);

        // Restore board
        unmake_move(board, m, undo);

#ifdef DEBUG_UNDO
        if (!boards_equal(board, before)) {
            debug_abort_board_mismatch(m);
        }
#endif

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
        return quiescence_search(board, alpha, beta, MAX_QS_DEPTH);
    }

    // Generate all legal moves
    std::vector<move> legal_moves = generate_legal_moves(board);

    // Terminal node: checkmate or stalemate
    // Pass depth so engine prefers faster checkmates
    if (legal_moves.empty()) {
        return evaluate_terminal(board, depth);
    }

        // MOVE ORDERING: Sort moves by score (highest first) using MVV-LVA heuristic
    // This drastically improves alpha-beta pruning efficiency
    // Good moves (captures, promotions) are tried first, leading to more cutoffs
    std::sort(legal_moves.begin(), legal_moves.end(), 
        [&board](const move& a, const move& b) {
            // Sort in descending order (highest score first)
            // Moves with higher scores will be searched first
            return calculate_move_score(board, a) > calculate_move_score(board, b);
        });

    int best_score = NEG_INF;

    // Search all moves
    for (const auto& candidate : legal_moves) {
#ifdef DEBUG_UNDO
        const Board before = board;
#endif
        // Apply move in-place and remember everything needed to undo it
        Undo undo;
        make_move(board, candidate, undo);

        // Negamax recursion with flipped alpha/beta window
        int score = -negamax(board, depth - 1, -beta, -alpha);

        // Undo move to restore previous board state
        unmake_move(board, candidate, undo);

#ifdef DEBUG_UNDO
        if (!boards_equal(board, before)) {
            debug_abort_board_mismatch(candidate);
        }
#endif

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
#ifdef DEBUG_UNDO
        const Board before = temp;
#endif
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

#ifdef DEBUG_UNDO
        if (!boards_equal(temp, before)) {
            debug_abort_board_mismatch(candidate);
        }
#endif

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
