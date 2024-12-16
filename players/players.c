// players/players.c

#include "players.h"
#include "random_player.h"
#include "human_player.h"
#include "minimax_player.h"
#include <stdlib.h>
#include <time.h>

static struct PyModuleDef players_module = {
    PyModuleDef_HEAD_INIT,
    "players",
    "C extension module for Othello Players",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_players(void) {
    PyObject* m;

    srand((unsigned int)time(NULL));

    if (PyType_Ready(&RandomPlayerType) < 0)
        return NULL;

    if (PyType_Ready(&HumanPlayerType) < 0)
        return NULL;

    if (PyType_Ready(&MiniMaxPlayerType) < 0)
        return NULL;

    m = PyModule_Create(&players_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&RandomPlayerType);
    if (PyModule_AddObject(m, "RandomPlayer", (PyObject*)&RandomPlayerType) < 0) {
        Py_DECREF(&RandomPlayerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&HumanPlayerType);
    if (PyModule_AddObject(m, "HumanPlayer", (PyObject*)&HumanPlayerType) < 0) {
        Py_DECREF(&HumanPlayerType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&MiniMaxPlayerType);
    if (PyModule_AddObject(m, "MiniMaxPlayer", (PyObject*)&MiniMaxPlayerType) < 0) {
        Py_DECREF(&MiniMaxPlayerType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
