import players
import othello
import time
import argparse
import pandas as pd
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
from collections import defaultdict
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font


def play_single_game(black_eval, white_eval, depth):
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
    for row in sheet.iter_rows():
        for cell in row:
            cell.alignment = Alignment(horizontal="center", vertical="center")
            cell.font = Font(name="Arial", size=10)


def results_to_excel(results, black_evals, white_evals, filename="othello_results.xlsx"):
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

        wb = writer.book
        sheet = wb["Detailed Results"]
        format_excel_sheet(sheet)

    print(f"Results saved to {filename}")

def main():
    parser = argparse.ArgumentParser(description="Run Othello evaluation testing with adjustable depth.")
    parser.add_argument(
        "--depth",
        type=int,
        default=4,
        help="The depth for the MiniMaxPlayer evaluation (default is 4)."
    )
    parser.add_argument(
        "--games_per_pair",
        type=int,
        default=10,
        help="Number of games per evaluation pair (default is 10)."
    )
    parser.add_argument(
        "--play",
        action="store_true",
        help="Whether you would like to play or not (default is False)."
    )
    parser.add_argument(
        "--time",
        action="store_true",
        help="Whether you would like to time a game or not (default is False)."
    )

    args = parser.parse_args()
    
    depth = args.depth
    games_per_pair = args.games_per_pair
    play_flag = args.play
    time_flag = args.time

    if play_flag:
        if time_flag:
            print("Not timing due to HumanPlayer presence.")
            
        white_player = players.MiniMaxPlayer(max_depth=depth, evaluation_strategy="combined_evaluate")
        game = othello.OthelloGame(black_player=players.HumanPlayer(), white_player=white_player, debug=True)
        game.play()
        return

    ALL_FUNCTIONS = [
        "win_evaluate", "material_evaluate", "mobility_evaluate",
        "positional_evaluate", "corner_evaluate", "edge_evaluate",
        "frontier_evaluate", "parity_evaluate", "combined_evaluate",
        "random_evaluate", "random_player"
    ]

    if time_flag:
        print("Timing games for each evaluation function...")
        timing_results = []
        
        for function in ALL_FUNCTIONS:
            if function == "random_player":
                black_player = players.RandomPlayer()
            else:
                black_player = players.MiniMaxPlayer(max_depth=depth, evaluation_strategy=function)

            white_player = players.RandomPlayer()
            
            start_time = time.perf_counter()
            
            game = othello.OthelloGame(black_player=black_player, white_player=white_player)
            game.play()
            
            end_time = time.perf_counter()
            elapsed_time = end_time - start_time
            
            timing_results.append({"Evaluation Function": function, "Time (s)": elapsed_time})
        
        timing_df = pd.DataFrame(timing_results)
        timing_df.to_excel("timing_results_" + str(depth) + ".xlsx", index=False)
        print("Timing results saved to 'timing_results_" + str(depth) + ".xlsx'.")
        return

    black_subset = ["combined_evaluate"]
    white_subset = ALL_FUNCTIONS
    results = test_evaluations(black_subset, white_subset, depth, games_per_pair)
    name = f"combined_vs_all_depth_{depth}"
    results_to_excel(results, black_subset, white_subset, filename=name + ".xlsx")

if __name__ == '__main__':
    main()
