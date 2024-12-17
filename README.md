# Othello Project
This is an implementation of Othello and its agents. This allows you to play against different types of agents and/or test them against eachother.

---

## Table of Contents
1. [Overview](#overview)
3. [Installation](#installation)
4. [Usage](#usage)

---

## Overview
This project implemented an Optimized Othello game and agent by leveraging the Minimax algorithm with Alpha-Beta pruning. It then also uses a variety of evaluation functions based upon Othello strategy such as Corner Control, Mobility, and many more. Using all of these it can make decently optimal decisions when it comes to deciding its move.

---

## Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/habigerallan/othello-project.git

   cd othello-project
   ```

2. **Install Python Modules**
   ```bash
   pip install -r requirements.txt

   python3 setup.py build
   python3 setup.py install
   ```

---

## Usage

**To Run the Program**
   ```bash
   python3 main.py
   ```

**Command Line Arguments**
1. --depth XXX (max depth, default=4)
2. --games-per-pair XXX (games per test, default=10)
3. --play (play against agent, default=False)
4. --time (times each function, default=False)

