from subprocess import Popen, PIPE
v0 = './cfish'
v1 = Popen([v0], stdin=PIPE, stdout=PIPE, text=True)
v1.stdin.write('uci\n')
v1.stdin.flush()
v2 = 'skibidi'
def chess_bot(obs):
    v1.stdin.write(f'position fen {obs.board}\n')
    if obs.mark[0] == 'w':
        v1.stdin.write(
f"""go wtime {int(obs.remainingOverageTime * 1000)} btime {int(obs.opponentRemainingOverageTime * 1000)}
"""
)
        v1.stdin.flush()
    else:
        v1.stdin.write(
f"""go wtime {int(obs.opponentRemainingOverageTime * 1000)} btime {int(obs.remainingOverageTime * 1000)}
"""
)
    v1.stdin.flush()
    while True:
        v3 = v1.stdout.readline().strip()
        if not v3:
            continue
        v4 = v3.split()
        if not v4:
            continue
        if v4[0] == 'bestmove':
            v5 = v4[1]
            return v5
