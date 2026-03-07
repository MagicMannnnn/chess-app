import NativeChessEngine from '@/modules/chess-engine'

import { Color, Piece, PieceType } from './types'

export class ChessEngine {
  private nativeEngine: NativeChessEngine | null = null
  private initialized: boolean = false

  constructor() {
    try {
      console.log('ChessEngine: Starting constructor')
      this.nativeEngine = new NativeChessEngine()
      console.log('ChessEngine: NativeChessEngine created successfully')
      this.initialized = true
    } catch (error) {
      console.error('ChessEngine: Failed to create NativeChessEngine:', error)
      this.nativeEngine = null
      this.initialized = false
    }
  }

  private ensureInitialized(): boolean {
    if (!this.initialized || !this.nativeEngine) {
      console.error('ChessEngine: Not initialized')
      return false
    }
    return true
  }

  newGame(): void {
    try {
      console.log('ChessEngine: newGame called')
      if (!this.ensureInitialized()) return
      this.nativeEngine!.newGame()
      console.log('ChessEngine: newGame completed')
    } catch (error) {
      console.error('ChessEngine: newGame failed:', error)
    }
  }

  getBoard(): (Piece | null)[] {
    try {
      console.log('ChessEngine: getBoard called')
      if (!this.ensureInitialized()) {
        console.log('ChessEngine: Returning empty board')
        return Array(64).fill(null)
      }
      const nativeBoard = this.nativeEngine!.getBoard()
      console.log('ChessEngine: Native board received, type:', typeof nativeBoard)
      console.log('ChessEngine: Native board is array:', Array.isArray(nativeBoard))
      console.log('ChessEngine: Native board length:', nativeBoard?.length)
      console.log('ChessEngine: Native board sample:', JSON.stringify(nativeBoard?.slice(0, 3)))

      if (!Array.isArray(nativeBoard)) {
        console.error('ChessEngine: Native board is not an array!')
        return Array(64).fill(null)
      }

      const result = nativeBoard.map((piece, index) => {
        // Check for null, undefined, or empty objects
        if (!piece || piece === null || Object.keys(piece).length === 0) return null
        try {
          return {
            type: piece.type as PieceType,
            color: piece.color as Color,
          }
        } catch (error) {
          console.error(`ChessEngine: Error mapping piece at index ${index}:`, error, piece)
          return null
        }
      })

      console.log('ChessEngine: Board mapping complete')
      return result
    } catch (error) {
      console.error('ChessEngine: getBoard failed:', error)
      return Array(64).fill(null)
    }
  }

  getCurrentPlayer(): Color {
    try {
      console.log('ChessEngine: getCurrentPlayer called')
      if (!this.ensureInitialized()) return Color.WHITE
      const player = this.nativeEngine!.getCurrentPlayer()
      console.log('ChessEngine: Current player:', player)
      return player === 'white' ? Color.WHITE : Color.BLACK
    } catch (error) {
      console.error('ChessEngine: getCurrentPlayer failed:', error)
      return Color.WHITE
    }
  }

  makeMove(algebraic: string): boolean {
    try {
      console.log('ChessEngine: makeMove called with:', algebraic)
      if (!this.ensureInitialized()) return false
      const result = this.nativeEngine!.makeMove(algebraic)
      console.log('ChessEngine: makeMove result:', result)
      return result
    } catch (error) {
      console.error('ChessEngine: makeMove failed:', error)
      return false
    }
  }

  undoMove(): void {
    try {
      if (!this.ensureInitialized()) return
      this.nativeEngine!.undoMove()
    } catch (error) {
      console.error('ChessEngine: undoMove failed:', error)
    }
  }

  getLegalMovesFrom(square: string): string[] {
    try {
      if (!this.ensureInitialized()) return []
      return this.nativeEngine!.getLegalMovesFrom(square)
    } catch (error) {
      console.error('ChessEngine: getLegalMovesFrom failed:', error)
      return []
    }
  }

  getLegalMoves(): string[] {
    try {
      if (!this.ensureInitialized()) return []
      return this.nativeEngine!.getLegalMoves()
    } catch (error) {
      console.error('ChessEngine: getLegalMoves failed:', error)
      return []
    }
  }

  isMoveLegal(algebraic: string): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.isMoveLegal(algebraic)
    } catch (error) {
      console.error('ChessEngine: isMoveLegal failed:', error)
      return false
    }
  }

  isInCheck(): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.isInCheck()
    } catch (error) {
      console.error('ChessEngine: isInCheck failed:', error)
      return false
    }
  }

  isCheckmate(): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.isCheckmate()
    } catch (error) {
      console.error('ChessEngine: isCheckmate failed:', error)
      return false
    }
  }

  isStalemate(): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.isStalemate()
    } catch (error) {
      console.error('ChessEngine: isStalemate failed:', error)
      return false
    }
  }

  isDraw(): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.isDraw()
    } catch (error) {
      console.error('ChessEngine: isDraw failed:', error)
      return false
    }
  }

  getFEN(): string {
    try {
      if (!this.ensureInitialized()) return ''
      return this.nativeEngine!.getFEN()
    } catch (error) {
      console.error('ChessEngine: getFEN failed:', error)
      return ''
    }
  }

  loadFromFEN(fen: string): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.loadFromFEN(fen)
    } catch (error) {
      console.error('ChessEngine: loadFromFEN failed:', error)
      return false
    }
  }

  async getBestMove(depth: number): Promise<string> {
    try {
      if (!this.ensureInitialized()) return ''
      return await this.nativeEngine!.getBestMove(depth)
    } catch (error) {
      console.error('ChessEngine: getBestMove failed:', error)
      return ''
    }
  }

  getMoveHistory(): string[] {
    try {
      if (!this.ensureInitialized()) return []
      return this.nativeEngine!.getMoveHistory()
    } catch (error) {
      console.error('ChessEngine: getMoveHistory failed:', error)
      return []
    }
  }

  canUndo(): boolean {
    try {
      if (!this.ensureInitialized()) return false
      return this.nativeEngine!.canUndo()
    } catch (error) {
      console.error('ChessEngine: canUndo failed:', error)
      return false
    }
  }

  evaluatePosition(): number {
    try {
      if (!this.ensureInitialized()) return 0
      return this.nativeEngine!.evaluatePosition()
    } catch (error) {
      console.error('ChessEngine: evaluatePosition failed:', error)
      return 0
    }
  }
}

export { Color, type Piece, PieceType } from './types'
