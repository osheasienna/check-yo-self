
#pragma once 
#include <cstdint>

struct Board;

void init_zobrist(); 

std::uint64_t compute_zobrist(const Board& b);

extern std::uint64_t Z_PIECE[2][7][64]; 
extern std::uint64_t Z_SIDE;
extern std::uint64_t Z_CASTLING[16]; 
extern std::uint64_t Z_ENPASSANT[8];
