import React, { useEffect, useRef, useState } from 'react'
import {
  Dimensions,
  type GestureResponderEvent,
  PanResponder,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native'
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
  showBestMove?: boolean
  searchDepth?: number
  onEvaluationChange?: (evaluation: number, depth: number) => void
}

export default function ChessBoard({
  flipped = false,
  autoFlip = false,
  showBestMove = false,
  searchDepth = 3,
  onEvaluationChange,
}: ChessBoardProps) {
  const [engine] = useState(() => {
    console.log('ChessBoard: Creating ChessEngine instance')
    try {
      const eng = new ChessEngine()
      console.log('ChessBoard: ChessEngine instance created')
      return eng
    } catch (error) {
      console.error('ChessBoard: Failed to create ChessEngine:', error)
      throw error
    }
  })
  const [board, setBoard] = useState<(Piece | null)[]>(Array(64).fill(null))
  const [selectedSquare, setSelectedSquare] = useState<number | null>(null)
  const [legalMoves, setLegalMoves] = useState<string[]>([])
  const [currentPlayer, setCurrentPlayer] = useState<Color>(Color.WHITE)
  const [isFlipped, setIsFlipped] = useState(flipped)
  const [gameStatus, setGameStatus] = useState<string>('')
  const [draggingSquare, setDraggingSquare] = useState<number | null>(null)
  const [dragPosition, setDragPosition] = useState<{ x: number; y: number } | null>(null)
  const [hoveredSquare, setHoveredSquare] = useState<number | null>(null)
  const [bestMove, setBestMove] = useState<string>('')
  const [moveHistory, setMoveHistory] = useState<string[]>([])
  const [historyIndex, setHistoryIndex] = useState<number>(-1)
  const [capturedPieces, setCapturedPieces] = useState<{
    white: Piece[]
    black: Piece[]
  }>({ white: [], black: [] })
  const boardRef = useRef<View>(null)
  const boardWrapperRef = useRef<View>(null)
  const boardLayoutRef = useRef({ x: 0, y: 0, width: 0, height: 0 })
  const onEvaluationChangeRef = useRef(onEvaluationChange)
  const { width: screenWidth, height: screenHeight } = Dimensions.get('window')
  // Use 90% of screen width or 90% of available height (minus space for status/controls), whichever is smaller
  const maxBoardSize = Math.min(screenWidth * 0.9, (screenHeight - 250) * 0.9)
  // Calculate square size and board size to ensure exactly 8x8 grid
  const squareSize = Math.floor(maxBoardSize / 8)
  const boardSize = squareSize * 8

  useEffect(() => {
    // Initialize the engine and board state after component mount
    console.log('ChessBoard: useEffect - initializing')
    try {
      console.log('ChessBoard: Calling engine.newGame()')
      engine.newGame()
      console.log('ChessBoard: Calling updateGameState()')
      updateGameState()
      console.log('ChessBoard: Initialization complete')
    } catch (error) {
      console.error('ChessBoard: Error initializing chess engine:', error)
    }
  }, [])

  useEffect(() => {
    setIsFlipped(flipped)
  }, [flipped])

  // Keep evaluation callback ref up to date
  useEffect(() => {
    onEvaluationChangeRef.current = onEvaluationChange
  }, [onEvaluationChange])

  const updateGameState = () => {
    try {
      console.log('ChessBoard: updateGameState - getting board')
      const newBoard = engine.getBoard()
      console.log(
        'ChessBoard: updateGameState - board received, sample pieces:',
        newBoard
          .slice(0, 8)
          .map((p, i) => `${i}:${p ? p.type : 'null'}`)
          .join(', '),
      )

      // Calculate captured pieces
      const startingPieces = {
        white: {
          [PieceType.PAWN]: 8,
          [PieceType.KNIGHT]: 2,
          [PieceType.BISHOP]: 2,
          [PieceType.ROOK]: 2,
          [PieceType.QUEEN]: 1,
          [PieceType.KING]: 1,
        },
        black: {
          [PieceType.PAWN]: 8,
          [PieceType.KNIGHT]: 2,
          [PieceType.BISHOP]: 2,
          [PieceType.ROOK]: 2,
          [PieceType.QUEEN]: 1,
          [PieceType.KING]: 1,
        },
      }

      // Count remaining pieces
      const remainingPieces = {
        white: {
          [PieceType.PAWN]: 0,
          [PieceType.KNIGHT]: 0,
          [PieceType.BISHOP]: 0,
          [PieceType.ROOK]: 0,
          [PieceType.QUEEN]: 0,
          [PieceType.KING]: 0,
        },
        black: {
          [PieceType.PAWN]: 0,
          [PieceType.KNIGHT]: 0,
          [PieceType.BISHOP]: 0,
          [PieceType.ROOK]: 0,
          [PieceType.QUEEN]: 0,
          [PieceType.KING]: 0,
        },
      }

      newBoard.forEach((piece) => {
        if (piece) {
          remainingPieces[piece.color][piece.type]++
        }
      })

      // Calculate captured pieces
      const captured = {
        white: [] as Piece[],
        black: [] as Piece[],
      }

      // Pieces captured from white (stored in black's captured list)
      Object.entries(startingPieces.white).forEach(([type, count]) => {
        const remaining = remainingPieces.white[type as PieceType]
        const capturedCount = count - remaining
        for (let i = 0; i < capturedCount; i++) {
          captured.black.push({ type: type as PieceType, color: Color.WHITE })
        }
      })

      // Pieces captured from black (stored in white's captured list)
      Object.entries(startingPieces.black).forEach(([type, count]) => {
        const remaining = remainingPieces.black[type as PieceType]
        const capturedCount = count - remaining
        for (let i = 0; i < capturedCount; i++) {
          captured.white.push({ type: type as PieceType, color: Color.BLACK })
        }
      })

      setCapturedPieces(captured)
      setBoard(newBoard)
      console.log('ChessBoard: updateGameState - getting current player')
      const player = engine.getCurrentPlayer()
      setCurrentPlayer(player)

      let status = `${player === Color.WHITE ? 'White' : 'Black'} to move`

      console.log('ChessBoard: updateGameState - checking game state')
      if (engine.isInCheck()) {
        status += ' - Check!'
      }

      if (engine.isCheckmate()) {
        status = `Checkmate! ${player === Color.WHITE ? 'Black' : 'White'} wins!`
      } else if (engine.isStalemate()) {
        status = 'Stalemate! Draw.'
      }

      setGameStatus(status)

      // Update move history
      const history = engine.getMoveHistory()
      setMoveHistory(history)
      setHistoryIndex(history.length - 1)

      // Get evaluation and notify parent (from white's perspective)
      const evalCallback = onEvaluationChangeRef.current
      if (evalCallback) {
        const evaluation = engine.evaluatePosition()
        // Convert to white's perspective: if black to move, negate the score
        const whiteEvaluation = player === Color.BLACK ? -evaluation : evaluation
        evalCallback(whiteEvaluation, 0)
      }

      console.log('ChessBoard: updateGameState - complete')
    } catch (error) {
      console.error('ChessBoard: updateGameState failed:', error)
    }
  }

  // Compute best move when game state changes and showBestMove is enabled
  // Uses iterative deepening to keep UI responsive
  useEffect(() => {
    if (!showBestMove || engine.isCheckmate() || engine.isStalemate()) {
      setBestMove('')
      return
    }

    let cancelled = false
    let currentDepth = 1
    // Cache current player to avoid repeated calls during search
    const playerColor = currentPlayer

    // Recursive function to compute one depth at a time
    const computeNextDepth = async () => {
      if (cancelled || currentDepth > searchDepth) return

      try {
        // Now getBestMove is async and runs on background thread
        const move = await engine.getBestMove(currentDepth)
        if (!cancelled) {
          setBestMove(move)

          // Update evaluation at this depth (from white's perspective)
          const evalCallback = onEvaluationChangeRef.current
          if (evalCallback) {
            const evaluation = engine.evaluatePosition()
            const whiteEvaluation = playerColor === Color.BLACK ? -evaluation : evaluation
            evalCallback(whiteEvaluation, currentDepth)
          }
        }
      } catch (error) {
        console.error(`ChessBoard: getBestMove(${currentDepth}) failed:`, error)
      }

      currentDepth++

      // Continue to next depth
      if (currentDepth <= searchDepth && !cancelled) {
        computeNextDepth()
      }
    }

    // Start computing after a short delay
    const timer = setTimeout(computeNextDepth, 50)

    return () => {
      cancelled = true
      clearTimeout(timer)
    }
  }, [board, showBestMove, searchDepth, engine])

  const handlePrevMove = () => {
    if (historyIndex < 0) return
    engine.undoMove()
    updateGameState()
    setHistoryIndex(historyIndex - 1)
  }

  const handleNextMove = () => {
    if (historyIndex >= moveHistory.length - 1) return
    // Redo by making the move again
    const move = moveHistory[historyIndex + 1]
    if (move) {
      engine.makeMove(move)
      updateGameState()
      setHistoryIndex(historyIndex + 1)
    }
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
        console.log('ChessBoard: Move successful:', move)
        if (autoFlip) {
          setIsFlipped(!isFlipped)
        }
        updateGameState()
      } else {
        console.log('ChessBoard: Move failed:', move)
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

  const createPiecePanResponder = (square: number, piece: Piece) => {
    return PanResponder.create({
      onStartShouldSetPanResponder: () => piece.color === currentPlayer,
      onMoveShouldSetPanResponder: () => piece.color === currentPlayer,
      onPanResponderGrant: (evt: GestureResponderEvent) => {
        const { pageX, pageY } = evt.nativeEvent
        // Remeasure board position to ensure accuracy
        boardRef.current?.measureInWindow((x, y, width, height) => {
          boardLayoutRef.current = { x, y, width, height }
          setDraggingSquare(square)
          setDragPosition({ x: pageX - x, y: pageY - y })
        })
        setSelectedSquare(square)
        const row = Math.floor(square / 8)
        const col = square % 8
        const squareNotation = String.fromCharCode('a'.charCodeAt(0) + col) + (row + 1)
        const moves = engine.getLegalMovesFrom(squareNotation)
        setLegalMoves(moves)
      },
      onPanResponderMove: (evt: GestureResponderEvent) => {
        const { pageX, pageY } = evt.nativeEvent
        const layout = boardLayoutRef.current
        const relativeX = pageX - layout.x
        const relativeY = pageY - layout.y
        setDragPosition({ x: relativeX, y: relativeY })

        // Calculate hovered square
        if (
          relativeX >= 0 &&
          relativeX < layout.width &&
          relativeY >= 0 &&
          relativeY < layout.height
        ) {
          const col = Math.floor(relativeX / squareSize)
          const row = 7 - Math.floor(relativeY / squareSize)
          const hovered = isFlipped ? (7 - row) * 8 + (7 - col) : row * 8 + col
          setHoveredSquare(hovered)
        } else {
          setHoveredSquare(null)
        }
      },
      onPanResponderRelease: (evt: GestureResponderEvent) => {
        const { pageX, pageY } = evt.nativeEvent
        const layout = boardLayoutRef.current

        // Calculate which square the piece was dropped on
        const relativeX = pageX - layout.x
        const relativeY = pageY - layout.y

        if (
          relativeX >= 0 &&
          relativeX < layout.width &&
          relativeY >= 0 &&
          relativeY < layout.height
        ) {
          const col = Math.floor(relativeX / squareSize)
          const row = 7 - Math.floor(relativeY / squareSize)

          const targetSquare = isFlipped ? (7 - row) * 8 + (7 - col) : row * 8 + col

          // Try to make the move
          if (targetSquare !== square) {
            handleSquarePress(targetSquare)
          }
        }
        // Clear drag state
        setDraggingSquare(null)
        setDragPosition(null)
        setHoveredSquare(null)
      },
      onPanResponderTerminate: () => {
        // Clear drag state on terminate
        setDraggingSquare(null)
        setDragPosition(null)
        setHoveredSquare(null)
      },
    })
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
    const isHovered = hoveredSquare === square

    const piece = board[square]
    const isDragging = draggingSquare === square

    const panResponder =
      piece && piece.color === currentPlayer ? createPiecePanResponder(square, piece) : null

    // Calculate background color with slight lightening for selected/hovered
    const baseColor = isLight ? '#f0d9b5' : '#b58863'
    const lightColor = isLight ? '#faf0e0' : '#cca47a'
    const backgroundColor = isSelected || isHovered ? lightColor : baseColor

    return (
      <TouchableOpacity
        key={square}
        style={[
          styles.square,
          {
            width: squareSize,
            height: squareSize,
            backgroundColor,
          },
        ]}
        onPress={() => handleSquarePress(square)}
      >
        {isLegal && <View style={[styles.legalMoveIndicator, { backgroundColor: '#00000040' }]} />}
        {piece && (
          <View
            {...(panResponder?.panHandlers || {})}
            style={{ opacity: isDragging ? 0 : 1, pointerEvents: 'box-only' }}
          >
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
          </View>
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

      {/* Captured Pieces Display */}
      <View style={styles.capturedPiecesContainer}>
        {/* Black's captured pieces (pieces taken from white) */}
        <View style={styles.capturedPiecesRow}>
          <Text style={styles.capturedLabel}>Black captured:</Text>
          <View style={styles.capturedPieces}>
            {capturedPieces.black.map((piece, idx) => (
              <Text
                key={idx}
                style={[
                  styles.capturedPiece,
                  { color: piece.color === Color.WHITE ? '#FFFFFF' : '#000000' },
                ]}
              >
                {UNICODE_PIECES[piece.color][piece.type]}
              </Text>
            ))}
            {capturedPieces.black.length === 0 && <Text style={styles.noPieces}>None</Text>}
          </View>
        </View>

        {/* White's captured pieces (pieces taken from black) */}
        <View style={styles.capturedPiecesRow}>
          <Text style={styles.capturedLabel}>White captured:</Text>
          <View style={styles.capturedPieces}>
            {capturedPieces.white.map((piece, idx) => (
              <Text
                key={idx}
                style={[
                  styles.capturedPiece,
                  { color: piece.color === Color.WHITE ? '#FFFFFF' : '#000000' },
                ]}
              >
                {UNICODE_PIECES[piece.color][piece.type]}
              </Text>
            ))}
            {capturedPieces.white.length === 0 && <Text style={styles.noPieces}>None</Text>}
          </View>
        </View>
      </View>

      <View style={styles.boardContainer}>
        <View style={styles.boardWrapper} ref={boardWrapperRef}>
          <View
            style={[styles.board, { width: boardSize, height: boardSize }]}
            ref={boardRef}
            onLayout={(_event) => {
              boardRef.current?.measureInWindow((x, y, width, height) => {
                boardLayoutRef.current = { x, y, width, height }
              })
            }}
          >
            {renderBoard()}

            {/* Dragged piece overlay */}
            {draggingSquare !== null && dragPosition && (
              <View
                style={{
                  position: 'absolute',
                  left: dragPosition.x - squareSize / 2,
                  top: dragPosition.y - squareSize / 2,
                  width: squareSize,
                  height: squareSize,
                  zIndex: 1000,
                  justifyContent: 'center',
                  alignItems: 'center',
                  pointerEvents: 'none',
                }}
              >
                <Text
                  style={[
                    styles.piece,
                    {
                      fontSize: squareSize * 0.7,
                      color: board[draggingSquare]?.color === Color.WHITE ? '#FFFFFF' : '#000000',
                      textShadowColor:
                        board[draggingSquare]?.color === Color.WHITE ? '#000000' : '#FFFFFF',
                      textShadowOffset: { width: 0, height: 0 },
                      textShadowRadius: 3,
                    },
                  ]}
                >
                  {board[draggingSquare] &&
                    UNICODE_PIECES[board[draggingSquare]!.color][board[draggingSquare]!.type]}
                </Text>
              </View>
            )}
          </View>

          {/* Best move arrow */}
          {showBestMove && bestMove && bestMove.length >= 4 && (
            <View
              style={{
                position: 'absolute',
                top: 0,
                left: 0,
                right: 0,
                bottom: 0,
                pointerEvents: 'none',
              }}
            >
              {renderBestMoveArrow()}
            </View>
          )}
        </View>
      </View>

      <View style={styles.controls}>
        {/* Navigation buttons */}
        <View style={styles.navigationRow}>
          <TouchableOpacity
            style={[styles.navButton, historyIndex < 0 && styles.navButtonDisabled]}
            onPress={handlePrevMove}
            disabled={historyIndex < 0}
          >
            <Text style={styles.navButtonText}>{'←'}</Text>
          </TouchableOpacity>

          <Text style={styles.moveCounter}>
            {moveHistory.length > 0 ? `${historyIndex + 1} / ${moveHistory.length}` : ''}
          </Text>

          <TouchableOpacity
            style={[
              styles.navButton,
              historyIndex >= moveHistory.length - 1 && styles.navButtonDisabled,
            ]}
            onPress={handleNextMove}
            disabled={historyIndex >= moveHistory.length - 1}
          >
            <Text style={styles.navButtonText}>{'→'}</Text>
          </TouchableOpacity>
        </View>

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
        {showBestMove && bestMove && <Text style={styles.bestMoveText}>Best move: {bestMove}</Text>}
      </View>
    </SafeAreaView>
  )

  function renderBestMoveArrow() {
    if (!bestMove || bestMove.length < 4) return null

    const fromCol = bestMove.charCodeAt(0) - 'a'.charCodeAt(0)
    const fromRow = parseInt(bestMove[1]) - 1
    const toCol = bestMove.charCodeAt(2) - 'a'.charCodeAt(0)
    const toRow = parseInt(bestMove[3]) - 1

    // Calculate visual positions using same logic as board rendering
    // The board renders squares with: for row=7 down to 0, col=0 to 7
    const getVisualPos = (row: number, col: number) => {
      // Apply flipping
      const visualRow = isFlipped ? row : 7 - row
      const visualCol = isFlipped ? 7 - col : col
      return {
        x: visualCol * squareSize + squareSize / 2,
        y: visualRow * squareSize + squareSize / 2,
      }
    }

    const fromPos = getVisualPos(fromRow, fromCol)
    const toPos = getVisualPos(toRow, toCol)

    const dx = toPos.x - fromPos.x
    const dy = toPos.y - fromPos.y
    const angle = Math.atan2(dy, dx)
    const length = Math.sqrt(dx * dx + dy * dy)

    const arrowHeight = 12

    // Position the arrow so its left-center starts at fromPos
    // After rotation, adjust position
    const startX = fromPos.x
    const startY = fromPos.y - arrowHeight / 2

    // Arrow goes from center to center
    return (
      <View
        style={{
          position: 'absolute',
          left: startX,
          top: startY,
          width: length,
          height: arrowHeight,
          backgroundColor: 'rgba(0, 255, 0, 0.4)',
          transform: [{ rotate: `${angle}rad` }],
          transformOrigin: 'left center',
          borderRadius: 6,
        }}
      >
        {/* Arrowhead */}
        <View
          style={{
            position: 'absolute',
            right: -12,
            top: -8,
            width: 0,
            height: 0,
            borderLeftWidth: 18,
            borderLeftColor: 'rgba(0, 255, 0, 0.4)',
            borderTopWidth: 14,
            borderTopColor: 'transparent',
            borderBottomWidth: 14,
            borderBottomColor: 'transparent',
          }}
        />
      </View>
    )
  }
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
    position: 'relative',
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
    paddingVertical: theme.spacing.sm,
    gap: theme.spacing.sm,
    alignItems: 'center',
  },
  navigationRow: {
    flexDirection: 'row',
    gap: theme.spacing.md,
    alignItems: 'center',
    justifyContent: 'center',
  },
  navButton: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.sm,
    borderRadius: theme.borderRadius.sm,
    minWidth: 50,
  },
  navButtonDisabled: {
    backgroundColor: theme.colors.background.dark,
    opacity: 0.5,
  },
  navButtonText: {
    color: '#ffffff',
    fontSize: 20,
    fontWeight: '600',
    textAlign: 'center',
  },
  moveCounter: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.text.light,
    fontWeight: '600',
    minWidth: 60,
    textAlign: 'center',
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
  bestMoveText: {
    fontSize: theme.fontSize.sm,
    color: 'rgb(0, 200, 0)',
    textAlign: 'center',
    fontWeight: '600',
  },
  capturedPiecesContainer: {
    width: '100%',
    paddingHorizontal: theme.spacing.md,
    paddingVertical: theme.spacing.xs,
    backgroundColor: theme.colors.background.dark,
  },
  capturedPiecesRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: theme.spacing.xs,
  },
  capturedLabel: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.text.light,
    fontWeight: '600',
    marginRight: theme.spacing.sm,
    width: 115,
  },
  capturedPieces: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 2,
  },
  capturedPiece: {
    fontSize: 20,
    textShadowColor: 'rgba(0, 0, 0, 0.5)',
    textShadowOffset: { width: 0, height: 0 },
    textShadowRadius: 2,
  },
  noPieces: {
    fontSize: theme.fontSize.xs,
    color: theme.colors.secondary,
    fontStyle: 'italic',
  },
})
