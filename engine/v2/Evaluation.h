#ifndef CHESS_V2_EVALUATION_H
#define CHESS_V2_EVALUATION_H

#include "../Types.h"
#include "../Board.h"
#include "../Move.h"
#include <vector>

namespace Chess {
namespace V2 {

// Simple castling rights structure for evaluation
struct CastlingRights {
    bool whiteKingSide;
    bool whiteQueenSide;
    bool blackKingSide;
    bool blackQueenSide;

    CastlingRights()
        : whiteKingSide(false),
          whiteQueenSide(false),
          blackKingSide(false),
          blackQueenSide(false) {}
};

// Material-first evaluation strategy
class EvaluatorV2 {
public:
    // Piece values in centipawns
    static const int PIECE_VALUES[7];

    // Main evaluation function - returns score from AI's perspective
    static int evaluate(
        const Board& board,
        Color aiColor,
        const CastlingRights& castling,
        bool whiteHasCastled,
        bool blackHasCastled,
        int moveCount
    );

    // Material counting
    static int countMaterial(const Board& board, Color color);

    // Endgame detection and evaluation
    static bool isEndgame(const Board& board);
    static int evaluateEndgame(
        const Board& board,
        Color aiColor,
        const CastlingRights& castling
    );

    // Position utilities
    static Position findKing(const Board& board, Color color);

    // Piece development evaluation
    static int evaluatePieceDevelopment(const Board& board, Color color, int moveCount);
    static bool isPieceOnStartingSquare(PieceType type, Color color, Position pos);
    static int countUndevelopedPieces(const Board& board, Color color);
    static bool isRookDeveloped(const Board& board, Color color);

    // Piece coordination - kept in API, but implemented cheaply
    static int evaluatePieceCoordination(const Board& board, Color color);

    // Mobility evaluation
    static int evaluateMobility(const Board& board, Color color, const CastlingRights& castling);
};

} // namespace V2
} // namespace Chess

#endif // CHESS_V2_EVALUATION_H