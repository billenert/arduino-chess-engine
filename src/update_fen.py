# update_fen.py

from board_to_fen import board_to_fen

def _algebraic_to_coords(square: str):
    """
    "e2" → (6, 4) in 0‑indexed [row, col] matrix terms.
    """
    file = ord(square[0]) - ord('a')
    rank = 8 - int(square[1])
    return rank, file

def update_fen(
    previous_fen: str,
    before_matrix: list[list[int]] | None = None,
    after_matrix:  list[list[int]] | None = None,
    enable_castling: bool = True,
    move: str | None = None
) -> str:
    """
    Update a FEN string either by:
      - diffing two 8×8 occupancy matrices (before/after), or
      - applying a UCI‑style move string (e.g. "e2e4").
    
    This version does NOT support en passant: the en passant field is always '-'.
    """
    # 1) Parse the old FEN
    parts            = previous_fen.split()
    board_fen       , active_color    = parts[0], parts[1]
    castling_rights , _old_en_passant = parts[2], parts[3]
    halfmove_clock , fullmove_number  = int(parts[4]), int(parts[5])

    # 2) Expand board_fen into an 8×8 of piece‐letters (spaces = empty)
    fen_rows = board_fen.split('/')
    fen_matrix = []
    for row in fen_rows:
        r = []
        for ch in row:
            if ch.isdigit():
                r.extend(' ' * int(ch))
            else:
                r.append(ch)
        fen_matrix.append(r)

    # 3) Figure out from_sq/to_sq
    if move:
        from_sq = _algebraic_to_coords(move[:2])
        to_sq   = _algebraic_to_coords(move[2:])
    else:
        if before_matrix is None or after_matrix is None:
            raise ValueError("Must supply before_matrix and after_matrix if no move string.")
        changed = [
            (i, j)
            for i in range(8) for j in range(8)
            if before_matrix[i][j] != after_matrix[i][j]
        ]
        if len(changed) == 2:
            # normal move or quiet capture
            from_sq = next(s for s in changed if before_matrix[s[0]][s[1]] == 1)
            to_sq   = next(s for s in changed if before_matrix[s[0]][s[1]] == 0)
        elif len(changed) == 1:
            # pawn‐capture: only one square went 1→0
            from_sq = changed[0]
            piece   = fen_matrix[from_sq[0]][from_sq[1]]
            if piece.lower() == 'p':
                direction = 1 if piece.islower() else -1
                cands = [
                    (from_sq[0] + direction, from_sq[1] - 1),
                    (from_sq[0] + direction, from_sq[1] + 1)
                ]
                to_sq = next((sq for sq in cands if 0 <= sq[0] < 8 and 0 <= sq[1] < 8), None)
                if to_sq is None:
                    raise ValueError("Could not infer pawn‐capture target square.")
            else:
                # fallback: just toggle side
                new_color = 'b' if active_color == 'w' else 'w'
                if new_color == 'w':
                    fullmove_number += 1
                return f"{board_fen} {new_color} {castling_rights} - {halfmove_clock} {fullmove_number}"
        else:
            # ambiguous change: toggle side
            new_color = 'b' if active_color == 'w' else 'w'
            if new_color == 'w':
                fullmove_number += 1
            return f"{board_fen} {new_color} {castling_rights} - {halfmove_clock} {fullmove_number}"

    # 4) Moving piece
    piece = fen_matrix[from_sq[0]][from_sq[1]]

    # 5) Castling rights
    if enable_castling:
        if piece.lower() == 'k':
            if piece.isupper():
                castling_rights = castling_rights.replace('K', '').replace('Q', '')
            else:
                castling_rights = castling_rights.replace('k', '').replace('q', '')
        elif piece.lower() == 'r':
            if from_sq[1] == 0:
                castling_rights = castling_rights.replace('Q' if piece.isupper() else 'q', '')
            elif from_sq[1] == 7:
                castling_rights = castling_rights.replace('K' if piece.isupper() else 'k', '')

    # 6) **En passant disabled**: always '-'
    en_passant = '-'

    # 7) Halfmove clock reset on pawn moves or captures
    dest_piece = fen_matrix[to_sq[0]][to_sq[1]]
    if piece.lower() == 'p' or dest_piece != ' ':
        halfmove_clock = 0
    else:
        halfmove_clock += 1

    # 8) Fullmove increment after Black moves
    new_color = 'b' if active_color == 'w' else 'w'
    if new_color == 'w':
        fullmove_number += 1

    # 9) Build the updated board matrix
    new_board = []
    for i in range(8):
        row = []
        for j in range(8):
            if (i, j) == to_sq:
                row.append(piece)
            elif (i, j) == from_sq:
                row.append(' ')
            else:
                row.append(fen_matrix[i][j])
        new_board.append(row)

    # 10) Convert back to FEN (with '-' for en passant)
    return board_to_fen(
        new_board,
        active_color=new_color,
        castling_rights=castling_rights,
        en_passant=en_passant,
        halfmove_clock=halfmove_clock,
        fullmove_number=fullmove_number
    )
