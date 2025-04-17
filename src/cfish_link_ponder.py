from subprocess import Popen, PIPE

cfish_path = "./cfish"
process = Popen([cfish_path], stdin=PIPE, stdout=PIPE, text=True)
process.stdin.write("uci\n")
process.stdin.flush()  # Flush after UCI command
process.stdin.write("setoption name Ponder value true\n")
process.stdin.flush() 
expected_ponder = "skibidi"
pondering = False
import time
import sys
f = open("debug.txt", "w")
o = open("output.txt", "w")
def chess_bot(obs):
    global expected_ponder
    global pondering
    if obs.lastMove == expected_ponder:
        if pondering: 
            process.stdin.write("ponderhit\n")
            f.write("")
            f.write("ponderhit\n")
            f.flush()
    else:
        if pondering: 
            process.stdin.write("ponderhit\n")
            process.stdin.flush()
            f.write("")
            f.write("ponderhit (ignore)\n")
            f.flush()
            while True:
                line = process.stdout.readline().strip()
                if not line:  # Skip empty lines
                    continue
                    
                parts = line.split()
                if not parts:  # Skip lines that split into empty lists
                    continue
                o.write(line)
                o.write("\n")
                o.flush()    
                if parts[0] == "bestmove":
                    f.flush()
                    break
             
        process.stdin.write(f"position fen {obs.board}\n")
        process.stdin.flush()
        f.write(f"position fen {obs.board}\n")
        if obs.mark[0] == 'w':
            process.stdin.write(f"go wtime {int(obs.remainingOverageTime * 1000)} btime {int(obs.opponentRemainingOverageTime * 1000)}\n")
            f.write(f"go wtime {int(obs.remainingOverageTime * 1000)} btime {int(obs.opponentRemainingOverageTime * 1000)}\n")
            f.flush()
            process.stdin.flush()
        else:
            process.stdin.write(f"go wtime {int(obs.opponentRemainingOverageTime * 1000)} btime {int(obs.remainingOverageTime * 1000)}\n")
            f.write(f"go wtime {int(obs.opponentRemainingOverageTime * 1000)} btime {int(obs.remainingOverageTime * 1000)}\n")            
            f.flush()
            process.stdin.flush()
    while True:
        line = process.stdout.readline().strip()
        if not line:  # Skip empty lines
            continue
            
        parts = line.split()
        if not parts:  # Skip lines that split into empty lists
            continue
        o.write(line)
        o.write("\n") 
        o.flush()   
        if parts[0] == "bestmove":
            #print(parts)
            f.write(line)
            f.write("\n")
            f.flush()
            move = parts[1]
            if len(parts) > 2:
                 expected_ponder = parts[3]
                 #print("pondering set to true")
                 pondering = True                 
                 process.stdin.write(f"position fen {obs.board} moves {move} {expected_ponder} {expected_ponder}\n")
                 process.stdin.write("go ponder\n")
                 process.stdin.flush()
                 f.write(f"position fen {obs.board} moves {move} {expected_ponder}\n")
                 f.flush()
                 f.write("go ponder\n")
                 f.flush()

            else: pondering = False
            return move