#include "board.h"
#include "move.h"
#include "../zobrist_h.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdint>

// forward declarations for functions in other files
extern Board make_starting_position();
extern std::vector<move> generate_legal_moves(const Board& board);
extern void make_move(Board& board, const move& m);

// from 4_select_best_move.cpp
extern move find_best_move(const Board& board, int depth);

// Repetition detection functions from 4_select_best_move.cpp
extern void add_position_to_history(std::uint64_t hash);
extern void clear_position_history();
extern size_t get_position_history_size();

// from 5_generate_output.cpp
std::string move_to_uci(move m);

namespace {

    // print usage message for incorrect arguments
    void print_usage(const char* program_name) {
        std::cerr << "Usage: " << program_name
                << " -H <path to input history file> -m <path to output move file>\n";
    }

    // struct to hold CLI options
    struct ProgramOptions {
        std::string history_path;
        std::string move_path;
    };

    // parse -H and -m command line arguments
    bool parse_arguments(int argc, char* argv[], ProgramOptions& options) {
        if (argc < 5) {
            print_usage(argv[0]);
            return false;
        }

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "-H" && i + 1 < argc) {
                options.history_path = argv[++i];
            } else if (arg == "-m" && i + 1 < argc) {
                options.move_path = argv[++i];
            } else {
                print_usage(argv[0]);
                return false;
            }
        }

        if (options.history_path.empty() || options.move_path.empty()) {
            print_usage(argv[0]);
            return false;
        }

        return true;
    }

    // Convert notation like "e2e4" to move struct
    move parse_move(const std::string& move_str) {
        if (move_str.length() < 4) {
            std::cerr << "Warning: Invalid move string '" << move_str << "' (too short)\n";
            return move(0, 0, 0, 0); // Invalid
        }
        int from_col = move_str[0] - 'a';
        int from_row = move_str[1] - '1';
        int to_col = move_str[2] - 'a';
        int to_row = move_str[3] - '1';
        
        promotion_piece_type promotion = NONE;
        if (move_str.length() >= 5) {
            switch (move_str[4]) {
                case 'q': promotion = QUEEN; break;
                case 'r': promotion = ROOK; break;
                case 'b': promotion = BISHOP; break;
                case 'k': promotion = KNIGHT; break;
            }
        }
        
        return move(from_row, from_col, to_row, to_col, promotion);
    }

    // Print the board state for debugging/verification
    // Displays the chess board in a readable format with piece positions,
    // current side to move, and castling rights
    void print_board(const Board& board) {
        std::cout << "\n  a b c d e f g h\n";
        for (int row = 7; row >= 0; --row) {
            std::cout << (row + 1) << " ";
            for (int col = 0; col < 8; ++col) {
                const Piece& p = board.squares[row][col];
                char c = '.';
                if (p.type != PieceType::None) {
                    // Map piece types to characters: P=Pawn, N=Knight, B=Bishop, R=Rook, Q=Queen, K=King
                    char pieces[] = {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};
                    c = pieces[static_cast<int>(p.type)];
                    // Lowercase for black pieces, uppercase for white
                    if (p.color == Color::Black) c = std::tolower(c);
                }
                std::cout << c << " ";
            }
            std::cout << "\n";
        }
        // Display current side to move
        std::cout << "Side to move: " << (board.side_to_move == Color::White ? "White" : "Black") << "\n";
        // Display castling rights: W-K=White Kingside, W-Q=White Queenside, B-K=Black Kingside, B-Q=Black Queenside
        std::cout << "Castling: W-K=" << board.white_can_castle_kingside 
                  << " W-Q=" << board.white_can_castle_queenside
                  << " B-K=" << board.black_can_castle_kingside
                  << " B-Q=" << board.black_can_castle_queenside << "\n\n";
    }

    // parse history file and reconstruct board state
    // ALSO tracks all positions in history for threefold repetition detection
    Board parse_history(const std::string& history_path) {
        Board board = make_starting_position();
        std::ifstream history_file(history_path);
        
        // Clear any previous position history and add starting position
        clear_position_history();
        add_position_to_history(compute_zobrist(board));
        
        if (!history_file.is_open()) {
            std::cerr << "Warning: History file not found. Assuming starting position.\n";
            return board;
        }

        std::string line;
        while (std::getline(history_file, line)) {
            // Trim whitespace (including \r)
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) {
                line.pop_back();
            }
            while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
                line.erase(0, 1);
            }

            if (!line.empty()) {
                move m = parse_move(line);
                
                // VALIDATION: Check that the move is legal before applying
                // This catches history file corruption or format mismatches
                std::vector<move> legal = generate_legal_moves(board);
                bool is_valid = false;
                for (const auto& lm : legal) {
                    if (lm.from_row == m.from_row && lm.from_col == m.from_col &&
                        lm.to_row == m.to_row && lm.to_col == m.to_col &&
                        lm.promotion == m.promotion) {
                        is_valid = true;
                        break;
                    }
                }
                
                if (!is_valid) {
                    std::cerr << "ERROR: Illegal move in history file: '" << line << "'\n";
                    std::cerr << "  Parsed as: (" << m.from_row << "," << m.from_col 
                              << ")->(" << m.to_row << "," << m.to_col << ")\n";
                    // Continue anyway to see full error output
                }
                
                // Replay the move
                make_move(board, m);
                
                // REPETITION TRACKING: Store position hash after each move
                // This builds the history of all positions in the game
                // so we can detect threefold repetition
                add_position_to_history(compute_zobrist(board));
            }
        }
        
        std::cerr << "Position history: " << get_position_history_size() << " positions tracked\n";
        
        return board;
    }

} // namespace

int main(int argc, char* argv[]) {

    ProgramOptions options;
    if (!parse_arguments(argc, argv, options)) {
        return 1;
    }

    // Initialize Zobrist hash tables (required for future transposition table)
    init_zobrist();

    std::cout << "chess-king running...\n";
    
    // 1. Parse history and reconstruct board state
    Board board = parse_history(options.history_path);
    
    // Display board state for verification (helps verify history was loaded correctly)
    print_board(board);
    
    // Display evaluation score for verification
    // Positive score = White advantage, negative score = Black advantage
    int eval = evaluate_board(board);
    std::cout << "Evaluation: " << eval << " (positive = White advantage, negative = Black advantage)\n";
    
    // 2. Generate legal moves for the current side
    std::vector<move> moves = generate_legal_moves(board);
    
    if (moves.empty()) {
        std::cerr << "No legal moves available! (Checkmate or Stalemate)\n";
        return 0; // safely exit
    }
    
    // 3. Search for the best move using Negamax with time control
    // Iterative deepening: starts at depth 1, increases until time runs out
    // TIME_LIMIT_MS: Stay well under tournament time limit to prevent timeouts
    constexpr int MAX_SEARCH_DEPTH = 6;  // Maximum depth to search
    constexpr int TIME_LIMIT_MS = 800;   // 800ms limit - stay under 1 second
    move best_move = find_best_move(board, MAX_SEARCH_DEPTH, TIME_LIMIT_MS);

    // SAFETY CHECK: Validate that best_move is actually in the legal moves list
    // This prevents writing invalid moves that could cause the engine to lose
    // This should never happen, but it's a critical safety check
    bool is_legal = false;
    for (const auto& legal_move : moves) {
        // Compare all fields: from/to squares and promotion
        if (legal_move.from_row == best_move.from_row &&
            legal_move.from_col == best_move.from_col &&
            legal_move.to_row == best_move.to_row &&
            legal_move.to_col == best_move.to_col &&
            legal_move.promotion == best_move.promotion) {
            is_legal = true;
            break;
        }
    }

    if (!is_legal) {
        std::cerr << "Warning: Best move " << move_to_uci(best_move) 
                  << " is not in legal moves list! Using first legal move as fallback.\n";
        if (!moves.empty()) {
            best_move = moves.front();
        } else {
            std::cerr << "Fatal: No legal moves available!\n";
            return 1;
        }
    }

    // 4. Write the move using move.h function
    if (!write_move_to_file(best_move, options.move_path)) {
        return 1;
    }

    std::cout << "Wrote move " << move_to_uci(best_move) << " to " << options.move_path << '\n';
    return 0;
}
