#include "Evaluation.h"
#include <algorithm>
#include <cmath>

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

int countNonKingPieces(const Board& board, Color color) {
    int total = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.color == color && p.type != PieceType::KING) {
                ++total;
            }
        }
    }
    return total;
}

bool isPassedPawnSimple(const Board& board, Color color, int row, int col) {
    const Color enemy = enemyOf(color);

    if (color == Color::WHITE) {
        for (int r = row + 1; r < 8; ++r) {
            for (int c = std::max(0, col - 1); c <= std::min(7, col + 1); ++c) {
                Piece p = board.getPiece(r, c);
                if (!p.isEmpty() && p.color == enemy && p.type == PieceType::PAWN) {
                    return false;
                }
            }
        }
    } else {
        for (int r = row - 1; r >= 0; --r) {
            for (int c = std::max(0, col - 1); c <= std::min(7, col + 1); ++c) {
                Piece p = board.getPiece(r, c);
                if (!p.isEmpty() && p.color == enemy && p.type == PieceType::PAWN) {
                    return false;
                }
            }
        }
    }

    return true;
}

int pawnAdvanceScore(Color color, int row) {
    return color == Color::WHITE ? row : (7 - row);
}

int manhattanDistance(const Position& a, const Position& b) {
    return std::abs(static_cast<int>(a.row) - static_cast<int>(b.row)) +
           std::abs(static_cast<int>(a.col) - static_cast<int>(b.col));
}

int chebyshevDistance(const Position& a, const Position& b) {
    return std::max(
        std::abs(static_cast<int>(a.row) - static_cast<int>(b.row)),
        std::abs(static_cast<int>(a.col) - static_cast<int>(b.col))
    );
}

int kingEdgePressure(const Position& king) {
    const int row = static_cast<int>(king.row);
    const int col = static_cast<int>(king.col);

    const int distTop = row;
    const int distBottom = 7 - row;
    const int distLeft = col;
    const int distRight = 7 - col;

    const int minEdgeDist = std::min(std::min(distTop, distBottom), std::min(distLeft, distRight));
    return 3 - std::min(3, minEdgeDist);
}

int kingCornerPressure(const Position& king) {
    const int row = static_cast<int>(king.row);
    const int col = static_cast<int>(king.col);

    const int d1 = row + col;
    const int d2 = row + (7 - col);
    const int d3 = (7 - row) + col;
    const int d4 = (7 - row) + (7 - col);

    const int minCornerDist = std::min(std::min(d1, d2), std::min(d3, d4));
    return 7 - std::min(7, minCornerDist);
}

int evaluateSparseEndgame(const Board& board, Color aiColor) {
    const Color oppColor = enemyOf(aiColor);

    if (board.isCheckmate()) {
        return board.getCurrentPlayer() == oppColor ? 30000 : -30000;
    }

    const Position aiKing = EvaluatorV2::findKing(board, aiColor);
    const Position oppKing = EvaluatorV2::findKing(board, oppColor);

    if (!aiKing.isValid() || !oppKing.isValid()) {
        return 0;
    }

    const int aiMaterial = EvaluatorV2::countMaterial(board, aiColor);
    const int oppMaterial = EvaluatorV2::countMaterial(board, oppColor);
    const int materialDiff = aiMaterial - oppMaterial;

    int score = materialDiff * 10;

    // Strong conversion incentives only when ahead
    if (materialDiff > 0) {
        const int kingDist = chebyshevDistance(aiKing, oppKing);
        score += (7 - std::min(7, kingDist)) * 30;

        score += kingEdgePressure(oppKing) * 40;
        score += kingCornerPressure(oppKing) * 18;

        // Extra reward when your king is close AND their king is boxed in
        score += kingEdgePressure(oppKing) * (7 - std::min(7, kingDist)) * 4;
    }

    // Passed/advanced pawns still matter a lot
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.type != PieceType::PAWN) {
                continue;
            }

            const int advance = pawnAdvanceScore(p.color, r);
            int pawnScore = advance * 12;

            if (isPassedPawnSimple(board, p.color, r, c)) {
                pawnScore += 30 + advance * 14;
            }

            if (p.color == aiColor) {
                score += pawnScore;
            } else {
                score -= pawnScore;
            }
        }
    }

    // Keep check bonus small so it does not farm endless checks
    if (materialDiff > 0 && board.isInCheck(oppColor)) {
        score += 12;
    }
    if (board.isInCheck(aiColor)) {
        score -= 40;
    }

    return score;
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

    const int oppNonKingPieces = countNonKingPieces(board, oppColor);
    if (oppNonKingPieces < 4) {
        return evaluateSparseEndgame(board, aiColor);
    }

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
    const int materialDiff = aiMaterial - oppMaterial;

    int score = 0;

    if (materialDiff > 0) {
        const int kingDist = chebyshevDistance(aiKing, oppKing);
        score += (7 - std::min(7, kingDist)) * 14;
        score += kingEdgePressure(oppKing) * 18;
        score += kingCornerPressure(oppKing) * 8;
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.type != PieceType::PAWN) {
                continue;
            }

            const int advance = pawnAdvanceScore(p.color, r);
            int pawnScore = advance * 8;

            if (isPassedPawnSimple(board, p.color, r, c)) {
                pawnScore += 20 + advance * 10;
            }

            if (p.color == aiColor) {
                score += pawnScore;
            } else {
                score -= pawnScore;
            }
        }
    }

    if (materialDiff > 0 && board.isInCheck(oppColor)) {
        score += 8;
    }
    if (board.isInCheck(aiColor)) {
        score -= 30;
    }

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