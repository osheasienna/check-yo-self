#include "board.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

extern Board make_starting_position();
extern std::vector<Move> generate_legal_moves(const Board& board);

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " -H <path to input history file> -m <path to output move file>\n";
    std::cerr << "   or: " << program_name << " --test\n";
}

struct ProgramOptions {
    std::string history_path;
    std::string move_path;
    bool test_mode = false;
};

bool parse_arguments(int argc, char* argv[], ProgramOptions& options) {
    if (argc == 2 && std::string(argv[1]) == "--test") {
        options.test_mode = true;
        return true;
    }

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

std::string summarize_history(const std::string& history_path) {
    std::ifstream history_file(history_path);
    if (!history_file.is_open()) {
        return "History file not found.";
    }

    std::size_t line_count = 0U;
    std::string line;
    while (std::getline(history_file, line)) {
        if (!line.empty()) {
            ++line_count;
        }
    }

    return "History lines: " + std::to_string(line_count);
}

bool write_dummy_move(const std::string& move_path, const std::string& move) {
    std::ofstream move_file(move_path, std::ios::trunc);
    if (!move_file.is_open()) {
        std::cerr << "Failed to open move file: " << move_path << '\n';
        return false;
    }
    move_file << move << '\n';
    return true;
}

// Convert notation like "e2e4" to Move
Move parse_move(const std::string& move_str) {
    if (move_str.length() < 4) {
        return {-1, -1, -1, -1};
    }
    int from_col = move_str[0] - 'a';
    int from_row = move_str[1] - '1';
    int to_col = move_str[2] - 'a';
    int to_row = move_str[3] - '1';
    return {from_row, from_col, to_row, to_col};
}

// Convert Move to notation like "e2e4"
std::string move_to_string(const Move& move) {
    std::string result;
    result += static_cast<char>('a' + move.from_col);
    result += static_cast<char>('1' + move.from_row);
    result += static_cast<char>('a' + move.to_col);
    result += static_cast<char>('1' + move.to_row);
    return result;
}

// Apply a move to the board
void apply_move(Board& board, const Move& move) {
    Piece piece = board.squares[move.from_row][move.from_col];
    board.squares[move.from_row][move.from_col] = {PieceType::None, Color::None};
    board.squares[move.to_row][move.to_col] = piece;
    // Switch side to move
    board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
}

// Print board (simple ASCII representation)
void print_board(const Board& board) {
    std::cout << "\n  a b c d e f g h\n";
    for (int row = 7; row >= 0; --row) {
        std::cout << (row + 1) << " ";
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Piece& piece = board.squares[row][col];
            char symbol = '.';
            if (piece.type != PieceType::None) {
                char symbols[] = {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};
                symbol = symbols[static_cast<int>(piece.type)];
                if (piece.color == Color::Black) {
                    symbol = static_cast<char>(std::tolower(symbol));
                }
            }
            std::cout << symbol << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\nSide to move: " 
              << (board.side_to_move == Color::White ? "White" : "Black") << "\n\n";
}

void run_test_mode() {
    // starting position
    Board board = make_starting_position();
    
    std::cout << "=== Starting Position ===\n";
    print_board(board);
    
    // Generate and display legal moves
    std::vector<Move> moves = generate_legal_moves(board);
    std::cout << "Legal moves for White: " << moves.size() << "\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        std::cout << move_to_string(moves[i]);
        if ((i + 1) % 10 == 0) {
            std::cout << "\n";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "\n\n";
    
    // Test: Apply a few moves
    std::cout << "=== Testing with moves: e2e4, e7e5, g1f3 ===\n";
    
    // Move 1: e2e4 (White)
    Move move1 = parse_move("e2e4");
    apply_move(board, move1);
    print_board(board);
    
    moves = generate_legal_moves(board);
    std::cout << "Legal moves for Black: " << moves.size() << "\n";
    for (size_t i = 0; i < moves.size() && i < 20; ++i) {
        std::cout << move_to_string(moves[i]) << " ";
    }
    if (moves.size() > 20) {
        std::cout << "... (showing first 20)";
    }
    std::cout << "\n\n";
    
    // Move 2: e7e5 (Black)
    Move move2 = parse_move("e7e5");
    apply_move(board, move2);
    print_board(board);
    
    moves = generate_legal_moves(board);
    std::cout << "Legal moves for White: " << moves.size() << "\n";
    for (size_t i = 0; i < moves.size() && i < 20; ++i) {
        std::cout << move_to_string(moves[i]) << " ";
    }
    if (moves.size() > 20) {
        std::cout << "... (showing first 20)";
    }
    std::cout << "\n\n";
    
    // Move 3: g1f3 (White)
    Move move3 = parse_move("g1f3");
    apply_move(board, move3);
    print_board(board);
    
    moves = generate_legal_moves(board);
    std::cout << "Legal moves for Black: " << moves.size() << "\n";
    for (size_t i = 0; i < moves.size() && i < 20; ++i) {
        std::cout << move_to_string(moves[i]) << " ";
    }
    if (moves.size() > 20) {
        std::cout << "... (showing first 20)";
    }
    std::cout << "\n\n";
}

} 

int main(int argc, char* argv[]) {
    ProgramOptions options;
    if (!parse_arguments(argc, argv, options)) {
        return 1;
    }

    if (options.test_mode) {
        run_test_mode();
        return 0;
    }

    std::cout << "chessbot running...\n";
    std::cout << summarize_history(options.history_path) << '\n';

    constexpr const char* kPlaceholderMove = "e2e4";
    if (!write_dummy_move(options.move_path, kPlaceholderMove)) {
        return 1;
    }

    std::cout << "Wrote move " << kPlaceholderMove << " to " << options.move_path << '\n';
    return 0;
}

