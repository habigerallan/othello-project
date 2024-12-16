// othello/othello.h

#ifndef OTHELLO_H
#define OTHELLO_H

#include <Python.h>
#include <stdint.h>
#include <stdbool.h>

#define BOARD_SIZE 8

// Macro definitions for exporting/importing functions
#ifdef _WIN32
    #ifdef OTHELLO_EXPORTS
        #define OTHELLO_API __declspec(dllexport)
    #else
        #define OTHELLO_API __declspec(dllimport)
    #endif
#else
    #define OTHELLO_API
#endif

typedef struct {
    int dr;
    int dc;
} Direction;

extern const Direction DIRECTIONS[8];

typedef struct {
    int moves[60];
    int count;
} MoveList;

typedef struct {
    int bits[64];
    int count;
} BitList;

typedef struct {
    PyObject_HEAD
    uint64_t black_board;
    uint64_t white_board;
    PyObject* black_player;
    PyObject* white_player;
    PyObject* current_player;
    bool debug;
} OthelloGameObject;

extern PyTypeObject OthelloGameType;

// Function declarations
OTHELLO_API void get_valid_moves(uint64_t player_board, uint64_t opponent_board, MoveList* move_list);
OTHELLO_API void get_flipped_bits(int move, uint64_t player_board, uint64_t opponent_board, BitList* bit_list);
OTHELLO_API int popcount64(uint64_t x);
OTHELLO_API bool is_valid_move(int move, uint64_t player_board, uint64_t opponent_board);
OTHELLO_API bool is_game_over(OthelloGameObject* self);
OTHELLO_API int OthelloGame_apply_move(OthelloGameObject* self, int move);

#endif /* OTHELLO_H */
