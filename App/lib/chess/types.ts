// Chess piece types and colors
export enum PieceType {
  PAWN = 'pawn',
  KNIGHT = 'knight',
  BISHOP = 'bishop',
  ROOK = 'rook',
  QUEEN = 'queen',
  KING = 'king',
}

export enum Color {
  WHITE = 'white',
  BLACK = 'black',
}

export interface Piece {
  type: PieceType
  color: Color
}

export type Square = number // 0-63

export interface Move {
  from: Square
  to: Square
  promotion?: PieceType
  isCapture?: boolean
  isEnPassant?: boolean
  isCastling?: boolean
  isDoublePawnPush?: boolean
}

export interface GameState {
  board: (Piece | null)[]
  currentPlayer: Color
  castlingRights: {
    whiteKingside: boolean
    whiteQueenside: boolean
    blackKingside: boolean
    blackQueenside: boolean
  }
  enPassantTarget: Square | null
  halfmoveClock: number
  fullmoveNumber: number
  moveHistory: Move[]
}

export enum GameResult {
  IN_PROGRESS = 'in_progress',
  WHITE_WINS = 'white_wins',
  BLACK_WINS = 'black_wins',
  DRAW_STALEMATE = 'draw_stalemate',
  DRAW_INSUFFICIENT = 'draw_insufficient_material',
  DRAW_FIFTY_MOVE = 'draw_fifty_move',
}
