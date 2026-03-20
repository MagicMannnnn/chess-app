import ChessEngineModule, {
  addSearchProgressListener,
  type SearchProgressData,
  type SearchResultData,
} from './src/ChessEngineModule'

export interface PieceInfo {
  type: string
  color: string
}

export class ChessEngine {
  constructor() {}

  newGame(): void {
    ChessEngineModule.newGame()
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
    const result = ChessEngineModule.getBoard()
    return result.map((piece) => {
      if (!piece || Object.keys(piece).length === 0) {
        return null
      }
      return piece
    })
  }

  getFEN(): string {
    return ChessEngineModule.getFEN()
  }

  loadFromFEN(fen: string): boolean {
    return ChessEngineModule.loadFromFEN(fen)
  }

  async getBestMove(
    depth: number,
    maxTimeMs: number = 0,
    aiVersion: 'v1' | 'v2' = 'v1',
  ): Promise<string> {
    return await ChessEngineModule.getBestMove(depth, maxTimeMs, aiVersion)
  }

  async getBestMoveAtDepth(
    depth: number,
    maxTimeMs: number = 0,
    aiVersion: 'v1' | 'v2' = 'v1',
  ): Promise<string> {
    return await ChessEngineModule.getBestMoveAtDepth(depth, maxTimeMs, aiVersion)
  }

  async searchBestMove(
    searchId: number,
    maxDepth: number,
    maxTimeMs: number = 0,
    aiVersion: 'v1' | 'v2' = 'v1',
  ): Promise<SearchResultData> {
    return await ChessEngineModule.searchBestMove(searchId, maxDepth, maxTimeMs, aiVersion)
  }

  addSearchProgressListener(listener: (progress: SearchProgressData) => void): { remove(): void } {
    return addSearchProgressListener(listener)
  }

  getMoveHistory(): string[] {
    return ChessEngineModule.getMoveHistory()
  }

  canUndo(): boolean {
    return ChessEngineModule.canUndo()
  }

  evaluatePosition(): number {
    return ChessEngineModule.evaluatePosition()
  }

  clearSearchCaches(): void {
    ChessEngineModule.clearSearchCaches()
  }
}

export default ChessEngine
export type { SearchProgressData, SearchResultData }
