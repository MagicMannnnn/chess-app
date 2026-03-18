import NativeChessEngine from '@/modules/chess-engine'

import type { SearchParams, SearchProgress, SearchResult } from './SearchTypes'
import { Color, Piece, PieceType } from './types'

export class ChessEngine {
  private nativeEngine: NativeChessEngine | null = null
  private initialized: boolean = false
  private activeSearchId: number = 0

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

  private nextSearchId(): number {
    this.activeSearchId += 1
    return this.activeSearchId
  }

  private invalidateInFlightSearches(): void {
    // Any search started before this increment becomes stale and must be ignored.
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
  ): SearchResult {
    return {
      bestMove,
      score,
      depthCompleted,
      nodesSearched,
      timedOut: false,
      cancelled: true,
      totalTimeMs,
    }
  }

  newGame(): void {
    try {
      console.log('ChessEngine: newGame called')
      if (!this.ensureInitialized()) return
      this.invalidateInFlightSearches()
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
      if (result) {
        this.invalidateInFlightSearches()
      }
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
      this.invalidateInFlightSearches()
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

  /**
   * New unified search API - performs iterative deepening with real-time updates
   * Calls engine once per depth for UI responsiveness while maintaining transposition table efficiency
   */
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

      const { maxDepth, maxTimeMs, aiVersion: _aiVersion, onProgress, signal } = params
      const aiVersion: 'v1' | 'v2' = 'v2'
      const searchId = this.nextSearchId()
      const startTime = Date.now()
      const rootFen = this.nativeEngine!.getFEN()
      let totalNodesSearched = 0
      let lastBestMove = ''
      let lastScore = 0
      let depthCompleted = 0
      let timedOut = false

      console.log(`ChessEngine: searchBestMove start id=${searchId} fen=${rootFen}`)

      // Check if already cancelled
      if (!this.isSearchCurrent(searchId, signal)) {
        return this.cancelledSearchResult('', 0, 0, 0, 0)
      }

      // Start this top-level search from a fresh native cache state.
      // Do this once before iterative deepening so depth-to-depth reuse still works.
      this.nativeEngine!.clearSearchCaches()

      if (!this.isSearchCurrent(searchId, signal)) {
        return this.cancelledSearchResult('', 0, 0, 0, Date.now() - startTime)
      }

      // Iterative deepening: call engine once per depth and stream progress.
      // Every awaited native result is validated against the active search id,
      // so late completions from older searches are ignored safely.
      for (let depth = 1; depth <= maxDepth; depth++) {
        // Check cancellation/invalidation before issuing native work.
        if (!this.isSearchCurrent(searchId, signal)) {
          return this.cancelledSearchResult(
            lastBestMove,
            lastScore,
            depthCompleted,
            totalNodesSearched,
            Date.now() - startTime,
          )
        }

        // Check time budget
        const elapsed = Date.now() - startTime
        if (maxTimeMs > 0 && elapsed >= maxTimeMs) {
          console.log(`ChessEngine: Time budget exhausted before depth ${depth}`)
          timedOut = true
          break
        }

        const remainingTime = maxTimeMs > 0 ? Math.max(0, maxTimeMs - elapsed) : 0

        // Search at EXACTLY this depth (not 1 to depth)
        // Uses transposition table from previous depths for efficiency
        const move = await this.nativeEngine!.getBestMoveAtDepth(depth, remainingTime, aiVersion)

        // Critical stale-result guard: AbortController cannot stop already-running native work.
        // We must reject late results from older searches after every await.
        if (!this.isSearchCurrent(searchId, signal)) {
          return this.cancelledSearchResult(
            lastBestMove,
            lastScore,
            depthCompleted,
            totalNodesSearched,
            Date.now() - startTime,
          )
        }

        const currentFen = this.nativeEngine!.getFEN()
        if (currentFen !== rootFen) {
          console.log(
            `ChessEngine: stale search rejected id=${searchId} depth=${depth} ` +
              `rootFen=${rootFen} currentFen=${currentFen}`,
          )
          return this.cancelledSearchResult(
            lastBestMove,
            lastScore,
            depthCompleted,
            totalNodesSearched,
            Date.now() - startTime,
          )
        }

        if (!move) {
          console.log(`ChessEngine: No move found at depth ${depth}`)
          break
        }

        // Update best move
        lastBestMove = move
        depthCompleted = depth

        // Get evaluation for progress callback
        const score = this.nativeEngine!.evaluatePosition()

        if (!this.isSearchCurrent(searchId, signal)) {
          return this.cancelledSearchResult(
            lastBestMove,
            lastScore,
            depthCompleted,
            totalNodesSearched,
            Date.now() - startTime,
          )
        }

        lastScore = score

        // Estimate nodes (rough estimate since we don't get exact count per call)
        totalNodesSearched += Math.pow(35, depth) / 10 // Rough branching factor estimate

        // Report progress immediately after this depth completes
        if (onProgress) {
          const progress: SearchProgress = {
            depth,
            bestMove: move,
            score,
            nodesSearched: totalNodesSearched,
            timeMs: Date.now() - startTime,
          }
          console.log(
            `ChessEngine: Depth ${depth} complete - move: ${move}, ` +
              `score: ${score}, time: ${progress.timeMs}ms`,
          )
          onProgress(progress)
        }

        // Check time after depth completes
        const elapsedAfter = Date.now() - startTime
        if (maxTimeMs > 0 && elapsedAfter >= maxTimeMs) {
          console.log(`ChessEngine: Time budget exhausted after depth ${depth}`)
          timedOut = true
          break
        }
      }

      if (!this.isSearchCurrent(searchId, signal)) {
        return this.cancelledSearchResult(
          lastBestMove,
          lastScore,
          depthCompleted,
          totalNodesSearched,
          Date.now() - startTime,
        )
      }

      const finalFen = this.nativeEngine!.getFEN()
      if (finalFen !== rootFen) {
        console.log(
          `ChessEngine: final stale search rejected id=${searchId} ` +
            `rootFen=${rootFen} currentFen=${finalFen}`,
        )
        return this.cancelledSearchResult(
          lastBestMove,
          lastScore,
          depthCompleted,
          totalNodesSearched,
          Date.now() - startTime,
        )
      }

      return {
        bestMove: lastBestMove,
        score: lastScore,
        depthCompleted,
        nodesSearched: totalNodesSearched,
        timedOut,
        cancelled: false,
        totalTimeMs: Date.now() - startTime,
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
