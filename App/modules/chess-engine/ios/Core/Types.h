#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

#include <cstdint>

namespace Chess {

// Piece types
enum class PieceType : uint8_t {
    NONE = 0,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

// Colors
enum class Color : uint8_t {
    WHITE = 0,
    BLACK = 1,
    NONE = 2
};

// Combined piece representation (type + color)
struct Piece {
    PieceType type;
    Color color;
    
    Piece() : type(PieceType::NONE), color(Color::NONE) {}
    Piece(PieceType t, Color c) : type(t), color(c) {}
    
    bool isEmpty() const { return type == PieceType::NONE; }
    bool isValid() const { return type != PieceType::NONE && color != Color::NONE; }
};

// Square representation (0-63, row-major: a1=0, h1=7, a8=56, h8=63)
using Square = int8_t;

// Position utilities
struct Position {
    int8_t row;
    int8_t col;
    
    Position() : row(0), col(0) {}
    Position(int8_t r, int8_t c) : row(r), col(c) {}
    Position(Square sq) : row(sq / 8), col(sq % 8) {}
    
    Square toSquare() const { return row * 8 + col; }
    bool isValid() const { return row >= 0 && row < 8 && col >= 0 && col < 8; }
};

// Castling rights flags
enum CastlingRights : uint8_t {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1 << 0,
    WHITE_QUEENSIDE = 1 << 1,
    BLACK_KINGSIDE = 1 << 2,
    BLACK_QUEENSIDE = 1 << 3,
    WHITE_CASTLING = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_CASTLING = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL_CASTLING = WHITE_CASTLING | BLACK_CASTLING
};

// Move flags
enum MoveFlags : uint8_t {
    NORMAL = 0,
    CAPTURE = 1 << 0,
    DOUBLE_PAWN_PUSH = 1 << 1,
    EN_PASSANT = 1 << 2,
    CASTLING = 1 << 3,
    PROMOTION = 1 << 4
};

// Game result
enum class GameResult {
    IN_PROGRESS,
    WHITE_WINS,
    BLACK_WINS,
    DRAW_STALEMATE,
    DRAW_INSUFFICIENT_MATERIAL,
    DRAW_FIFTY_MOVE,
    DRAW_THREEFOLD_REPETITION
};

// Helper functions
inline Color oppositeColor(Color c) {
    return c == Color::WHITE ? Color::BLACK : Color::WHITE;
}

inline int8_t pawnDirection(Color c) {
    return c == Color::WHITE ? 1 : -1;
}

inline int8_t pawnStartRow(Color c) {
    return c == Color::WHITE ? 1 : 6;
}

inline int8_t pawnPromotionRow(Color c) {
    return c == Color::WHITE ? 7 : 0;
}

} // namespace Chess

#endif // CHESS_TYPES_H
