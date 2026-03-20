#include "Board.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Chess {

namespace {

constexpr int WHITE_INDEX = 0;
constexpr int BLACK_INDEX = 1;
constexpr Square NO_SQUARE = -1;

inline int colorIndex(Color color) {
    return color == Color::WHITE ? WHITE_INDEX : BLACK_INDEX;
}

inline int pieceIndex(PieceType type) {
    return static_cast<int>(type);
}

inline char pieceToFenChar(const Piece& p) {
    char c = '?';
    switch (p.type) {
        case PieceType::PAWN:   c = 'p'; break;
        case PieceType::KNIGHT: c = 'n'; break;
        case PieceType::BISHOP: c = 'b'; break;
        case PieceType::ROOK:   c = 'r'; break;
        case PieceType::QUEEN:  c = 'q'; break;
        case PieceType::KING:   c = 'k'; break;
        default: break;
    }
    if (p.color == Color::WHITE) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return c;
}

inline uint64_t splitmix64(uint64_t& state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

struct ZobristTables {
    bool initialized = false;
    uint64_t pieceSquare[2][7][64] = {};
    uint64_t sideToMove = 0;
    uint64_t castling[16] = {};
    uint64_t enPassant[65] = {};
};

ZobristTables& zobrist() {
    static ZobristTables tables;
    if (!tables.initialized) {
        uint64_t seed = 0xC0FFEE1234567890ULL;

        for (int color = 0; color < 2; ++color) {
            for (int piece = 0; piece < 7; ++piece) {
                for (int sq = 0; sq < 64; ++sq) {
                    tables.pieceSquare[color][piece][sq] = splitmix64(seed);
                }
            }
        }

        tables.sideToMove = splitmix64(seed);

        for (int i = 0; i < 16; ++i) {
            tables.castling[i] = splitmix64(seed);
        }

        for (int i = 0; i < 65; ++i) {
            tables.enPassant[i] = splitmix64(seed);
        }

        tables.initialized = true;
    }

    return tables;
}

inline uint64_t pieceHash(const Piece& piece, Square sq) {
    if (sq < 0 || sq >= 64 || piece.isEmpty()) {
        return 0ULL;
    }

    const int pIdx = pieceIndex(piece.type);
    if (pIdx < 0 || pIdx >= 7) {
        return 0ULL;
    }

    return zobrist().pieceSquare[colorIndex(piece.color)][pIdx][sq];
}

inline bool isRookOrQueen(const Piece& p) {
    return p.type == PieceType::ROOK || p.type == PieceType::QUEEN;
}

inline bool isBishopOrQueen(const Piece& p) {
    return p.type == PieceType::BISHOP || p.type == PieceType::QUEEN;
}

bool isSquareAttackedOnBoard(const std::array<Piece, 64>& board, Square sq, Color attackerColor) {
    if (sq < 0 || sq >= 64) {
        return false;
    }

    const int row = sq / 8;
    const int col = sq % 8;

    // Pawn attacks
    if (attackerColor == Color::WHITE) {
        if (row > 0) {
            if (col > 0) {
                const Piece& p = board[(row - 1) * 8 + (col - 1)];
                if (p.type == PieceType::PAWN && p.color == attackerColor) {
                    return true;
                }
            }
            if (col < 7) {
                const Piece& p = board[(row - 1) * 8 + (col + 1)];
                if (p.type == PieceType::PAWN && p.color == attackerColor) {
                    return true;
                }
            }
        }
    } else {
        if (row < 7) {
            if (col > 0) {
                const Piece& p = board[(row + 1) * 8 + (col - 1)];
                if (p.type == PieceType::PAWN && p.color == attackerColor) {
                    return true;
                }
            }
            if (col < 7) {
                const Piece& p = board[(row + 1) * 8 + (col + 1)];
                if (p.type == PieceType::PAWN && p.color == attackerColor) {
                    return true;
                }
            }
        }
    }

    // Knight attacks
    static constexpr int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}
    };

    for (const auto& offset : knightOffsets) {
        const int r = row + offset[0];
        const int c = col + offset[1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = board[r * 8 + c];
            if (p.type == PieceType::KNIGHT && p.color == attackerColor) {
                return true;
            }
        }
    }

    // King attacks
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            const int r = row + dr;
            const int c = col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                const Piece& p = board[r * 8 + c];
                if (p.type == PieceType::KING && p.color == attackerColor) {
                    return true;
                }
            }
        }
    }

    // Bishop / queen diagonals
    static constexpr int bishopDirs[4][2] = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    for (const auto& dir : bishopDirs) {
        int r = row + dir[0];
        int c = col + dir[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = board[r * 8 + c];
            if (!p.isEmpty()) {
                if (p.color == attackerColor && isBishopOrQueen(p)) {
                    return true;
                }
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }

    // Rook / queen orthogonals
    static constexpr int rookDirs[4][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    for (const auto& dir : rookDirs) {
        int r = row + dir[0];
        int c = col + dir[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = board[r * 8 + c];
            if (!p.isEmpty()) {
                if (p.color == attackerColor && isRookOrQueen(p)) {
                    return true;
                }
                break;
            }
            r += dir[0];
            c += dir[1];
        }
    }

    return false;
}

} // namespace

Board::Board() {
    reset();
}

void Board::recomputePositionHash() {
    positionHash_ = 0ULL;

    for (Square sq = 0; sq < 64; ++sq) {
        positionHash_ ^= pieceHash(board_[sq], sq);
    }

    if (currentPlayer_ == Color::BLACK) {
        positionHash_ ^= zobrist().sideToMove;
    }

    positionHash_ ^= zobrist().castling[castlingRights_ & 0x0F];
    positionHash_ ^= zobrist().enPassant[(enPassantTarget_ >= 0 && enPassantTarget_ < 64)
        ? (enPassantTarget_ + 1)
        : 0];
}

void Board::reset() {
    board_.fill(Piece());

    // White back rank
    board_[0] = Piece(PieceType::ROOK, Color::WHITE);
    board_[1] = Piece(PieceType::KNIGHT, Color::WHITE);
    board_[2] = Piece(PieceType::BISHOP, Color::WHITE);
    board_[3] = Piece(PieceType::QUEEN, Color::WHITE);
    board_[4] = Piece(PieceType::KING, Color::WHITE);
    board_[5] = Piece(PieceType::BISHOP, Color::WHITE);
    board_[6] = Piece(PieceType::KNIGHT, Color::WHITE);
    board_[7] = Piece(PieceType::ROOK, Color::WHITE);

    for (int i = 8; i < 16; ++i) {
        board_[i] = Piece(PieceType::PAWN, Color::WHITE);
    }

    // Black back rank
    board_[56] = Piece(PieceType::ROOK, Color::BLACK);
    board_[57] = Piece(PieceType::KNIGHT, Color::BLACK);
    board_[58] = Piece(PieceType::BISHOP, Color::BLACK);
    board_[59] = Piece(PieceType::QUEEN, Color::BLACK);
    board_[60] = Piece(PieceType::KING, Color::BLACK);
    board_[61] = Piece(PieceType::BISHOP, Color::BLACK);
    board_[62] = Piece(PieceType::KNIGHT, Color::BLACK);
    board_[63] = Piece(PieceType::ROOK, Color::BLACK);

    for (int i = 48; i < 56; ++i) {
        board_[i] = Piece(PieceType::PAWN, Color::BLACK);
    }

    currentPlayer_ = Color::WHITE;
    castlingRights_ = CastlingRights::ALL_CASTLING;
    enPassantTarget_ = NO_SQUARE;
    halfmoveClock_ = 0;
    fullmoveNumber_ = 1;
    kingSquares_[WHITE_INDEX] = 4;
    kingSquares_[BLACK_INDEX] = 60;

    recomputePositionHash();

    moveHistory_.clear();
    positionHistory_.clear();
    positionCounts_.clear();
    recordCurrentPosition();
}

std::string Board::getPositionKey() const {
    std::ostringstream key;
    key << std::hex << positionHash_;
    return key.str();
}

void Board::recordCurrentPosition() {
    positionHistory_.push_back(positionHash_);
    positionCounts_[positionHash_]++;
}

void Board::removeCurrentPositionRecord() {
    if (positionHistory_.empty()) {
        return;
    }

    const uint64_t key = positionHistory_.back();
    auto it = positionCounts_.find(key);
    if (it != positionCounts_.end()) {
        if (--it->second <= 0) {
            positionCounts_.erase(it);
        }
    }

    positionHistory_.pop_back();
}

bool Board::isThreefoldRepetition() const {
    if (positionHistory_.empty()) {
        return false;
    }

    const uint64_t currentKey = positionHistory_.back();
    auto it = positionCounts_.find(currentKey);
    return it != positionCounts_.end() && it->second >= 3;
}

Piece Board::getPiece(Square sq) const {
    if (sq < 0 || sq >= 64) {
        return Piece();
    }
    return board_[sq];
}

Piece Board::getPiece(int row, int col) const {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return Piece();
    }
    return board_[row * 8 + col];
}

void Board::setPiece(Square sq, const Piece& piece) {
    if (sq >= 0 && sq < 64) {
        board_[sq] = piece;
    }
}

bool Board::makeMove(const Move& move) {
    if (!isLegalMove(move)) {
        return false;
    }
    makeMoveUnchecked(move);
    return true;
}

void Board::makeMoveUnchecked(const Move& move) {
    const Square from = move.getFrom();
    const Square to = move.getTo();
    const Piece movingPiece = board_[from];

    Square captureSquare = to;
    Piece capturedPiece = board_[to];
    if (move.isEnPassant()) {
        captureSquare = (from / 8) * 8 + (to % 8);
        capturedPiece = board_[captureSquare];
    }

    MoveRecord record;
    record.move = move;
    record.capturedPiece = capturedPiece;
    record.castlingRights = castlingRights_;
    record.enPassantTarget = enPassantTarget_;
    record.halfmoveClock = halfmoveClock_;
    record.previousPositionHash = positionHash_;
    record.previousKingSquares = kingSquares_;
    moveHistory_.push_back(record);

    // Remove old state keys
    positionHash_ ^= zobrist().castling[castlingRights_ & 0x0F];
    positionHash_ ^= zobrist().enPassant[(enPassantTarget_ >= 0 && enPassantTarget_ < 64)
        ? (enPassantTarget_ + 1)
        : 0];
    if (currentPlayer_ == Color::BLACK) {
        positionHash_ ^= zobrist().sideToMove;
    }

    // Halfmove clock
    if (movingPiece.type == PieceType::PAWN || move.isCapture()) {
        halfmoveClock_ = 0;
    } else {
        ++halfmoveClock_;
    }

    // Remove moving piece from source
    positionHash_ ^= pieceHash(movingPiece, from);
    board_[from] = Piece();

    // Remove captured piece
    if (!capturedPiece.isEmpty()) {
        positionHash_ ^= pieceHash(capturedPiece, captureSquare);
        board_[captureSquare] = Piece();
    }

    // Castling rook move
    if (move.isCastling()) {
        const int row = from / 8;
        if ((to % 8) == 6) {
            const Square rookFrom = row * 8 + 7;
            const Square rookTo = row * 8 + 5;
            const Piece rook = board_[rookFrom];
            positionHash_ ^= pieceHash(rook, rookFrom);
            board_[rookFrom] = Piece();
            board_[rookTo] = rook;
            positionHash_ ^= pieceHash(rook, rookTo);
        } else {
            const Square rookFrom = row * 8 + 0;
            const Square rookTo = row * 8 + 3;
            const Piece rook = board_[rookFrom];
            positionHash_ ^= pieceHash(rook, rookFrom);
            board_[rookFrom] = Piece();
            board_[rookTo] = rook;
            positionHash_ ^= pieceHash(rook, rookTo);
        }
    }

    Piece finalPiece = movingPiece;
    if (move.isPromotion()) {
        finalPiece = Piece(move.getPromotion(), movingPiece.color);
    }

    board_[to] = finalPiece;
    positionHash_ ^= pieceHash(finalPiece, to);

    if (movingPiece.type == PieceType::KING) {
        kingSquares_[colorIndex(movingPiece.color)] = to;
    }

    // Update en passant target
    enPassantTarget_ = NO_SQUARE;
    if (move.isDoublePawnPush()) {
        enPassantTarget_ = from + pawnDirection(movingPiece.color) * 8;
    }

    // Update castling rights for mover
    if (movingPiece.type == PieceType::KING) {
        if (movingPiece.color == Color::WHITE) {
            castlingRights_ &= ~CastlingRights::WHITE_CASTLING;
        } else {
            castlingRights_ &= ~CastlingRights::BLACK_CASTLING;
        }
    } else if (movingPiece.type == PieceType::ROOK) {
        if (from == 0) castlingRights_ &= ~CastlingRights::WHITE_QUEENSIDE;
        if (from == 7) castlingRights_ &= ~CastlingRights::WHITE_KINGSIDE;
        if (from == 56) castlingRights_ &= ~CastlingRights::BLACK_QUEENSIDE;
        if (from == 63) castlingRights_ &= ~CastlingRights::BLACK_KINGSIDE;
    }

    // Update castling rights for captured rook
    if (captureSquare == 0) castlingRights_ &= ~CastlingRights::WHITE_QUEENSIDE;
    if (captureSquare == 7) castlingRights_ &= ~CastlingRights::WHITE_KINGSIDE;
    if (captureSquare == 56) castlingRights_ &= ~CastlingRights::BLACK_QUEENSIDE;
    if (captureSquare == 63) castlingRights_ &= ~CastlingRights::BLACK_KINGSIDE;

    // Switch player / move number
    currentPlayer_ = oppositeColor(currentPlayer_);
    if (currentPlayer_ == Color::WHITE) {
        ++fullmoveNumber_;
    }

    // Add new state keys
    if (currentPlayer_ == Color::BLACK) {
        positionHash_ ^= zobrist().sideToMove;
    }
    positionHash_ ^= zobrist().castling[castlingRights_ & 0x0F];
    positionHash_ ^= zobrist().enPassant[(enPassantTarget_ >= 0 && enPassantTarget_ < 64)
        ? (enPassantTarget_ + 1)
        : 0];

    recordCurrentPosition();
}

void Board::unmakeMove() {
    unmakeMoveUnchecked();
}

void Board::unmakeMoveUnchecked() {
    if (moveHistory_.empty()) {
        return;
    }

    removeCurrentPositionRecord();

    const MoveRecord record = moveHistory_.back();
    moveHistory_.pop_back();

    const Move move = record.move;
    const Square from = move.getFrom();
    const Square to = move.getTo();

    currentPlayer_ = oppositeColor(currentPlayer_);
    if (currentPlayer_ == Color::BLACK) {
        --fullmoveNumber_;
    }

    castlingRights_ = record.castlingRights;
    enPassantTarget_ = record.enPassantTarget;
    halfmoveClock_ = record.halfmoveClock;
    positionHash_ = record.previousPositionHash;
    kingSquares_ = record.previousKingSquares;

    Piece movingPiece = board_[to];
    if (move.isPromotion()) {
        movingPiece = Piece(PieceType::PAWN, movingPiece.color);
    }

    board_[from] = movingPiece;

    if (move.isEnPassant()) {
        const Square captureSquare = (from / 8) * 8 + (to % 8);
        board_[to] = Piece();
        board_[captureSquare] = record.capturedPiece;
    } else {
        board_[to] = record.capturedPiece;
    }

    if (move.isCastling()) {
        const int row = from / 8;
        if ((to % 8) == 6) {
            const Square rookFrom = row * 8 + 7;
            const Square rookTo = row * 8 + 5;
            board_[rookFrom] = board_[rookTo];
            board_[rookTo] = Piece();
        } else {
            const Square rookFrom = row * 8 + 0;
            const Square rookTo = row * 8 + 3;
            board_[rookFrom] = board_[rookTo];
            board_[rookTo] = Piece();
        }
    }
}

Square Board::findKing(Color color) const {
    return kingSquares_[colorIndex(color)];
}

bool Board::isSquareAttacked(Square sq, Color attackerColor) const {
    return isSquareAttackedOnBoard(board_, sq, attackerColor);
}

bool Board::isInCheck(Color color) const {
    const Square kingSquare = kingSquares_[colorIndex(color)];
    return kingSquare >= 0 && isSquareAttacked(kingSquare, oppositeColor(color));
}

void Board::generateSlidingMoves(
    Square from,
    const int directions[][2],
    int directionCount,
    std::vector<Move>& moves
) const {
    const Piece piece = board_[from];
    const int startRow = from / 8;
    const int startCol = from % 8;

    for (int i = 0; i < directionCount; ++i) {
        int row = startRow + directions[i][0];
        int col = startCol + directions[i][1];

        while (row >= 0 && row < 8 && col >= 0 && col < 8) {
            const Square to = row * 8 + col;
            const Piece target = board_[to];

            if (target.isEmpty()) {
                moves.emplace_back(from, to);
            } else {
                if (target.color != piece.color) {
                    moves.emplace_back(from, to, MoveFlags::CAPTURE);
                }
                break;
            }

            row += directions[i][0];
            col += directions[i][1];
        }
    }
}

void Board::generatePawnMoves(Square from, std::vector<Move>& moves) const {
    const Piece pawn = board_[from];
    const int row = from / 8;
    const int col = from % 8;
    const int direction = pawnDirection(pawn.color);
    const int startRow = pawnStartRow(pawn.color);
    const int promoRow = pawnPromotionRow(pawn.color);

    const int forwardRow = row + direction;
    if (forwardRow >= 0 && forwardRow < 8) {
        const Square oneForward = forwardRow * 8 + col;
        if (board_[oneForward].isEmpty()) {
            if (forwardRow == promoRow) {
                moves.emplace_back(from, oneForward, MoveFlags::PROMOTION, PieceType::QUEEN);
                moves.emplace_back(from, oneForward, MoveFlags::PROMOTION, PieceType::ROOK);
                moves.emplace_back(from, oneForward, MoveFlags::PROMOTION, PieceType::BISHOP);
                moves.emplace_back(from, oneForward, MoveFlags::PROMOTION, PieceType::KNIGHT);
            } else {
                moves.emplace_back(from, oneForward);

                if (row == startRow) {
                    const Square twoForward = (row + 2 * direction) * 8 + col;
                    if (board_[twoForward].isEmpty()) {
                        moves.emplace_back(from, twoForward, MoveFlags::DOUBLE_PAWN_PUSH);
                    }
                }
            }
        }
    }

    const int captureRow = row + direction;
    if (captureRow >= 0 && captureRow < 8) {
        for (int dcol : {-1, 1}) {
            const int captureCol = col + dcol;
            if (captureCol < 0 || captureCol >= 8) {
                continue;
            }

            const Square to = captureRow * 8 + captureCol;
            const Piece target = board_[to];

            if (!target.isEmpty() && target.color != pawn.color) {
                if (captureRow == promoRow) {
                    moves.emplace_back(from, to, MoveFlags::CAPTURE | MoveFlags::PROMOTION, PieceType::QUEEN);
                    moves.emplace_back(from, to, MoveFlags::CAPTURE | MoveFlags::PROMOTION, PieceType::ROOK);
                    moves.emplace_back(from, to, MoveFlags::CAPTURE | MoveFlags::PROMOTION, PieceType::BISHOP);
                    moves.emplace_back(from, to, MoveFlags::CAPTURE | MoveFlags::PROMOTION, PieceType::KNIGHT);
                } else {
                    moves.emplace_back(from, to, MoveFlags::CAPTURE);
                }
            }

            if (to == enPassantTarget_) {
                moves.emplace_back(from, to, MoveFlags::CAPTURE | MoveFlags::EN_PASSANT);
            }
        }
    }
}

void Board::generateKnightMoves(Square from, std::vector<Move>& moves) const {
    static constexpr int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}
    };

    const Piece knight = board_[from];
    const int row = from / 8;
    const int col = from % 8;

    for (const auto& offset : knightOffsets) {
        const int r = row + offset[0];
        const int c = col + offset[1];
        if (r < 0 || r >= 8 || c < 0 || c >= 8) {
            continue;
        }

        const Square to = r * 8 + c;
        const Piece target = board_[to];
        if (target.isEmpty()) {
            moves.emplace_back(from, to);
        } else if (target.color != knight.color) {
            moves.emplace_back(from, to, MoveFlags::CAPTURE);
        }
    }
}

void Board::generateBishopMoves(Square from, std::vector<Move>& moves) const {
    static constexpr int dirs[4][2] = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    generateSlidingMoves(from, dirs, 4, moves);
}

void Board::generateRookMoves(Square from, std::vector<Move>& moves) const {
    static constexpr int dirs[4][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    generateSlidingMoves(from, dirs, 4, moves);
}

void Board::generateQueenMoves(Square from, std::vector<Move>& moves) const {
    static constexpr int dirs[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
        { 0, 1}, { 1, -1}, { 1, 0}, { 1, 1}
    };
    generateSlidingMoves(from, dirs, 8, moves);
}

void Board::generateKingMoves(Square from, std::vector<Move>& moves) const {
    const Piece king = board_[from];
    const int row = from / 8;
    const int col = from % 8;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }

            const int r = row + dr;
            const int c = col + dc;
            if (r < 0 || r >= 8 || c < 0 || c >= 8) {
                continue;
            }

            const Square to = r * 8 + c;
            const Piece target = board_[to];
            if (target.isEmpty()) {
                moves.emplace_back(from, to);
            } else if (target.color != king.color) {
                moves.emplace_back(from, to, MoveFlags::CAPTURE);
            }
        }
    }

    const uint8_t kingsideMask = (king.color == Color::WHITE)
        ? CastlingRights::WHITE_KINGSIDE
        : CastlingRights::BLACK_KINGSIDE;
    const uint8_t queensideMask = (king.color == Color::WHITE)
        ? CastlingRights::WHITE_QUEENSIDE
        : CastlingRights::BLACK_QUEENSIDE;
    const Color enemy = oppositeColor(king.color);

    if (castlingRights_ & kingsideMask) {
        const Square rookSq = row * 8 + 7;
        const Square fSq = row * 8 + 5;
        const Square gSq = row * 8 + 6;
        const Piece rook = board_[rookSq];

        if (!rook.isEmpty() &&
            rook.type == PieceType::ROOK &&
            rook.color == king.color &&
            board_[fSq].isEmpty() &&
            board_[gSq].isEmpty() &&
            !isSquareAttacked(from, enemy) &&
            !isSquareAttacked(fSq, enemy) &&
            !isSquareAttacked(gSq, enemy)) {
            moves.emplace_back(from, gSq, MoveFlags::CASTLING);
        }
    }

    if (castlingRights_ & queensideMask) {
        const Square rookSq = row * 8 + 0;
        const Square bSq = row * 8 + 1;
        const Square cSq = row * 8 + 2;
        const Square dSq = row * 8 + 3;
        const Piece rook = board_[rookSq];

        if (!rook.isEmpty() &&
            rook.type == PieceType::ROOK &&
            rook.color == king.color &&
            board_[bSq].isEmpty() &&
            board_[cSq].isEmpty() &&
            board_[dSq].isEmpty() &&
            !isSquareAttacked(from, enemy) &&
            !isSquareAttacked(dSq, enemy) &&
            !isSquareAttacked(cSq, enemy)) {
            moves.emplace_back(from, cSq, MoveFlags::CASTLING);
        }
    }
}

void Board::generatePieceMovesInto(Square from, Color color, std::vector<Move>& moves) const {
    if (from < 0 || from >= 64) {
        return;
    }

    const Piece piece = board_[from];
    if (piece.isEmpty() || piece.color != color) {
        return;
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
}

std::vector<Move> Board::generatePieceMoves(Square from) const {
    std::vector<Move> moves;
    moves.reserve(16);
    generatePieceMovesInto(from, currentPlayer_, moves);
    return moves;
}

std::vector<Move> Board::generatePseudoLegalMoves(Color color) const {
    std::vector<Move> moves;
    moves.reserve(64);

    for (Square sq = 0; sq < 64; ++sq) {
        generatePieceMovesInto(sq, color, moves);
    }

    return moves;
}

bool Board::wouldBeInCheck(const Move& move, Color color) const {
    const Square from = move.getFrom();
    const Square to = move.getTo();

    std::array<Piece, 64> temp = board_;
    const Piece movingPiece = temp[from];
    Square kingSquare = kingSquares_[colorIndex(color)];

    temp[from] = Piece();

    if (move.isEnPassant()) {
        const Square captureSquare = (from / 8) * 8 + (to % 8);
        temp[captureSquare] = Piece();
    }

    if (move.isCastling()) {
        const int row = from / 8;
        if ((to % 8) == 6) {
            temp[row * 8 + 5] = temp[row * 8 + 7];
            temp[row * 8 + 7] = Piece();
        } else {
            temp[row * 8 + 3] = temp[row * 8 + 0];
            temp[row * 8 + 0] = Piece();
        }
    }

    Piece placed = movingPiece;
    if (move.isPromotion()) {
        placed = Piece(move.getPromotion(), movingPiece.color);
    }
    temp[to] = placed;

    if (movingPiece.type == PieceType::KING) {
        kingSquare = to;
    }

    return isSquareAttackedOnBoard(temp, kingSquare, oppositeColor(color));
}

bool Board::isLegalMove(const Move& move) const {
    if (!move.isValid()) {
        return false;
    }

    const Square from = move.getFrom();
    const Piece piece = getPiece(from);
    if (piece.isEmpty() || piece.color != currentPlayer_) {
        return false;
    }

    std::vector<Move> moves;
    moves.reserve(16);
    generatePieceMovesInto(from, currentPlayer_, moves);

    const auto it = std::find(moves.begin(), moves.end(), move);
    if (it == moves.end()) {
        return false;
    }

    return !wouldBeInCheck(move, currentPlayer_);
}

std::vector<Move> Board::generateLegalMoves() const {
    std::vector<Move> legal;
    legal.reserve(64);

    for (Square sq = 0; sq < 64; ++sq) {
        const Piece piece = board_[sq];
        if (piece.isEmpty() || piece.color != currentPlayer_) {
            continue;
        }

        std::vector<Move> pieceMoves;
        pieceMoves.reserve(16);
        generatePieceMovesInto(sq, currentPlayer_, pieceMoves);

        for (const Move& move : pieceMoves) {
            if (!wouldBeInCheck(move, currentPlayer_)) {
                legal.push_back(move);
            }
        }
    }

    return legal;
}

std::vector<Move> Board::generateLegalMoves(Square from) const {
    std::vector<Move> pieceMoves;
    pieceMoves.reserve(16);
    generatePieceMovesInto(from, currentPlayer_, pieceMoves);

    std::vector<Move> legal;
    legal.reserve(pieceMoves.size());

    for (const Move& move : pieceMoves) {
        if (!wouldBeInCheck(move, currentPlayer_)) {
            legal.push_back(move);
        }
    }

    return legal;
}

bool Board::isCheckmate() const {
    if (!isInCheck(currentPlayer_)) {
        return false;
    }
    return generateLegalMoves().empty();
}

bool Board::isStalemate() const {
    if (isInCheck(currentPlayer_)) {
        return false;
    }
    return generateLegalMoves().empty();
}

bool Board::isDraw() const {
    if (isThreefoldRepetition()) {
        return true;
    }

    if (halfmoveClock_ >= 100) {
        return true;
    }

    if (isStalemate()) {
        return true;
    }

    int whitePieces = 0;
    int blackPieces = 0;
    bool hasWhitePawn = false;
    bool hasBlackPawn = false;
    bool hasWhiteRookOrQueen = false;
    bool hasBlackRookOrQueen = false;

    for (Square sq = 0; sq < 64; ++sq) {
        const Piece p = board_[sq];
        if (p.isEmpty()) {
            continue;
        }

        if (p.color == Color::WHITE) {
            ++whitePieces;
            if (p.type == PieceType::PAWN) hasWhitePawn = true;
            if (p.type == PieceType::ROOK || p.type == PieceType::QUEEN) hasWhiteRookOrQueen = true;
        } else {
            ++blackPieces;
            if (p.type == PieceType::PAWN) hasBlackPawn = true;
            if (p.type == PieceType::ROOK || p.type == PieceType::QUEEN) hasBlackRookOrQueen = true;
        }
    }

    if (whitePieces == 1 && blackPieces == 1) {
        return true;
    }

    if (whitePieces <= 2 && blackPieces == 1 && !hasWhitePawn && !hasWhiteRookOrQueen) {
        return true;
    }
    if (blackPieces <= 2 && whitePieces == 1 && !hasBlackPawn && !hasBlackRookOrQueen) {
        return true;
    }

    return false;
}

GameResult Board::getGameResult() const {
    if (isCheckmate()) {
        return currentPlayer_ == Color::WHITE
            ? GameResult::BLACK_WINS
            : GameResult::WHITE_WINS;
    }

    if (isStalemate()) return GameResult::DRAW_STALEMATE;
    if (isThreefoldRepetition()) return GameResult::DRAW_THREEFOLD_REPETITION;
    if (halfmoveClock_ >= 100) return GameResult::DRAW_FIFTY_MOVE;
    if (isDraw()) return GameResult::DRAW_INSUFFICIENT_MATERIAL;

    return GameResult::IN_PROGRESS;
}

std::string Board::toFEN() const {
    std::ostringstream fen;

    for (int row = 7; row >= 0; --row) {
        int emptyCount = 0;
        for (int col = 0; col < 8; ++col) {
            const Piece p = board_[row * 8 + col];
            if (p.isEmpty()) {
                ++emptyCount;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                fen << pieceToFenChar(p);
            }
        }
        if (emptyCount > 0) {
            fen << emptyCount;
        }
        if (row > 0) {
            fen << '/';
        }
    }

    fen << ' ' << (currentPlayer_ == Color::WHITE ? 'w' : 'b');
    fen << ' ';

    if (castlingRights_ == 0) {
        fen << '-';
    } else {
        if (castlingRights_ & CastlingRights::WHITE_KINGSIDE)  fen << 'K';
        if (castlingRights_ & CastlingRights::WHITE_QUEENSIDE) fen << 'Q';
        if (castlingRights_ & CastlingRights::BLACK_KINGSIDE)  fen << 'k';
        if (castlingRights_ & CastlingRights::BLACK_QUEENSIDE) fen << 'q';
    }

    fen << ' ';
    if (enPassantTarget_ >= 0) {
        fen << static_cast<char>('a' + (enPassantTarget_ % 8))
            << static_cast<char>('1' + (enPassantTarget_ / 8));
    } else {
        fen << '-';
    }

    fen << ' ' << halfmoveClock_ << ' ' << fullmoveNumber_;
    return fen.str();
}

bool Board::fromFEN(const std::string& fen) {
    board_.fill(Piece());
    kingSquares_[WHITE_INDEX] = NO_SQUARE;
    kingSquares_[BLACK_INDEX] = NO_SQUARE;

    std::istringstream ss(fen);
    std::string position;
    std::string activeColor;
    std::string castling;
    std::string enPassant;
    int halfmove = 0;
    int fullmove = 1;

    if (!(ss >> position >> activeColor >> castling >> enPassant >> halfmove >> fullmove)) {
        return false;
    }

    int row = 7;
    int col = 0;

    for (char raw : position) {
        if (raw == '/') {
            --row;
            col = 0;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(raw))) {
            col += raw - '0';
            continue;
        }

        const Color color = std::isupper(static_cast<unsigned char>(raw))
            ? Color::WHITE
            : Color::BLACK;
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(raw)));

        PieceType type = PieceType::NONE;
        switch (c) {
            case 'p': type = PieceType::PAWN; break;
            case 'n': type = PieceType::KNIGHT; break;
            case 'b': type = PieceType::BISHOP; break;
            case 'r': type = PieceType::ROOK; break;
            case 'q': type = PieceType::QUEEN; break;
            case 'k': type = PieceType::KING; break;
            default: return false;
        }

        const Square sq = row * 8 + col;
        board_[sq] = Piece(type, color);
        if (type == PieceType::KING) {
            kingSquares_[colorIndex(color)] = sq;
        }
        ++col;
    }

    currentPlayer_ = (activeColor == "w") ? Color::WHITE : Color::BLACK;

    castlingRights_ = 0;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': castlingRights_ |= CastlingRights::WHITE_KINGSIDE; break;
                case 'Q': castlingRights_ |= CastlingRights::WHITE_QUEENSIDE; break;
                case 'k': castlingRights_ |= CastlingRights::BLACK_KINGSIDE; break;
                case 'q': castlingRights_ |= CastlingRights::BLACK_QUEENSIDE; break;
                default: break;
            }
        }
    }

    if (enPassant == "-" || enPassant.size() < 2) {
        enPassantTarget_ = NO_SQUARE;
    } else {
        const int epCol = enPassant[0] - 'a';
        const int epRow = enPassant[1] - '1';
        enPassantTarget_ = epRow * 8 + epCol;
    }

    halfmoveClock_ = halfmove;
    fullmoveNumber_ = fullmove;

    if (kingSquares_[WHITE_INDEX] < 0 || kingSquares_[BLACK_INDEX] < 0) {
        return false;
    }

    recomputePositionHash();

    moveHistory_.clear();
    positionHistory_.clear();
    positionCounts_.clear();
    recordCurrentPosition();

    return true;
}

} // namespace Chess
