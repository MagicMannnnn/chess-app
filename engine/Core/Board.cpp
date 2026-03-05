#include "Board.h"
#include <sstream>
#include <algorithm>

namespace Chess {

Board::Board() {
    reset();
}

void Board::reset() {
    // Clear board
    for (auto& piece : board_) {
        piece = Piece();
    }
    
    // Set up white pieces
    board_[0] = Piece(PieceType::ROOK, Color::WHITE);
    board_[1] = Piece(PieceType::KNIGHT, Color::WHITE);
    board_[2] = Piece(PieceType::BISHOP, Color::WHITE);
    board_[3] = Piece(PieceType::QUEEN, Color::WHITE);
    board_[4] = Piece(PieceType::KING, Color::WHITE);
    board_[5] = Piece(PieceType::BISHOP, Color::WHITE);
    board_[6] = Piece(PieceType::KNIGHT, Color::WHITE);
    board_[7] = Piece(PieceType::ROOK, Color::WHITE);
    
    for (int i = 8; i < 16; i++) {
        board_[i] = Piece(PieceType::PAWN, Color::WHITE);
    }
    
    // Set up black pieces
    board_[56] = Piece(PieceType::ROOK, Color::BLACK);
    board_[57] = Piece(PieceType::KNIGHT, Color::BLACK);
    board_[58] = Piece(PieceType::BISHOP, Color::BLACK);
    board_[59] = Piece(PieceType::QUEEN, Color::BLACK);
    board_[60] = Piece(PieceType::KING, Color::BLACK);
    board_[61] = Piece(PieceType::BISHOP, Color::BLACK);
    board_[62] = Piece(PieceType::KNIGHT, Color::BLACK);
    board_[63] = Piece(PieceType::ROOK, Color::BLACK);
    
    for (int i = 48; i < 56; i++) {
        board_[i] = Piece(PieceType::PAWN, Color::BLACK);
    }
    
    currentPlayer_ = Color::WHITE;
    castlingRights_ = CastlingRights::ALL_CASTLING;
    enPassantTarget_ = -1;
    halfmoveClock_ = 0;
    fullmoveNumber_ = 1;
    moveHistory_.clear();
}

Piece Board::getPiece(Square sq) const {
    if (sq < 0 || sq >= 64) return Piece();
    return board_[sq];
}

Piece Board::getPiece(int row, int col) const {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return Piece();
    return board_[row * 8 + col];
}

void Board::setPiece(Square sq, const Piece& piece) {
    if (sq >= 0 && sq < 64) {
        board_[sq] = piece;
    }
}

bool Board::makeMove(const Move& move) {
    if (!isLegalMove(move)) return false;
    
    Square from = move.getFrom();
    Square to = move.getTo();
    Piece movingPiece = getPiece(from);
    Piece capturedPiece = getPiece(to);
    
    // Save state for unmake
    MoveRecord record;
    record.move = move;
    record.capturedPiece = capturedPiece;
    record.castlingRights = castlingRights_;
    record.enPassantTarget = enPassantTarget_;
    record.halfmoveClock = halfmoveClock_;
    moveHistory_.push_back(record);
    
    // Update halfmove clock
    if (movingPiece.type == PieceType::PAWN || move.isCapture()) {
        halfmoveClock_ = 0;
    } else {
        halfmoveClock_++;
    }
    
    // Handle en passant capture
    if (move.isEnPassant()) {
        int captureRow = Position(from).row;
        int captureCol = Position(to).col;
        setPiece(captureRow * 8 + captureCol, Piece());
    }
    
    // Handle castling
    if (move.isCastling()) {
        int row = Position(from).row;
        if (Position(to).col == 6) { // Kingside
            setPiece(row * 8 + 5, getPiece(row * 8 + 7));
            setPiece(row * 8 + 7, Piece());
        } else { // Queenside
            setPiece(row * 8 + 3, getPiece(row * 8 + 0));
            setPiece(row * 8 + 0, Piece());
        }
    }
    
    // Move the piece
    setPiece(to, movingPiece);
    setPiece(from, Piece());
    
    // Handle promotion
    if (move.isPromotion()) {
        setPiece(to, Piece(move.getPromotion(), movingPiece.color));
    }
    
    // Update en passant target
    enPassantTarget_ = -1;
    if (move.isDoublePawnPush()) {
        int direction = pawnDirection(movingPiece.color);
        enPassantTarget_ = from + direction * 8;
    }
    
    // Update castling rights
    if (movingPiece.type == PieceType::KING) {
        if (movingPiece.color == Color::WHITE) {
            castlingRights_ &= ~CastlingRights::WHITE_CASTLING;
        } else {
            castlingRights_ &= ~CastlingRights::BLACK_CASTLING;
        }
    }
    
    if (movingPiece.type == PieceType::ROOK) {
        if (from == 0) castlingRights_ &= ~CastlingRights::WHITE_QUEENSIDE;
        if (from == 7) castlingRights_ &= ~CastlingRights::WHITE_KINGSIDE;
        if (from == 56) castlingRights_ &= ~CastlingRights::BLACK_QUEENSIDE;
        if (from == 63) castlingRights_ &= ~CastlingRights::BLACK_KINGSIDE;
    }
    
    // If a rook is captured, remove castling rights
    if (to == 0) castlingRights_ &= ~CastlingRights::WHITE_QUEENSIDE;
    if (to == 7) castlingRights_ &= ~CastlingRights::WHITE_KINGSIDE;
    if (to == 56) castlingRights_ &= ~CastlingRights::BLACK_QUEENSIDE;
    if (to == 63) castlingRights_ &= ~CastlingRights::BLACK_KINGSIDE;
    
    // Switch player
    currentPlayer_ = oppositeColor(currentPlayer_);
    if (currentPlayer_ == Color::WHITE) {
        fullmoveNumber_++;
    }
    
    return true;
}

void Board::unmakeMove() {
    if (moveHistory_.empty()) return;
    
    MoveRecord record = moveHistory_.back();
    moveHistory_.pop_back();
    
    Move move = record.move;
    Square from = move.getFrom();
    Square to = move.getTo();
    
    // Restore player
    currentPlayer_ = oppositeColor(currentPlayer_);
    if (currentPlayer_ == Color::BLACK) {
        fullmoveNumber_--;
    }
    
    // Restore state
    castlingRights_ = record.castlingRights;
    enPassantTarget_ = record.enPassantTarget;
    halfmoveClock_ = record.halfmoveClock;
    
    // Move piece back
    Piece movingPiece = getPiece(to);
    if (move.isPromotion()) {
        movingPiece = Piece(PieceType::PAWN, movingPiece.color);
    }
    setPiece(from, movingPiece);
    setPiece(to, record.capturedPiece);
    
    // Handle en passant
    if (move.isEnPassant()) {
        int captureRow = Position(from).row;
        int captureCol = Position(to).col;
        setPiece(captureRow * 8 + captureCol, record.capturedPiece);
        setPiece(to, Piece());
    }
    
    // Handle castling
    if (move.isCastling()) {
        int row = Position(from).row;
        if (Position(to).col == 6) { // Kingside
            setPiece(row * 8 + 7, getPiece(row * 8 + 5));
            setPiece(row * 8 + 5, Piece());
        } else { // Queenside
            setPiece(row * 8 + 0, getPiece(row * 8 + 3));
            setPiece(row * 8 + 3, Piece());
        }
    }
}

Square Board::findKing(Color color) const {
    for (Square sq = 0; sq < 64; sq++) {
        Piece p = getPiece(sq);
        if (p.type == PieceType::KING && p.color == color) {
            return sq;
        }
    }
    return -1;
}

bool Board::isSquareAttacked(Square sq, Color attackerColor) const {
    Position target(sq);
    
    // Check for pawn attacks
    int pawnDir = pawnDirection(attackerColor);
    for (int dcol : {-1, 1}) {
        Position pawnPos(target.row - pawnDir, target.col + dcol);
        if (pawnPos.isValid()) {
            Piece p = getPiece(pawnPos.toSquare());
            if (p.type == PieceType::PAWN && p.color == attackerColor) {
                return true;
            }
        }
    }
    
    // Check for knight attacks
    const int knightMoves[][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto& [dr, dc] : knightMoves) {
        Position knightPos(target.row + dr, target.col + dc);
        if (knightPos.isValid()) {
            Piece p = getPiece(knightPos.toSquare());
            if (p.type == PieceType::KNIGHT && p.color == attackerColor) {
                return true;
            }
        }
    }
    
    // Check for king attacks
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            Position kingPos(target.row + dr, target.col + dc);
            if (kingPos.isValid()) {
                Piece p = getPiece(kingPos.toSquare());
                if (p.type == PieceType::KING && p.color == attackerColor) {
                    return true;
                }
            }
        }
    }
    
    // Check for sliding piece attacks (bishop, rook, queen)
    const std::vector<std::pair<int,int>> bishopDirs = {{-1,-1},{-1,1},{1,-1},{1,1}};
    const std::vector<std::pair<int,int>> rookDirs = {{-1,0},{1,0},{0,-1},{0,1}};
    
    for (auto& [dr, dc] : bishopDirs) {
        Position pos = target;
        while (true) {
            pos.row += dr;
            pos.col += dc;
            if (!pos.isValid()) break;
            
            Piece p = getPiece(pos.toSquare());
            if (!p.isEmpty()) {
                if (p.color == attackerColor && 
                    (p.type == PieceType::BISHOP || p.type == PieceType::QUEEN)) {
                    return true;
                }
                break;
            }
        }
    }
    
    for (auto& [dr, dc] : rookDirs) {
        Position pos = target;
        while (true) {
            pos.row += dr;
            pos.col += dc;
            if (!pos.isValid()) break;
            
            Piece p = getPiece(pos.toSquare());
            if (!p.isEmpty()) {
                if (p.color == attackerColor && 
                    (p.type == PieceType::ROOK || p.type == PieceType::QUEEN)) {
                    return true;
                }
                break;
            }
        }
    }
    
    return false;
}

bool Board::isInCheck(Color color) const {
    Square kingSquare = findKing(color);
    if (kingSquare < 0) return false;
    return isSquareAttacked(kingSquare, oppositeColor(color));
}

void Board::generateSlidingMoves(Square from, const std::vector<std::pair<int, int>>& directions,
                                std::vector<Move>& moves) const {
    Position startPos(from);
    Piece piece = getPiece(from);
    
    for (auto& [dr, dc] : directions) {
        Position pos = startPos;
        while (true) {
            pos.row += dr;
            pos.col += dc;
            if (!pos.isValid()) break;
            
            Square to = pos.toSquare();
            Piece target = getPiece(to);
            
            if (target.isEmpty()) {
                moves.push_back(Move(from, to));
            } else {
                if (target.color != piece.color) {
                    moves.push_back(Move(from, to, MoveFlags::CAPTURE));
                }
                break;
            }
        }
    }
}

void Board::generatePawnMoves(Square from, std::vector<Move>& moves) const {
    Position pos(from);
    Piece pawn = getPiece(from);
    int direction = pawnDirection(pawn.color);
    int startRow = pawnStartRow(pawn.color);
    int promoRow = pawnPromotionRow(pawn.color);
    
    // Forward move
    Position forward(pos.row + direction, pos.col);
    if (forward.isValid() && getPiece(forward.toSquare()).isEmpty()) {
        Square to = forward.toSquare();
        if (forward.row == promoRow) {
            // Promotion
            for (auto promoPiece : {PieceType::QUEEN, PieceType::ROOK, 
                                   PieceType::BISHOP, PieceType::KNIGHT}) {
                moves.push_back(Move(from, to, MoveFlags::PROMOTION, promoPiece));
            }
        } else {
            moves.push_back(Move(from, to));
            
            // Double push from starting position
            if (pos.row == startRow) {
                Position doublePush(pos.row + direction * 2, pos.col);
                if (getPiece(doublePush.toSquare()).isEmpty()) {
                    moves.push_back(Move(from, doublePush.toSquare(), 
                                       MoveFlags::DOUBLE_PAWN_PUSH));
                }
            }
        }
    }
    
    // Captures
    for (int dcol : {-1, 1}) {
        Position capture(pos.row + direction, pos.col + dcol);
        if (!capture.isValid()) continue;
        
        Square to = capture.toSquare();
        Piece target = getPiece(to);
        
        if (!target.isEmpty() && target.color != pawn.color) {
            if (capture.row == promoRow) {
                for (auto promoPiece : {PieceType::QUEEN, PieceType::ROOK,
                                       PieceType::BISHOP, PieceType::KNIGHT}) {
                    moves.push_back(Move(from, to, 
                                       MoveFlags::CAPTURE | MoveFlags::PROMOTION, 
                                       promoPiece));
                }
            } else {
                moves.push_back(Move(from, to, MoveFlags::CAPTURE));
            }
        }
        
        // En passant
        if (to == enPassantTarget_) {
            moves.push_back(Move(from, to, 
                               MoveFlags::CAPTURE | MoveFlags::EN_PASSANT));
        }
    }
}

void Board::generateKnightMoves(Square from, std::vector<Move>& moves) const {
    Position pos(from);
    Piece knight = getPiece(from);
    
    const int knightMoves[][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    
    for (auto& [dr, dc] : knightMoves) {
        Position newPos(pos.row + dr, pos.col + dc);
        if (!newPos.isValid()) continue;
        
        Square to = newPos.toSquare();
        Piece target = getPiece(to);
        
        if (target.isEmpty()) {
            moves.push_back(Move(from, to));
        } else if (target.color != knight.color) {
            moves.push_back(Move(from, to, MoveFlags::CAPTURE));
        }
    }
}

void Board::generateBishopMoves(Square from, std::vector<Move>& moves) const {
    const std::vector<std::pair<int,int>> directions = {{-1,-1},{-1,1},{1,-1},{1,1}};
    generateSlidingMoves(from, directions, moves);
}

void Board::generateRookMoves(Square from, std::vector<Move>& moves) const {
    const std::vector<std::pair<int,int>> directions = {{-1,0},{1,0},{0,-1},{0,1}};
    generateSlidingMoves(from, directions, moves);
}

void Board::generateQueenMoves(Square from, std::vector<Move>& moves) const {
    const std::vector<std::pair<int,int>> directions = {
        {-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}
    };
    generateSlidingMoves(from, directions, moves);
}

void Board::generateKingMoves(Square from, std::vector<Move>& moves) const {
    Position pos(from);
    Piece king = getPiece(from);
    
    // Normal king moves
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            
            Position newPos(pos.row + dr, pos.col + dc);
            if (!newPos.isValid()) continue;
            
            Square to = newPos.toSquare();
            Piece target = getPiece(to);
            
            if (target.isEmpty()) {
                moves.push_back(Move(from, to));
            } else if (target.color != king.color) {
                moves.push_back(Move(from, to, MoveFlags::CAPTURE));
            }
        }
    }
    
    // Castling
    int row = pos.row;
    uint8_t kingsideMask = (king.color == Color::WHITE) ? 
                           CastlingRights::WHITE_KINGSIDE : 
                           CastlingRights::BLACK_KINGSIDE;
    uint8_t queensideMask = (king.color == Color::WHITE) ? 
                            CastlingRights::WHITE_QUEENSIDE : 
                            CastlingRights::BLACK_QUEENSIDE;
    
    // Kingside castling
    if ((castlingRights_ & kingsideMask) && 
        getPiece(row * 8 + 5).isEmpty() && 
        getPiece(row * 8 + 6).isEmpty() &&
        !isSquareAttacked(row * 8 + 4, oppositeColor(king.color)) &&
        !isSquareAttacked(row * 8 + 5, oppositeColor(king.color)) &&
        !isSquareAttacked(row * 8 + 6, oppositeColor(king.color))) {
        moves.push_back(Move(from, row * 8 + 6, MoveFlags::CASTLING));
    }
    
    // Queenside castling
    if ((castlingRights_ & queensideMask) && 
        getPiece(row * 8 + 1).isEmpty() && 
        getPiece(row * 8 + 2).isEmpty() && 
        getPiece(row * 8 + 3).isEmpty() &&
        !isSquareAttacked(row * 8 + 4, oppositeColor(king.color)) &&
        !isSquareAttacked(row * 8 + 3, oppositeColor(king.color)) &&
        !isSquareAttacked(row * 8 + 2, oppositeColor(king.color))) {
        moves.push_back(Move(from, row * 8 + 2, MoveFlags::CASTLING));
    }
}

std::vector<Move> Board::generatePieceMoves(Square from) const {
    std::vector<Move> moves;
    Piece piece = getPiece(from);
    
    if (piece.isEmpty() || piece.color != currentPlayer_) {
        return moves;
    }
    
    switch (piece.type) {
        case PieceType::PAWN:   generatePawnMoves(from, moves); break;
        case PieceType::KNIGHT: generateKnightMoves(from, moves); break;
        case PieceType::BISHOP: generateBishopMoves(from, moves); break;
        case PieceType::ROOK:   generateRookMoves(from, moves); break;
        case PieceType::QUEEN:  generateQueenMoves(from, moves); break;
        case PieceType::KING:   generateKingMoves(from, moves); break;
        default: break;
    }
    
    return moves;
}

std::vector<Move> Board::generatePseudoLegalMoves(Color color) const {
    std::vector<Move> moves;
    
    for (Square sq = 0; sq < 64; sq++) {
        Piece piece = getPiece(sq);
        if (piece.color == color) {
            std::vector<Move> pieceMoves = generatePieceMoves(sq);
            moves.insert(moves.end(), pieceMoves.begin(), pieceMoves.end());
        }
    }
    
    return moves;
}

bool Board::wouldBeInCheck(const Move& move, Color color) const {
    Board tempBoard = *this;
    
    Square from = move.getFrom();
    Square to = move.getTo();
    Piece movingPiece = tempBoard.getPiece(from);
    
    // Make the move on temp board
    tempBoard.setPiece(to, movingPiece);
    tempBoard.setPiece(from, Piece());
    
    // Handle en passant
    if (move.isEnPassant()) {
        int captureRow = Position(from).row;
        int captureCol = Position(to).col;
        tempBoard.setPiece(captureRow * 8 + captureCol, Piece());
    }
    
    // Handle castling
    if (move.isCastling()) {
        int row = Position(from).row;
        if (Position(to).col == 6) { // Kingside
            tempBoard.setPiece(row * 8 + 5, tempBoard.getPiece(row * 8 + 7));
            tempBoard.setPiece(row * 8 + 7, Piece());
        } else { // Queenside
            tempBoard.setPiece(row * 8 + 3, tempBoard.getPiece(row * 8 + 0));
            tempBoard.setPiece(row * 8 + 0, Piece());
        }
    }
    
    return tempBoard.isInCheck(color);
}

bool Board::isLegalMove(const Move& move) const {
    if (!move.isValid()) return false;
    
    Square from = move.getFrom();
    Square to = move.getTo();
    Piece piece = getPiece(from);
    
    if (piece.isEmpty() || piece.color != currentPlayer_) {
        return false;
    }
    
    // Generate pseudo-legal moves for this piece
    std::vector<Move> moves = generatePieceMoves(from);
    
    // Check if move is in the list
    auto it = std::find(moves.begin(), moves.end(), move);
    if (it == moves.end()) return false;
    
    // Check if move would leave king in check
    return !wouldBeInCheck(move, currentPlayer_);
}

std::vector<Move> Board::generateLegalMoves() const {
    std::vector<Move> pseudoLegal = generatePseudoLegalMoves(currentPlayer_);
    std::vector<Move> legal;
    
    for (const Move& move : pseudoLegal) {
        if (!wouldBeInCheck(move, currentPlayer_)) {
            legal.push_back(move);
        }
    }
    
    return legal;
}

std::vector<Move> Board::generateLegalMoves(Square from) const {
    std::vector<Move> pieceMoves = generatePieceMoves(from);
    std::vector<Move> legal;
    
    for (const Move& move : pieceMoves) {
        if (!wouldBeInCheck(move, currentPlayer_)) {
            legal.push_back(move);
        }
    }
    
    return legal;
}

bool Board::isCheckmate() const {
    return isInCheck(currentPlayer_) && generateLegalMoves().empty();
}

bool Board::isStalemate() const {
    return !isInCheck(currentPlayer_) && generateLegalMoves().empty();
}

bool Board::isDraw() const {
    // Stalemate
    if (isStalemate()) return true;
    
    // Fifty-move rule
    if (halfmoveClock_ >= 100) return true;
    
    // Insufficient material (simplified check)
    int whitePieces = 0, blackPieces = 0;
    bool hasWhitePawn = false, hasBlackPawn = false;
    bool hasWhiteRookOrQueen = false, hasBlackRookOrQueen = false;
    
    for (Square sq = 0; sq < 64; sq++) {
        Piece p = getPiece(sq);
        if (p.isEmpty()) continue;
        
        if (p.color == Color::WHITE) {
            whitePieces++;
            if (p.type == PieceType::PAWN) hasWhitePawn = true;
            if (p.type == PieceType::ROOK || p.type == PieceType::QUEEN) 
                hasWhiteRookOrQueen = true;
        } else {
            blackPieces++;
            if (p.type == PieceType::PAWN) hasBlackPawn = true;
            if (p.type == PieceType::ROOK || p.type == PieceType::QUEEN) 
                hasBlackRookOrQueen = true;
        }
    }
    
    // King vs King
    if (whitePieces == 1 && blackPieces == 1) return true;
    
    // King + minor piece vs King
    if (whitePieces <= 2 && blackPieces == 1 && !hasWhitePawn && !hasWhiteRookOrQueen)
        return true;
    if (blackPieces <= 2 && whitePieces == 1 && !hasBlackPawn && !hasBlackRookOrQueen)
        return true;
    
    return false;
}

GameResult Board::getGameResult() const {
    if (isCheckmate()) {
        return currentPlayer_ == Color::WHITE ? 
               GameResult::BLACK_WINS : GameResult::WHITE_WINS;
    }
    
    if (isStalemate()) return GameResult::DRAW_STALEMATE;
    if (halfmoveClock_ >= 100) return GameResult::DRAW_FIFTY_MOVE;
    if (isDraw()) return GameResult::DRAW_INSUFFICIENT_MATERIAL;
    
    return GameResult::IN_PROGRESS;
}

std::string Board::toFEN() const {
    std::ostringstream fen;
    
    // Board position
    for (int row = 7; row >= 0; row--) {
        int emptyCount = 0;
        for (int col = 0; col < 8; col++) {
            Piece p = getPiece(row, col);
            if (p.isEmpty()) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                
                char c;
                switch (p.type) {
                    case PieceType::PAWN:   c = 'p'; break;
                    case PieceType::KNIGHT: c = 'n'; break;
                    case PieceType::BISHOP: c = 'b'; break;
                    case PieceType::ROOK:   c = 'r'; break;
                    case PieceType::QUEEN:  c = 'q'; break;
                    case PieceType::KING:   c = 'k'; break;
                    default: c = '?'; break;
                }
                
                if (p.color == Color::WHITE) {
                    c = std::toupper(c);
                }
                fen << c;
            }
        }
        if (emptyCount > 0) fen << emptyCount;
        if (row > 0) fen << '/';
    }
    
    // Active color
    fen << ' ' << (currentPlayer_ == Color::WHITE ? 'w' : 'b');
    
    // Castling rights
    fen << ' ';
    if (castlingRights_ == 0) {
        fen << '-';
    } else {
        if (castlingRights_ & CastlingRights::WHITE_KINGSIDE) fen << 'K';
        if (castlingRights_ & CastlingRights::WHITE_QUEENSIDE) fen << 'Q';
        if (castlingRights_ & CastlingRights::BLACK_KINGSIDE) fen << 'k';
        if (castlingRights_ & CastlingRights::BLACK_QUEENSIDE) fen << 'q';
    }
    
    // En passant target
    fen << ' ';
    if (enPassantTarget_ >= 0) {
        Position pos(enPassantTarget_);
        fen << static_cast<char>('a' + pos.col) << (pos.row + 1);
    } else {
        fen << '-';
    }
    
    // Halfmove clock and fullmove number
    fen << ' ' << halfmoveClock_ << ' ' << fullmoveNumber_;
    
    return fen.str();
}

bool Board::fromFEN(const std::string& fen) {
    // Reset board
    for (auto& piece : board_) {
        piece = Piece();
    }
    
    std::istringstream ss(fen);
    std::string position, activeColor, castling, enPassant;
    int halfmove, fullmove;
    
    ss >> position >> activeColor >> castling >> enPassant >> halfmove >> fullmove;
    
    // Parse position
    int row = 7, col = 0;
    for (char c : position) {
        if (c == '/') {
            row--;
            col = 0;
        } else if (std::isdigit(c)) {
            col += (c - '0');
        } else {
            Color color = std::isupper(c) ? Color::WHITE : Color::BLACK;
            c = std::tolower(c);
            
            PieceType type;
            switch (c) {
                case 'p': type = PieceType::PAWN; break;
                case 'n': type = PieceType::KNIGHT; break;
                case 'b': type = PieceType::BISHOP; break;
                case 'r': type = PieceType::ROOK; break;
                case 'q': type = PieceType::QUEEN; break;
                case 'k': type = PieceType::KING; break;
                default: return false;
            }
            
            setPiece(row * 8 + col, Piece(type, color));
            col++;
        }
    }
    
    // Parse active color
    currentPlayer_ = (activeColor == "w") ? Color::WHITE : Color::BLACK;
    
    // Parse castling rights
    castlingRights_ = 0;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castlingRights_ |= CastlingRights::WHITE_KINGSIDE; break;
                case 'Q': castlingRights_ |= CastlingRights::WHITE_QUEENSIDE; break;
                case 'k': castlingRights_ |= CastlingRights::BLACK_KINGSIDE; break;
                case 'q': castlingRights_ |= CastlingRights::BLACK_QUEENSIDE; break;
            }
        }
    }
    
    // Parse en passant
    if (enPassant == "-") {
        enPassantTarget_ = -1;
    } else {
        int col = enPassant[0] - 'a';
        int row = enPassant[1] - '1';
        enPassantTarget_ = row * 8 + col;
    }
    
    halfmoveClock_ = halfmove;
    fullmoveNumber_ = fullmove;
    moveHistory_.clear();
    
    return true;
}

} // namespace Chess
