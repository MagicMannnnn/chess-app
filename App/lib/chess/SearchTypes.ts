export interface SearchProgress {
  depth: number
  bestMove: string
  score: number
  nodesSearched: number
  timeMs: number
}

export interface SearchResult {
  bestMove: string
  score: number
  depthCompleted: number
  nodesSearched: number
  timedOut: boolean
  cancelled: boolean
  totalTimeMs: number
}

export interface SearchParams {
  maxDepth: number
  maxTimeMs: number
  aiVersion: 'v1' | 'v2'
  onProgress?: (progress: SearchProgress) => void
  signal?: AbortSignal
}

export interface NativeSearchProgress extends SearchProgress {
  searchId: number
}

export interface NativeSearchResult extends SearchResult {
  searchId: number
  progressHistory: NativeSearchProgress[]
}
