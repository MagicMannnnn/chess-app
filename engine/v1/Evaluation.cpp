#include "Evaluation.h"

namespace Chess {
namespace V1 {

int Evaluation::getPieceValue(PieceType type) {
    switch (type) {
        case PieceType::PAWN:   return PAWN_VALUE;
        case PieceType::KNIGHT: return KNIGHT_VALUE;
        case PieceType::BISHOP: return BISHOP_VALUE;
        case PieceType::ROOK:   return ROOK_VALUE;
        case PieceType::QUEEN:  return QUEEN_VALUE;
        case PieceType::KING:   return KING_VALUE;
        default: return 0;
    }
}

int Evaluation::evaluateMaterial(const Board& board, Color color) {
    int material = 0;
    
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty() && piece.color == color) {
            material += getPieceValue(piece.type);
        }
    }
    
    return material;
}

int Evaluation::evaluateMobility(const Board& board, Color color) {
    // Mobility: number of legal moves available
    // This is a simple measure of piece activity
    return static_cast<int>(board.generatePseudoLegalMoves(color).size());
}

int Evaluation::evaluate(const Board& board) {
    Color currentPlayer = board.getCurrentPlayer();
    Color opponent = oppositeColor(currentPlayer);
    
    // Check for game over conditions
    if (board.isCheckmate()) {
        return -KING_VALUE; // Current player is checkmated
    }
    
    if (board.isStalemate() || board.isDraw()) {
        return 0; // Draw
    }
    
    // Material evaluation
    int ourMaterial = evaluateMaterial(board, currentPlayer);
    int theirMaterial = evaluateMaterial(board, opponent);
    int materialScore = ourMaterial - theirMaterial;
    
    // Mobility evaluation (less weight than material)
    int ourMobility = evaluateMobility(board, currentPlayer);
    int theirMobility = evaluateMobility(board, opponent);
    int mobilityScore = (ourMobility - theirMobility) * 5;
    
    return materialScore + mobilityScore;
}

} // namespace V1
} // namespace Chess
