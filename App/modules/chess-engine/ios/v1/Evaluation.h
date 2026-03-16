#ifndef CHESS_V1_EVALUATION_H
#define CHESS_V1_EVALUATION_H

#include "../Core/Types.h"
#include "../Core/Board.h"
#include <vector>

namespace Chess {
namespace V1 {

class Evaluation {
public:
    // Evaluate the position from the perspective of the side to move
    // Positive scores favor the side to move
    static int evaluate(const Board& board);
    
    // Evaluate with depth for mate detection
    static int evaluate(const Board& board, int ply);
    
    // Fast material-only evaluation (for quiescence search)
    static int evaluateMaterialOnly(const Board& board);
    
private:
    // Piece values in centipawns
    static constexpr int PAWN_VALUE = 100;
    static constexpr int KNIGHT_VALUE = 320;
    static constexpr int BISHOP_VALUE = 330;
    static constexpr int ROOK_VALUE = 500;
    static constexpr int QUEEN_VALUE = 900;
    static constexpr int KING_VALUE = 20000;
    static constexpr int MATE_SCORE = 100000;
    
    // Evaluation weights
    static constexpr int PIECE_ACTIVITY_WEIGHT = 4;  // Per square attacked/defended
    static constexpr int KING_SAFETY_WEIGHT = 15;    // Per pawn shield
    static constexpr int KING_ATTACK_WEIGHT = 20;    // Per attacker near enemy king
    static constexpr int POSITION_WEIGHT = 2;        // Centralization bonus
    
    static int getPieceValue(PieceType type);
    static int evaluateMaterial(const Board& board, Color color);
    static int evaluatePieceActivity(const Board& board, Color color);
    static int evaluateKingSafety(const Board& board, Color color);
    static int evaluatePositionBonus(const Board& board, Color color);
    static int getPositionBonus(PieceType type, int rank, int file, Color color);
    static int countAttackedSquares(const Board& board, Square from, Color attackerColor);
};

} // namespace V1
} // namespace Chess

#endif // CHESS_V1_EVALUATION_H
