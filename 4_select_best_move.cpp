#include "board.h"
#include "../zobrist_h.h"
#include "../t_table.h"   // Transposition table for caching positions
#include <vector>
#include <iostream>
#include <cstdlib>
#include <algorithm>  // For std::sort to order moves
#include <cmath>
#include <cstdint>
#include <chrono>        // For time management
#include <unordered_map> // For O(1) repetition detection
#include "attacks.h"

// ============================================================================
// TRANSPOSITION TABLE - Caches evaluated positions for faster search
// ============================================================================
// The TT stores positions we've already evaluated so we don't re-evaluate them.
// This dramatically speeds up iterative deepening and positions reached via
// different move orders (transpositions).
// ============================================================================
static TranspositionTable g_tt(64);  // 64 MB transposition table

// Clear the transposition table (call between games if needed)
void clear_transposition_table() {
    g_tt.clear();
}

// implemented negamax with alpha-beta pruning
// generate legal moves, simulate the move, evaluate the board, choose best score

// ============================================================================
// TIME MANAGEMENT - Prevents timeouts by checking elapsed time during search
// ============================================================================
// The engine uses iterative deepening with time control:
// - Start with depth 1, increase depth after each completed search
// - Check time periodically during search and abort if time runs out
// - Always return the best move found so far
// ============================================================================

static std::chrono::steady_clock::time_point g_search_start;
static int g_time_limit_ms = 0;
static bool g_search_aborted = false;

// Initialize time limit for search
void set_time_limit(int ms) {
    g_search_start = std::chrono::steady_clock::now();
    g_time_limit_ms = ms;
    g_search_aborted = false;
}

// Check if time limit has been exceeded
bool time_is_up() {
    if (g_time_limit_ms <= 0) return false;  // No time limit set
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_search_start).count();
    return elapsed >= g_time_limit_ms;
}

// Check if search was aborted due to timeout
bool search_was_aborted() {
    return g_search_aborted;
} 

// ============================================================================
// REPETITION DETECTION - Prevents threefold repetition draws (O(1) lookup)
// ============================================================================
// We store Zobrist hashes of all positions that occurred in the game.
// If a position appears 3 times, it's a draw. We also treat 2 repetitions
// as a "likely draw" in the search to discourage repeating positions.
//
// OPTIMIZATION: Uses unordered_map for O(1) count lookup instead of O(n) scan.
// The vector maintains order for push/pop operations during search.
// ============================================================================

// Global history of positions (hashes) from the game - maintains order for undo
static std::vector<std::uint64_t> g_position_history;
// Hash count map for O(1) repetition counting
static std::unordered_map<std::uint64_t, int> g_position_count;

// Count how many times a hash appears in history - O(1) lookup
static int count_repetitions(std::uint64_t hash) {
    auto it = g_position_count.find(hash);
    if (it != g_position_count.end()) {
        return it->second;
    }
    return 0;
}

// Add a position to history (called when parsing game history and during search)
void add_position_to_history(std::uint64_t hash) {
    g_position_history.push_back(hash);
    g_position_count[hash]++;
}

// Remove the last position from history (for undoing moves in search)
void remove_last_position_from_history() {
    if (!g_position_history.empty()) {
        std::uint64_t hash = g_position_history.back();
        g_position_history.pop_back();
        
        // Decrement count in map
        auto it = g_position_count.find(hash);
        if (it != g_position_count.end()) {
            it->second--;
            if (it->second <= 0) {
                g_position_count.erase(it);  // Clean up zero-count entries
            }
        }
    }
}

// Clear all position history (called at start of new game)
void clear_position_history() {
    g_position_history.clear();
    g_position_count.clear();
}

// Get current history size (for debugging)
size_t get_position_history_size() {
    return g_position_history.size();
}

// namespace for local functions
// prevents other files from calling negamax(), select_move(), find_best_move() directly
// can only call find_best_move()
namespace {
constexpr int NEG_INF = -1000000; // negative infinity
constexpr int POS_INF = 1000000; // positive infinity
constexpr int MAX_QS_DEPTH = 10;  // Limit quiescence to 10 plies of captures
constexpr int DRAW_SCORE = 0;    // Score for draw by repetition

// ============================================================================
// CONTEMPT FACTOR - Makes the engine avoid draws
// ============================================================================
// A positive contempt means "I think I'm stronger, so draws are bad for me"
// This encourages the engine to keep playing rather than accept a draw.
// Set to 0 for objective play, positive to avoid draws, negative to seek draws.
// ============================================================================
constexpr int CONTEMPT = 25;     // Treat draws as -25 centipawns (slight loss)

// ============================================================================
// NULL MOVE PRUNING CONSTANTS
// ============================================================================
// Null move pruning: If we can "pass" and still have a good position, cut off.
// This dramatically reduces the search tree size.
// ============================================================================
constexpr int NULL_MOVE_REDUCTION = 3;  // Reduce depth by 3 for null move search
constexpr int NULL_MOVE_MIN_DEPTH = 3;  // Only apply null move at depth >= 3

// ============================================================================
// KILLER MOVES - Quiet moves that caused cutoffs at each depth
// ============================================================================
// Killer move heuristic: Remember quiet (non-capture) moves that caused
// beta cutoffs. These moves are likely good in sibling positions at the
// same depth, so we try them early in move ordering.
// 
// We store 2 killers per depth (slots 0 and 1). When we find a new killer,
// we shift the old killer[0] to killer[1] and store the new one in killer[0].
// ============================================================================
constexpr int MAX_KILLER_DEPTH = 64;
static move g_killers[MAX_KILLER_DEPTH][2];  // 2 killer moves per depth

// Initialize killer move table (call at start of search)
void clear_killers() {
    for (int d = 0; d < MAX_KILLER_DEPTH; ++d) {
        g_killers[d][0] = move(0, 0, 0, 0);
        g_killers[d][1] = move(0, 0, 0, 0);
    }
}

// Store a killer move at the given depth
void store_killer(int depth, const move& m) {
    if (depth < 0 || depth >= MAX_KILLER_DEPTH) return;
    
    // Don't store if it's already killer[0]
    if (g_killers[depth][0].from_row == m.from_row &&
        g_killers[depth][0].from_col == m.from_col &&
        g_killers[depth][0].to_row == m.to_row &&
        g_killers[depth][0].to_col == m.to_col) {
        return;
    }
    
    // Shift killer[0] to killer[1], store new killer in [0]
    g_killers[depth][1] = g_killers[depth][0];
    g_killers[depth][0] = m;
}

// Check if a move is a killer at the given depth
bool is_killer(int depth, const move& m) {
    if (depth < 0 || depth >= MAX_KILLER_DEPTH) return false;
    
    for (int i = 0; i < 2; ++i) {
        if (g_killers[depth][i].from_row == m.from_row &&
            g_killers[depth][i].from_col == m.from_col &&
            g_killers[depth][i].to_row == m.to_row &&
            g_killers[depth][i].to_col == m.to_col) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// HISTORY HEURISTIC - Track quiet moves that caused cutoffs historically
// ============================================================================
// The history heuristic keeps track of which quiet moves have been good in the
// past. When a quiet move causes a beta cutoff, we increase its history score.
// This helps order quiet moves better, putting historically good moves first.
//
// Array: g_history[color][from_square][to_square]
// - color: 0 = White, 1 = Black
// - from_square: 0-63 (row * 8 + col)
// - to_square: 0-63 (row * 8 + col)
// ============================================================================
static int g_history[2][64][64];  // History scores for quiet moves

// Clear history table (call at start of search)
void clear_history() {
    for (int c = 0; c < 2; ++c) {
        for (int from = 0; from < 64; ++from) {
            for (int to = 0; to < 64; ++to) {
                g_history[c][from][to] = 0;
            }
        }
    }
}

// Update history score for a move that caused a cutoff
// Bonus is depth * depth to favor deeper cutoffs
void update_history(Color side, const move& m, int depth) {
    int color_idx = (side == Color::White) ? 0 : 1;
    int from_sq = m.from_row * 8 + m.from_col;
    int to_sq = m.to_row * 8 + m.to_col;
    
    // Add depth^2 bonus (deeper cutoffs are more valuable)
    g_history[color_idx][from_sq][to_sq] += depth * depth;
    
    // Cap history values to prevent overflow and keep values reasonable
    if (g_history[color_idx][from_sq][to_sq] > 10000) {
        g_history[color_idx][from_sq][to_sq] = 10000;
    }
}

// Get history score for a move
int get_history_score(Color side, const move& m) {
    int color_idx = (side == Color::White) ? 0 : 1;
    int from_sq = m.from_row * 8 + m.from_col;
    int to_sq = m.to_row * 8 + m.to_col;
    return g_history[color_idx][from_sq][to_sq];
}

// Check if position has enough material to avoid zugzwang
// Zugzwang = position where any move worsens the position
// Common in endgames with only pawns, so we require at least one non-pawn piece
bool has_non_pawn_material(const Board& board, Color side) {
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& p = board.squares[row][col];
            if (p.color == side && 
                p.type != PieceType::None && 
                p.type != PieceType::Pawn && 
                p.type != PieceType::King) {
                return true;  // Has knight, bishop, rook, or queen
            }
        }
    }
    return false;
}

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
 * Move ordering priority:
 * 1. TT move (handled separately)
 * 2. Captures ordered by MVV-LVA (Most Valuable Victim - Least Valuable Aggressor)
 * 3. Killer moves (quiet moves that caused cutoffs at this depth)
 * 4. Other quiet moves
 * 
 * MVV-LVA: Most Valuable Victim - Least Valuable Aggressor
 * - Captures: 10 * value_of_victim - value_of_attacker
 *   Example: PxQ (Pawn takes Queen) = 10*900 - 100 = 8900
 *   Example: RxN (Rook takes Knight) = 10*320 - 500 = 2700
 * - Promotions: High score (+1000)
 * - Killer moves: Medium score (+800)
 * - Quiet moves: 0 or small positional bonus
 */
 int calculate_move_score(const Board& board, const move& m, int depth = 0) {
    // Check if this is a promotion move
    // Promotions are very valuable, give them high priority
    if (m.promotion != NONE) {
        return 1000;  // High score for promotions
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

    // Quiet-moves
    // These only matter when moves are otherwise equal (no capture/promo)
    // Keep magnitudes small so move ordering still dominates

    // Encourage castling (king moves 2 squares)
    if (moving_piece.type == PieceType::King && std::abs(m.to_col - m.from_col) == 2) {
        return 50;
    }

    // Discourage early king moves (non-castling)
    if (moving_piece.type == PieceType::King) {
        return -20;
    }

    // Discourage early rook moves a bit ( pointless before development)
    if (moving_piece.type == PieceType::Rook) {
        return -10;
    }

    // Encourage developing knights and bishops off the back rank
    if (moving_piece.type == PieceType::Knight || moving_piece.type == PieceType::Bishop) {
        // White back rank = row 0, Black back rank = row 7 in your board representation
        if ((moving_piece.color == Color::White && m.from_row == 0) ||
            (moving_piece.color == Color::Black && m.from_row == 7)) {
            return 10;
        }
    }

    // KILLER MOVE BONUS: Quiet moves that caused cutoffs at this depth
    // Give them high priority (after captures but before other quiet moves)
    if (is_killer(depth, m)) {
        return 800;  // High score for killer moves
    }

    // HISTORY HEURISTIC: Use historical success of quiet moves
    // Moves that caused cutoffs in the past are likely good here too
    int history_score = get_history_score(board.side_to_move, m);
    if (history_score > 0) {
        // Scale history to be between 0 and 700 (below killer moves)
        return std::min(700, history_score / 15);
    }

    // Quiet move (no capture, no promotion, not a killer, no history)
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
    // TIME CHECK: Abort search if time limit exceeded
    if (time_is_up()) {
        g_search_aborted = true;
        return evaluate_for_current_player(board);  // Return current eval
    }

    // If quiescence depth exhausted, return static eval
    if (qs_depth <= 0) {
        return evaluate_for_current_player(board);
    }

    // If side to move is in check, we MUST consider all legal evasions.
    // Stand-pat is not legal while in check.
    const bool in_check = is_in_check(board, board.side_to_move);
    if (in_check) {
        std::vector<move> moves = generate_legal_moves(board);

        // If no legal moves, it's mate/stalemate
        if (moves.empty()) {
            return evaluate_terminal(board);
        }

        for (const move& m : moves) {
#ifdef DEBUG_UNDO
            const Board before = board;
#endif
            Undo undo;
            make_move(board, m, undo);

            int score = -quiescence_search(board, -beta, -alpha, qs_depth - 1);

            unmake_move(board, m, undo);

#ifdef DEBUG_UNDO
            if (!boards_equal(board, before)) {
                debug_abort_board_mismatch(m);
            }
#endif

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    // Normal quiescence: stand-pat + captures only
    int stand_pat = evaluate_for_current_player(board);

    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    std::vector<move> moves = generate_legal_moves(board);

    if (moves.empty()) {
        return evaluate_terminal(board);
    }

    for (const move& m : moves) {
        bool dominated = !is_capture_move(board, m);
        
        // Also consider moves that give check (might be mate!)
        if (dominated) {
            Undo check_undo;
            make_move(board, m, check_undo);
            bool gives_check = is_in_check(board, board.side_to_move);
            unmake_move(board, m, check_undo);
            if (!gives_check) continue;  // Skip quiet non-checking moves
        }

#ifdef DEBUG_UNDO
        const Board before = board;
#endif
        Undo undo;
        make_move(board, m, undo);

        int score = -quiescence_search(board, -beta, -alpha, qs_depth - 1);

        unmake_move(board, m, undo);

#ifdef DEBUG_UNDO
        if (!boards_equal(board, before)) {
            debug_abort_board_mismatch(m);
        }
#endif

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

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
    // TIME CHECK: Abort search if time limit exceeded
    if (time_is_up()) {
        g_search_aborted = true;
        return 0;  // Return immediately with neutral score
    }

    // BASE CASE
    // If depth is exhausted, switch to quiescence search (captures only)
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, MAX_QS_DEPTH);
    }

    // Save original alpha for determining TT bound type later
    const int original_alpha = alpha;
    
    // =========================================================================
    // TRANSPOSITION TABLE PROBE
    // =========================================================================
    // Check if we've seen this position before at sufficient depth
    std::uint64_t pos_hash = compute_zobrist(board);
    TTentry* tt_entry = g_tt.probe(pos_hash);
    move tt_move;  // Best move from TT (for move ordering)
    bool has_tt_move = false;
    
    if (tt_entry != nullptr && tt_entry->depth >= depth) {
        // We have a cached result at sufficient depth
        if (tt_entry->flag == TT_EXACT) {
            // Exact score - can return immediately
            return tt_entry->value;
        } else if (tt_entry->flag == TT_LOWER) {
            // Lower bound - raise alpha if possible
            alpha = std::max(alpha, tt_entry->value);
        } else if (tt_entry->flag == TT_UPPER) {
            // Upper bound - lower beta if possible
            beta = std::min(beta, tt_entry->value);
        }
        
        // Check for cutoff from TT bounds
        if (alpha >= beta) {
            return tt_entry->value;
        }
    }
    
    // Extract TT best move for move ordering (even if depth was insufficient)
    if (tt_entry != nullptr && tt_entry->has_move) {
        tt_move = tt_entry->best_move;
        has_tt_move = true;
    }

    // =========================================================================
    // NULL MOVE PRUNING
    // =========================================================================
    // If we're not in check and have enough material, try "passing" our turn.
    // If opponent can't beat beta even with a free move, we can cut off.
    // This gives ~2 plies of extra effective depth.
    // =========================================================================
    const bool in_check_before_moves = is_in_check(board, board.side_to_move);
    
    // Only apply null move at sufficient depth (need depth > reduction + 1)
    // Also avoid null move when we might be in zugzwang (low material)
    if (!in_check_before_moves && 
        depth >= NULL_MOVE_MIN_DEPTH + NULL_MOVE_REDUCTION &&  // Ensure positive search depth
        beta < POS_INF - 1000 &&  // Not searching for mate
        beta > NEG_INF + 1000 &&  // Not in a losing position
        has_non_pawn_material(board, board.side_to_move)) {
        
        // Make null move: just switch sides without moving
        Board null_board = board;
        null_board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
        null_board.en_passant_row = -1;  // Clear en passant after null move
        null_board.en_passant_col = -1;
        
        // Search with reduced depth and null window
        int null_depth = depth - 1 - NULL_MOVE_REDUCTION;
        if (null_depth > 0) {  // Extra safety check
            int null_score = -negamax(null_board, null_depth, -beta, -beta + 1);
            
            // If null move search fails high (score >= beta), we can cut off
            // The idea: if we can pass and still be >= beta, actually moving will be even better
            if (null_score >= beta && !g_search_aborted) {
                return beta;  // Trust the null move result
            }
        }
    }

    // Generate all legal moves
    std::vector<move> legal_moves = generate_legal_moves(board);

    // Terminal node: checkmate or stalemate
    // Pass depth so engine prefers faster checkmates
    if (legal_moves.empty()) {
        return evaluate_terminal(board, depth);
    }

    // =========================================================================
    // MOVE ORDERING: TT move first, then MVV-LVA
    // =========================================================================
    // The TT move (if valid) is likely the best move from previous search
    if (has_tt_move) {
        // Find and move TT move to front if it's in the legal moves list
        for (size_t i = 0; i < legal_moves.size(); ++i) {
            if (legal_moves[i].from_row == tt_move.from_row &&
                legal_moves[i].from_col == tt_move.from_col &&
                legal_moves[i].to_row == tt_move.to_row &&
                legal_moves[i].to_col == tt_move.to_col &&
                legal_moves[i].promotion == tt_move.promotion) {
                // Swap TT move to front
                std::swap(legal_moves[0], legal_moves[i]);
                break;
            }
        }
    }
    
    // Sort remaining moves (skip first if it's TT move) by MVV-LVA + killers
    auto sort_start = has_tt_move ? legal_moves.begin() + 1 : legal_moves.begin();
    std::stable_sort(sort_start, legal_moves.end(), 
        [&board, depth](const move& a, const move& b) {
            return calculate_move_score(board, a, depth) > calculate_move_score(board, b, depth);
        });

    int best_score = NEG_INF;
    move best_move = legal_moves.front();  // Track best move for TT storage
    
    // Use the check status we already computed for null move pruning
    const bool in_check = in_check_before_moves;

    // Search all moves
    for (size_t move_index = 0; move_index < legal_moves.size(); ++move_index) {
        const move& candidate = legal_moves[move_index];
        
        // LMR checks - must be done BEFORE make_move changes the board
        bool is_capture = is_capture_move(board, candidate);
        bool is_promotion = (candidate.promotion != NONE);
        bool can_reduce = (move_index >= 4) && (depth >= 3) && 
                          !is_capture && !is_promotion && !in_check;
        
#ifdef DEBUG_UNDO
        const Board before = board;
#endif
        // Apply move in-place and remember everything needed to undo it
        Undo undo;
        make_move(board, candidate, undo);

        // =====================================================================
        // CHECK EXTENSION
        // =====================================================================
        // extending by 1 ply helps find these sequences that would otherwise
        // be cut off by the horizon effect
        // =====================================================================
        bool gives_check = is_in_check(board, board.side_to_move);
        int extension = gives_check ? 1 : 0;

        // REPETITION DETECTION: Check if this position has been seen before
        // Compute hash of the new position and check history
        std::uint64_t pos_hash = compute_zobrist(board);
        int repetition_count = count_repetitions(pos_hash);
        
        int score;
        if (repetition_count >= 2) {
            // Position would appear 3+ times = forced draw
            // Apply CONTEMPT: treat draws as slightly bad (we want to keep playing!)
            score = -(DRAW_SCORE - CONTEMPT);  // With contempt, draws are negative
        } else {
            // Late Move Reductions (LMR): Search late quiet moves at reduced depth first
            // Don't reduce if the move gives check
            
            add_position_to_history(pos_hash);
            
            // Don't apply LMR to checking moves
            bool can_reduce_this_move = can_reduce && !gives_check;
            
            if (can_reduce_this_move) {
                // LMR: Reduced depth search with null window
                int reduction = 1 + (depth > 6 ? 1 : 0);  // Reduce by 1-2 plies
                score = -negamax(board, depth - 1 - reduction + extension, -alpha - 1, -alpha);
                
                // If reduced search beats alpha, re-search at full depth
                if (score > alpha) {
                    score = -negamax(board, depth - 1 + extension, -beta, -alpha);
                }
            } else {
                // Full depth search (with extension if giving check)
                score = -negamax(board, depth - 1 + extension, -beta, -alpha);
            }
            
            remove_last_position_from_history();
            
            // STRONGER repetition penalty - avoid repeating when we have ANY advantage
            // The bigger our advantage, the more we should avoid repetition
            if (repetition_count == 1 && score > DRAW_SCORE - CONTEMPT) {
                int penalty = 25;                   // Base penalty
                if (score > 50) penalty = 50;       // We're slightly better
                if (score > 100) penalty = 75;      // We're up ~1 pawn
                if (score > 200) penalty = 100;     // We're up ~2 pawns
                if (score > 300) penalty = 150;     // We're clearly winning
                score = score - penalty;
            }
        }

        // Undo move to restore previous board state
        unmake_move(board, candidate, undo);

#ifdef DEBUG_UNDO
        if (!boards_equal(board, before)) {
            debug_abort_board_mismatch(candidate);
        }
#endif

        // Update best score and best move
        if (score > best_score) {
            best_score = score;
            best_move = candidate;
        }

        // Update alpha
        if (score > alpha) {
            alpha = score;
        }

        // Alpha-beta cutoff
        if (alpha >= beta) {
            // KILLER MOVE: Store quiet moves that cause cutoffs
            // These are likely good in sibling positions at the same depth
            if (!is_capture && !is_promotion) {
                store_killer(depth, candidate);
                // HISTORY HEURISTIC: Update history for this quiet move
                update_history(board.side_to_move, candidate, depth);
            }
            break;
        }
    }

    // =========================================================================
    // TRANSPOSITION TABLE STORE
    // =========================================================================
    // Store the result for future lookups (if search wasn't aborted)
    if (!g_search_aborted) {
        TTflag flag;
        if (best_score <= original_alpha) {
            // Failed low - this is an upper bound (we didn't find anything better)
            flag = TT_UPPER;
        } else if (best_score >= beta) {
            // Failed high - this is a lower bound (real score is at least this good)
            flag = TT_LOWER;
        } else {
            // Score is within window - this is the exact score
            flag = TT_EXACT;
        }
        
        g_tt.store(pos_hash, depth, best_score, flag, &best_move);
    }

    return best_score;
}

// Result struct to return both move and score from search
struct SearchResult {
    move best_move;
    int score;
};

// SELECT_MOVE FUNCTION
// BOARD: current board position
// DEPTH: number of moves to look ahead
// INIT_ALPHA, INIT_BETA: Optional bounds for aspiration windows (default: full window)
// returns the best move and score for the current player
SearchResult select_move(const Board& board, int depth, int init_alpha = NEG_INF, int init_beta = POS_INF) {
    // generate all legal moves
    // legal moves is now a vector of all these moves
    std::vector<move> legal_moves = generate_legal_moves(board);
    // if there are no legal moves => game is over (checkmate or stalemate)
    // returns a dummy move
    if (legal_moves.empty()) {
        return {move(0, 0, 0, 0), 0};
    }

    // BUG FIX: Sort moves by MVV-LVA + killers for better alpha-beta pruning efficiency
    // This was missing - without sorting, alpha-beta is much less effective
    // We use the same calculate_move_score() function that negamax() uses
    // Good moves (captures, promotions, killers) are tried first, leading to more cutoffs
    std::stable_sort(legal_moves.begin(), legal_moves.end(), 
        [&board, depth](const move& a, const move& b) {
            // Sort in descending order (highest score first)
            // Moves with higher scores will be searched first
            return calculate_move_score(board, a, depth) > calculate_move_score(board, b, depth);
        });

    // initialise best_move to the first move in the vector
    move best_move = legal_moves.front();
    // initialise best_score to the worst possible score
    int best_score = NEG_INF;
    // Use provided alpha/beta for aspiration windows
    int alpha = init_alpha;
    int beta = init_beta;

    // loop through each legal move
    for (const auto& candidate : legal_moves) {
        // TIME CHECK: Abort search if time limit exceeded
        if (time_is_up()) {
            g_search_aborted = true;
            break;  // Return best move found so far
        }

        Board temp = board;
#ifdef DEBUG_UNDO
        const Board before = temp;
#endif
        // make a copy of the current board
        Undo undo;
        // apply candidate move to the copy
        // next_board = new position after we play candidate move
        make_move(temp, candidate, undo);
        
        // REPETITION DETECTION at root level
        // Check if this move leads to a repeated position
        std::uint64_t pos_hash = compute_zobrist(temp);
        int repetition_count = count_repetitions(pos_hash);
        
        int score;
        if (repetition_count >= 2) {
            // This move would cause threefold repetition = draw
            // Apply CONTEMPT: treat draws as slightly bad (we want to keep playing!)
            score = -(DRAW_SCORE - CONTEMPT);
            std::cerr << "Note: Move leads to repetition, applying contempt\n";
        } else {
            // Track this position during search
            add_position_to_history(pos_hash);
            // call the recursive negamax search on resulting position
            // next_board = child position (DEPTH - 1)
            // -alpha and -beta to flip for the opponent
            score = -negamax(temp, depth - 1, -beta, -alpha);
            remove_last_position_from_history();
            
            // STRONGER repetition penalty - avoid repeating when we have ANY advantage
            if (repetition_count == 1 && score > DRAW_SCORE - CONTEMPT) {
                int penalty = 30;                   // Base penalty (stronger at root)
                if (score > 50) penalty = 60;       // We're slightly better
                if (score > 100) penalty = 90;      // We're up ~1 pawn
                if (score > 200) penalty = 120;     // We're up ~2 pawns
                if (score > 300) penalty = 200;     // We're clearly winning - NEVER repeat!
                score = score - penalty;
            }
        }

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

    // return the best move and score for the current player
    return {best_move, best_score};
}

} // namespace

// Mate detection threshold - scores near CHECKMATE_SCORE indicate forced mate
constexpr int MATE_THRESHOLD = 90000;    // CHECKMATE_SCORE is 100000
// Minimum advantage to consider "clearly winning" for early stop
constexpr int CLEARLY_WINNING = 300;     // ~3 pawns or a piece up

// FIND_BEST_MOVE FUNCTION WITH ITERATIVE DEEPENING AND TIME CONTROL
// BOARD: current board position
// MAX_DEPTH: maximum number of moves to look ahead
// TIME_LIMIT_MS: time limit in milliseconds (0 = no limit)
// returns the best move for the current player
// Uses iterative deepening: searches depth 1, then 2, etc. until time runs out
// Always returns a valid move (at minimum, depth 1 result or first legal move)
move find_best_move(const Board& board, int max_depth, int time_limit_ms) {
    // Initialize time control
    set_time_limit(time_limit_ms);
    
    // Clear killer moves and history from previous search
    clear_killers();
    clear_history();
    
    // CHECK if depth is valid
    if (max_depth < 1) {
        max_depth = 1;
    }
    
    // Generate legal moves first to have a fallback
    std::vector<move> legal_moves = generate_legal_moves(board);
    if (legal_moves.empty()) {
        return move(0, 0, 0, 0);  // No legal moves (checkmate/stalemate)
    }
    
    // Get static evaluation to help decide early termination
    // Positive = good for white, need to adjust for side to move
    int static_eval = evaluate_board(board);
    int eval_for_us = (board.side_to_move == Color::White) ? static_eval : -static_eval;
    std::cerr << "Static eval for side to move: " << eval_for_us << "\n";
    
    // Always have a fallback move (first legal move)
    move best_move = legal_moves.front();
    int best_score = NEG_INF;
    
    // =========================================================================
    // ITERATIVE DEEPENING WITH ASPIRATION WINDOWS
    // =========================================================================
    // After the first iteration, we use a narrow window around the previous
    // score. If the search fails outside this window, we widen and re-search.
    // This typically saves time because most searches stay within the window.
    // =========================================================================
    constexpr int ASPIRATION_DELTA = 50;  // Initial window size (+/- 50 centipawns)
    
    for (int depth = 1; depth <= max_depth; ++depth) {
        // Check time before starting new depth
        if (time_is_up()) {
            std::cerr << "Time limit reached before depth " << depth << "\n";
            break;
        }
        
        SearchResult result;
        
        // Use aspiration windows only after depth 4 with a valid previous score
        // This provides speedup without the instability at low depths
        bool use_aspiration = (depth >= 5 && 
                               best_score > NEG_INF + 5000 && 
                               best_score < POS_INF - 5000 &&
                               std::abs(best_score) < MATE_THRESHOLD);
        
        if (!use_aspiration) {
            result = select_move(board, depth);
        } else {
            // ASPIRATION WINDOWS: Start with narrow window around previous score
            constexpr int ASPIRATION_DELTA = 50;
            int delta = ASPIRATION_DELTA;
            int asp_alpha = std::max(best_score - delta, NEG_INF);
            int asp_beta = std::min(best_score + delta, POS_INF);
            
            result = select_move(board, depth, asp_alpha, asp_beta);
            
            // If search failed outside window and wasn't aborted, re-search with full window
            if (!g_search_aborted && (result.score <= asp_alpha || result.score >= asp_beta)) {
                std::cerr << "Aspiration re-search at depth " << depth << "\n";
                result = select_move(board, depth);
            }
        }
        
        // Only update best move if search completed without timeout
        if (!g_search_aborted) {
            best_move = result.best_move;
            best_score = result.score;
            std::cerr << "Completed depth " << depth << " (score: " << best_score << ")\n";
            
            // EARLY TERMINATION: Stop if we found a forced mate
            if (best_score >= MATE_THRESHOLD) {
                std::cerr << "Found forced mate! Stopping search early.\n";
                break;
            }
            
            // EARLY TERMINATION: Stop if static eval shows we're winning
            // AND search confirms it at depth 4+
            // This saves time in clearly winning positions
            if (depth >= 4 && eval_for_us >= CLEARLY_WINNING && best_score >= CLEARLY_WINNING) {
                std::cerr << "Position is clearly winning (eval: " << eval_for_us 
                          << ", search: " << best_score << "), stopping search.\n";
                break;
            }
        } else {
            std::cerr << "Search aborted at depth " << depth << "\n";
            break;  // Stop iterating if search was aborted
        }
    }
    
    return best_move;
}

// Backward-compatible overload for existing code
move find_best_move(const Board& board, int depth) {
    // No time limit - use original behavior
    return find_best_move(board, depth, 0);
}
