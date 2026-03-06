#ifndef CHESS_V1_EVALUATION_H
#define CHESS_V1_EVALUATION_H

#include "../Core/Types.h"
#include "../Core/Board.h"

namespace Chess {
namespace V1 {

class Evaluation {
public:
    // Evaluate the position from the perspective of the side to move
    // Positive scores favor the side to move
    static int evaluate(const Board& board);
    
private:
    // Piece values in centipawns
    static constexpr int PAWN_VALUE = 100;
    static constexpr int KNIGHT_VALUE = 320;
    static constexpr int BISHOP_VALUE = 330;
    static constexpr int ROOK_VALUE = 500;
    static constexpr int QUEEN_VALUE = 900;
    static constexpr int KING_VALUE = 20000;
    
    static int getPieceValue(PieceType type);
    static int evaluateMaterial(const Board& board, Color color);
    static int evaluateMobility(const Board& board, Color color);
};

} // namespace V1
} // namespace Chess

#endif // CHESS_V1_EVALUATION_H
