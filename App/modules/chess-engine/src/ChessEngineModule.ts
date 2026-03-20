import { EventEmitter, NativeModule, requireNativeModule } from 'expo-modules-core'

export interface PieceInfo {
  type: string
  color: string
}

export interface SearchProgressData {
  searchId: number
  depth: number
  bestMove: string
  score: number
  nodesSearched: number
  timeMs: number
}

export interface SearchResultData {
  searchId: number
  bestMove: string
  score: number
  depthCompleted: number
  nodesSearched: number
  timedOut: boolean
  cancelled?: boolean
  totalTimeMs: number
  progressHistory: SearchProgressData[]
}

export interface ChessEngineEventsMap {
  onSearchProgress: (event: SearchProgressData) => void
  [key: string]: (...args: any[]) => void
}

export interface ChessEngineModuleInterface extends NativeModule<ChessEngineEventsMap> {
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
    searchId: number,
    maxDepth: number,
    maxTimeMs?: number,
    aiVersion?: 'v1' | 'v2',
  ): Promise<SearchResultData>
  getMoveHistory(): string[]
  canUndo(): boolean
  evaluatePosition(): number
  clearSearchCaches(): void
}

const ChessEngineModule = requireNativeModule<ChessEngineModuleInterface>('ChessEngine')

export type SearchProgressListener = (event: SearchProgressData) => void

export function addSearchProgressListener(listener: SearchProgressListener): { remove(): void } {
  // Native modules extend EventEmitter at runtime, cast to access event methods
  const emitter = ChessEngineModule as unknown as InstanceType<
    typeof EventEmitter<ChessEngineEventsMap>
  >
  return emitter.addListener('onSearchProgress', listener)
}

export default ChessEngineModule
