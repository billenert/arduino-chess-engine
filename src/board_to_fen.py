def board_to_fen(board_matrix, active_color='w', castling_rights='KQkq', en_passant='-', halfmove_clock=0, fullmove_number=1):
    """
    Convert a chess board matrix to FEN notation.
    
    Args:
        board_matrix: 2D list representing the chess board
        active_color: 'w' for white to move, 'b' for black to move
        castling_rights: String of available castling rights (e.g., 'KQkq')
        en_passant: En passant target square or '-'
        halfmove_clock: Number of halfmoves since last capture or pawn advance
        fullmove_number: Current move number
    
    Returns:
        String in FEN notation
    """
    fen_parts = []
    
    # Convert board matrix to FEN ranks
    for rank in board_matrix:
        fen_rank = []
        empty_count = 0
        
        for square in rank:
            if square == ' ' or square == '':
                empty_count += 1
            else:
                if empty_count > 0:
                    fen_rank.append(str(empty_count))
                    empty_count = 0
                fen_rank.append(square)
        
        if empty_count > 0:
            fen_rank.append(str(empty_count))
        
        fen_parts.append(''.join(fen_rank))
    
    # Join ranks with '/'
    fen_board = '/'.join(fen_parts)
    
    # Combine all FEN parts
    fen = f"{fen_board} {active_color} {castling_rights} {en_passant} {halfmove_clock} {fullmove_number}"
    
    return fen

# Example usage:
if __name__ == "__main__":
    # Example board matrix (8x8)
    board = [
        ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
        ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
        [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '],
        [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '],
        [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '],
        [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '],
        ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
        ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']
    ]
    
    fen = board_to_fen(board)
    print(fen)  # Should output: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 