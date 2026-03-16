/**
 * Search API Types
 *
 * Defines the interface for the iterative deepening search that
 * happens entirely inside the C++ engine.
 */

/**
 * Progress update received after each completed depth
 */
export interface SearchProgress {
  /** Current depth level that was just completed */
  depth: number
  /** Best move found at this depth */
  bestMove: string
  /** Evaluation score (in centipawns from current player's perspective) */
  score: number
  /** Number of nodes searched so far */
  nodesSearched: number
  /** Time elapsed in milliseconds */
  timeMs: number
}

/**
 * Final result returned when search completes or is cancelled
 */
export interface SearchResult {
  /** Best move found (from last completed depth) */
  bestMove: string
  /** Evaluation score for the best move */
  score: number
  /** Deepest fully completed depth */
  depthCompleted: number
  /** Total nodes searched across all depths */
  nodesSearched: number
  /** Whether search timed out before reaching maxDepth */
  timedOut: boolean
  /** Whether search was cancelled via AbortSignal */
  cancelled: boolean
  /** Total time taken in milliseconds */
  totalTimeMs: number
}

/**
 * Parameters for initiating a search
 */
export interface SearchParams {
  /** Maximum search depth (e.g., 5 means depth 1,2,3,4,5) */
  maxDepth: number
  /** Maximum time budget in milliseconds (0 = unlimited) */
  maxTimeMs: number
  /** AI version to use ('v1' or 'v2') */
  aiVersion: 'v1' | 'v2'
  /** Optional callback invoked after each completed depth */
  onProgress?: (progress: SearchProgress) => void
  /** Optional AbortSignal for cancellation */
  signal?: AbortSignal
}
