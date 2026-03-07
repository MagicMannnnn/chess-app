#include "Evaluation.h"
#include <cmath>

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

int Evaluation::countAttackedSquares(const Board& board, Square from, Color attackerColor) {
    Piece piece = board.getPiece(from);
    if (piece.isEmpty() || piece.color != attackerColor) {
        return 0;
    }
    
    int count = 0;
    int row = from / 8;
    int col = from % 8;
    
    // Direction vectors for different piece types
    static const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    static const int kingMoves[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    static const int bishopDirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    static const int rookDirs[4][2] = {{-1,0},{0,-1},{0,1},{1,0}};
    
    switch (piece.type) {
        case PieceType::PAWN: {
            int direction = (piece.color == Color::WHITE) ? 1 : -1;
            // Pawn attacks diagonally
            for (int dc = -1; dc <= 1; dc += 2) {
                int newRow = row + direction;
                int newCol = col + dc;
                if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                }
            }
            break;
        }
        case PieceType::KNIGHT: {
            for (int i = 0; i < 8; i++) {
                int newRow = row + knightMoves[i][0];
                int newCol = col + knightMoves[i][1];
                if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                }
            }
            break;
        }
        case PieceType::BISHOP: {
            for (int i = 0; i < 4; i++) {
                int newRow = row + bishopDirs[i][0];
                int newCol = col + bishopDirs[i][1];
                while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                    Square sq = newRow * 8 + newCol;
                    if (!board.getPiece(sq).isEmpty()) break;
                    newRow += bishopDirs[i][0];
                    newCol += bishopDirs[i][1];
                }
            }
            break;
        }
        case PieceType::ROOK: {
            for (int i = 0; i < 4; i++) {
                int newRow = row + rookDirs[i][0];
                int newCol = col + rookDirs[i][1];
                while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                    Square sq = newRow * 8 + newCol;
                    if (!board.getPiece(sq).isEmpty()) break;
                    newRow += rookDirs[i][0];
                    newCol += rookDirs[i][1];
                }
            }
            break;
        }
        case PieceType::QUEEN: {
            // Queen combines rook and bishop movement
            for (int i = 0; i < 4; i++) {
                // Bishop directions
                int newRow = row + bishopDirs[i][0];
                int newCol = col + bishopDirs[i][1];
                while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                    Square sq = newRow * 8 + newCol;
                    if (!board.getPiece(sq).isEmpty()) break;
                    newRow += bishopDirs[i][0];
                    newCol += bishopDirs[i][1];
                }
                // Rook directions
                newRow = row + rookDirs[i][0];
                newCol = col + rookDirs[i][1];
                while (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                    Square sq = newRow * 8 + newCol;
                    if (!board.getPiece(sq).isEmpty()) break;
                    newRow += rookDirs[i][0];
                    newCol += rookDirs[i][1];
                }
            }
            break;
        }
        case PieceType::KING: {
            for (int i = 0; i < 8; i++) {
                int newRow = row + kingMoves[i][0];
                int newCol = col + kingMoves[i][1];
                if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                    count++;
                }
            }
            break;
        }
        default:
            break;
    }
    
    return count;
}

int Evaluation::evaluatePieceActivity(const Board& board, Color color) {
    // Simplified activity evaluation - just count mobility bonus per piece
    // Much faster than counting all attacked squares
    int activity = 0;
    
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty() && piece.color == color) {
            // Simple bonus based on piece type and centralization
            // instead of expensive square counting
            int rank = sq / 8;
            int file = sq % 8;
            int centerDist = std::abs(rank - 4) + std::abs(file - 4);
            
            switch (piece.type) {
                case PieceType::KNIGHT:
                case PieceType::BISHOP:
                    // Knights and bishops benefit from centralization
                    activity += (8 - centerDist);
                    break;
                case PieceType::QUEEN:
                    // Queen gets small bonus
                    activity += (8 - centerDist) / 2;
                    break;
                default:
                    break;
            }
        }
    }
    
    return activity;
}

int Evaluation::evaluateKingSafety(const Board& board, Color color) {
    int safety = 0;
    Color opponent = oppositeColor(color);
    
    // Find king position
    Square kingSquare = -1;
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty() && piece.type == PieceType::KING && piece.color == color) {
            kingSquare = sq;
            break;
        }
    }
    
    if (kingSquare < 0) return -1000; // King not found (should never happen)
    
    int kingRow = kingSquare / 8;
    int kingCol = kingSquare % 8;
    
    // Pawn shield: pawns in front of king
    int pawnShield = 0;
    int direction = (color == Color::WHITE) ? 1 : -1;
    
    for (int dc = -1; dc <= 1; dc++) {
        int shieldRow = kingRow + direction;
        int shieldCol = kingCol + dc;
        
        if (shieldCol >= 0 && shieldCol < 8 && shieldRow >= 0 && shieldRow < 8) {
            Square sq = shieldRow * 8 + shieldCol;
            Piece piece = board.getPiece(sq);
            if (!piece.isEmpty() && piece.type == PieceType::PAWN && piece.color == color) {
                pawnShield++;
            }
        }
        
        // Check second rank of shield
        shieldRow = kingRow + direction * 2;
        if (shieldCol >= 0 && shieldCol < 8 && shieldRow >= 0 && shieldRow < 8) {
            Square sq = shieldRow * 8 + shieldCol;
            Piece piece = board.getPiece(sq);
            if (!piece.isEmpty() && piece.type == PieceType::PAWN && piece.color == color) {
                pawnShield++;
            }
        }
    }
    
    safety += pawnShield * KING_SAFETY_WEIGHT;
    
    return safety;
}

int Evaluation::getPositionBonus(PieceType type, int rank, int file, Color color) {
    // Simple centralization bonus - pieces get small bonuses for being in/near center
    // Bonus based on distance from center (3.5, 3.5)
    float centerRank = 3.5f;
    float centerFile = 3.5f;
    
    float distFromCenter = std::abs(rank - centerRank) + std::abs(file - centerFile);
    
    // Base bonus decreases with distance from center
    int bonus = static_cast<int>(10.0f - distFromCenter * 2.0f);
    
    // Different pieces benefit differently from centralization
    switch (type) {
        case PieceType::KNIGHT:
        case PieceType::BISHOP:
            // Knights and bishops benefit most from centralization
            return bonus;
        case PieceType::PAWN:
            // Pawns get small bonus for advancing + centralization
            if (color == Color::WHITE) {
                return bonus / 2 + rank * 2;  // Encourage white pawns to advance
            } else {
                return bonus / 2 + (7 - rank) * 2;  // Encourage black pawns to advance
            }
        case PieceType::QUEEN:
            // Queen benefits somewhat from centralization
            return bonus / 2;
        case PieceType::KING:
            // King should stay safe in the opening/middlegame
            // In endgame (not detected here), king should centralize
            return -bonus / 2;  // Small penalty for centralizing too early
        case PieceType::ROOK:
            // Rooks don't benefit much from centralization
            return bonus / 3;
        default:
            return 0;
    }
}

int Evaluation::evaluatePositionBonus(const Board& board, Color color) {
    int bonus = 0;
    
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty() && piece.color == color) {
            int rank = sq / 8;
            int file = sq % 8;
            bonus += getPositionBonus(piece.type, rank, file, color);
        }
    }
    
    return bonus;
}

int Evaluation::evaluate(const Board& board) {
    return evaluate(board, 0);
}

int Evaluation::evaluateMaterialOnly(const Board& board) {
    Color currentPlayer = board.getCurrentPlayer();
    Color opponent = oppositeColor(currentPlayer);
    
    // Quick material-only evaluation for quiescence search
    int ourMaterial = evaluateMaterial(board, currentPlayer);
    int theirMaterial = evaluateMaterial(board, opponent);
    
    return ourMaterial - theirMaterial;
}

int Evaluation::evaluate(const Board& board, int ply) {
    Color currentPlayer = board.getCurrentPlayer();
    Color opponent = oppositeColor(currentPlayer);
    
    // Check for checkmate - prioritize based on depth
    if (board.isCheckmate()) {
        // Negative score (we're checkmated)
        // Closer mates are worse, so subtract ply to make them more negative
        return -MATE_SCORE + ply;
    }
    
    if (board.isStalemate() || board.isDraw()) {
        return 0; // Draw
    }
    
    // Material evaluation (most important)
    int ourMaterial = evaluateMaterial(board, currentPlayer);
    int theirMaterial = evaluateMaterial(board, opponent);
    int materialScore = ourMaterial - theirMaterial;
    
    // Piece activity evaluation (second priority)
    int ourActivity = evaluatePieceActivity(board, currentPlayer);
    int theirActivity = evaluatePieceActivity(board, opponent);
    int activityScore = (ourActivity - theirActivity) * PIECE_ACTIVITY_WEIGHT;
    
    // King safety evaluation (third priority)
    int ourKingSafety = evaluateKingSafety(board, currentPlayer);
    int theirKingSafety = evaluateKingSafety(board, opponent);
    int kingSafetyScore = ourKingSafety - theirKingSafety;
    
    // Position bonus (centralization - small weight)
    int ourPosition = evaluatePositionBonus(board, currentPlayer);
    int theirPosition = evaluatePositionBonus(board, opponent);
    int positionScore = (ourPosition - theirPosition) * POSITION_WEIGHT;
    
    return materialScore + activityScore + kingSafetyScore + positionScore;
}

} // namespace V1
} // namespace Chess
