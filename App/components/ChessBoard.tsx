import React, { useEffect, useImperativeHandle, useRef, useState } from 'react'
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
import { useSettings } from '@/contexts/SettingsContext'
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
    [PieceType.KING]: '♚',
    [PieceType.QUEEN]: '♛',
    [PieceType.ROOK]: '♜',
    [PieceType.BISHOP]: '♝',
    [PieceType.KNIGHT]: '♞',
    [PieceType.PAWN]: '♟',
  },
}

export interface ChessBoardRef {
  refresh: () => void
  resetGame: () => void
}

interface ChessBoardProps {
  flipped?: boolean
  autoFlip?: boolean
  searchDepth?: number
  maxSearchTime?: number // milliseconds
  isAIEnabled?: boolean
  aiVersion?: 'v1' | 'v2'
  hideBestMove?: boolean
  useChessClock?: boolean
  clockTimeMinutes?: number
  showControls?: boolean
  onEvaluationChange?: (evaluation: number, depth: number) => void
  onBestMoveChange?: (move: string) => void
  onCurrentPlayerChange?: (player: Color) => void
  onEngineReady?: (engine: ChessEngine) => void
}

const ChessBoard = React.forwardRef<ChessBoardRef, ChessBoardProps>(
  (
    {
      flipped = false,
      autoFlip = false,
      searchDepth = 5,
      maxSearchTime,
      isAIEnabled = false,
      aiVersion,
      hideBestMove = false,
      useChessClock: _useChessClock = false,
      clockTimeMinutes: _clockTimeMinutes = 10,
      showControls = true,
      onEvaluationChange,
      onBestMoveChange,
      onCurrentPlayerChange,
      onEngineReady,
    },
    ref,
  ) => {
    const { settings } = useSettings()
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
    const [searchComplete, setSearchComplete] = useState(false)
    const [lastMoveFrom, setLastMoveFrom] = useState<number | null>(null)
    const [lastMoveTo, setLastMoveTo] = useState<number | null>(null)
    const [capturedPieces, setCapturedPieces] = useState<{
      white: Piece[]
      black: Piece[]
      materialAdvantage?: number
    }>({ white: [], black: [] })
    const boardRef = useRef<View>(null)
    const boardWrapperRef = useRef<View>(null)
    const boardLayoutRef = useRef({ x: 0, y: 0, width: 0, height: 0 })
    const onEvaluationChangeRef = useRef(onEvaluationChange)
    const onBestMoveChangeRef = useRef(onBestMoveChange)
    const onCurrentPlayerChangeRef = useRef(onCurrentPlayerChange)
    const { width: screenWidth, height: screenHeight } = Dimensions.get('window')
    // Maximize board size - account for header, controls, and clock if present
    const maxBoardSize = Math.min(screenWidth * 0.96, (screenHeight - 180) * 0.96)
    // Calculate square size and board size to ensure exactly 8x8 grid
    const squareSize = Math.floor(maxBoardSize / 8)
    const boardSize = squareSize * 8

    // Expose methods to parent components
    useImperativeHandle(ref, () => ({
      refresh: () => {
        updateGameState()
      },
      resetGame: () => {
        try {
          console.log('ChessBoard: resetGame called')
          engine.newGame()
          setSelectedSquare(null)
          setLegalMoves([])
          setIsFlipped(flipped)
          setBestMove('')
          setSearchComplete(false)
          setLastMoveFrom(null)
          setLastMoveTo(null)
          setDraggingSquare(null)
          setDragPosition(null)
          setHoveredSquare(null)
          setCapturedPieces({ white: [], black: [] })
          updateGameState()
          console.log('ChessBoard: resetGame complete')
        } catch (error) {
          console.error('ChessBoard: resetGame failed:', error)
        }
      },
    }))

    useEffect(() => {
      // Initialize the engine and board state after component mount
      console.log('ChessBoard: useEffect - initializing')
      try {
        console.log('ChessBoard: Calling engine.newGame()')
        engine.newGame()
        console.log('ChessBoard: Calling updateGameState()')
        updateGameState()
        console.log('ChessBoard: Initialization complete')
        if (onEngineReady) {
          onEngineReady(engine)
        }
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

    useEffect(() => {
      onBestMoveChangeRef.current = onBestMoveChange
    }, [onBestMoveChange])

    useEffect(() => {
      onCurrentPlayerChangeRef.current = onCurrentPlayerChange
    }, [onCurrentPlayerChange])

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

        // Calculate material values
        const pieceValues = {
          [PieceType.PAWN]: 1,
          [PieceType.KNIGHT]: 3,
          [PieceType.BISHOP]: 3,
          [PieceType.ROOK]: 5,
          [PieceType.QUEEN]: 9,
          [PieceType.KING]: 0,
        }

        const whiteMaterial = captured.white.reduce((sum, p) => sum + pieceValues[p.type], 0)
        const blackMaterial = captured.black.reduce((sum, p) => sum + pieceValues[p.type], 0)
        const materialAdvantage = whiteMaterial - blackMaterial

        setCapturedPieces({ ...captured, materialAdvantage } as any)
        setBoard(newBoard)
        console.log('ChessBoard: updateGameState - getting current player')
        const player = engine.getCurrentPlayer()
        setCurrentPlayer(player)

        // Update last move highlighting
        const history = engine.getMoveHistory()
        if (history.length > 0) {
          const lastMove = history[history.length - 1]
          // Parse move notation (e.g., "e2e4" or "e7e8q")
          const fromCol = lastMove.charCodeAt(0) - 'a'.charCodeAt(0)
          const fromRow = parseInt(lastMove[1]) - 1
          const toCol = lastMove.charCodeAt(2) - 'a'.charCodeAt(0)
          const toRow = parseInt(lastMove[3]) - 1

          setLastMoveFrom(fromRow * 8 + fromCol)
          setLastMoveTo(toRow * 8 + toCol)
        } else {
          setLastMoveFrom(null)
          setLastMoveTo(null)
        }

        // Notify parent of player change
        const playerCallback = onCurrentPlayerChangeRef.current
        if (playerCallback) {
          playerCallback(player)
        }

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

    // Compute best move when game state changes
    // Uses new unified search API - iterative deepening happens entirely in engine
    useEffect(() => {
      if (engine.isCheckmate() || engine.isStalemate()) {
        setBestMove('')
        setSearchComplete(true)
        return
      }

      // Reset search complete flag when starting new search
      setSearchComplete(false)

      const playerColor = currentPlayer
      const abortController = new AbortController()

      const computeBestMove = async () => {
        try {
          console.log(
            `ChessBoard: Starting search with maxDepth=${searchDepth}, maxTime=${maxSearchTime}ms`,
          )

          // Call the new unified search API
          // Iterative deepening happens inside the engine with full time budget
          const result = await engine.searchBestMove({
            maxDepth: searchDepth,
            maxTimeMs: maxSearchTime || 0,
            aiVersion: aiVersion || settings.aiVersion,
            signal: abortController.signal,
            onProgress: (progress) => {
              // Update UI after each completed depth
              if (!abortController.signal.aborted && progress.bestMove) {
                setBestMove(progress.bestMove)

                // Notify parent of best move change
                const moveCallback = onBestMoveChangeRef.current
                if (moveCallback) {
                  moveCallback(progress.bestMove)
                }

                // Update evaluation (from white's perspective)
                const evalCallback = onEvaluationChangeRef.current
                if (evalCallback) {
                  const whiteEvaluation =
                    playerColor === Color.BLACK ? -progress.score : progress.score
                  evalCallback(whiteEvaluation, progress.depth)
                }

                // console.log(
                //   `ChessBoard: Depth ${progress.depth} complete, best move: ${progress.bestMove}, ` +
                //     `score: ${progress.score}, nodes: ${progress.nodesSearched}, time: ${progress.timeMs}ms`,
                // )
              }
            },
          })

          if (!abortController.signal.aborted) {
            console.log(
              `ChessBoard: Search complete - bestMove: ${result.bestMove}, ` +
                `depth: ${result.depthCompleted}/${searchDepth}, ` +
                `nodes: ${result.nodesSearched}, time: ${result.totalTimeMs}ms, ` +
                `timedOut: ${result.timedOut}, cancelled: ${result.cancelled}`,
            )

            // Ensure final best move is set
            if (result.bestMove) {
              setBestMove(result.bestMove)
            }

            setSearchComplete(true)
          }
        } catch (error) {
          console.error('ChessBoard: searchBestMove failed:', error)
          if (!abortController.signal.aborted) {
            setSearchComplete(true)
          }
        }
      }

      // Start computing after a short delay
      const timer = setTimeout(computeBestMove, 50)

      return () => {
        abortController.abort()
        clearTimeout(timer)
      }
    }, [board, searchDepth, maxSearchTime, engine, currentPlayer])

    // Auto-play AI moves when enabled
    useEffect(() => {
      if (
        !isAIEnabled ||
        !bestMove ||
        !searchComplete ||
        engine.isCheckmate() ||
        engine.isStalemate()
      ) {
        return
      }

      // Use shorter delay when there's a time limit (for responsive gameplay)
      const delay = maxSearchTime ? 100 : 500

      // Wait a brief moment before making the AI move (for better UX)
      const timer = setTimeout(() => {
        try {
          console.log(`ChessBoard: AI making move: ${bestMove}`)
          if (engine.makeMove(bestMove)) {
            if (autoFlip) {
              setIsFlipped(!isFlipped)
            }
            updateGameState()
          }
        } catch (error) {
          console.error('ChessBoard: AI move failed:', error)
        }
      }, delay)

      return () => clearTimeout(timer)
    }, [isAIEnabled, bestMove, searchComplete, engine, autoFlip, isFlipped, maxSearchTime])

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
      const isLastMoveFrom = lastMoveFrom === square
      const isLastMoveTo = lastMoveTo === square

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
          {isLastMoveFrom && (
            <View style={[styles.lastMoveHighlight, styles.lastMoveFromHighlight]} />
          )}
          {isLastMoveTo && <View style={[styles.lastMoveHighlight, styles.lastMoveToHighlight]} />}
          {isLegal && (
            <View style={[styles.legalMoveIndicator, { backgroundColor: '#00000040' }]} />
          )}
          {piece && (
            <View
              {...(panResponder?.panHandlers || {})}
              style={{ opacity: isDragging ? 0 : 1, pointerEvents: 'box-only' }}
            >
              <Text
                style={[
                  styles.piece,
                  {
                    fontSize: squareSize * 0.75,
                    color: piece.color === Color.WHITE ? '#FFFFFF' : '#000000',
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

    const materialAdv = capturedPieces.materialAdvantage || 0

    return (
      <SafeAreaView style={styles.container} edges={[]}>
        {/* Compact Header with Status and Captured Pieces */}
        <View style={styles.compactHeader}>
          <Text style={styles.statusText}>{gameStatus}</Text>

          {/* Material Advantage & Captured Pieces in one line */}
          <View style={styles.materialContainer}>
            <View style={styles.capturedRow}>
              <Text style={styles.sideLabel}>⚫</Text>
              <View style={styles.capturedList}>
                {capturedPieces.black.map((piece, idx) => (
                  <Text key={idx} style={styles.capturedPieceCompact}>
                    {UNICODE_PIECES[piece.color][piece.type]}
                  </Text>
                ))}
              </View>
              {materialAdv < 0 && (
                <Text style={styles.materialAdvantage}>+{Math.abs(materialAdv)}</Text>
              )}
            </View>

            <View style={styles.capturedRow}>
              <Text style={styles.sideLabel}>⚪</Text>
              <View style={styles.capturedList}>
                {capturedPieces.white.map((piece, idx) => (
                  <Text key={idx} style={styles.capturedPieceCompact}>
                    {UNICODE_PIECES[piece.color][piece.type]}
                  </Text>
                ))}
              </View>
              {materialAdv > 0 && <Text style={styles.materialAdvantage}>+{materialAdv}</Text>}
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
                        fontSize: squareSize * 0.75,
                        color: board[draggingSquare]?.color === Color.WHITE ? '#FFFFFF' : '#000000',
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
            {!hideBestMove && bestMove && bestMove.length >= 4 && (
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

        {/* Compact Controls */}
        {showControls && (
          <View style={styles.compactControls}>
            {!autoFlip && (
              <View style={styles.controlRow}>
                <TouchableOpacity style={styles.compactButton} onPress={handleFlipBoard}>
                  <Text style={styles.buttonText}>Flip Board</Text>
                </TouchableOpacity>
              </View>
            )}
            {!hideBestMove && bestMove && <Text style={styles.bestMoveText}>Best: {bestMove}</Text>}
          </View>
        )}
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
  },
)

ChessBoard.displayName = 'ChessBoard'

export default ChessBoard

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    backgroundColor: theme.colors.background.light,
  },
  compactHeader: {
    width: '100%',
    paddingHorizontal: 10,
    paddingVertical: 4,
    backgroundColor: '#f5f5f5',
  },
  statusText: {
    fontSize: 13,
    fontWeight: '600',
    color: theme.colors.text.light,
    textAlign: 'center',
    marginBottom: 3,
  },
  materialContainer: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    gap: 6,
  },
  capturedRow: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 3,
    flex: 1,
  },
  sideLabel: {
    fontSize: 14,
  },
  capturedList: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 2,
    flex: 1,
  },
  capturedPieceCompact: {
    fontSize: 24,
    color: '#333',
  },
  materialAdvantage: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#2f95dc',
    marginLeft: 4,
  },
  boardContainer: {
    justifyContent: 'center',
    alignItems: 'center',
  },
  boardWrapper: {
    borderWidth: 1,
    borderColor: '#333',
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
  lastMoveHighlight: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
  },
  lastMoveFromHighlight: {
    backgroundColor: 'rgba(255, 255, 0, 0.3)',
  },
  lastMoveToHighlight: {
    backgroundColor: 'rgba(255, 255, 0, 0.5)',
  },
  compactControls: {
    width: '100%',
    paddingHorizontal: 12,
    paddingVertical: 8,
    alignItems: 'center',
    gap: 6,
  },
  controlRow: {
    flexDirection: 'row',
    gap: 8,
    justifyContent: 'center',
  },
  compactButton: {
    backgroundColor: theme.colors.primary,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    minWidth: 80,
  },
  buttonDisabled: {
    backgroundColor: '#ccc',
    opacity: 0.5,
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
    textAlign: 'center',
  },
  bestMoveText: {
    fontSize: 14,
    color: 'rgb(0, 150, 0)',
    fontWeight: '600',
  },
})
