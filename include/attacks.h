#ifndef ATTACK_DETECTION_H
#define ATTACK_DETECTION_H

#include "board.h"

// Check if a specific square is attacked by pieces of a given color
// Returns true if any piece of 'by_color' can capture on (target_row, target_col)
bool is_attacked(const Board& board, int target_row, int target_col, Color by_color);

// Check if the king of the given color is in check
bool is_in_check(const Board& board, Color color);

#endif // ATTACK_DETECTION_H