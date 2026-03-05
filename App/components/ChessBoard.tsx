import React, { useEffect, useState } from 'react'
import { Dimensions, StyleSheet, Text, TouchableOpacity, View } from 'react-native'
import { SafeAreaView } from 'react-native-safe-area-context'

import { theme } from '@/constants/theme'
import { ChessEngine, Color, Piece, PieceType } from '@/lib/chess'

const UNICODE_PIECES: Record<Color, Record<PieceType, string>> = {
  [Color.WHITE]: {
    [PieceType.KING]: '♔',
    [PieceType.QUEEN]: '♕',
    [PieceType.ROOK]: '♖',
    [PieceType.BISHOP]: '♗',
    [PieceType.KNIGHT]: '♘',
    [PieceType.PAWN]: '♙',
  },
  [Color.BLACK]: {
    [PieceType.KING]: '♔',
    [PieceType.QUEEN]: '♕',
    [PieceType.ROOK]: '♖',
    [PieceType.BISHOP]: '♗',
    [PieceType.KNIGHT]: '♘',
    [PieceType.PAWN]: '♙',
  },
}

interface ChessBoardProps {
  flipped?: boolean
  autoFlip?: boolean
}

export default function ChessBoard({ flipped = false, autoFlip = false }: ChessBoardProps) {
  const [engine] = useState(() => new ChessEngine())
  const [board, setBoard] = useState<(Piece | null)[]>(engine.getBoard())
  const [selectedSquare, setSelectedSquare] = useState<number | null>(null)
  const [legalMoves, setLegalMoves] = useState<string[]>([])
  const [currentPlayer, setCurrentPlayer] = useState<Color>(engine.getCurrentPlayer())
  const [isFlipped, setIsFlipped] = useState(flipped)
  const [gameStatus, setGameStatus] = useState<string>('')
  const { width: screenWidth, height: screenHeight } = Dimensions.get('window')
  // Use 90% of screen width or 90% of available height (minus space for status/controls), whichever is smaller
  const maxBoardSize = Math.min(screenWidth * 0.9, (screenHeight - 250) * 0.9)
  // Calculate square size and board size to ensure exactly 8x8 grid
  const squareSize = Math.floor(maxBoardSize / 8)
  const boardSize = squareSize * 8

  useEffect(() => {
    updateGameState()
  }, [])

  useEffect(() => {
    setIsFlipped(flipped)
  }, [flipped])

  const updateGameState = () => {
    setBoard(engine.getBoard())
    setCurrentPlayer(engine.getCurrentPlayer())

    let status = `${engine.getCurrentPlayer() === Color.WHITE ? 'White' : 'Black'} to move`

    if (engine.isInCheck()) {
      status += ' - Check!'
    }

    if (engine.isCheckmate()) {
      status = `Checkmate! ${engine.getCurrentPlayer() === Color.WHITE ? 'Black' : 'White'} wins!`
    } else if (engine.isStalemate()) {
      status = 'Stalemate! Draw.'
    }

    setGameStatus(status)
  }

  const handleSquarePress = (square: number) => {
    const piece = board[square]

    if (selectedSquare === null) {
      // Select piece
      if (piece && piece.color === currentPlayer) {
        setSelectedSquare(square)
        const row = Math.floor(square / 8)
        const col = square % 8
        const squareNotation = String.fromCharCode('a'.charCodeAt(0) + col) + (row + 1)
        const moves = engine.getLegalMovesFrom(squareNotation)
        setLegalMoves(moves)
      }
    } else {
      // Check if clicking on another piece of the same color
      if (piece && piece.color === currentPlayer && square !== selectedSquare) {
        // Change selection to new piece
        setSelectedSquare(square)
        const row = Math.floor(square / 8)
        const col = square % 8
        const squareNotation = String.fromCharCode('a'.charCodeAt(0) + col) + (row + 1)
        const moves = engine.getLegalMovesFrom(squareNotation)
        setLegalMoves(moves)
        return
      }

      // Try to make move
      const fromRow = Math.floor(selectedSquare / 8)
      const fromCol = selectedSquare % 8
      const toRow = Math.floor(square / 8)
      const toCol = square % 8

      const from = String.fromCharCode('a'.charCodeAt(0) + fromCol) + (fromRow + 1)
      const to = String.fromCharCode('a'.charCodeAt(0) + toCol) + (toRow + 1)
      const move = from + to

      // Check if pawn promotion
      const selectedPiece = board[selectedSquare]
      if (selectedPiece && selectedPiece.type === PieceType.PAWN) {
        const promotionRow = selectedPiece.color === Color.WHITE ? 7 : 0
        if (toRow === promotionRow) {
          // Auto-promote to queen for now
          const moveWithPromotion = move + 'q'
          if (engine.makeMove(moveWithPromotion)) {
            if (autoFlip) {
              setIsFlipped(!isFlipped)
            }
            updateGameState()
          }
          setSelectedSquare(null)
          setLegalMoves([])
          return
        }
      }

      if (engine.makeMove(move)) {
        if (autoFlip) {
          setIsFlipped(!isFlipped)
        }
        updateGameState()
      }

      setSelectedSquare(null)
      setLegalMoves([])
    }
  }

  const handleRestart = () => {
    engine.newGame()
    setSelectedSquare(null)
    setLegalMoves([])
    setIsFlipped(flipped)
    updateGameState()
  }

  const handleFlipBoard = () => {
    setIsFlipped(!isFlipped)
  }

  const isSquareLegalMove = (square: number): boolean => {
    if (selectedSquare === null) return false
    const toRow = Math.floor(square / 8)
    const toCol = square % 8
    const toSquare = String.fromCharCode('a'.charCodeAt(0) + toCol) + (toRow + 1)

    // Check if any legal move ends at this square
    return legalMoves.some((move) => {
      const moveTo = move.substring(2, 4)
      return moveTo === toSquare
    })
  }

  const renderSquare = (square: number) => {
    const row = Math.floor(square / 8)
    const col = square % 8
    const isLight = (row + col) % 2 === 0
    const isSelected = selectedSquare === square
    const isLegal = isSquareLegalMove(square)

    const piece = board[square]

    return (
      <TouchableOpacity
        key={square}
        style={[
          styles.square,
          {
            width: squareSize,
            height: squareSize,
            backgroundColor: isSelected ? '#7fa650' : isLight ? '#f0d9b5' : '#b58863',
          },
        ]}
        onPress={() => handleSquarePress(square)}
      >
        {isLegal && (
          <View
            style={[
              styles.legalMoveIndicator,
              { backgroundColor: piece ? '#ff000080' : '#00000040' },
            ]}
          />
        )}
        {piece && (
          <Text
            style={[
              styles.piece,
              {
                fontSize: squareSize * 0.7,
                color: piece.color === Color.WHITE ? '#FFFFFF' : '#000000',
                textShadowColor: piece.color === Color.WHITE ? '#000000' : '#FFFFFF',
                textShadowOffset: { width: 0, height: 0 },
                textShadowRadius: 3,
              },
            ]}
          >
            {UNICODE_PIECES[piece.color][piece.type]}
          </Text>
        )}
      </TouchableOpacity>
    )
  }

  const renderBoard = () => {
    const squares = []
    for (let row = 7; row >= 0; row--) {
      for (let col = 0; col < 8; col++) {
        const square = (isFlipped ? 7 - row : row) * 8 + (isFlipped ? 7 - col : col)
        squares.push(renderSquare(square))
      }
    }
    return squares
  }

  return (
    <SafeAreaView style={styles.container} edges={['top', 'left', 'right']}>
      <View style={styles.header}>
        <Text style={styles.statusText}>{gameStatus}</Text>
      </View>

      <View style={styles.boardContainer}>
        <View style={styles.boardWrapper}>
          <View style={[styles.board, { width: boardSize, height: boardSize }]}>
            {renderBoard()}
          </View>
        </View>
      </View>

      <View style={styles.controls}>
        <TouchableOpacity style={styles.button} onPress={handleRestart}>
          <Text style={styles.buttonText}>New Game</Text>
        </TouchableOpacity>

        {!autoFlip && (
          <TouchableOpacity style={styles.button} onPress={handleFlipBoard}>
            <Text style={styles.buttonText}>Flip</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.info}>
        <Text style={styles.infoText}>Tap piece, then tap destination</Text>
      </View>
    </SafeAreaView>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: theme.colors.background.light,
    paddingHorizontal: theme.spacing.sm,
  },
  header: {
    width: '100%',
    paddingVertical: theme.spacing.sm,
    paddingHorizontal: theme.spacing.md,
  },
  statusText: {
    fontSize: theme.fontSize.md,
    fontWeight: '600',
    color: theme.colors.text.light,
    textAlign: 'center',
  },
  boardContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  boardWrapper: {
    borderWidth: 2,
    borderColor: '#333',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
    elevation: 5,
  },
  board: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  square: {
    justifyContent: 'center',
    alignItems: 'center',
    position: 'relative',
  },
  piece: {
    fontWeight: 'bold',
  },
  legalMoveIndicator: {
    position: 'absolute',
    width: '30%',
    height: '30%',
    borderRadius: 100,
  },
  controls: {
    flexDirection: 'row',
    paddingVertical: theme.spacing.sm,
    gap: theme.spacing.sm,
    justifyContent: 'center',
  },
  button: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
    borderRadius: theme.borderRadius.sm,
    minWidth: 80,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: theme.fontSize.sm,
    fontWeight: '600',
    textAlign: 'center',
  },
  info: {
    paddingVertical: theme.spacing.xs,
    paddingHorizontal: theme.spacing.md,
  },
  infoText: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.secondary,
    textAlign: 'center',
    marginBottom: theme.spacing.xs,
  },
})
