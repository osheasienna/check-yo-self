#include "board.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <ctime>

extern Board make_starting_position();
extern std::vector<Move> generate_legal_moves(const Board& board);
extern void make_move(Board& board, const Move& move);

namespace {

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " -H <path to input history file> -m <path to output move file>\n";
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

// Convert notation like "e2e4" to Move
Move parse_move(const std::string& move_str) {
    if (move_str.length() < 4) {
        return {-1, -1, -1, -1};
    }
    int from_col = move_str[0] - 'a';
    int from_row = move_str[1] - '1';
    int to_col = move_str[2] - 'a';
    int to_row = move_str[3] - '1';
    
    PieceType promotion = PieceType::None;
    if (move_str.length() >= 5) {
        switch (move_str[4]) {
            case 'q': promotion = PieceType::Queen; break;
            case 'r': promotion = PieceType::Rook; break;
            case 'b': promotion = PieceType::Bishop; break;
            case 'n': promotion = PieceType::Knight; break;
        }
    }
    
    return {from_row, from_col, to_row, to_col, promotion};
}

// Convert Move to notation like "e2e4"
std::string move_to_string(const Move& move) {
    std::string result;
    result += static_cast<char>('a' + move.from_col);
    result += static_cast<char>('1' + move.from_row);
    result += static_cast<char>('a' + move.to_col);
    result += static_cast<char>('1' + move.to_row);
    
    if (move.promotion != PieceType::None) {
        switch (move.promotion) {
            case PieceType::Queen: result += 'q'; break;
            case PieceType::Rook: result += 'r'; break;
            case PieceType::Bishop: result += 'b'; break;
            case PieceType::Knight: result += 'n'; break;
            default: break;
        }
    }
    return result;
}

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
        while (!line.empty() && std::isspace(line.back())) {
            line.pop_back();
        }
        while (!line.empty() && std::isspace(line.front())) {
            line.erase(0, 1);
        }

        if (!line.empty()) {
            Move move = parse_move(line);
            // Replay the move
            make_move(board, move);
        }
    }
    
    return board;
}

bool write_move(const std::string& move_path, const Move& move) {
    std::ofstream move_file(move_path, std::ios::trunc);
    if (!move_file.is_open()) {
        std::cerr << "Failed to open move file: " << move_path << '\n';
        return false;
    }
    move_file << move_to_string(move) << '\n';
    return true;
}

// Apply a move to the board (wrapper for make_move for compatibility with test)
void apply_move(Board& board, const Move& move) {
    make_move(board, move);
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
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    ProgramOptions options;
    if (!parse_arguments(argc, argv, options)) {
        return 1;
    }

    if (options.test_mode) {
        run_test_mode();
        return 0;
    }

    std::cout << "chess-king running...\n";
    
    Board board = parse_history(options.history_path);


    std::cerr << "=== Debug: Board State after history ===\n";
    std::cerr << "  a b c d e f g h\n";
    for (int row = 7; row >= 0; --row) {
        std::cerr << (row + 1) << " ";
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
            std::cerr << symbol << " ";
        }
        std::cerr << "\n";
    }
    std::cerr << "\nSide to move: " 
              << (board.side_to_move == Color::White ? "White" : "Black") << "\n\n";
    

    std::vector<Move> moves = generate_legal_moves(board);
    
    if (moves.empty()) {
        std::cerr << "No legal moves available! (Checkmate or Stalemate)\n";

        return 1;
    }
    
    int random_index = std::rand() % moves.size();
    Move best_move = moves[random_index];
    if (!write_move(options.move_path, best_move)) {
        return 1;
    }

    std::cout << "Wrote move " << move_to_string(best_move) << " to " << options.move_path << '\n';
    return 0;
}
