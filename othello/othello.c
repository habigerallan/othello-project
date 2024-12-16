// othello/othello.c

#define OTHELLO_EXPORTS

#include "othello.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <intrin.h>

OTHELLO_API int popcount64(uint64_t x) {
    return (int)__popcnt64(x);
}

#elif defined(__GNUC__) || defined(__clang__)
OTHELLO_API int popcount64(uint64_t x) {
    return __builtin_popcountll(x);
}
#else
OTHELLO_API int popcount64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return (int)(x & 0x7F);
}
#endif

const Direction DIRECTIONS[8] = {
    {-1, 0},  // Up
    {1, 0},   // Down
    {0, -1},  // Left
    {0, 1},   // Right
    {-1, -1}, // Up-Left
    {-1, 1},  // Up-Right
    {1, -1},  // Down-Left
    {1, 1}    // Down-Right
};

static void OthelloGame_initialize_boards(OthelloGameObject* self);
static uint64_t set_piece(int row, int col, uint64_t board);

static PyObject* OthelloGame_display_board(OthelloGameObject* self);
static PyObject* OthelloGame_make_move(OthelloGameObject* self);

static uint64_t set_piece(int row, int col, uint64_t board) {
    int bit = (row << 3) + col;
    board |= 1ULL << bit;
    return board;
}

static void OthelloGame_initialize_boards(OthelloGameObject* self) {
    self->black_board = 0ULL;
    self->white_board = 0ULL;
    self->black_board = set_piece(3, 4, self->black_board);
    self->black_board = set_piece(4, 3, self->black_board);
    self->white_board = set_piece(3, 3, self->white_board);
    self->white_board = set_piece(4, 4, self->white_board);
}

OTHELLO_API bool is_valid_move(int move, uint64_t player_board, uint64_t opponent_board) {
    uint64_t all_occupied = player_board | opponent_board;
    if ((all_occupied >> move) & 1ULL) {
        return false;
    }

    int move_row = move / BOARD_SIZE;
    int move_col = move % BOARD_SIZE;
    for (int i = 0; i < 8; i++) {
        int dr = DIRECTIONS[i].dr;
        int dc = DIRECTIONS[i].dc;
        int r = move_row + dr;
        int c = move_col + dc;
        bool bits_to_flip = false;
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            int bit = (r << 3) + c;
            if ((opponent_board >> bit) & 1ULL) {
                bits_to_flip = true;
            } else if ((player_board >> bit) & 1ULL) {
                if (bits_to_flip) {
                    return true;
                }
                break;
            } else {
                break;
            }
            r += dr;
            c += dc;
        }
    }
    return false;
}

OTHELLO_API void get_valid_moves(uint64_t player_board, uint64_t opponent_board, MoveList* move_list) {
    uint64_t all_occupied = player_board | opponent_board;
    bool potential_moves[64] = {false};
    for (int bit = 0; bit < 64; bit++) {
        if ((opponent_board >> bit) & 1ULL) {
            int row = bit / BOARD_SIZE;
            int col = bit % BOARD_SIZE;
            for (int i = 0; i < 8; i++) {
                int dr = DIRECTIONS[i].dr;
                int dc = DIRECTIONS[i].dc;
                int r = row + dr;
                int c = col + dc;
                if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                    int candidate_bit = (r << 3) + c;
                    if (!((all_occupied >> candidate_bit) & 1ULL)) {
                        potential_moves[candidate_bit] = true;
                    }
                }
            }
        }
    }
    move_list->count = 0;
    for (int move = 0; move < 64; move++) {
        if (potential_moves[move]) {
            if (is_valid_move(move, player_board, opponent_board)) {
                move_list->moves[move_list->count++] = move;
            }
        }
    }
}

OTHELLO_API void get_flipped_bits(int move, uint64_t player_board, uint64_t opponent_board, BitList* bit_list) {
    int move_row = move / BOARD_SIZE;
    int move_col = move % BOARD_SIZE;
    bit_list->count = 0;

    for (int i = 0; i < 8; i++) {
        int dr = DIRECTIONS[i].dr;
        int dc = DIRECTIONS[i].dc;
        int r = move_row + dr;
        int c = move_col + dc;
        int temp_flip[64];
        int temp_count = 0;
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
            int bit = (r << 3) + c;
            if ((opponent_board >> bit) & 1ULL) {
                temp_flip[temp_count++] = bit;
            } else if ((player_board >> bit) & 1ULL) {
                if (temp_count > 0) {
                    memcpy(&bit_list->bits[bit_list->count], temp_flip, temp_count * sizeof(int));
                    bit_list->count += temp_count;
                }
                break;
            } else {
                break;
            }
            r += dr;
            c += dc;
        }
    }
}

OTHELLO_API bool is_game_over(OthelloGameObject* self) {
    MoveList black_moves;
    MoveList white_moves;
    get_valid_moves(self->black_board, self->white_board, &black_moves);
    get_valid_moves(self->white_board, self->black_board, &white_moves);
    return (black_moves.count == 0) && (white_moves.count == 0);
}

OTHELLO_API int OthelloGame_apply_move(OthelloGameObject* self, int move) {
    uint64_t player_board, opponent_board;
    if (self->current_player == self->black_player) {
        player_board = self->black_board;
        opponent_board = self->white_board;
    } else {
        player_board = self->white_board;
        opponent_board = self->black_board;
    }

    BitList bits_to_flip;
    get_flipped_bits(move, player_board, opponent_board, &bits_to_flip);

    if (bits_to_flip.count > 0) {
        player_board |= 1ULL << move;
        for (int i = 0; i < bits_to_flip.count; i++) {
            int bit = bits_to_flip.bits[i];
            player_board |= 1ULL << bit;
            opponent_board &= ~(1ULL << bit);
        }

        if (self->current_player == self->black_player) {
            self->black_board = player_board;
            self->white_board = opponent_board;
        } else {
            self->white_board = player_board;
            self->black_board = opponent_board;
        }

        self->current_player = (self->current_player == self->black_player) ? self->white_player : self->black_player;

        return 1;
    } else {
        return 0;
    }
}

static PyObject* OthelloGame_display_board(OthelloGameObject* self) {
    const char* BLACK_CELL = " B ";
    const char* WHITE_CELL = " W ";
    const char* EMPTY_CELL = " . ";

    const char* HLINE = "---";

    printf("\n");
    printf("    ");
    for (int col = 0; col < BOARD_SIZE; col++) {
        printf(" %d  ", col);
    }
    printf("\n");

    printf("   +");
    for (int col = 0; col < BOARD_SIZE; col++) {
        printf("%s+", HLINE);
    }
    printf("\n");

    for (int row = 0; row < BOARD_SIZE; row++) {
        printf("%2d |", row);

        for (int col = 0; col < BOARD_SIZE; col++) {
            int bit = (row << 3) + col;
            const char* cell;
            if ((self->black_board >> bit) & 1ULL) {
                cell = BLACK_CELL;
            } else if ((self->white_board >> bit) & 1ULL) {
                cell = WHITE_CELL;
            } else {
                cell = EMPTY_CELL;
            }
            printf("%s", cell);
            printf("|");
        }

        printf("\n");
        
        printf("   +");
        for (int col = 0; col < BOARD_SIZE; col++) {
            printf("%s+", HLINE);
        }
        printf("\n");
    }

    Py_RETURN_NONE;
}


static PyObject* OthelloGame_make_move(OthelloGameObject* self) {
    uint64_t player_board, opponent_board;
    if (self->current_player == self->black_player) {
        player_board = self->black_board;
        opponent_board = self->white_board;
    } else {
        player_board = self->white_board;
        opponent_board = self->black_board;
    }

    MoveList valid_moves;
    get_valid_moves(player_board, opponent_board, &valid_moves);

    if (valid_moves.count == 0) {
        self->current_player = (self->current_player == self->black_player) ? self->white_player : self->black_player;
        if (self->current_player == self->black_player) {
            player_board = self->black_board;
            opponent_board = self->white_board;
        } else {
            player_board = self->white_board;
            opponent_board = self->black_board;
        }
        get_valid_moves(player_board, opponent_board, &valid_moves);
        if (valid_moves.count == 0) {
            Py_RETURN_FALSE;
        }
    }

    PyObject* move_obj = PyObject_CallMethod(self->current_player, "decide_move", "KKK",
                                             (unsigned long long)valid_moves.count,
                                             (unsigned long long)player_board,
                                             (unsigned long long)opponent_board);
    if (!move_obj) {
        return NULL;
    }

    if (move_obj == Py_None) {
        Py_DECREF(move_obj);
        self->current_player = (self->current_player == self->black_player) ? self->white_player : self->black_player;
        Py_RETURN_TRUE;
    }

    int move = (int)PyLong_AsLong(move_obj);
    Py_DECREF(move_obj);

    bool is_valid = false;
    for (int i = 0; i < valid_moves.count; i++) {
        if (valid_moves.moves[i] == move) {
            is_valid = true;
            break;
        }
    }
    if (!is_valid) {
        PyErr_SetString(PyExc_ValueError, "Invalid move selected.");
        return NULL;
    }

    int apply_result = OthelloGame_apply_move(self, move);
    if (!apply_result) {
        PyErr_SetString(PyExc_ValueError, "Failed to apply the move.");
        return NULL;
    }

    Py_RETURN_TRUE;
}

static PyObject* OthelloGame_play(OthelloGameObject* self, PyObject* Py_UNUSED(ignored)) {
    while (!is_game_over(self)) {
        PyObject* result = OthelloGame_make_move(self);
        if (result == NULL) {
            return NULL;
        }
        Py_DECREF(result);
        if (self->debug) {
            OthelloGame_display_board(self);
        }
    }

    if (self->debug) {
        printf("Game over!\n");
    }

    int black_count = popcount64(self->black_board);
    int white_count = popcount64(self->white_board);
    int winner = 0;

    if (black_count > white_count) {
        if (self->debug) {
            printf("Black wins!\n");
        }
        winner = 1;
    } else if (white_count > black_count) {
        if (self->debug) {
            printf("White wins!\n");
        }
        winner = -1;
    } else {
        if (self->debug) {
            printf("It's a tie!\n");
        }
        winner = 0;
    }

    return PyLong_FromLong(winner);
}

static int OthelloGame_init(OthelloGameObject* self, PyObject* args, PyObject* kwds) {
    PyObject* black_player = NULL;
    PyObject* white_player = NULL;
    int debug = 0;

    static char* kwlist[] = {"black_player", "white_player", "debug", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|p", kwlist,
                                     &black_player, &white_player, &debug)) {
        return -1;
    }

    Py_INCREF(black_player);
    Py_INCREF(white_player);
    self->black_player = black_player;
    self->white_player = white_player;
    self->current_player = black_player;
    self->debug = debug;

    OthelloGame_initialize_boards(self);

    return 0;
}

static void OthelloGame_dealloc(OthelloGameObject* self) {
    Py_XDECREF(self->black_player);
    Py_XDECREF(self->white_player);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef OthelloGame_methods[] = {
    {"play", (PyCFunction)OthelloGame_play, METH_NOARGS,
     "Play the game until completion."},
    {NULL}
};

PyTypeObject OthelloGameType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "othello.OthelloGame",
    .tp_basicsize = sizeof(OthelloGameObject),
    .tp_dealloc = (destructor)OthelloGame_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "OthelloGame objects",
    .tp_methods = OthelloGame_methods,
    .tp_init = (initproc)OthelloGame_init,
    .tp_new = PyType_GenericNew,
};

static PyModuleDef othello_module = {
    PyModuleDef_HEAD_INIT,
    "othello",
    "C extension module for Othello Game",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_othello(void) {
    PyObject* m;

    if (PyType_Ready(&OthelloGameType) < 0)
        return NULL;

    m = PyModule_Create(&othello_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&OthelloGameType);
    if (PyModule_AddObject(m, "OthelloGame", (PyObject*)&OthelloGameType) < 0) {
        Py_DECREF(&OthelloGameType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
