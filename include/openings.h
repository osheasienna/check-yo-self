#ifndef OPENING_BOOK_H
#define OPENING_BOOK_H

#include "move.h"
#include <string>
#include <vector>

// Returns a book move if position is in the opening book
// Returns empty move (0,0,0,0) if not in book
move get_book_move(const std::vector<std::string>& move_history);

// Check if we're still in the "opening phase" (first ~10 moves)
bool is_opening_phase(int move_count);

#endif