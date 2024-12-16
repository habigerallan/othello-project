#include "minimax_player.h"
#include "othello.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <Python.h>

static int win_evaluate(uint64_t player_board, uint64_t opponent_board);
static int material_evaluate(uint64_t player_board, uint64_t opponent_board);
static int mobility_evaluate(uint64_t player_board, uint64_t opponent_board);
static int positional_evaluate(uint64_t player_board, uint64_t opponent_board);
static int corner_evaluate(uint64_t player_board, uint64_t opponent_board);
static int edge_evaluate(uint64_t player_board, uint64_t opponent_board);
static int frontier_evaluate(uint64_t player_board, uint64_t opponent_board);
static int parity_evaluate(uint64_t player_board, uint64_t opponent_board);
static int random_evaluate(uint64_t player_board, uint64_t opponent_board);
static int combined_evaluate(uint64_t player_board, uint64_t opponent_board);

static inline bool is_terminal_state(uint64_t player_board, uint64_t opponent_board) {
    MoveList player_moves;
    get_valid_moves(player_board, opponent_board, &player_moves);

    MoveList opponent_moves;
    get_valid_moves(opponent_board, player_board, &opponent_moves);

    return (player_moves.count == 0 && opponent_moves.count == 0);
}

static int minimax(uint64_t player_board, uint64_t opponent_board, int depth, bool maximizing_player, MiniMaxPlayer* self) {
    self->iter++;

    if (self->debug && self->iter % 1000000 == 0) {
        printf("Iteration: %d\n", self->iter);
    }

    if (depth == 0 || is_terminal_state(player_board, opponent_board)) {
        return self->evaluate_func(player_board, opponent_board);
    }

    MoveList valid_moves;
    get_valid_moves(player_board, opponent_board, &valid_moves);

    if (valid_moves.count == 0) {
        return minimax(opponent_board, player_board, depth - 1, !maximizing_player, self);
    }

    int best_value = maximizing_player ? INT_MIN : INT_MAX;

    for (int i = 0; i < valid_moves.count; i++) {
        int move = valid_moves.moves[i];

        uint64_t new_player_board = player_board;
        uint64_t new_opponent_board = opponent_board;

        BitList bits_to_flip;
        get_flipped_bits(move, player_board, opponent_board, &bits_to_flip);
        new_player_board |= 1ULL << move;
        for (int j = 0; j < bits_to_flip.count; j++) {
            int bit = bits_to_flip.bits[j];
            new_player_board |= 1ULL << bit;
            new_opponent_board &= ~(1ULL << bit);
        }

        int eval = minimax(new_opponent_board, new_player_board, depth - 1, !maximizing_player, self);

        if (maximizing_player) {
            if (eval > best_value) {
                best_value = eval;
            }
        } else {
            if (eval < best_value) {
                best_value = eval;
            }
        }
    }

    return best_value;
}

static int minimax_abp(uint64_t player_board, uint64_t opponent_board, int depth, int alpha, int beta, bool maximizing_player, MiniMaxPlayer* self) {
    self->iter++;

    if (self->debug && self->iter % 1000000 == 0) {
        printf("Iteration: %d\n", self->iter);
    }

    if (depth == 0 || is_terminal_state(player_board, opponent_board)) {
        return self->evaluate_func(player_board, opponent_board);
    }

    MoveList valid_moves;
    get_valid_moves(player_board, opponent_board, &valid_moves);

    if (valid_moves.count == 0) {
        return minimax_abp(opponent_board, player_board, depth - 1, alpha, beta, !maximizing_player, self);
    }

    int best_value = maximizing_player ? INT_MIN : INT_MAX;

    for (int i = 0; i < valid_moves.count; i++) {
        int move = valid_moves.moves[i];

        uint64_t new_player_board = player_board;
        uint64_t new_opponent_board = opponent_board;

        BitList bits_to_flip;
        get_flipped_bits(move, player_board, opponent_board, &bits_to_flip);
        new_player_board |= 1ULL << move;
        for (int j = 0; j < bits_to_flip.count; j++) {
            int bit = bits_to_flip.bits[j];
            new_player_board |= 1ULL << bit;
            new_opponent_board &= ~(1ULL << bit);
        }

        int eval = minimax_abp(new_opponent_board, new_player_board, depth - 1, alpha, beta, !maximizing_player, self);

        if (maximizing_player) {
            if (eval > best_value) {
                best_value = eval;
            }
            if (best_value > alpha) {
                alpha = best_value;
            }
            if (beta <= alpha) {
                break;
            }
        } else {
            if (eval < best_value) {
                best_value = eval;
            }
            if (best_value < beta) {
                beta = best_value;
            }
            if (beta <= alpha) {
                break;
            }
        }
    }

    return best_value;
}

static int win_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int player_count = popcount64(player_board);
    int opponent_count = popcount64(opponent_board);
    bool is_over = is_terminal_state(player_board, opponent_board);

    if (is_over) {
        if (player_count > opponent_count) {
            return INT_MAX;
        } else if (player_count < opponent_count) {
            return INT_MIN + 1;
        }
    }

    return 0;
}

static int material_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int player_count = popcount64(player_board);
    int opponent_count = popcount64(opponent_board);
    
    return player_count - opponent_count;
}

static int mobility_evaluate(uint64_t player_board, uint64_t opponent_board) {
    MoveList player_moves;
    MoveList opponent_moves;
    get_valid_moves(player_board, opponent_board, &player_moves);
    get_valid_moves(opponent_board, player_board, &opponent_moves);

    int player_mobility = player_moves.count;
    int opponent_mobility = opponent_moves.count;
    
    return player_mobility - opponent_mobility;
}

static const int POSITION_VALUES[8][8] = {
    {3, -2, 2, 2, 2, 2, -2, 3},
    {-2, -3, 1, 1, 1, 1, -3, -2},
    {2, 1, 0, 0, 0, 0, 1, 2},
    {2, 1, 0, 0, 0, 0, 1, 2},
    {2, 1, 0, 0, 0, 0, 1, 2},
    {2, 1, 0, 0, 0, 0, 1, 2},
    {-2, -3, 1, 1, 1, 1, -3, -2},
    {3, -2, 2, 2, 2, 2, -2, 3}
};

static int positional_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int score = 0;

    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        if (player_board & (1ULL << i)) {
            score += POSITION_VALUES[row][col];
        } else if (opponent_board & (1ULL << i)) {
            score -= POSITION_VALUES[row][col];
        }
    }

    return score;
}

static const int CORNER_SQUARES[4] = {0, 7, 56, 63};

static int corner_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int score = 0;

    for (int i = 0; i < 4; i++) {
        int row = i / 8;
        int col = i % 8;
        uint64_t mask = 1ULL << CORNER_SQUARES[i];
        if (player_board & mask) {
            score += 1;
        } else if (opponent_board & mask) {
            score -= 1;
        }
    }

    return score;
}

static const int EDGE_SQUARES[24] = {
    1, 2, 3, 4, 5, 6,
    8, 16, 24, 32, 40, 48,
    55, 54, 53, 52, 51, 50,
    57, 58, 59, 60, 61, 62
};

static int edge_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int score = 0;

    for (int i = 0; i < 24; i++) {
        int row = i / 8;
        int col = i % 8;
        uint64_t mask = 1ULL << EDGE_SQUARES[i];
        if (player_board & mask) {
            score += 1;
        } else if (opponent_board & mask) {
            score -= 1;
        }
    }

    return score;
}

static int frontier_evaluate(uint64_t player_board, uint64_t opponent_board) {
    uint64_t empty = ~(player_board | opponent_board);

    uint64_t adjacent_to_empty = 0;
    adjacent_to_empty |= empty << 8; // North
    adjacent_to_empty |= empty >> 8; // South
    adjacent_to_empty |= (empty & 0xFEFEFEFEFEFEFEFEULL) << 1;  // East
    adjacent_to_empty |= (empty & 0x7F7F7F7F7F7F7F7FULL) >> 1;  // West
    adjacent_to_empty |= (empty & 0xFEFEFEFEFEFEFEFEULL) << 9;  // Northeast
    adjacent_to_empty |= (empty & 0x7F7F7F7F7F7F7F7FULL) << 7;  // Northwest
    adjacent_to_empty |= (empty & 0xFEFEFEFEFEFEFEFEULL) >> 7;  // Southeast
    adjacent_to_empty |= (empty & 0x7F7F7F7F7F7F7F7FULL) >> 9;  // Southwest

    int player_frontier = popcount64(player_board & adjacent_to_empty);
    int opponent_frontier = popcount64(opponent_board & adjacent_to_empty);

    return player_frontier - opponent_frontier;
}

static int parity_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int empty_squares = popcount64(~(player_board | opponent_board));

    if (empty_squares % 2 == 0) {
        return 1;
    } else {
        return -1;
    }
}

static int combined_evaluate(uint64_t player_board, uint64_t opponent_board) {
    int win = win_evaluate(player_board, opponent_board);
    if (win) {
        return win;
    }

    int total_pieces = popcount64(player_board) + popcount64(opponent_board);
    int material_weight, mobility_weight, positional_weight, corner_weight, edge_weight, frontier_weight, parity_weight;
    int normalization_factor;

    if (total_pieces <= 15) {
        material_weight = 1;
        mobility_weight = 4;
        positional_weight = 4;
        corner_weight = 5;
        edge_weight = 4;
        frontier_weight = 1;
        parity_weight = 1;
    } else if (total_pieces <= 45) {
        material_weight = 3;
        mobility_weight = 4;
        positional_weight = 4;
        corner_weight = 5;
        edge_weight = 4;
        frontier_weight = 2;
        parity_weight = 1;
    } else {
        material_weight = 4;
        mobility_weight = 4;
        positional_weight = 4;
        corner_weight = 5;
        edge_weight = 4;
        frontier_weight = 2;
        parity_weight = 2;
    }

    int material = material_evaluate(player_board, opponent_board);
    int mobility = mobility_evaluate(player_board, opponent_board);
    int positional = positional_evaluate(player_board, opponent_board);
    int corner = corner_evaluate(player_board, opponent_board);
    int edge = edge_evaluate(player_board, opponent_board);
    int frontier = frontier_evaluate(player_board, opponent_board);
    int parity = parity_evaluate(player_board, opponent_board);

    int total_score =
        material * material_weight +
         mobility * mobility_weight +
         positional * positional_weight +
         corner * corner_weight +
         edge * edge_weight +
         frontier * frontier_weight +
         parity * parity_weight;

    return total_score;
}


static int random_evaluate(uint64_t player_board, uint64_t opponent_board) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    int min_value = -50;
    int max_value = 50;
    int range = max_value - min_value + 1;
    int random_value = rand() % range + min_value;
    return random_value;
}

typedef struct {
    const char* name;
    int (*func)(uint64_t, uint64_t);
} EvalFuncMapping;

static EvalFuncMapping eval_functions[] = {
    {"win_evaluate", win_evaluate},
    {"material_evaluate", material_evaluate},
    {"mobility_evaluate", mobility_evaluate},
    {"positional_evaluate", positional_evaluate},
    {"corner_evaluate", corner_evaluate},
    {"edge_evaluate", edge_evaluate},
    {"frontier_evaluate", frontier_evaluate},
    {"parity_evaluate", parity_evaluate},
    {"combined_evaluate", combined_evaluate},
    {"random_evaluate", random_evaluate},
    {NULL, NULL}
};

static PyObject* MiniMaxPlayer_decide_move(PyObject* self_obj, PyObject* args) {
    MiniMaxPlayer* player = (MiniMaxPlayer*)self_obj;
    unsigned long long num_moves;
    unsigned long long player_board;
    unsigned long long opponent_board;

    if (!PyArg_ParseTuple(args, "KKK", &num_moves, &player_board, &opponent_board)) {
        PyErr_SetString(PyExc_TypeError, "decide_move() arguments must be (num_moves, player_board, opponent_board).");
        return NULL;
    }

    if (num_moves == 0) {
        Py_RETURN_NONE;
    }

    player->iter = 0;

    MoveList valid_moves;
    get_valid_moves(player_board, opponent_board, &valid_moves);

    if (valid_moves.count == 0) {
        Py_RETURN_NONE;
    }

    int best_move = -1;
    int best_score = INT_MIN;

    for (int i = 0; i < valid_moves.count; i++) {
        int move = valid_moves.moves[i];

        uint64_t new_player_board = player_board;
        uint64_t new_opponent_board = opponent_board;

        BitList bits_to_flip;
        get_flipped_bits(move, player_board, opponent_board, &bits_to_flip);
        new_player_board |= 1ULL << move;
        for (int j = 0; j < bits_to_flip.count; j++) {
            int bit = bits_to_flip.bits[j];
            new_player_board |= 1ULL << bit;
            new_opponent_board &= ~(1ULL << bit);
        }

        int score;
        if (player->abp) {
            score = minimax_abp(new_opponent_board, new_player_board, player->max_depth - 1, INT_MIN, INT_MAX, false, player);
        } else {
            score = minimax(new_opponent_board, new_player_board, player->max_depth - 1, false, player);
        }

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    if (best_move == -1) {
        Py_RETURN_NONE;
    } else {
        return PyLong_FromLong(best_move);
    }
}

static PyObject* MiniMaxPlayer_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    MiniMaxPlayer* self = (MiniMaxPlayer*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->iter = 0;
    }
    return (PyObject*)self;
}

static int MiniMaxPlayer_init(MiniMaxPlayer* self, PyObject* args, PyObject* kwds) {
    static char* kwlist[] = {"max_depth", "debug", "evaluation_strategy", "abp", NULL};

    int max_depth = 3;
    int debug = 0;
    const char* eval_strategy = "combined_evaluate";
    int abp = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|iisi", kwlist, &max_depth, &debug, &eval_strategy, &abp)) {
        return -1;
    }

    self->max_depth = max_depth;
    self->debug = debug ? true : false;
    self->abp = abp ? true : false;

    bool found = false;
    for (int i = 0; eval_functions[i].name != NULL; i++) {
        if (strcmp(eval_strategy, eval_functions[i].name) == 0) {
            self->evaluate_func = eval_functions[i].func;
            found = true;
            break;
        }
    }

    if (!found) {
        PyErr_Format(PyExc_ValueError, "Unknown evaluation strategy: '%s'", eval_strategy);
        return -1;
    }

    return 0;
}

static PyMethodDef MiniMaxPlayer_methods[] = {
    {"decide_move", (PyCFunction)MiniMaxPlayer_decide_move, METH_VARARGS,
     "Selects the optimal move based on the Minimax with Alpha-Beta Pruning algorithm."},
    {NULL, NULL, 0, NULL}
};

PyTypeObject MiniMaxPlayerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "players.MiniMaxPlayer",
    .tp_basicsize = sizeof(MiniMaxPlayer),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Player using a minimax strategy with alpha-beta pruning",
    .tp_methods = MiniMaxPlayer_methods,
    .tp_new = MiniMaxPlayer_new,
    .tp_init = (initproc)MiniMaxPlayer_init,
};
