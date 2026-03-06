// Import the native module. On web, it will be resolved to ChessEngine.web.ts
// and on native platforms to ChessEngineModule native module
import ChessEngineModule from './src/ChessEngineModule'

export interface PieceInfo {
  type: string
  color: string
}

export class ChessEngine {
  constructor() {
    console.log('ChessEngine module wrapper: constructor called')
  }

  newGame(): void {
    console.log('ChessEngine module wrapper: newGame called')
    try {
      ChessEngineModule.newGame()
      console.log('ChessEngine module wrapper: newGame completed')
    } catch (error) {
      console.error('ChessEngine module wrapper: newGame failed:', error)
      throw error
    }
  }

  makeMove(move: string): boolean {
    return ChessEngineModule.makeMove(move)
  }

  undoMove(): void {
    ChessEngineModule.undoMove()
  }

  getLegalMoves(): string[] {
    return ChessEngineModule.getLegalMoves()
  }

  getLegalMovesFrom(square: string): string[] {
    return ChessEngineModule.getLegalMovesFrom(square)
  }

  isMoveLegal(move: string): boolean {
    return ChessEngineModule.isMoveLegal(move)
  }

  getCurrentPlayer(): string {
    return ChessEngineModule.getCurrentPlayer()
  }

  isInCheck(): boolean {
    return ChessEngineModule.isInCheck()
  }

  isCheckmate(): boolean {
    return ChessEngineModule.isCheckmate()
  }

  isStalemate(): boolean {
    return ChessEngineModule.isStalemate()
  }

  isDraw(): boolean {
    return ChessEngineModule.isDraw()
  }

  getBoard(): (PieceInfo | null)[] {
    console.log('ChessEngine module wrapper: getBoard called')
    try {
      const result = ChessEngineModule.getBoard()
      console.log('ChessEngine module wrapper: getBoard returned, length:', result?.length)
      // Convert empty objects to null
      return result.map((piece) => {
        if (!piece || Object.keys(piece).length === 0) {
          return null
        }
        return piece
      })
    } catch (error) {
      console.error('ChessEngine module wrapper: getBoard failed:', error)
      throw error
    }
  }

  getFEN(): string {
    return ChessEngineModule.getFEN()
  }

  loadFromFEN(fen: string): boolean {
    return ChessEngineModule.loadFromFEN(fen)
  }

  getBestMove(depth: number): string {
    return ChessEngineModule.getBestMove(depth)
  }

  getMoveHistory(): string[] {
    return ChessEngineModule.getMoveHistory()
  }

  canUndo(): boolean {
    return ChessEngineModule.canUndo()
  }
}

export default ChessEngine
