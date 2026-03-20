#include "Evaluation.h"
#include <algorithm>

namespace Chess {
namespace V2 {

const int EvaluatorV2::PIECE_VALUES[7] = {
    100,   // PAWN
    320,   // KNIGHT
    330,   // BISHOP
    500,   // ROOK
    900,   // QUEEN
    20000, // KING
    0      // NONE
};

namespace {

constexpr int PAWN_PST[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },
    {  2,  4,  6,  8,  8,  6,  4,  2 },
    {  2,  5,  8, 10, 10,  8,  5,  2 },
    {  1,  4,  6, 10, 10,  6,  4,  1 },
    {  0,  2,  4,  8,  8,  4,  2,  0 },
    {  0,  1,  2,  4,  4,  2,  1,  0 },
    {  0,  0,  0, -2, -2,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0 }
};

constexpr int KNIGHT_PST[8][8] = {
    { -16, -10,  -6,  -4,  -4,  -6, -10, -16 },
    { -10,  -2,   2,   4,   4,   2,  -2, -10 },
    {  -6,   4,   8,  10,  10,   8,   4,  -6 },
    {  -4,   6,  10,  12,  12,  10,   6,  -4 },
    {  -4,   6,  10,  12,  12,  10,   6,  -4 },
    {  -6,   4,   8,  10,  10,   8,   4,  -6 },
    { -10,  -2,   2,   4,   4,   2,  -2, -10 },
    { -16, -10,  -6,  -4,  -4,  -6, -10, -16 }
};

constexpr int BISHOP_PST[8][8] = {
    {  -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8 },
    {  -4,   2,   4,   5,   5,   4,   2,  -4 },
    {  -4,   4,   6,   8,   8,   6,   4,  -4 },
    {  -4,   5,   8,  10,  10,   8,   5,  -4 },
    {  -4,   5,   8,  10,  10,   8,   5,  -4 },
    {  -4,   4,   6,   8,   8,   6,   4,  -4 },
    {  -4,   2,   4,   5,   5,   4,   2,  -4 },
    {  -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8 }
};

constexpr int ROOK_PST[8][8] = {
    {  0,  1,  2,  3,  3,  2,  1,  0 },
    {  1,  2,  3,  4,  4,  3,  2,  1 },
    {  0,  1,  2,  3,  3,  2,  1,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 },
    {  1,  2,  3,  4,  4,  3,  2,  1 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

constexpr int QUEEN_PST[8][8] = {
    {  -4,  -2,  -1,   0,   0,  -1,  -2,  -4 },
    {  -2,   0,   1,   2,   2,   1,   0,  -2 },
    {  -1,   1,   2,   3,   3,   2,   1,  -1 },
    {   0,   2,   3,   4,   4,   3,   2,   0 },
    {   0,   2,   3,   4,   4,   3,   2,   0 },
    {  -1,   1,   2,   3,   3,   2,   1,  -1 },
    {  -2,   0,   1,   2,   2,   1,   0,  -2 },
    {  -4,  -2,  -1,   0,   0,  -1,  -2,  -4 }
};

constexpr int KING_MG_PST[8][8] = {
    {   6,   8,   4,   0,   0,   4,   8,   6 },
    {  -8,  -6,  -4,  -8,  -8,  -4,  -6,  -8 },
    { -10, -10, -10, -12, -12, -10, -10, -10 },
    { -12, -12, -12, -16, -16, -12, -12, -12 },
    { -14, -14, -14, -18, -18, -14, -14, -14 },
    { -16, -16, -16, -20, -20, -16, -16, -16 },
    { -18, -18, -18, -22, -22, -18, -18, -18 },
    { -20, -20, -20, -24, -24, -20, -20, -20 }
};

constexpr int KING_EG_PST[8][8] = {
    {  -8,  -4,  -2,   0,   0,  -2,  -4,  -8 },
    {  -4,   0,   2,   4,   4,   2,   0,  -4 },
    {  -2,   2,   6,   8,   8,   6,   2,  -2 },
    {   0,   4,   8,  10,  10,   8,   4,   0 },
    {   0,   4,   8,  10,  10,   8,   4,   0 },
    {  -2,   2,   6,   8,   8,   6,   2,  -2 },
    {  -4,   0,   2,   4,   4,   2,   0,  -4 },
    {  -8,  -4,  -2,   0,   0,  -2,  -4,  -8 }
};

inline Color enemyOf(Color color) {
    return Chess::oppositeColor(color);
}

inline int mirrorRow(Color color, int row) {
    return color == Color::WHITE ? row : (7 - row);
}

inline int pieceSquareValue(PieceType type, Color color, int row, int col, bool endgame) {
    const int rr = mirrorRow(color, row);
    switch (type) {
        case PieceType::PAWN:   return PAWN_PST[rr][col];
        case PieceType::KNIGHT: return KNIGHT_PST[rr][col];
        case PieceType::BISHOP: return BISHOP_PST[rr][col];
        case PieceType::ROOK:   return ROOK_PST[rr][col];
        case PieceType::QUEEN:  return QUEEN_PST[rr][col];
        case PieceType::KING:   return endgame ? KING_EG_PST[rr][col] : KING_MG_PST[rr][col];
        default:                return 0;
    }
}

int countNonKingMaterial(const Board& board, Color color) {
    int total = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.color == color && p.type != PieceType::KING) {
                total += EvaluatorV2::PIECE_VALUES[static_cast<int>(p.type)];
            }
        }
    }
    return total;
}

} // namespace

int EvaluatorV2::evaluate(
    const Board& board,
    Color aiColor,
    const CastlingRights& castling,
    bool whiteHasCastled,
    bool blackHasCastled,
    int moveCount
) {
    (void)castling;
    (void)whiteHasCastled;
    (void)blackHasCastled;
    (void)moveCount;

    const Color oppColor = enemyOf(aiColor);
    const bool endgame = isEndgame(board);

    const int aiMaterial = countMaterial(board, aiColor);
    const int oppMaterial = countMaterial(board, oppColor);
    const int materialDiff = aiMaterial - oppMaterial;

    int score = materialDiff * 8;
    if (materialDiff < 0) {
        score += materialDiff * 4;
    } else if (materialDiff > 0) {
        score += materialDiff;
    }

    int pstDiff = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty()) {
                continue;
            }
            const int pst = pieceSquareValue(p.type, p.color, r, c, endgame);
            if (p.color == aiColor) {
                pstDiff += pst;
            } else {
                pstDiff -= pst;
            }
        }
    }

    score += pstDiff;

    if (endgame) {
        score += evaluateEndgame(board, aiColor, castling);
    }

    return score;
}

int EvaluatorV2::countMaterial(const Board& board, Color color) {
    int material = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.color == color) {
                const int typeIdx = static_cast<int>(p.type);
                if (typeIdx >= 0 && typeIdx < 7) {
                    material += PIECE_VALUES[typeIdx];
                }
            }
        }
    }
    return material;
}

bool EvaluatorV2::isEndgame(const Board& board) {
    const int whiteMaterial = countNonKingMaterial(board, Color::WHITE);
    const int blackMaterial = countNonKingMaterial(board, Color::BLACK);
    return whiteMaterial <= 1400 && blackMaterial <= 1400;
}

Position EvaluatorV2::findKing(const Board& board, Color color) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.type == PieceType::KING && p.color == color) {
                return Position(r, c);
            }
        }
    }
    return Position(-1, -1);
}

int EvaluatorV2::evaluateEndgame(
    const Board& board,
    Color aiColor,
    const CastlingRights& castling
) {
    (void)castling;

    const Color oppColor = enemyOf(aiColor);
    const Position aiKing = findKing(board, aiColor);
    const Position oppKing = findKing(board, oppColor);
    if (!aiKing.isValid() || !oppKing.isValid()) {
        return 0;
    }

    const int aiMaterial = countMaterial(board, aiColor);
    const int oppMaterial = countMaterial(board, oppColor);
    if (aiMaterial <= oppMaterial) {
        return 0;
    }

    const int oppRow = static_cast<int>(oppKing.row);
    const int oppCol = static_cast<int>(oppKing.col);
    const int edgeDist = std::min(std::min(oppRow, 7 - oppRow), std::min(oppCol, 7 - oppCol));
    const int kingDist = std::abs(static_cast<int>(aiKing.row) - oppRow) + std::abs(static_cast<int>(aiKing.col) - oppCol);

    int score = 0;
    score += (3 - std::min(3, edgeDist)) * 8;
    score += (14 - kingDist) * 2;
    return score;
}

bool EvaluatorV2::isPieceOnStartingSquare(PieceType type, Color color, Position pos) {
    const int row = static_cast<int>(pos.row);
    const int col = static_cast<int>(pos.col);

    if (color == Color::WHITE) {
        switch (type) {
            case PieceType::KNIGHT: return row == 0 && (col == 1 || col == 6);
            case PieceType::BISHOP: return row == 0 && (col == 2 || col == 5);
            case PieceType::ROOK:   return row == 0 && (col == 0 || col == 7);
            case PieceType::QUEEN:  return row == 0 && col == 3;
            default:                return false;
        }
    }

    switch (type) {
        case PieceType::KNIGHT: return row == 7 && (col == 1 || col == 6);
        case PieceType::BISHOP: return row == 7 && (col == 2 || col == 5);
        case PieceType::ROOK:   return row == 7 && (col == 0 || col == 7);
        case PieceType::QUEEN:  return row == 7 && col == 3;
        default:                return false;
    }
}

int EvaluatorV2::countUndevelopedPieces(const Board& board, Color color) {
    int undeveloped = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }
            const Position pos(r, c);
            if ((p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) &&
                isPieceOnStartingSquare(p.type, color, pos)) {
                ++undeveloped;
            }
        }
    }
    return undeveloped;
}

bool EvaluatorV2::isRookDeveloped(const Board& board, Color color) {
    const int startRow = (color == Color::WHITE) ? 0 : 7;
    const Piece leftRook = board.getPiece(startRow, 0);
    const Piece rightRook = board.getPiece(startRow, 7);
    const bool leftMoved = leftRook.isEmpty() || leftRook.type != PieceType::ROOK || leftRook.color != color;
    const bool rightMoved = rightRook.isEmpty() || rightRook.type != PieceType::ROOK || rightRook.color != color;
    return leftMoved || rightMoved;
}

int EvaluatorV2::evaluatePieceDevelopment(const Board& board, Color color, int moveCount) {
    (void)board;
    (void)color;
    (void)moveCount;
    return 0;
}

int EvaluatorV2::evaluatePieceCoordination(const Board& board, Color color) {
    (void)board;
    (void)color;
    return 0;
}

int EvaluatorV2::evaluateMobility(const Board& board, Color color, const CastlingRights& castling) {
    (void)board;
    (void)color;
    (void)castling;
    return 0;
}

} // namespace V2
} // namespace Chess
