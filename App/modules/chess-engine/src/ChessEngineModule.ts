import { requireNativeModule } from 'expo-modules-core'

export interface PieceInfo {
  type: string
  color: string
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
  getBestMove(depth: number): string
  getMoveHistory(): string[]
  canUndo(): boolean
}

// It loads the native module object from the JSI or falls back to
// the bridge module (from NativeModulesProxy) if the remote debugger is on.
export default requireNativeModule<ChessEngineModuleInterface>('ChessEngine')
