#include "Evaluation.h"
#include <cmath>
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

// -----------------------------------------------------------------------------
// Lightweight PSTs
// White perspective; black uses mirrored rows.
// -----------------------------------------------------------------------------

constexpr int PAWN_PST[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },
    {  2,  4,  8, 12, 12,  8,  4,  2 },
    {  2,  8, 14, 20, 20, 14,  8,  2 },
    {  1,  7, 12, 18, 18, 12,  7,  1 },
    {  0,  5, 10, 16, 16, 10,  5,  0 },
    {  0,  2,  4,  6,  6,  4,  2,  0 },
    {  0,  0,  0, -6, -6,  0,  0,  0 },
    {  0,  0,  0,  0,  0,  0,  0,  0 }
};

constexpr int KNIGHT_PST[8][8] = {
    { -38, -24, -14, -12, -12, -14, -24, -38 },
    { -24,  -8,   2,   4,   4,   2,  -8, -24 },
    { -14,   4,  12,  16,  16,  12,   4, -14 },
    { -12,   6,  16,  22,  22,  16,   6, -12 },
    { -12,   6,  16,  22,  22,  16,   6, -12 },
    { -14,   4,  12,  16,  16,  12,   4, -14 },
    { -24,  -8,   2,   4,   4,   2,  -8, -24 },
    { -38, -24, -14, -12, -12, -14, -24, -38 }
};

constexpr int BISHOP_PST[8][8] = {
    { -12,  -8,  -8,  -8,  -8,  -8,  -8, -12 },
    {  -6,   2,   4,   6,   6,   4,   2,  -6 },
    {  -4,   6,  10,  12,  12,  10,   6,  -4 },
    {  -4,   8,  12,  16,  16,  12,   8,  -4 },
    {  -4,   8,  12,  16,  16,  12,   8,  -4 },
    {  -4,   6,  10,  12,  12,  10,   6,  -4 },
    {  -6,   2,   4,   6,   6,   4,   2,  -6 },
    { -12,  -8,  -8,  -8,  -8,  -8,  -8, -12 }
};

constexpr int ROOK_PST[8][8] = {
    {   0,   2,   3,   4,   4,   3,   2,   0 },
    {   2,   4,   6,   8,   8,   6,   4,   2 },
    {   0,   1,   2,   4,   4,   2,   1,   0 },
    {   0,   1,   2,   4,   4,   2,   1,   0 },
    {   0,   1,   2,   4,   4,   2,   1,   0 },
    {   0,   1,   2,   4,   4,   2,   1,   0 },
    {   4,   6,   8,  12,  12,   8,   6,   4 },
    {   0,   2,   3,   4,   4,   3,   2,   0 }
};

constexpr int QUEEN_PST[8][8] = {
    {  -8,  -4,  -2,   0,   0,  -2,  -4,  -8 },
    {  -4,   0,   2,   4,   4,   2,   0,  -4 },
    {  -2,   2,   4,   6,   6,   4,   2,  -2 },
    {   0,   4,   6,   8,   8,   6,   4,   0 },
    {   0,   4,   6,   8,   8,   6,   4,   0 },
    {  -2,   2,   4,   6,   6,   4,   2,  -2 },
    {  -4,   0,   2,   4,   4,   2,   0,  -4 },
    {  -8,  -4,  -2,   0,   0,  -2,  -4,  -8 }
};

constexpr int KING_MG_PST[8][8] = {
    { -18, -10,   4,  -2,  -4,   6,  12,   8 },
    { -14, -10,  -6, -12, -14,  -6,   2,   0 },
    { -12, -12, -12, -16, -16, -12, -12, -12 },
    { -14, -14, -16, -22, -22, -16, -14, -14 },
    { -18, -18, -20, -26, -26, -20, -18, -18 },
    { -22, -22, -24, -30, -30, -24, -22, -22 },
    { -26, -26, -28, -34, -34, -28, -26, -26 },
    { -30, -30, -32, -38, -38, -32, -30, -30 }
};

constexpr int KING_EG_PST[8][8] = {
    { -20, -12,  -8,  -6,  -6,  -8, -12, -20 },
    { -12,  -4,   0,   4,   4,   0,  -4, -12 },
    {  -8,   0,   8,  12,  12,   8,   0,  -8 },
    {  -6,   4,  12,  18,  18,  12,   4,  -6 },
    {  -6,   4,  12,  18,  18,  12,   4,  -6 },
    {  -8,   0,   8,  12,  12,   8,   0,  -8 },
    { -12,  -4,   0,   4,   4,   0,  -4, -12 },
    { -20, -12,  -8,  -6,  -6,  -8, -12, -20 }
};

inline Color enemyOf(Color color) {
    return Chess::oppositeColor(color);
}

inline int mirrorRow(Color color, int row) {
    return color == Color::WHITE ? row : (7 - row);
}

inline bool isCentralFile(int col) {
    return col == 3 || col == 4;
}

inline bool isSemiCentralFile(int col) {
    return col == 2 || col == 5;
}

inline bool isEdgeFile(int col) {
    return col == 0 || col == 7;
}

inline bool isNearEdgeFile(int col) {
    return col == 1 || col == 6;
}

inline bool isCoreCenterSquare(int row, int col) {
    return (row == 3 || row == 4) && (col == 3 || col == 4);
}

inline bool isExtendedCenterSquare(int row, int col) {
    return row >= 2 && row <= 5 && col >= 2 && col <= 5;
}

int countPiecesExcludingKings(const Board& board) {
    int count = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.type != PieceType::KING) {
                ++count;
            }
        }
    }
    return count;
}

int countMajorPieces(const Board& board) {
    int count = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && (p.type == PieceType::QUEEN || p.type == PieceType::ROOK)) {
                ++count;
            }
        }
    }
    return count;
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

int countPieceType(const Board& board, Color color, PieceType type) {
    int count = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.color == color && p.type == type) {
                ++count;
            }
        }
    }
    return count;
}

bool fileHasPawn(const Board& board, Color color, int file) {
    for (int r = 0; r < 8; ++r) {
        Piece p = board.getPiece(r, file);
        if (!p.isEmpty() && p.color == color && p.type == PieceType::PAWN) {
            return true;
        }
    }
    return false;
}

int countPawnsOnFile(const Board& board, Color color, int file) {
    int count = 0;
    for (int r = 0; r < 8; ++r) {
        Piece p = board.getPiece(r, file);
        if (!p.isEmpty() && p.color == color && p.type == PieceType::PAWN) {
            ++count;
        }
    }
    return count;
}

bool likelyHasCastled(const Board& board, Color color) {
    Position king = EvaluatorV2::findKing(board, color);
    if (!king.isValid()) {
        return false;
    }

    const int row = static_cast<int>(king.row);
    const int col = static_cast<int>(king.col);

    if (color == Color::WHITE) {
        return row == 0 && (col == 6 || col == 2);
    }
    return row == 7 && (col == 6 || col == 2);
}

bool isPassedPawn(const Board& board, Color color, int row, int col) {
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

bool isIsolatedPawn(const Board& board, Color color, int col) {
    const bool left = (col > 0) ? fileHasPawn(board, color, col - 1) : false;
    const bool right = (col < 7) ? fileHasPawn(board, color, col + 1) : false;
    return !left && !right;
}

bool isConnectedPawn(const Board& board, Color color, int row, int col) {
    for (int dc = -1; dc <= 1; dc += 2) {
        const int adjFile = col + dc;
        if (adjFile < 0 || adjFile > 7) {
            continue;
        }

        for (int rr = std::max(0, row - 1); rr <= std::min(7, row + 1); ++rr) {
            Piece p = board.getPiece(rr, adjFile);
            if (!p.isEmpty() && p.color == color && p.type == PieceType::PAWN) {
                return true;
            }
        }
    }
    return false;
}

int pawnAdvance(Color color, int row) {
    return color == Color::WHITE ? (row - 1) : (6 - row);
}

int passedPawnBonus(Color color, int row, bool endgame) {
    int advance = pawnAdvance(color, row);
    if (advance < 0) {
        advance = 0;
    }

    static const int MG[8] = {0, 0, 4, 8, 14, 24, 40, 0};
    static const int EG[8] = {0, 0, 10, 18, 30, 50, 80, 0};

    return endgame ? EG[std::min(7, advance)] : MG[std::min(7, advance)];
}

int pieceSquareValue(PieceType type, Color color, int row, int col, bool endgame) {
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

bool attacksSquare(const Board& board, int fromRow, int fromCol, Piece piece, int targetRow, int targetCol) {
    const int dr = targetRow - fromRow;
    const int dc = targetCol - fromCol;

    switch (piece.type) {
        case PieceType::PAWN: {
            if (piece.color == Color::WHITE) {
                return dr == 1 && (dc == -1 || dc == 1);
            }
            return dr == -1 && (dc == -1 || dc == 1);
        }

        case PieceType::KNIGHT: {
            const int adr = std::abs(dr);
            const int adc = std::abs(dc);
            return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
        }

        case PieceType::BISHOP: {
            if (std::abs(dr) != std::abs(dc) || dr == 0) {
                return false;
            }
            const int stepR = (dr > 0) ? 1 : -1;
            const int stepC = (dc > 0) ? 1 : -1;
            int r = fromRow + stepR;
            int c = fromCol + stepC;
            while (r != targetRow && c != targetCol) {
                if (!board.getPiece(r, c).isEmpty()) {
                    return false;
                }
                r += stepR;
                c += stepC;
            }
            return true;
        }

        case PieceType::ROOK: {
            if (dr != 0 && dc != 0) {
                return false;
            }
            if (dr == 0 && dc == 0) {
                return false;
            }
            const int stepR = (dr == 0) ? 0 : ((dr > 0) ? 1 : -1);
            const int stepC = (dc == 0) ? 0 : ((dc > 0) ? 1 : -1);
            int r = fromRow + stepR;
            int c = fromCol + stepC;
            while (r != targetRow || c != targetCol) {
                if (!board.getPiece(r, c).isEmpty()) {
                    return false;
                }
                r += stepR;
                c += stepC;
            }
            return true;
        }

        case PieceType::QUEEN: {
            if (dr == 0 || dc == 0) {
                return attacksSquare(board, fromRow, fromCol, Piece(PieceType::ROOK, piece.color), targetRow, targetCol);
            }
            if (std::abs(dr) == std::abs(dc)) {
                return attacksSquare(board, fromRow, fromCol, Piece(PieceType::BISHOP, piece.color), targetRow, targetCol);
            }
            return false;
        }

        case PieceType::KING:
            return std::max(std::abs(dr), std::abs(dc)) == 1;

        default:
            return false;
    }
}

int countAttackersToSquare(const Board& board, Color color, int targetRow, int targetCol) {
    int count = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }

            if (attacksSquare(board, r, c, p, targetRow, targetCol)) {
                ++count;
            }
        }
    }

    return count;
}

int evaluatePieceSquareTables(const Board& board, Color color, bool endgame) {
    int score = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }

            score += pieceSquareValue(p.type, color, r, c, endgame);

            if ((p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) &&
                r >= 2 && r <= 5 && c >= 2 && c <= 5) {
                score += 4;
            }

            if ((p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) &&
                isCoreCenterSquare(r, c)) {
                score += 6;
            }
        }
    }

    return score;
}

int evaluateCenterControlInternal(const Board& board, Color color, bool endgame) {
    (void)endgame;

    int score = 0;
    const Color enemy = enemyOf(color);

    const int coreCenter[4][2] = {
        {3, 3}, {3, 4}, {4, 3}, {4, 4}
    };

    const int extendedCenter[12][2] = {
        {2, 2}, {2, 3}, {2, 4}, {2, 5},
        {3, 2},         {3, 5},
        {4, 2},         {4, 5},
        {5, 2}, {5, 3}, {5, 4}, {5, 5}
    };

    for (const auto& sq : coreCenter) {
        const int r = sq[0];
        const int c = sq[1];

        const Piece occupant = board.getPiece(r, c);
        if (!occupant.isEmpty()) {
            if (occupant.color == color) {
                if (occupant.type == PieceType::PAWN) {
                    score += 20;
                } else {
                    score += 10;
                }
            } else {
                if (occupant.type == PieceType::PAWN) {
                    score -= 20;
                } else {
                    score -= 10;
                }
            }
        }

        const int myAttackers = countAttackersToSquare(board, color, r, c);
        const int enemyAttackers = countAttackersToSquare(board, enemy, r, c);
        score += myAttackers * 5;
        score -= enemyAttackers * 5;
    }

    for (const auto& sq : extendedCenter) {
        const int r = sq[0];
        const int c = sq[1];

        const Piece occupant = board.getPiece(r, c);
        if (!occupant.isEmpty()) {
            if (occupant.color == color) {
                if (occupant.type == PieceType::PAWN) {
                    score += 6;
                } else if (occupant.type == PieceType::KNIGHT || occupant.type == PieceType::BISHOP) {
                    score += 5;
                } else {
                    score += 2;
                }
            } else {
                if (occupant.type == PieceType::PAWN) {
                    score -= 6;
                } else if (occupant.type == PieceType::KNIGHT || occupant.type == PieceType::BISHOP) {
                    score -= 5;
                } else {
                    score -= 2;
                }
            }
        }

        const int myAttackers = countAttackersToSquare(board, color, r, c);
        const int enemyAttackers = countAttackersToSquare(board, enemy, r, c);
        score += myAttackers * 2;
        score -= enemyAttackers * 2;
    }

    return score;
}

int evaluatePieceSupportInternal(const Board& board, Color color, bool endgame) {
    int score = 0;
    const Color enemy = enemyOf(color);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }

            const int defenders = countAttackersToSquare(board, color, r, c);
            const int attackers = countAttackersToSquare(board, enemy, r, c);

            if (p.type == PieceType::PAWN) {
                if (defenders > 0) {
                    score += 2;
                }

                if (isCoreCenterSquare(r, c) && defenders > 0) {
                    score += 8;
                } else if (isExtendedCenterSquare(r, c) && defenders > 0) {
                    score += 4;
                }

                if (attackers > defenders && (isCentralFile(c) || isSemiCentralFile(c))) {
                    score -= 8;
                }
            } else if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (defenders > 0) {
                    score += 6;
                } else if (!endgame) {
                    score -= 6;
                }

                if (isExtendedCenterSquare(r, c) && defenders > 0) {
                    score += 6;
                }

                if (attackers > defenders) {
                    score -= 12;
                }
            } else if (p.type == PieceType::ROOK || p.type == PieceType::QUEEN) {
                if (defenders > 0) {
                    score += 2;
                }
                if (attackers > defenders && !endgame) {
                    score -= 6;
                }
            }
        }
    }

    return score;
}

int evaluateBishopPairInternal(const Board& board, Color color) {
    return countPieceType(board, color, PieceType::BISHOP) >= 2 ? 28 : 0;
}

int evaluateRookActivityInternal(const Board& board, Color color) {
    int score = 0;
    const Color enemy = enemyOf(color);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color || p.type != PieceType::ROOK) {
                continue;
            }

            const bool ownPawnOnFile = fileHasPawn(board, color, c);
            const bool enemyPawnOnFile = fileHasPawn(board, enemy, c);

            if (!ownPawnOnFile && !enemyPawnOnFile) {
                score += 14;
            } else if (!ownPawnOnFile) {
                score += 8;
            }

            if ((color == Color::WHITE && r == 6) || (color == Color::BLACK && r == 1)) {
                score += 10;
            }
        }
    }

    return score;
}

int evaluatePawnStructureInternal(
    const Board& board,
    Color color,
    bool endgame,
    int moveCount,
    bool hasCastled
) {
    int score = 0;

    const bool centralDFile = fileHasPawn(board, color, 3);
    const bool centralEFile = fileHasPawn(board, color, 4);
    const int centralPawnCount = (centralDFile ? 1 : 0) + (centralEFile ? 1 : 0);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color || p.type != PieceType::PAWN) {
                continue;
            }

            if (isCentralFile(c)) {
                score += 10;
            } else if (isSemiCentralFile(c)) {
                score += 4;
            }

            if (isCoreCenterSquare(r, c)) {
                score += 18;
            } else if (isExtendedCenterSquare(r, c) && isCentralFile(c)) {
                score += 8;
            }

            if (isPassedPawn(board, color, r, c)) {
                int bonus = passedPawnBonus(color, r, endgame);

                // Edge passed pawns are often overrated early.
                if (!endgame) {
                    if (isEdgeFile(c)) {
                        bonus = (bonus * 45) / 100;
                    } else if (isNearEdgeFile(c)) {
                        bonus = (bonus * 70) / 100;
                    }
                }

                score += bonus;

                if (isConnectedPawn(board, color, r, c)) {
                    score += endgame ? 12 : 6;
                }
            }

            if (isIsolatedPawn(board, color, c)) {
                score -= 10;
            }

            const int pawnsOnFile = countPawnsOnFile(board, color, c);
            if (pawnsOnFile > 1) {
                score -= (pawnsOnFile - 1) * 8;
            }

            if (isConnectedPawn(board, color, r, c)) {
                score += 5;
            }

            // Stronger discouragement for early flank pawn pushes.
            if (!endgame && moveCount <= 18) {
                const int advance = pawnAdvance(color, r);

                if (!hasCastled) {
                    if (isEdgeFile(c)) {
                        if (advance >= 1) {
                            score -= 18;
                        }
                        if (advance >= 2) {
                            score -= 14;
                        }
                    } else if (isNearEdgeFile(c)) {
                        if (advance >= 2) {
                            score -= 8;
                        }
                    }
                }

                // If central structure is still weak, flank pushes are even less desirable.
                if (centralPawnCount < 2 && (isEdgeFile(c) || isNearEdgeFile(c)) && advance >= 1) {
                    score -= 8;
                }
            }
        }
    }

    // Encourage actually having central pawns.
    score += centralPawnCount * 12;

    return score;
}

int manhattanDistance(const Position& a, const Position& b) {
    const int ar = static_cast<int>(a.row);
    const int ac = static_cast<int>(a.col);
    const int br = static_cast<int>(b.row);
    const int bc = static_cast<int>(b.col);
    return std::abs(ar - br) + std::abs(ac - bc);
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
    const Color oppColor = enemyOf(aiColor);
    const bool endgame = isEndgame(board);

    const bool inferredWhiteCastled = whiteHasCastled || likelyHasCastled(board, Color::WHITE);
    const bool inferredBlackCastled = blackHasCastled || likelyHasCastled(board, Color::BLACK);

    const bool aiHasCastled =
        (aiColor == Color::WHITE) ? inferredWhiteCastled : inferredBlackCastled;
    const bool oppHasCastled =
        (aiColor == Color::WHITE) ? inferredBlackCastled : inferredWhiteCastled;

    const int aiMaterial = countMaterial(board, aiColor);
    const int oppMaterial = countMaterial(board, oppColor);
    const int materialDiff = aiMaterial - oppMaterial;

    int score = materialDiff * 10;

    if (materialDiff < -50) {
        score += materialDiff * 12;
    }

    score += evaluatePieceSquareTables(board, aiColor, endgame);
    score -= evaluatePieceSquareTables(board, oppColor, endgame);

    score += evaluateCenterControlInternal(board, aiColor, endgame);
    score -= evaluateCenterControlInternal(board, oppColor, endgame);

    score += evaluatePawnStructureInternal(board, aiColor, endgame, moveCount, aiHasCastled);
    score -= evaluatePawnStructureInternal(board, oppColor, endgame, moveCount, oppHasCastled);

    score += evaluateBishopPairInternal(board, aiColor);
    score -= evaluateBishopPairInternal(board, oppColor);

    score += evaluateRookActivityInternal(board, aiColor);
    score -= evaluateRookActivityInternal(board, oppColor);

    score += evaluatePieceSupportInternal(board, aiColor, endgame);
    score -= evaluatePieceSupportInternal(board, oppColor, endgame);

    if (moveCount < 20 && materialDiff >= -120) {
        const int aiDevelopment = evaluatePieceDevelopment(board, aiColor, moveCount);
        const int oppDevelopment = evaluatePieceDevelopment(board, oppColor, moveCount);
        score += (aiDevelopment - oppDevelopment) / 2;
    }

    score += evaluatePieceCoordination(board, aiColor);
    score -= evaluatePieceCoordination(board, oppColor);

    if (materialDiff >= -100) {
        const int aiMobility = evaluateMobility(board, aiColor, castling);
        const int oppMobility = evaluateMobility(board, oppColor, castling);
        score += (aiMobility - oppMobility) / 10;
    }

    if (!endgame && materialDiff >= -140) {
        const int CASTLED_BONUS = 28;
        const int CASTLING_RIGHTS_BONUS = 7;
        const int UNCENTRALIZED_KING_PENALTY = 18;

        if (aiHasCastled) {
            score += CASTLED_BONUS;
        }
        if (oppHasCastled) {
            score -= CASTLED_BONUS;
        }

        if (aiColor == Color::WHITE) {
            if (!aiHasCastled && moveCount < 16 &&
                (castling.whiteKingSide || castling.whiteQueenSide)) {
                score += CASTLING_RIGHTS_BONUS;
            }
            if (!oppHasCastled && moveCount < 16 &&
                (castling.blackKingSide || castling.blackQueenSide)) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        } else {
            if (!aiHasCastled && moveCount < 16 &&
                (castling.blackKingSide || castling.blackQueenSide)) {
                score += CASTLING_RIGHTS_BONUS;
            }
            if (!oppHasCastled && moveCount < 16 &&
                (castling.whiteKingSide || castling.whiteQueenSide)) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        }

        Position aiKing = findKing(board, aiColor);
        Position oppKing = findKing(board, oppColor);

        if (aiKing.isValid() && !aiHasCastled && moveCount >= 8) {
            const int kcol = static_cast<int>(aiKing.col);
            if (kcol == 3 || kcol == 4) {
                score -= UNCENTRALIZED_KING_PENALTY;
            }
        }

        if (oppKing.isValid() && !oppHasCastled && moveCount >= 8) {
            const int kcol = static_cast<int>(oppKing.col);
            if (kcol == 3 || kcol == 4) {
                score += UNCENTRALIZED_KING_PENALTY;
            }
        }
    }

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
    const int totalPieces = countPiecesExcludingKings(board);
    const int majorPieces = countMajorPieces(board);
    const int whiteMaterial = countNonKingMaterial(board, Color::WHITE);
    const int blackMaterial = countNonKingMaterial(board, Color::BLACK);

    if (totalPieces <= 8) {
        return true;
    }

    if (majorPieces == 0 && totalPieces <= 12) {
        return true;
    }

    return (whiteMaterial <= 1300 && blackMaterial <= 1300);
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
    const int aiMaterial = countMaterial(board, aiColor);
    const int oppMaterial = countMaterial(board, oppColor);
    int score = 0;

    const Position aiKing = findKing(board, aiColor);
    const Position oppKing = findKing(board, oppColor);

    score += evaluatePawnStructureInternal(board, aiColor, true, 99, true);
    score -= evaluatePawnStructureInternal(board, oppColor, true, 99, true);

    if (aiMaterial > oppMaterial + 250 && aiKing.isValid() && oppKing.isValid()) {
        const int kingRow = static_cast<int>(oppKing.row);
        const int kingCol = static_cast<int>(oppKing.col);

        const int edgeDist = std::min(
            std::min(kingRow, 7 - kingRow),
            std::min(kingCol, 7 - kingCol)
        );

        score += (3 - std::min(3, edgeDist)) * 16;

        const int kingDist = manhattanDistance(aiKing, oppKing);
        score += (14 - kingDist) * 4;
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != oppColor || p.type != PieceType::PAWN) {
                continue;
            }

            if (isPassedPawn(board, oppColor, r, c)) {
                score -= passedPawnBonus(oppColor, r, true) / 2;
            }
        }
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

    const bool leftMoved =
        leftRook.isEmpty() || leftRook.type != PieceType::ROOK || leftRook.color != color;
    const bool rightMoved =
        rightRook.isEmpty() || rightRook.type != PieceType::ROOK || rightRook.color != color;

    return leftMoved || rightMoved;
}

int EvaluatorV2::evaluatePieceDevelopment(const Board& board, Color color, int moveCount) {
    int development = 0;

    const int undevelopedCount = countUndevelopedPieces(board, color);
    const bool hasUndevelopedMinors = undevelopedCount > 0;
    const bool rooksDeveloped = isRookDeveloped(board, color);
    const bool hasCastled = likelyHasCastled(board, color);

    if (moveCount > 4) {
        development -= undevelopedCount * 14;
    }

    if (hasUndevelopedMinors && rooksDeveloped && moveCount < 16) {
        development -= 36;
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }

            const Position pos(r, c);

            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development += 14;

                    if (isExtendedCenterSquare(r, c)) {
                        development += 8;
                    }
                    if (isCoreCenterSquare(r, c)) {
                        development += 6;
                    }
                } else if (moveCount > 8) {
                    development -= 12;
                }
            }

            if (p.type == PieceType::PAWN && moveCount < 14) {
                if (color == Color::WHITE) {
                    if ((c == 3 || c == 4) && r >= 2) {
                        development += 12;
                    }
                    if ((c == 2 || c == 5) && r >= 2) {
                        development += 4;
                    }
                } else {
                    if ((c == 3 || c == 4) && r <= 5) {
                        development += 12;
                    }
                    if ((c == 2 || c == 5) && r <= 5) {
                        development += 4;
                    }
                }

                if (!hasCastled && (c == 0 || c == 7)) {
                    const int advance = pawnAdvance(color, r);
                    if (advance >= 1) {
                        development -= 14;
                    }
                    if (advance >= 2) {
                        development -= 10;
                    }
                }
            }

            if (p.type == PieceType::QUEEN && moveCount < 10) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 16;
                }
            }

            if (p.type == PieceType::ROOK && moveCount < 14 && hasUndevelopedMinors) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 36;
                }
            }

            if (p.type == PieceType::ROOK && moveCount < 12 && !hasCastled) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 22;
                }
            }
        }
    }

    return development;
}

int EvaluatorV2::evaluatePieceCoordination(const Board& board, Color color) {
    int coordination = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) {
                continue;
            }

            if (p.type == PieceType::PAWN) {
                const int supportRow = (color == Color::WHITE) ? (r - 1) : (r + 1);
                for (int dc = -1; dc <= 1; dc += 2) {
                    const int cc = c + dc;
                    if (supportRow >= 0 && supportRow < 8 && cc >= 0 && cc < 8) {
                        Piece support = board.getPiece(supportRow, cc);
                        if (!support.isEmpty() && support.color == color &&
                            support.type == PieceType::PAWN) {
                            coordination += 4;
                        }
                    }
                }
            } else if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (r >= 2 && r <= 5 && c >= 2 && c <= 5) {
                    coordination += 5;
                }
            } else if (p.type == PieceType::ROOK) {
                for (int rr = 0; rr < 8; ++rr) {
                    for (int cc = 0; cc < 8; ++cc) {
                        if (rr == r && cc == c) {
                            continue;
                        }

                        Piece other = board.getPiece(rr, cc);
                        if (other.isEmpty() || other.color != color || other.type != PieceType::ROOK) {
                            continue;
                        }

                        if (rr == r) {
                            bool clear = true;
                            for (int x = std::min(c, cc) + 1; x < std::max(c, cc); ++x) {
                                if (!board.getPiece(r, x).isEmpty()) {
                                    clear = false;
                                    break;
                                }
                            }
                            if (clear) {
                                coordination += 6;
                            }
                        } else if (cc == c) {
                            bool clear = true;
                            for (int x = std::min(r, rr) + 1; x < std::max(r, rr); ++x) {
                                if (!board.getPiece(x, c).isEmpty()) {
                                    clear = false;
                                    break;
                                }
                            }
                            if (clear) {
                                coordination += 6;
                            }
                        }
                    }
                }
            }
        }
    }

    return coordination;
}

int EvaluatorV2::evaluateMobility(const Board& board, Color color, const CastlingRights& castling) {
    (void)castling;

    int mobility = 0;
    const Color enemy = enemyOf(color);

    for (int targetRow = 0; targetRow < 8; ++targetRow) {
        for (int targetCol = 0; targetCol < 8; ++targetCol) {
            const Piece occupant = board.getPiece(targetRow, targetCol);
            if (!occupant.isEmpty() && occupant.color == color) {
                continue;
            }

            int attacks = countAttackersToSquare(board, color, targetRow, targetCol);
            if (attacks == 0) {
                continue;
            }

            mobility += attacks;

            if (isCoreCenterSquare(targetRow, targetCol)) {
                mobility += attacks * 3;
            } else if (isExtendedCenterSquare(targetRow, targetCol)) {
                mobility += attacks * 2;
            }

            if (!occupant.isEmpty() && occupant.color == enemy) {
                mobility += 2;
            }
        }
    }

    return mobility;
}

} // namespace V2
} // namespace Chess