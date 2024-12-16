#include "human_player.h"
#include "othello.h"
#include <stdio.h>

static PyObject* HumanPlayer_decide_move(PyObject* self, PyObject* args) {
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

    MoveList valid_moves;
    get_valid_moves((uint64_t)player_board, (uint64_t)opponent_board, &valid_moves);

    if (valid_moves.count == 0) {
        Py_RETURN_NONE;
    }

    printf("Valid moves:\n");
    for (int i = 0; i < valid_moves.count; i++) {
        int move = valid_moves.moves[i];
        int row = move / BOARD_SIZE;
        int col = move % BOARD_SIZE;
        printf("[%d]: Row %d, Col %d (Board position: %d)\n", i, row, col, move);
    }

    printf("Enter the index of your move: ");
    fflush(stdout);

    unsigned long long choice;
    if (scanf("%llu", &choice) != 1 || choice >= (unsigned long long)valid_moves.count) {
        PyErr_SetString(PyExc_ValueError, "Invalid move selection.");
        return NULL;
    }

    int selected_move = valid_moves.moves[choice];
    return PyLong_FromLong(selected_move);
}

static PyMethodDef HumanPlayer_methods[] = {
    {"decide_move", (PyCFunction)HumanPlayer_decide_move, METH_VARARGS,
     "Prompts the human player to select a move."},
    {NULL, NULL, 0, NULL}
};

PyTypeObject HumanPlayerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "players.HumanPlayer",
    .tp_basicsize = sizeof(BasicPlayer),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "Player that prompts the human for input",
    .tp_methods = HumanPlayer_methods,
    .tp_new = PyType_GenericNew,
};
