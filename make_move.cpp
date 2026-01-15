#include "board.h"
#include "zobrist_h.h"
#include <cmath>


static inline int zc(Color c){
    if (c == Color::White) return 0;
    if (c == Color::Black) return 1;
   return -1;
}

static inline int castle_index(const Board& b){
    int castle = 0;
    if (b.white_can_castle_kingside)  castle |= 1;
    if (b.white_can_castle_queenside) castle |= 2;
    if (b.black_can_castle_kingside)  castle |= 4;
    if (b.black_can_castle_queenside) castle |= 8;
    return castle;
}

static inline int sq_index(int row, int col)
{
    return row * 8 + col;
}


void make_move(Board& board, const move& m, Undo& undo) {

    // Save current board state for undo
    undo.white_can_castle_kingside = board.white_can_castle_kingside;
    undo.white_can_castle_queenside = board.white_can_castle_queenside;
    undo.black_can_castle_kingside = board.black_can_castle_kingside;
    undo.black_can_castle_queenside = board.black_can_castle_queenside;
    undo.side_to_move = board.side_to_move;
    undo.en_passant_row = board.en_passant_row;  // Save en passant state
    undo.en_passant_col = board.en_passant_col;
    undo.was_en_passant = false;

    undo.zobrist_hash = board.zobrist_hash;

    if (board.en_passant_col != -1) {
        // Clear the en passant hash if it was set
        board.zobrist_hash ^= Z_ENPASSANT[board.en_passant_col];
    }

    board.zobrist_hash ^= Z_CASTLING[castle_index(board)];

    // Reset en passant at the start of each move (will be set again if a 2-square pawn move happens)
    board.en_passant_row = -1;
    board.en_passant_col = -1;

    Piece piece = board.squares[m.from_row][m.from_col];
    Piece captured = board.squares[m.to_row][m.to_col];

    const int pc = zc(piece.color);
    if (pc < 0) return;

    PieceType final_type = piece.type;
    if (piece.type == PieceType::Pawn && m.promotion != NONE){
        switch(m.promotion){
            case QUEEN: final_type = PieceType::Queen; break;
            case ROOK: final_type = PieceType::Rook; break;
            case BISHOP: final_type = PieceType::Bishop; break;
            case KNIGHT: final_type = PieceType::Knight; break;
            default: break;
        }
    }

    board.zobrist_hash ^= Z_PIECE[pc][static_cast<int>(piece.type)][sq_index(m.from_row, m.from_col)];

    // Check if this is an en passant capture
    // En passant happens when: pawn moves diagonally, destination is empty, and destination matches en passant target square
    if (piece.type == PieceType::Pawn &&
        m.from_col != m.to_col &&  // Moving diagonally (capture)
        captured.type == PieceType::None &&  // Destination square is empty
        undo.en_passant_row == m.to_row &&
        undo.en_passant_col == m.to_col) {  // Destination matches en passant target square
        
        // This is an en passant capture!
        undo.was_en_passant = true;
        // The captured pawn is on the "passed over" square (same row as source, destination column)
        int captured_pawn_row = m.from_row;
        int captured_pawn_col = m.to_col;
        captured = board.squares[captured_pawn_row][captured_pawn_col];
        // Remove the captured pawn from its original position
       if (captured.type != PieceType::None)
        {
            int cc = zc(captured.color);
            board.zobrist_hash ^= Z_PIECE[cc][(int)captured.type][sq_index(captured_pawn_row, captured_pawn_col)];
        }

        board.squares[captured_pawn_row][captured_pawn_col] = {PieceType::None, Color::None};
    }

    if (!undo.was_en_passant && captured.type != PieceType::None)
    {
        int cc = zc(captured.color);
        board.zobrist_hash ^= Z_PIECE[cc][(int)captured.type][sq_index(m.to_row, m.to_col)];
    }


    // store info needed for unmake move, piece type and captured
    undo.moved_piece_type = piece.type;
    undo.captured = captured;

    // if rook is captured, then opponent castling rights might be lost
    if (!undo.was_en_passant && captured.type == PieceType::Rook)
    {
        if (captured.color == Color::White)
        {
            if (m.to_row == 0 && m.to_col == 0)
                board.white_can_castle_queenside = false;
            if (m.to_row == 0 && m.to_col == 7)
                board.white_can_castle_kingside = false;
        } else if (captured.color == Color::Black)
        {
            if (m.to_row == 7 && m.to_col == 0)
                board.black_can_castle_queenside = false;
            if(m.to_row == 7 && m.to_col == 7)
                board.black_can_castle_kingside = false;
        }
    }

    board.squares[m.from_row][m.from_col] = {PieceType::None, Color::None};

    if (piece.type == PieceType::King)
    {
        int row = m.from_row;
        int delta_col = m.to_col - m.from_col;

        if (piece.color == Color::White)
        {
            board.white_can_castle_kingside = false;
            board.white_can_castle_queenside = false;
        } else 
        {
            board.black_can_castle_kingside = false;
            board.black_can_castle_queenside = false;
        }

        if (std::abs(delta_col) == 2)
        {
            if (delta_col > 0)
            {
                Piece rook = board.squares[row][7];
                board.squares[row][7] = {PieceType::None, Color::None};
                board.squares[row][5] = rook;

                int rc = zc(rook.color);
                board.zobrist_hash ^= Z_PIECE[rc][(int)rook.type][sq_index(row, 7)];
                board.zobrist_hash ^= Z_PIECE[rc][(int)rook.type][sq_index(row, 5)];
            } else 
            {
                Piece rook = board.squares[row][0];
                board.squares[row][0] = {PieceType::None, Color::None};
                board.squares[row][3] = rook;

                int rc = zc(rook.color);
                board.zobrist_hash ^= Z_PIECE[rc][(int)rook.type][sq_index(row, 0)];
                board.zobrist_hash ^= Z_PIECE[rc][(int)rook.type][sq_index(row, 3)];
            }
        }
    }

    if (piece.type == PieceType::Rook)
    {
        if (piece.color == Color::White)
        {
            if(m.from_row == 0 && m.from_col == 0)
                board.white_can_castle_queenside = false;
            if(m.from_row == 0 && m.from_col == 7)
                board.white_can_castle_kingside = false;
        } else if  (piece.color == Color::Black)
        {
            if (m.from_row == 7 && m.from_col == 0)
                board.black_can_castle_queenside = false;
            if(m.from_row == 7 && m.from_col == 7)
                board.black_can_castle_kingside = false;
        }
    }

    piece.type = final_type;
    board.squares[m.to_row][m.to_col] = piece;

    board.zobrist_hash ^= Z_PIECE[pc][(int)piece.type][sq_index(m.to_row, m.to_col)];

    // Detect if a pawn moved 2 squares forward from its starting position
    // This creates an en passant opportunity for the opponent
    if (undo.moved_piece_type == PieceType::Pawn) {
        int start_row = (piece.color == Color::White) ? 1 : 6;
        int row_delta = std::abs(m.to_row - m.from_row);
        
        // If pawn moved exactly 2 squares forward from starting row, set en passant target square
        if (m.from_row == start_row && row_delta == 2) {
            // The en passant target is the square the pawn "passed over"
            int direction = (piece.color == Color::White) ? 1 : -1;
            board.en_passant_row = m.from_row + direction;
            board.en_passant_col = m.from_col;

            board.zobrist_hash ^= Z_ENPASSANT[board.en_passant_col];
        }
    }

    board.zobrist_hash ^= Z_CASTLING[castle_index(board)];

    // Switch side to move
    board.side_to_move = (board.side_to_move == Color::White) ? Color::Black : Color::White;
    board.zobrist_hash ^= Z_SIDE;
}

// introduce a dummy variable to avoid duplicate move logic while keeping plain make move for places where undo is unecessary
void make_move(Board& board, const move& m)
{
    Undo dummy; 
    make_move(board, m, dummy);
}

void unmake_move(Board& board, const move& m, const Undo& undo)
{
    // Restore board state
    board.side_to_move = undo.side_to_move;
    board.en_passant_row = undo.en_passant_row;
    board.en_passant_col = undo.en_passant_col;  // Restore en passant state

    board.white_can_castle_kingside = undo.white_can_castle_kingside;
    board.white_can_castle_queenside = undo.white_can_castle_queenside;
    board.black_can_castle_kingside = undo.black_can_castle_kingside;
    board.black_can_castle_queenside = undo.black_can_castle_queenside;

    Piece piece = board.squares[m.to_row][m.to_col];

    // Handle castling undo (restore rook position)
    if(undo.moved_piece_type == PieceType::King)
    {
        int row = m.from_row;
        int delta_col = m.to_col - m.from_col;

        if (std::abs(delta_col) == 2)
        {
            if (delta_col > 0)
            {
                board.squares[row][7] = board.squares[row][5];
                board.squares[row][5] = {PieceType::None, Color::None};

            } else
            {
                board.squares[row][0] =  board.squares[row][3];
                board.squares[row][3] = {PieceType::None, Color::None};

            }
        }
    }

    // Restore piece type (in case of promotion)
    piece.type = undo.moved_piece_type;

    // Restore the moved piece to its original position
    board.squares[m.from_row][m.from_col] = piece;
    
    // Restore captured piece.
    // For en passant, the destination square was empty and the captured pawn lives on the "passed over" square.
    if (!undo.was_en_passant) {
        board.squares[m.to_row][m.to_col] = undo.captured;
    } else {
        // Destination was empty in en passant
        board.squares[m.to_row][m.to_col] = {PieceType::None, Color::None};

        // Restore the captured pawn on the passed-over square (same row as source, destination column)
        int captured_pawn_row = m.from_row;
        int captured_pawn_col = m.to_col;
        board.squares[captured_pawn_row][captured_pawn_col] = undo.captured;
    }

    board.zobrist_hash = undo.zobrist_hash;
}
