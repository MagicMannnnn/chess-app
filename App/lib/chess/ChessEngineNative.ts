import NativeChessEngine from '@/modules/chess-engine'
import type {
  SearchProgressData as NativeSearchProgressData,
  SearchResultData as NativeSearchResultData,
} from '@/modules/chess-engine/src/ChessEngineModule'

import type { SearchParams, SearchResult } from './SearchTypes'
import { Color, Piece, PieceType } from './types'

export class ChessEngine {
  private nativeEngine: NativeChessEngine | null = null
  private initialized = false
  private activeSearchId = 0

  constructor() {
    try {
      this.nativeEngine = new NativeChessEngine()
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

  private nextSearchId(): number {
    this.activeSearchId += 1
    return this.activeSearchId
  }

  private invalidateInFlightSearches(): void {
    this.activeSearchId += 1
  }

  private isSearchCurrent(searchId: number, signal?: AbortSignal): boolean {
    return this.activeSearchId === searchId && !signal?.aborted
  }

  private cancelledSearchResult(
    bestMove: string,
    score: number,
    depthCompleted: number,
    nodesSearched: number,
    totalTimeMs: number,
    timedOut = false,
  ): SearchResult {
    return {
      bestMove,
      score,
      depthCompleted,
      nodesSearched,
      timedOut,
      cancelled: true,
      totalTimeMs,
    }
  }

  newGame(): void {
    try {
      if (!this.ensureInitialized()) return
      this.invalidateInFlightSearches()
      this.nativeEngine!.clearSearchCaches()
      this.nativeEngine!.newGame()
    } catch (error) {
      console.error('ChessEngine: newGame failed:', error)
    }
  }

  getBoard(): (Piece | null)[] {
    try {
      if (!this.ensureInitialized()) return Array(64).fill(null)
      const nativeBoard = this.nativeEngine!.getBoard()
      if (!Array.isArray(nativeBoard)) return Array(64).fill(null)

      return nativeBoard.map((piece) => {
        if (!piece || piece === null || Object.keys(piece).length === 0) return null
        return {
          type: piece.type as PieceType,
          color: piece.color as Color,
        }
      })
    } catch (error) {
      console.error('ChessEngine: getBoard failed:', error)
      return Array(64).fill(null)
    }
  }

  getCurrentPlayer(): Color {
    try {
      if (!this.ensureInitialized()) return Color.WHITE
      const player = this.nativeEngine!.getCurrentPlayer()
      return player === 'white' ? Color.WHITE : Color.BLACK
    } catch (error) {
      console.error('ChessEngine: getCurrentPlayer failed:', error)
      return Color.WHITE
    }
  }

  makeMove(algebraic: string): boolean {
    try {
      if (!this.ensureInitialized()) return false
      const result = this.nativeEngine!.makeMove(algebraic)
      if (result) {
        this.invalidateInFlightSearches()
        this.nativeEngine!.clearSearchCaches()
      }
      return result
    } catch (error) {
      console.error('ChessEngine: makeMove failed:', error)
      return false
    }
  }

  undoMove(): void {
    try {
      if (!this.ensureInitialized()) return
      this.invalidateInFlightSearches()
      this.nativeEngine!.clearSearchCaches()
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
      const loaded = this.nativeEngine!.loadFromFEN(fen)
      if (loaded) {
        this.invalidateInFlightSearches()
        this.nativeEngine!.clearSearchCaches()
      }
      return loaded
    } catch (error) {
      console.error('ChessEngine: loadFromFEN failed:', error)
      return false
    }
  }

  async getBestMove(
    depth: number,
    maxTimeMs: number = 0,
    aiVersion: 'v1' | 'v2' = 'v1',
  ): Promise<string> {
    try {
      if (!this.ensureInitialized()) return ''
      return await this.nativeEngine!.getBestMove(depth, maxTimeMs, aiVersion)
    } catch (error) {
      console.error('ChessEngine: getBestMove failed:', error)
      return ''
    }
  }

  async getBestMoveAtDepth(
    depth: number,
    maxTimeMs: number = 0,
    aiVersion: 'v1' | 'v2' = 'v1',
  ): Promise<string> {
    try {
      if (!this.ensureInitialized()) return ''
      return await this.nativeEngine!.getBestMoveAtDepth(depth, maxTimeMs, aiVersion)
    } catch (error) {
      console.error('ChessEngine: getBestMoveAtDepth failed:', error)
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

  clearSearchCaches(): void {
    try {
      if (!this.ensureInitialized()) return
      this.nativeEngine!.clearSearchCaches()
    } catch (error) {
      console.error('ChessEngine: clearSearchCaches failed:', error)
    }
  }

  async searchBestMove(params: SearchParams): Promise<SearchResult> {
    try {
      if (!this.ensureInitialized()) {
        return {
          bestMove: '',
          score: 0,
          depthCompleted: 0,
          nodesSearched: 0,
          timedOut: false,
          cancelled: false,
          totalTimeMs: 0,
        }
      }

      const { maxDepth, maxTimeMs, aiVersion, onProgress, signal } = params
      const searchId = this.nextSearchId()
      const startTime = Date.now()
      let lastProgress: NativeSearchProgressData | null = null

      const subscription = this.nativeEngine!.addSearchProgressListener(
        (event: NativeSearchProgressData) => {
          if (event.searchId !== searchId) {
            return
          }
          if (!this.isSearchCurrent(searchId, signal)) {
            return
          }
          lastProgress = event
          onProgress?.({
            depth: event.depth,
            bestMove: event.bestMove,
            score: event.score,
            nodesSearched: event.nodesSearched,
            timeMs: event.timeMs,
          })
        },
      )

      const abortHandler = () => {
        this.invalidateInFlightSearches()
        this.nativeEngine?.clearSearchCaches()
      }
      signal?.addEventListener('abort', abortHandler, { once: true })

      try {
        const nativeResult = (await this.nativeEngine!.searchBestMove(
          searchId,
          maxDepth,
          maxTimeMs,
          aiVersion,
        )) as NativeSearchResultData

        const {
          bestMove: resultBestMove,
          score: resultScore,
          depthCompleted: resultDepthCompleted,
          nodesSearched: resultNodesSearched,
          timedOut: resultTimedOut,
          totalTimeMs: resultTotalTimeMs,
        } = nativeResult

        if (!this.isSearchCurrent(searchId, signal)) {
          // Type assertion needed due to TypeScript narrowing issue
          const progress = lastProgress as NativeSearchProgressData | null
          const progressBestMove = progress?.bestMove
          const progressScore = progress?.score
          const progressDepth = progress?.depth
          const progressNodes = progress?.nodesSearched

          return this.cancelledSearchResult(
            progressBestMove ?? resultBestMove ?? '',
            progressScore ?? resultScore ?? 0,
            progressDepth ?? resultDepthCompleted ?? 0,
            progressNodes ?? resultNodesSearched ?? 0,
            Date.now() - startTime,
            resultTimedOut ?? false,
          )
        }

        return {
          bestMove: resultBestMove,
          score: resultScore,
          depthCompleted: resultDepthCompleted,
          nodesSearched: resultNodesSearched,
          timedOut: resultTimedOut,
          cancelled: false,
          totalTimeMs: resultTotalTimeMs,
        }
      } finally {
        subscription.remove()
        signal?.removeEventListener('abort', abortHandler)
      }
    } catch (error) {
      console.error('ChessEngine: searchBestMove failed:', error)
      return {
        bestMove: '',
        score: 0,
        depthCompleted: 0,
        nodesSearched: 0,
        timedOut: false,
        cancelled: false,
        totalTimeMs: 0,
      }
    }
  }
}

export { Color, type Piece, PieceType } from './types'
