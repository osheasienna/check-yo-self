/*

We are going to implement zobrist hasing with transposition tables 
to speed up the algorith 

Here we implement the zobrist hashing


zobrist hasing assigns a random 64-bit number to:
- each piecetype, colour, square
- black or white to move
-castling rights and en passant

*/

#include "zobrist_h.h"
#include "board.h"
#include <cstdint>


//random key tables -> we will use later to compute the hash 
std::uint64_t Z_PIECE[2][7][64]; //2 -> black or white piece, 7 -> 7 types of pieces, 64 -> squares on board
std::uint64_t Z_SIDE;
std::uint64_t Z_CASTLING[16]; 
std::uint64_t Z_ENPASSANT[8];

//we make random looking 64-bit numbers
//but make sure that is we start with the same seed we always get the same random number
static std::uint64_t splitmix64(std::uint64_t& x)
{
    std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL); //the big numbers here are from a classical example of the splitmix464 found online, they are know to work well
    z = (z ^ (z >> 30) * 0xbf58476d1ce4e5b9ULL);
    z = (z ^ (z >> 27) * 0x94d049bb133111eULL);

    return z ^ (z >> 31);

}

void init_zobrist() //we initilaise our hash table with random 64-bit numbers
{
    std::uint64_t seed = 0xC0FFEE123456789ULL;

    for (int c=0; c < 2; ++c)
    {
        for(int pt = 0; pt < 7; ++pt)
        {
            for (int sq = 0; sq < 64; sq++)
            {
                Z_PIECE[c][pt][sq] = splitmix64(seed);
            }
        }
    }

    Z_SIDE = splitmix64(seed);

    for(int i = 0; i < 16; ++i) Z_CASTLING[i] = splitmix64(seed);
    for(int f = 0; f < 8; ++f) Z_ENPASSANT[f] = splitmix64(seed);
}

static inline int sq_index(int row, int col)
{
    return row * 8 + col;
}


//we are now going to compute the hash of the actual position
//combines all the hashes from above that is true (XOR)
std::uint64_t compute_zobrist(const Board& b)
{
    std::uint64_t h = 0; // we start with empty h

    for (int r =0; r < 8; ++ r)
    {
        for (int c=0; c< 8; ++c)
        {
            const Piece& p = b.squares[r][c];
            if(p.type != PieceType::None)
            {
                
                int pt = static_cast<int>(p.type);

                int color_index;

                if(p.color == Color::White)
                    color_index = 0;
                else if (p.color == Color::Black)
                    color_index = 1;
                else
                    continue;

                h ^= Z_PIECE[color_index][pt][sq_index(r, c)];
            }
        }
    }

    if (b.side_to_move == Color::Black) h ^= Z_SIDE;

    int castle = 0;
    if (b.white_can_castle_kingside) castle |= 1;
    if (b.white_can_castle_queenside) castle |= 2;
    if (b.black_can_castle_kingside) castle |= 4;
    if (b.black_can_castle_queenside) castle |= 8;

    h ^= Z_CASTLING[castle];

    if(b.en_passant_col >= 0)
    {
        h ^= Z_ENPASSANT[b.en_passant_col];
    }

    return h;

}