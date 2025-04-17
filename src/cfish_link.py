from subprocess import Popen, PIPE
import psutil
cfish_path = "./cfish"
process = Popen([cfish_path], stdin=PIPE, stdout=PIPE, text=True)
process.stdin.write("uci\n")
process.stdin.flush()  # Flush after UCI command
process.stdin.write("setoption name Hash value 1\n")
process.stdin.flush()
expected_ponder = "skibidi"


def chess_bot(obs):
    #expected_ponder = expected_ponder
    #if obs.lastMove == expected_ponder:
    #    process.stdin.write("ponderhit\n")
    #else: 
    
    process.stdin.write(f"position fen {obs.board}\n")
    if obs.mark[0] == 'w':
        process.stdin.write(f"go wtime {int(obs.remainingOverageTime * 1000)} btime {int(obs.opponentRemainingOverageTime * 1000)}\n")
        process.stdin.flush()
    else:
        process.stdin.write(f"go wtime {int(obs.opponentRemainingOverageTime * 1000)} btime {int(obs.remainingOverageTime * 1000)}\n")
        process.stdin.flush()
    proc = psutil.Process(process.pid)
    mem_info=proc.memory_info()
    print(f"Memory Usage: {mem_info.rss / (1024 * 1024)} MB")  # Convert to MB
    while True:
        line = process.stdout.readline().strip()
        if not line:  # Skip empty lines
            continue
            
        parts = line.split()
        if not parts:  # Skip lines that split into empty lists
            continue
            
        if parts[0] == "bestmove":
            #print(parts)
            move = parts[1]
            #expected_ponder = parts[3]
            #process.stdin.write(f"go ponder {obs.board} moves {expected_ponder}\n")
            #process.stdin.flush()
            return move