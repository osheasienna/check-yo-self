#include "board.h"
#include "move.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

// forward declarations for functions in other files
extern Board make_starting_position();
extern std::vector<move> generate_legal_moves(const Board& board);
extern void make_move(Board& board, const move& m);

// from 4_select_best_move.cpp
extern move find_best_move(const Board& board, int depth);

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
                case 'n': promotion = KNIGHT; break;
            }
        }
        
        return move(from_row, from_col, to_row, to_col, promotion);
    }

    // parse history file and reconstruct board state
    Board parse_history(const std::string& history_path) {
        Board board = make_starting_position();
        std::ifstream history_file(history_path);
        
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
                // Replay the move
                make_move(board, m);
            }
        }
        
        return board;
    }

} // namespace

int main(int argc, char* argv[]) {

    ProgramOptions options;
    if (!parse_arguments(argc, argv, options)) {
        return 1;
    }

    std::cout << "chess-king running...\n";
    
    // 1. Parse history and reconstruct board state
    Board board = parse_history(options.history_path);
    
    // 2. Generate legal moves for the current side
    std::vector<move> moves = generate_legal_moves(board);
    
    if (moves.empty()) {
        std::cerr << "No legal moves available! (Checkmate or Stalemate)\n";
        return 0; // safely exit
    }
    
    // 3. Search for the best move using Negamax
    constexpr int SEARCH_DEPTH = 2; // safer depth for now
    move best_move = find_best_move(board, SEARCH_DEPTH);

    // 4. Write the move using move.h function
    if (!write_move_to_file(best_move, options.move_path)) {
        return 1;
    }

    std::cout << "Wrote move " << move_to_uci(best_move) << " to " << options.move_path << '\n';
    return 0;
}
