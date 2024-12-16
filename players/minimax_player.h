#ifndef MINIMAX_PLAYER_H
#define MINIMAX_PLAYER_H

#include "players.h"
#include <stdbool.h>
#include <stdint.h>
#include <Python.h>

typedef struct {
    BasicPlayer base;
    int max_depth;
    bool debug;
    int iter;
    bool abp;
    int (*evaluate_func)(uint64_t player_board, uint64_t opponent_board);
} MiniMaxPlayer;

extern PyTypeObject MiniMaxPlayerType;

#endif /* MINIMAX_PLAYER_H */
