import players
import othello
import pandas as pd
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
from collections import defaultdict
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font


def play_single_game(black_eval, white_eval, depth):
    """
    Simulate a single game between two players.
    If black_eval or white_eval is 'random_player', use a RandomPlayer instead of MiniMaxPlayer.
    """
    if black_eval == "random_player":
        black_player = players.RandomPlayer()
    else:
        black_player = players.MiniMaxPlayer(
            max_depth=depth, evaluation_strategy=black_eval, debug=False
        )

    if white_eval == "random_player":
        white_player = players.RandomPlayer()
    else:
        white_player = players.MiniMaxPlayer(
            max_depth=depth, evaluation_strategy=white_eval, debug=False
        )

    game = othello.OthelloGame(black_player=black_player, white_player=white_player)
    result = game.play()

    return black_eval, white_eval, result


def test_evaluations(black_evals, white_evals, depth=3, games_per_pair=10):
    """
    Test given sets of evaluation functions on black and white sides.
    For each black_eval in black_evals and each white_eval in white_evals,
    play `games_per_pair` games and gather results.
    """
    tasks = [
        (black_eval, white_eval)
        for black_eval in black_evals
        for white_eval in white_evals
        for _ in range(games_per_pair)
    ]
    total_games = len(tasks)

    results_dict = defaultdict(lambda: {"Black Wins": 0, "White Wins": 0, "Ties": 0})

    with ProcessPoolExecutor() as executor, tqdm(total=total_games, desc="Total Games Completed") as pbar:
        futures = [executor.submit(play_single_game, black_eval, white_eval, depth) for black_eval, white_eval in tasks]

        for future in as_completed(futures):
            black_eval, white_eval, result = future.result()

            if result == 1:
                results_dict[(black_eval, white_eval)]["Black Wins"] += 1
            elif result == -1:
                results_dict[(black_eval, white_eval)]["White Wins"] += 1
            else:
                results_dict[(black_eval, white_eval)]["Ties"] += 1

            pbar.update(1)

    # Convert cumulative results to percentages
    results = []
    for (black_eval, white_eval), counts in results_dict.items():
        total_games_for_pair = counts["Black Wins"] + counts["White Wins"] + counts["Ties"]
        results.append({
            "Black Evaluation": black_eval,
            "White Evaluation": white_eval,
            "Black Win Rate (%)": (counts["Black Wins"] / total_games_for_pair) * 100,
            "White Win Rate (%)": (counts["White Wins"] / total_games_for_pair) * 100,
            "Tie Rate (%)": (counts["Ties"] / total_games_for_pair) * 100
        })

    return results


def format_excel_sheet(sheet):
    """
    Apply formatting to an Excel sheet.
    """
    for row in sheet.iter_rows():
        for cell in row:
            cell.alignment = Alignment(horizontal="center", vertical="center")
            cell.font = Font(name="Arial", size=10)


def results_to_excel(results, black_evals, white_evals, filename="othello_results.xlsx"):
    """
    Save the results to an Excel file with detailed win/loss/tie percentages.
    """
    # Create a pivot-like structure for rates
    black_win_rates = pd.DataFrame(index=black_evals, columns=white_evals, dtype=float)
    white_win_rates = pd.DataFrame(index=black_evals, columns=white_evals, dtype=float)
    tie_rates = pd.DataFrame(index=black_evals, columns=white_evals, dtype=float)

    for result in results:
        black_eval = result["Black Evaluation"]
        white_eval = result["White Evaluation"]

        black_win_rates.loc[black_eval, white_eval] = result["Black Win Rate (%)"]
        white_win_rates.loc[black_eval, white_eval] = result["White Win Rate (%)"]
        tie_rates.loc[black_eval, white_eval] = result["Tie Rate (%)"]

    with pd.ExcelWriter(filename, engine='openpyxl') as writer:
        pd.DataFrame(results).to_excel(writer, sheet_name="Detailed Results", index=False)

        # Apply formatting
        wb = writer.book
        sheet = wb["Detailed Results"]
        format_excel_sheet(sheet)

    print(f"Results saved to {filename}")


def main():
    ALL_FUNCTIONS = [
        "win_evaluate", "material_evaluate", "mobility_evaluate",
        "positional_evaluate", "corner_evaluate", "edge_evaluate",
        "frontier_evaluate", "parity_evaluate", "combined_evaluate",
        "random_evaluate", "random_player"
    ]

    # white_player = players.MiniMaxPlayer(
    #         max_depth=8, evaluation_strategy="combined_evaluate", debug=False
    #     )
    # game = othello.OthelloGame(black_player=white_player, white_player=players.HumanPlayer(), debug=True)
    # result = game.play()

    depth = 6
    games_per_pair = 100

    black_subset = ["combined_evaluate"]
    white_subset = ALL_FUNCTIONS
    results = test_evaluations(black_subset, white_subset, depth, games_per_pair)
    results_to_excel(results, black_subset, white_subset, filename="combined_vs_all.xlsx")


if __name__ == '__main__':
    main()
