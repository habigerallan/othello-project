# setup.py

from setuptools import setup, Extension
import sysconfig

python_include_dir = sysconfig.get_path('include')

othello_module = Extension(
    'othello',
    sources=['othello/othello.c'],
    include_dirs=['othello', python_include_dir],
)

players_module = Extension(
    'players',
    sources=[
        'players/players.c',
        'players/random_player.c',
        'players/human_player.c',
        'players/minimax_player.c',
        'othello/othello.c'
    ],
    include_dirs=['players', 'othello', python_include_dir],
)

setup(
    name='othello_project',
    version='1.0',
    description='Othello Game and Players with C Extensions',
    ext_modules=[othello_module, players_module],
)
