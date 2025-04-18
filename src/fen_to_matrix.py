def fen_to_matrix(fen):
    """
    Convert a FEN string to an 8x8 matrix where 1 represents a piece and 0 represents empty.
    
    Args:
        fen: A FEN string representing a chess position
        
    Returns:
        An 8x8 matrix where 1 represents a piece and 0 represents empty
    """
    # Initialize an 8x8 matrix with 0's
    matrix = [[0 for _ in range(8)] for _ in range(8)]
    
    # Split FEN into parts and take only the board part
    board_fen = fen.split()[0]
    
    # Parse the FEN string
    rank = 0
    for row in board_fen.split('/'):
        file = 0
        for char in row:
            if char.isdigit():
                # Skip empty squares
                file += int(char)
            else:
                # Place a 1 for any piece
                matrix[rank][file] = 1
                file += 1
        rank += 1
    
    return matrix

def print_matrix(matrix):
    """
    Print the matrix in a readable format.
    """
    for row in matrix:
        print(' '.join(str(cell) for cell in row))

if __name__ == "__main__":
    # Example usage
    fen = "rnbqkbnr/pppp1ppp/8/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR w - - 0 2"
    matrix = fen_to_matrix(fen)
    print_matrix(matrix)

