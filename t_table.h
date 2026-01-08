/*

In this file we will setup the transposition table

*/

#pragma once
#include <cstint>
#include <vector>
#include "move.h"


enum TTflag : std::uint8_t 
{
    TT_EXACT = 0, //exact value
    TT_LOWER = 1, //lower bound
    TT_UPPER = 2 //upper bound
};

//represents an entry in the transposition table
struct TTentry 
{
    std::uint64_t key = 0; //hash key
    int depth = -1; //search depth (when entry was stored)
    int value = 0; //evaluation value
    TTflag flag = TT_EXACT; //type of bound
    move best_move{}; //best move found
    bool has_move = false; //indicates if best_move is valid
};

//representing the table
class TranspositionTable
{
    public: //
    explicit TranspositionTable(std::size_t mb = 64) //makes sure the table is the right size
    {
        resize_mb(mb);
    }

    //resizes table to the specified megabytes
    void resize_mb(std::size_t mb)
    {
        std::size_t bytes = mb * 1024ULL * 1024ULL; //converts megabytes to bytes
        std::size_t n = bytes / sizeof(TTentry); 
        if (n < 1) n = 1; //at least one entry
        table.assign(n , TTentry{});
        mask = n -1; //mask matches size of table


        std::size_t pow2 = 1;
        while (pow2 < n) pow2 <<=1;
        table.assign(pow2, TTentry{});
        mask = pow2 - 1;
    }

    //clears table by resenting entries 
    void clear()
    {
        std::fill(table.begin(), table.end(), TTentry{});
    }

    TTentry* probe(std::uint64_t key)
    {
        TTentry& e = table[ley & mask];
        if (e.key == key) return &e;
        return nullptr;
    }

    //stores an entry in the table
    void store(std::uint64_t key, int depth, int value, TTflag flag, const move* best)
    {
        TTentry& e = table[key & mask]; //gets the entry corresponding to the key

        if (e.key != key || depth >= e.depth) //if key is different or depth is greater or equal, we replace the entry
        {
            e.key = key;
            e.depth = depth;
            e.value = value;
            e.flag = flag;
            if(best) //if there is a best move we save it, otherwuse store best move is false
            {
                e.best_move = *best;
                e.has_move = true;
            } else
            {
                e.has_move= false;

            }
        }
    }

    private:
        std::vector<TTentry> table;
        std::size_t mask = 0;
};