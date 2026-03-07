#ifndef CHESS_V1_EVALUATION_H
#define CHESS_V1_EVALUATION_H

#include "../Types.h"
#include "../Board.h"

namespace Chess {
namespace V1 {

class Evaluation {
public:
    // Evaluate the position from the perspective of the side to move
    // Positive scores favor the side to move
    static int evaluate(const Board& board);
    
    // Evaluate with ply depth for mate distance scoring
    static int evaluate(const Board& board, int ply);
    
    // Mate score constants
    static constexpr int MATE_SCORE = 100000;
    static constexpr int MAX_MATE_PLY = 100;
    
private:
    // Piece values in centipawns
    static constexpr int PAWN_VALUE = 100;
    static constexpr int KNIGHT_VALUE = 320;
    static constexpr int BISHOP_VALUE = 330;
    static constexpr int ROOK_VALUE = 500;
    static constexpr int QUEEN_VALUE = 900;
    static constexpr int KING_VALUE = 20000;
    
    // Evaluation weights
    static constexpr int ACTIVITY_WEIGHT = 4;
    static constexpr int PAWN_SHIELD_WEIGHT = 15;
    static constexpr int KING_ATTACK_WEIGHT = 20;
    static constexpr int POSITION_WEIGHT = 2;        // Centralization bonus
    
    static int getPieceValue(PieceType type);
    static int evaluateMaterial(const Board& board, Color color);
    static int evaluateMobility(const Board& board, Color color);
    
    // New evaluation methods
    static int evaluatePieceActivity(const Board& board, Color color);
    static int evaluateKingSafety(const Board& board, Color color);
    static int evaluatePositionBonus(const Board& board, Color color);
    static int getPositionBonus(PieceType type, int rank, int file, Color color);
    static int countAttackedSquares(const Board& board, Color color, int rank, int file);
};

} // namespace V1
} // namespace Chess

#endif // CHESS_V1_EVALUATION_H
