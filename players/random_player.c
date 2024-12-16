#include <Python.h>
#include <stdlib.h>
#include <time.h>
#include "random_player.h"
#include "othello.h"

static PyObject* RandomPlayer_init(PyObject* self, PyObject* args) {
    srand((unsigned int)time(NULL));
    return 0;
}

static PyObject* RandomPlayer_decide_move(PyObject* self, PyObject* args) {
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
    get_valid_moves(player_board, opponent_board, &valid_moves);

    PyObject* valid_moves_list = PyList_New(num_moves);
    for (int i = 0; i < num_moves; i++) {
        PyList_SetItem(valid_moves_list, i, PyLong_FromUnsignedLongLong(valid_moves.moves[i]));
    }

    Py_ssize_t random_index = rand() % num_moves;
    PyObject* move = PyList_GetItem(valid_moves_list, random_index);
    Py_INCREF(move);

    Py_DECREF(valid_moves_list);
    return move;
}

static PyMethodDef RandomPlayer_methods[] = {
    {"decide_move", (PyCFunction)RandomPlayer_decide_move, METH_VARARGS,
     "Selects a random move from the list of valid moves."},
    {NULL, NULL, 0, NULL}
};

PyTypeObject RandomPlayerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "players.RandomPlayer",
    .tp_basicsize = sizeof(BasicPlayer),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "RandomPlayer that picks a move at random",
    .tp_methods = RandomPlayer_methods,
    .tp_init = (initproc)RandomPlayer_init,
    .tp_new = PyType_GenericNew,
};