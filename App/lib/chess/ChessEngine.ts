import { Color, GameResult, GameState, Move, Piece, PieceType, Square } from './types'

export class ChessEngine {
  private state: GameState

  constructor() {
    this.state = this.getInitialState()
  }

  private getInitialState(): GameState {
    return {
      board: this.getStartingPosition(),
      currentPlayer: Color.WHITE,
      castlingRights: {
        whiteKingside: true,
        whiteQueenside: true,
        blackKingside: true,
        blackQueenside: true,
      },
      enPassantTarget: null,
      halfmoveClock: 0,
      fullmoveNumber: 1,
      moveHistory: [],
    }
  }

  private getStartingPosition(): (Piece | null)[] {
    const board: (Piece | null)[] = Array(64).fill(null)

    // White pieces
    board[0] = { type: PieceType.ROOK, color: Color.WHITE }
    board[1] = { type: PieceType.KNIGHT, color: Color.WHITE }
    board[2] = { type: PieceType.BISHOP, color: Color.WHITE }
    board[3] = { type: PieceType.QUEEN, color: Color.WHITE }
    board[4] = { type: PieceType.KING, color: Color.WHITE }
    board[5] = { type: PieceType.BISHOP, color: Color.WHITE }
    board[6] = { type: PieceType.KNIGHT, color: Color.WHITE }
    board[7] = { type: PieceType.ROOK, color: Color.WHITE }
    for (let i = 8; i < 16; i++) {
      board[i] = { type: PieceType.PAWN, color: Color.WHITE }
    }

    // Black pieces
    board[56] = { type: PieceType.ROOK, color: Color.BLACK }
    board[57] = { type: PieceType.KNIGHT, color: Color.BLACK }
    board[58] = { type: PieceType.BISHOP, color: Color.BLACK }
    board[59] = { type: PieceType.QUEEN, color: Color.BLACK }
    board[60] = { type: PieceType.KING, color: Color.BLACK }
    board[61] = { type: PieceType.BISHOP, color: Color.BLACK }
    board[62] = { type: PieceType.KNIGHT, color: Color.BLACK }
    board[63] = { type: PieceType.ROOK, color: Color.BLACK }
    for (let i = 48; i < 56; i++) {
      board[i] = { type: PieceType.PAWN, color: Color.BLACK }
    }

    return board
  }

  newGame(): void {
    this.state = this.getInitialState()
  }

  getPiece(square: Square): Piece | null {
    return this.state.board[square]
  }

  getPieceAt(row: number, col: number): Piece | null {
    return this.state.board[row * 8 + col]
  }

  getCurrentPlayer(): Color {
    return this.state.currentPlayer
  }

  getBoard(): (Piece | null)[] {
    return [...this.state.board]
  }

  // Convert algebraic notation (e2e4) to move
  private parseMove(algebraic: string): Move | null {
    if (algebraic.length < 4) return null

    const fromCol = algebraic.charCodeAt(0) - 'a'.charCodeAt(0)
    const fromRow = parseInt(algebraic[1]) - 1
    const toCol = algebraic.charCodeAt(2) - 'a'.charCodeAt(0)
    const toRow = parseInt(algebraic[3]) - 1

    if (
      fromCol < 0 ||
      fromCol > 7 ||
      fromRow < 0 ||
      fromRow > 7 ||
      toCol < 0 ||
      toCol > 7 ||
      toRow < 0 ||
      toRow > 7
    ) {
      return null
    }

    const from = fromRow * 8 + fromCol
    const to = toRow * 8 + toCol

    const move: Move = { from, to }

    // Check for promotion
    if (algebraic.length === 5) {
      const promoPiece = algebraic[4]
      switch (promoPiece) {
        case 'q':
          move.promotion = PieceType.QUEEN
          break
        case 'r':
          move.promotion = PieceType.ROOK
          break
        case 'b':
          move.promotion = PieceType.BISHOP
          break
        case 'n':
          move.promotion = PieceType.KNIGHT
          break
      }
    }

    return move
  }

  // Convert move to algebraic notation
  private moveToAlgebraic(move: Move): string {
    const fromRow = Math.floor(move.from / 8)
    const fromCol = move.from % 8
    const toRow = Math.floor(move.to / 8)
    const toCol = move.to % 8

    let result =
      String.fromCharCode('a'.charCodeAt(0) + fromCol) +
      (fromRow + 1) +
      String.fromCharCode('a'.charCodeAt(0) + toCol) +
      (toRow + 1)

    if (move.promotion) {
      const promoChar =
        move.promotion === PieceType.QUEEN
          ? 'q'
          : move.promotion === PieceType.ROOK
            ? 'r'
            : move.promotion === PieceType.BISHOP
              ? 'b'
              : 'n'
      result += promoChar
    }

    return result
  }

  makeMove(algebraic: string): boolean {
    const move = this.parseMove(algebraic)
    if (!move) return false

    const legalMoves = this.generateLegalMoves()
    const matchingMove = legalMoves.find(
      (m) => m.from === move.from && m.to === move.to && m.promotion === move.promotion,
    )

    if (!matchingMove) return false

    return this.executeMoveInternal(matchingMove)
  }

  private executeMoveInternal(move: Move): boolean {
    const piece = this.state.board[move.from]
    if (!piece || piece.color !== this.state.currentPlayer) return false

    // Save move to history
    this.state.moveHistory.push({ ...move })

    // Update halfmove clock
    if (piece.type === PieceType.PAWN || this.state.board[move.to]) {
      this.state.halfmoveClock = 0
    } else {
      this.state.halfmoveClock++
    }

    // Handle en passant capture
    if (move.isEnPassant) {
      const captureRow = Math.floor(move.from / 8)
      const captureCol = move.to % 8
      this.state.board[captureRow * 8 + captureCol] = null
    }

    // Handle castling
    if (move.isCastling) {
      const row = Math.floor(move.from / 8)
      const toCol = move.to % 8
      if (toCol === 6) {
        // Kingside
        this.state.board[row * 8 + 5] = this.state.board[row * 8 + 7]
        this.state.board[row * 8 + 7] = null
      } else {
        // Queenside
        this.state.board[row * 8 + 3] = this.state.board[row * 8 + 0]
        this.state.board[row * 8 + 0] = null
      }
    }

    // Move the piece
    this.state.board[move.to] = piece
    this.state.board[move.from] = null

    // Handle promotion
    if (move.promotion) {
      this.state.board[move.to] = { type: move.promotion, color: piece.color }
    }

    // Update en passant target
    this.state.enPassantTarget = null
    if (move.isDoublePawnPush) {
      const direction = piece.color === Color.WHITE ? 1 : -1
      this.state.enPassantTarget = move.from + direction * 8
    }

    // Update castling rights
    if (piece.type === PieceType.KING) {
      if (piece.color === Color.WHITE) {
        this.state.castlingRights.whiteKingside = false
        this.state.castlingRights.whiteQueenside = false
      } else {
        this.state.castlingRights.blackKingside = false
        this.state.castlingRights.blackQueenside = false
      }
    }

    if (piece.type === PieceType.ROOK) {
      if (move.from === 0) this.state.castlingRights.whiteQueenside = false
      if (move.from === 7) this.state.castlingRights.whiteKingside = false
      if (move.from === 56) this.state.castlingRights.blackQueenside = false
      if (move.from === 63) this.state.castlingRights.blackKingside = false
    }

    // If rook captured, remove castling rights
    if (move.to === 0) this.state.castlingRights.whiteQueenside = false
    if (move.to === 7) this.state.castlingRights.whiteKingside = false
    if (move.to === 56) this.state.castlingRights.blackQueenside = false
    if (move.to === 63) this.state.castlingRights.blackKingside = false

    // Switch player
    this.state.currentPlayer = this.state.currentPlayer === Color.WHITE ? Color.BLACK : Color.WHITE
    if (this.state.currentPlayer === Color.WHITE) {
      this.state.fullmoveNumber++
    }

    return true
  }

  undoMove(): void {
    // TODO: Implement full undo with state restoration
    if (this.state.moveHistory.length === 0) return
    this.state.moveHistory.pop()
    // For now, just reset to initial position if we need undo
    // Full implementation would require saving board states
  }

  getLegalMoves(): string[] {
    const moves = this.generateLegalMoves()
    return moves.map((m) => this.moveToAlgebraic(m))
  }

  getLegalMovesFrom(square: string): string[] {
    if (square.length !== 2) return []
    const col = square.charCodeAt(0) - 'a'.charCodeAt(0)
    const row = parseInt(square[1]) - 1
    if (col < 0 || col > 7 || row < 0 || row > 7) return []

    const from = row * 8 + col
    const moves = this.generateLegalMoves().filter((m) => m.from === from)
    return moves.map((m) => this.moveToAlgebraic(m))
  }

  private generateLegalMoves(): Move[] {
    const pseudoLegal = this.generatePseudoLegalMoves()
    return pseudoLegal.filter((move) => !this.wouldBeInCheck(move))
  }

  private generatePseudoLegalMoves(): Move[] {
    const moves: Move[] = []

    for (let sq = 0; sq < 64; sq++) {
      const piece = this.state.board[sq]
      if (piece && piece.color === this.state.currentPlayer) {
        moves.push(...this.generatePieceMoves(sq))
      }
    }

    return moves
  }

  private generatePieceMoves(from: Square): Move[] {
    const piece = this.state.board[from]
    if (!piece) return []

    switch (piece.type) {
      case PieceType.PAWN:
        return this.generatePawnMoves(from)
      case PieceType.KNIGHT:
        return this.generateKnightMoves(from)
      case PieceType.BISHOP:
        return this.generateBishopMoves(from)
      case PieceType.ROOK:
        return this.generateRookMoves(from)
      case PieceType.QUEEN:
        return this.generateQueenMoves(from)
      case PieceType.KING:
        return this.generateKingMoves(from)
      default:
        return []
    }
  }

  private generatePawnMoves(from: Square): Move[] {
    const moves: Move[] = []
    const piece = this.state.board[from]
    if (!piece) return moves

    const row = Math.floor(from / 8)
    const col = from % 8
    const direction = piece.color === Color.WHITE ? 1 : -1
    const startRow = piece.color === Color.WHITE ? 1 : 6
    const promoRow = piece.color === Color.WHITE ? 7 : 0

    // Forward move
    const forwardSq = from + direction * 8
    if (forwardSq >= 0 && forwardSq < 64 && !this.state.board[forwardSq]) {
      const forwardRow = Math.floor(forwardSq / 8)
      if (forwardRow === promoRow) {
        // Promotions
        for (const promo of [PieceType.QUEEN, PieceType.ROOK, PieceType.BISHOP, PieceType.KNIGHT]) {
          moves.push({ from, to: forwardSq, promotion: promo })
        }
      } else {
        moves.push({ from, to: forwardSq })

        // Double push
        if (row === startRow) {
          const doubleSq = from + direction * 16
          if (!this.state.board[doubleSq]) {
            moves.push({ from, to: doubleSq, isDoublePawnPush: true })
          }
        }
      }
    }

    // Captures
    for (const dcol of [-1, 1]) {
      const newCol = col + dcol
      if (newCol < 0 || newCol > 7) continue

      const captureSq = from + direction * 8 + dcol
      if (captureSq < 0 || captureSq >= 64) continue

      const captureRow = Math.floor(captureSq / 8)
      const target = this.state.board[captureSq]

      if (target && target.color !== piece.color) {
        if (captureRow === promoRow) {
          for (const promo of [
            PieceType.QUEEN,
            PieceType.ROOK,
            PieceType.BISHOP,
            PieceType.KNIGHT,
          ]) {
            moves.push({ from, to: captureSq, promotion: promo, isCapture: true })
          }
        } else {
          moves.push({ from, to: captureSq, isCapture: true })
        }
      }

      // En passant
      if (captureSq === this.state.enPassantTarget) {
        moves.push({ from, to: captureSq, isCapture: true, isEnPassant: true })
      }
    }

    return moves
  }

  private generateKnightMoves(from: Square): Move[] {
    const moves: Move[] = []
    const piece = this.state.board[from]
    if (!piece) return moves

    const row = Math.floor(from / 8)
    const col = from % 8

    const knightMoves = [
      [-2, -1],
      [-2, 1],
      [-1, -2],
      [-1, 2],
      [1, -2],
      [1, 2],
      [2, -1],
      [2, 1],
    ]

    for (const [dr, dc] of knightMoves) {
      const newRow = row + dr
      const newCol = col + dc

      if (newRow < 0 || newRow > 7 || newCol < 0 || newCol > 7) continue

      const to = newRow * 8 + newCol
      const target = this.state.board[to]

      if (!target) {
        moves.push({ from, to })
      } else if (target.color !== piece.color) {
        moves.push({ from, to, isCapture: true })
      }
    }

    return moves
  }

  private generateSlidingMoves(from: Square, directions: [number, number][]): Move[] {
    const moves: Move[] = []
    const piece = this.state.board[from]
    if (!piece) return moves

    const row = Math.floor(from / 8)
    const col = from % 8

    for (const [dr, dc] of directions) {
      let newRow = row + dr
      let newCol = col + dc

      while (newRow >= 0 && newRow <= 7 && newCol >= 0 && newCol <= 7) {
        const to = newRow * 8 + newCol
        const target = this.state.board[to]

        if (!target) {
          moves.push({ from, to })
        } else {
          if (target.color !== piece.color) {
            moves.push({ from, to, isCapture: true })
          }
          break
        }

        newRow += dr
        newCol += dc
      }
    }

    return moves
  }

  private generateBishopMoves(from: Square): Move[] {
    return this.generateSlidingMoves(from, [
      [-1, -1],
      [-1, 1],
      [1, -1],
      [1, 1],
    ])
  }

  private generateRookMoves(from: Square): Move[] {
    return this.generateSlidingMoves(from, [
      [-1, 0],
      [1, 0],
      [0, -1],
      [0, 1],
    ])
  }

  private generateQueenMoves(from: Square): Move[] {
    return this.generateSlidingMoves(from, [
      [-1, -1],
      [-1, 0],
      [-1, 1],
      [0, -1],
      [0, 1],
      [1, -1],
      [1, 0],
      [1, 1],
    ])
  }

  private generateKingMoves(from: Square): Move[] {
    const moves: Move[] = []
    const piece = this.state.board[from]
    if (!piece) return moves

    const row = Math.floor(from / 8)
    const col = from % 8

    // Normal king moves
    for (let dr = -1; dr <= 1; dr++) {
      for (let dc = -1; dc <= 1; dc++) {
        if (dr === 0 && dc === 0) continue

        const newRow = row + dr
        const newCol = col + dc

        if (newRow < 0 || newRow > 7 || newCol < 0 || newCol > 7) continue

        const to = newRow * 8 + newCol
        const target = this.state.board[to]

        if (!target) {
          moves.push({ from, to })
        } else if (target.color !== piece.color) {
          moves.push({ from, to, isCapture: true })
        }
      }
    }

    // Castling
    const canCastleKingside =
      piece.color === Color.WHITE
        ? this.state.castlingRights.whiteKingside
        : this.state.castlingRights.blackKingside
    const canCastleQueenside =
      piece.color === Color.WHITE
        ? this.state.castlingRights.whiteQueenside
        : this.state.castlingRights.blackQueenside

    if (canCastleKingside) {
      if (
        !this.state.board[row * 8 + 5] &&
        !this.state.board[row * 8 + 6] &&
        !this.isSquareAttacked(row * 8 + 4, piece.color) &&
        !this.isSquareAttacked(row * 8 + 5, piece.color) &&
        !this.isSquareAttacked(row * 8 + 6, piece.color)
      ) {
        moves.push({ from, to: row * 8 + 6, isCastling: true })
      }
    }

    if (canCastleQueenside) {
      if (
        !this.state.board[row * 8 + 1] &&
        !this.state.board[row * 8 + 2] &&
        !this.state.board[row * 8 + 3] &&
        !this.isSquareAttacked(row * 8 + 4, piece.color) &&
        !this.isSquareAttacked(row * 8 + 3, piece.color) &&
        !this.isSquareAttacked(row * 8 + 2, piece.color)
      ) {
        moves.push({ from, to: row * 8 + 2, isCastling: true })
      }
    }

    return moves
  }

  private isSquareAttacked(square: Square, defendingColor: Color): boolean {
    const attackingColor = defendingColor === Color.WHITE ? Color.BLACK : Color.WHITE
    const row = Math.floor(square / 8)
    const col = square % 8

    // Check for pawn attacks
    const pawnDir = attackingColor === Color.WHITE ? 1 : -1
    for (const dcol of [-1, 1]) {
      const pawnRow = row - pawnDir
      const pawnCol = col + dcol
      if (pawnRow >= 0 && pawnRow <= 7 && pawnCol >= 0 && pawnCol <= 7) {
        const pawnSq = pawnRow * 8 + pawnCol
        const piece = this.state.board[pawnSq]
        if (piece && piece.type === PieceType.PAWN && piece.color === attackingColor) {
          return true
        }
      }
    }

    // Check for knight attacks
    const knightMoves = [
      [-2, -1],
      [-2, 1],
      [-1, -2],
      [-1, 2],
      [1, -2],
      [1, 2],
      [2, -1],
      [2, 1],
    ]
    for (const [dr, dc] of knightMoves) {
      const newRow = row + dr
      const newCol = col + dc
      if (newRow >= 0 && newRow <= 7 && newCol >= 0 && newCol <= 7) {
        const sq = newRow * 8 + newCol
        const piece = this.state.board[sq]
        if (piece && piece.type === PieceType.KNIGHT && piece.color === attackingColor) {
          return true
        }
      }
    }

    // Check for king attacks
    for (let dr = -1; dr <= 1; dr++) {
      for (let dc = -1; dc <= 1; dc++) {
        if (dr === 0 && dc === 0) continue
        const newRow = row + dr
        const newCol = col + dc
        if (newRow >= 0 && newRow <= 7 && newCol >= 0 && newCol <= 7) {
          const sq = newRow * 8 + newCol
          const piece = this.state.board[sq]
          if (piece && piece.type === PieceType.KING && piece.color === attackingColor) {
            return true
          }
        }
      }
    }

    // Check for sliding piece attacks
    const bishopDirs: [number, number][] = [
      [-1, -1],
      [-1, 1],
      [1, -1],
      [1, 1],
    ]
    const rookDirs: [number, number][] = [
      [-1, 0],
      [1, 0],
      [0, -1],
      [0, 1],
    ]

    for (const [dr, dc] of bishopDirs) {
      let newRow = row + dr
      let newCol = col + dc
      while (newRow >= 0 && newRow <= 7 && newCol >= 0 && newCol <= 7) {
        const sq = newRow * 8 + newCol
        const piece = this.state.board[sq]
        if (piece) {
          if (
            piece.color === attackingColor &&
            (piece.type === PieceType.BISHOP || piece.type === PieceType.QUEEN)
          ) {
            return true
          }
          break
        }
        newRow += dr
        newCol += dc
      }
    }

    for (const [dr, dc] of rookDirs) {
      let newRow = row + dr
      let newCol = col + dc
      while (newRow >= 0 && newRow <= 7 && newCol >= 0 && newCol <= 7) {
        const sq = newRow * 8 + newCol
        const piece = this.state.board[sq]
        if (piece) {
          if (
            piece.color === attackingColor &&
            (piece.type === PieceType.ROOK || piece.type === PieceType.QUEEN)
          ) {
            return true
          }
          break
        }
        newRow += dr
        newCol += dc
      }
    }

    return false
  }

  private findKing(color: Color): Square {
    for (let sq = 0; sq < 64; sq++) {
      const piece = this.state.board[sq]
      if (piece && piece.type === PieceType.KING && piece.color === color) {
        return sq
      }
    }
    return -1
  }

  private wouldBeInCheck(move: Move): boolean {
    // Make temporary move
    const tempBoard = [...this.state.board]
    const piece = tempBoard[move.from]
    if (!piece) return false

    tempBoard[move.to] = piece
    tempBoard[move.from] = null

    // Handle en passant
    if (move.isEnPassant) {
      const captureRow = Math.floor(move.from / 8)
      const captureCol = move.to % 8
      tempBoard[captureRow * 8 + captureCol] = null
    }

    // Handle castling
    if (move.isCastling) {
      const row = Math.floor(move.from / 8)
      const toCol = move.to % 8
      if (toCol === 6) {
        tempBoard[row * 8 + 5] = tempBoard[row * 8 + 7]
        tempBoard[row * 8 + 7] = null
      } else {
        tempBoard[row * 8 + 3] = tempBoard[row * 8 + 0]
        tempBoard[row * 8 + 0] = null
      }
    }

    // Find king position
    let kingSquare = -1
    for (let sq = 0; sq < 64; sq++) {
      const p = tempBoard[sq]
      if (p && p.type === PieceType.KING && p.color === this.state.currentPlayer) {
        kingSquare = sq
        break
      }
    }

    if (kingSquare === -1) return true

    // Check if king is attacked
    const originalBoard = this.state.board
    this.state.board = tempBoard
    const inCheck = this.isSquareAttacked(kingSquare, this.state.currentPlayer)
    this.state.board = originalBoard

    return inCheck
  }

  isInCheck(): boolean {
    const kingSquare = this.findKing(this.state.currentPlayer)
    if (kingSquare === -1) return false
    return this.isSquareAttacked(kingSquare, this.state.currentPlayer)
  }

  isCheckmate(): boolean {
    return this.isInCheck() && this.generateLegalMoves().length === 0
  }

  isStalemate(): boolean {
    return !this.isInCheck() && this.generateLegalMoves().length === 0
  }

  getGameResult(): GameResult {
    if (this.isCheckmate()) {
      return this.state.currentPlayer === Color.WHITE
        ? GameResult.BLACK_WINS
        : GameResult.WHITE_WINS
    }
    if (this.isStalemate()) return GameResult.DRAW_STALEMATE
    if (this.state.halfmoveClock >= 100) return GameResult.DRAW_FIFTY_MOVE

    return GameResult.IN_PROGRESS
  }

  getFEN(): string {
    // Simplified FEN export
    let fen = ''

    for (let row = 7; row >= 0; row--) {
      let empty = 0
      for (let col = 0; col < 8; col++) {
        const piece = this.state.board[row * 8 + col]
        if (!piece) {
          empty++
        } else {
          if (empty > 0) {
            fen += empty
            empty = 0
          }
          const char = this.pieceToFEN(piece)
          fen += piece.color === Color.WHITE ? char.toUpperCase() : char.toLowerCase()
        }
      }
      if (empty > 0) fen += empty
      if (row > 0) fen += '/'
    }

    fen += ' ' + (this.state.currentPlayer === Color.WHITE ? 'w' : 'b')
    fen += ' '

    let castling = ''
    if (this.state.castlingRights.whiteKingside) castling += 'K'
    if (this.state.castlingRights.whiteQueenside) castling += 'Q'
    if (this.state.castlingRights.blackKingside) castling += 'k'
    if (this.state.castlingRights.blackQueenside) castling += 'q'
    fen += castling || '-'

    fen += ' '
    if (this.state.enPassantTarget !== null) {
      const col = this.state.enPassantTarget % 8
      const row = Math.floor(this.state.enPassantTarget / 8)
      fen += String.fromCharCode('a'.charCodeAt(0) + col) + (row + 1)
    } else {
      fen += '-'
    }

    fen += ' ' + this.state.halfmoveClock + ' ' + this.state.fullmoveNumber

    return fen
  }

  private pieceToFEN(piece: Piece): string {
    switch (piece.type) {
      case PieceType.PAWN:
        return 'p'
      case PieceType.KNIGHT:
        return 'n'
      case PieceType.BISHOP:
        return 'b'
      case PieceType.ROOK:
        return 'r'
      case PieceType.QUEEN:
        return 'q'
      case PieceType.KING:
        return 'k'
    }
  }
}
