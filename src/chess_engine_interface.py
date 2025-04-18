# chess_engine_interface.py

from subprocess import Popen, PIPE
import sys
from update_fen import update_fen

# Initialize the chess engine
cfish_path = "./cfish.exe"
process = Popen([cfish_path], stdin=PIPE, stdout=PIPE, text=True)

# Initialize UCI protocol
process.stdin.write("uci\n")
process.stdin.flush()
process.stdin.write("setoption name Hash value 1\n")
process.stdin.flush()

# Initial FEN position
current_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1"

def get_best_move(fen: str) -> str:
    """Get the best move from the chess engine for the given FEN position."""
    if not fen:
        raise ValueError("Invalid FEN string")
    process.stdin.write(f"position fen {fen}\n")
    process.stdin.write("go movetime 1000\n")  # 1 second per move
    process.stdin.flush()
    while True:
        line = process.stdout.readline().strip()
        if not line:
            continue
        parts = line.split()
        if parts[0] == "bestmove":
            return parts[1]

def main():
    global current_fen

    while True:
        try:
            # Read the user’s move
            print("Enter your move (e.g., g8f6):", file=sys.stderr)
            user_move = input().strip()

            # Quick format check: four chars, files a–h, ranks 1–8
            if (
                len(user_move) != 4
                or user_move[0] not in "abcdefgh"
                or user_move[2] not in "abcdefgh"
                or user_move[1] not in "12345678"
                or user_move[3] not in "12345678"
            ):
                raise ValueError("Invalid move format. Please use format like 'g8f6'")

            # Update FEN by telling update_fen the move string directly
            current_fen = update_fen(current_fen, move=user_move)
            print(f"Updated FEN after your move: {current_fen}", file=sys.stderr)

            # Ask the engine for its reply
            best_move = get_best_move(current_fen)
            print(f"Bot plays: {best_move}", file=sys.stderr)

            # Apply the engine’s move to the FEN
            current_fen = update_fen(current_fen, move=best_move)
            print(f"New FEN after bot's move: {current_fen}", file=sys.stderr)

            # Output the engine’s move to stdout
            print(best_move)
            sys.stdout.flush()

        except KeyboardInterrupt:
            print("\nExiting...", file=sys.stderr)
            break
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            print("Please try again with valid input.", file=sys.stderr)
            continue

if __name__ == "__main__":
    main()
