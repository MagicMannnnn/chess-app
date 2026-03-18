import { requireNativeModule } from 'expo-modules-core'

export interface PieceInfo {
  type: string
  color: string
}

export interface SearchProgressData {
  depth: number
  bestMove: string
  score: number
  nodesSearched: number
  timeMs: number
}

export interface SearchResultData {
  bestMove: string
  score: number
  depthCompleted: number
  nodesSearched: number
  timedOut: boolean
  totalTimeMs: number
  progressHistory: SearchProgressData[]
}

export interface ChessEngineModuleInterface {
  newGame(): void
  makeMove(move: string): boolean
  undoMove(): void
  getLegalMoves(): string[]
  getLegalMovesFrom(square: string): string[]
  isMoveLegal(move: string): boolean
  getCurrentPlayer(): string
  isInCheck(): boolean
  isCheckmate(): boolean
  isStalemate(): boolean
  isDraw(): boolean
  getBoard(): PieceInfo[]
  getFEN(): string
  loadFromFEN(fen: string): boolean
  getBestMove(depth: number, maxTimeMs?: number, aiVersion?: 'v1' | 'v2'): Promise<string>
  getBestMoveAtDepth(depth: number, maxTimeMs?: number, aiVersion?: 'v1' | 'v2'): Promise<string>
  searchBestMove(
    maxDepth: number,
    maxTimeMs?: number,
    aiVersion?: 'v1' | 'v2',
  ): Promise<SearchResultData>
  getMoveHistory(): string[]
  canUndo(): boolean
  evaluatePosition(): number
  clearSearchCaches(): void
}

// It loads the native module object from the JSI or falls back to
// the bridge module (from NativeModulesProxy) if the remote debugger is on.
export default requireNativeModule<ChessEngineModuleInterface>('ChessEngine')
