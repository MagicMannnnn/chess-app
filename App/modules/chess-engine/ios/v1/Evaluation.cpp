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

int Evaluation::countAttackedSquares(const Board& board, Color color, int rank, int file) {
    int count = 0;
    Piece piece = board.getPiece(rank * 8 + file);
    
    if (piece.isEmpty() || piece.color != color) {
        return 0;
    }
    
    // Helper to check if a square is valid and count it
    auto checkSquare = [&board, color, &count](int r, int f) {
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            Piece target = board.getPiece(r * 8 + f);
            // Count empty squares and enemy pieces (attacked), and friendly pieces (defended)
            if (target.isEmpty() || target.color != color || target.color == color) {
                count++;
            }
        }
    };
    
    switch (piece.type) {
        case PieceType::PAWN: {
            int direction = (color == Color::WHITE) ? 1 : -1;
            // Pawns attack diagonally
            checkSquare(rank + direction, file - 1);
            checkSquare(rank + direction, file + 1);
            break;
        }
        
        case PieceType::KNIGHT: {
            // Knight moves in L shape
            const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
            for (const auto& move : knightMoves) {
                checkSquare(rank + move[0], file + move[1]);
            }
            break;
        }
        
        case PieceType::BISHOP: {
            // Bishop moves diagonally
            const int directions[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
            for (const auto& dir : directions) {
                int r = rank + dir[0];
                int f = file + dir[1];
                while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                    Piece target = board.getPiece(r * 8 + f);
                    count++;
                    if (!target.isEmpty()) break; // Blocked by piece
                    r += dir[0];
                    f += dir[1];
                }
            }
            break;
        }
        
        case PieceType::ROOK: {
            // Rook moves horizontally and vertically
            const int directions[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (const auto& dir : directions) {
                int r = rank + dir[0];
                int f = file + dir[1];
                while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                    Piece target = board.getPiece(r * 8 + f);
                    count++;
                    if (!target.isEmpty()) break; // Blocked by piece
                    r += dir[0];
                    f += dir[1];
                }
            }
            break;
        }
        
        case PieceType::QUEEN: {
            // Queen moves in all 8 directions
            const int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
            for (const auto& dir : directions) {
                int r = rank + dir[0];
                int f = file + dir[1];
                while (r >= 0 && r < 8 && f >= 0 && f < 8) {
                    Piece target = board.getPiece(r * 8 + f);
                    count++;
                    if (!target.isEmpty()) break; // Blocked by piece
                    r += dir[0];
                    f += dir[1];
                }
            }
            break;
        }
        
        case PieceType::KING: {
            // King moves one square in any direction
            const int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
            for (const auto& dir : directions) {
                checkSquare(rank + dir[0], file + dir[1]);
            }
            break;
        }
        
        default:
            break;
    }
    
    return count;
}

int Evaluation::evaluatePieceActivity(const Board& board, Color color) {
    int activity = 0;
    
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty() && piece.color == color) {
            int rank = sq / 8;
            int file = sq % 8;
            activity += countAttackedSquares(board, color, rank, file);
        }
    }
    
    return activity;
}

int Evaluation::evaluateKingSafety(const Board& board, Color color) {
    int safety = 0;
    
    // Find the king
    int kingSquare = -1;
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (piece.type == PieceType::KING && piece.color == color) {
            kingSquare = sq;
            break;
        }
    }
    
    if (kingSquare == -1) return 0;
    
    int kingRank = kingSquare / 8;
    int kingFile = kingSquare % 8;
    
    // Evaluate pawn shield (friendly pawns near the king)
    int pawnShield = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            if (dr == 0 && df == 0) continue;
            
            int r = kingRank + dr;
            int f = kingFile + df;
            
            if (r >= 0 && r < 8 && f >= 0 && f < 8) {
                Piece piece = board.getPiece(r * 8 + f);
                if (piece.type == PieceType::PAWN && piece.color == color) {
                    pawnShield++;
                }
            }
        }
    }
    
    safety += pawnShield * PAWN_SHIELD_WEIGHT;
    
    // Evaluate enemy piece attacks near the king
    Color enemy = oppositeColor(color);
    int enemyAttacks = 0;
    
    for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            int r = kingRank + dr;
            int f = kingFile + df;
            
            if (r >= 0 && r < 8 && f >= 0 && f < 8) {
                // Count enemy pieces that can attack this square
                for (int sq = 0; sq < 64; sq++) {
                    Piece piece = board.getPiece(sq);
                    if (!piece.isEmpty() && piece.color == enemy) {
                        int pieceRank = sq / 8;
                        int pieceFile = sq % 8;
                        
                        // Simple check if enemy piece attacks this square
                        bool attacks = false;
                        
                        switch (piece.type) {
                            case PieceType::PAWN: {
                                int dir = (enemy == Color::WHITE) ? 1 : -1;
                                if (pieceRank + dir == r && (pieceFile - 1 == f || pieceFile + 1 == f)) {
                                    attacks = true;
                                }
                                break;
                            }
                            case PieceType::KNIGHT: {
                                int dr2 = abs(pieceRank - r);
                                int df2 = abs(pieceFile - f);
                                if ((dr2 == 2 && df2 == 1) || (dr2 == 1 && df2 == 2)) {
                                    attacks = true;
                                }
                                break;
                            }
                            case PieceType::KING: {
                                if (abs(pieceRank - r) <= 1 && abs(pieceFile - f) <= 1) {
                                    attacks = true;
                                }
                                break;
                            }
                            default:
                                // For sliding pieces, we'd need to check paths
                                // Simplified for now
                                break;
                        }
                        
                        if (attacks) {
                            enemyAttacks++;
                        }
                    }
                }
            }
        }
    }
    
    safety -= enemyAttacks * KING_ATTACK_WEIGHT;
    
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

int Evaluation::evaluate(const Board& board, int ply) {
    Color currentPlayer = board.getCurrentPlayer();
    Color opponent = oppositeColor(currentPlayer);
    
    // Check for game over conditions
    if (board.isCheckmate()) {
        // Return mate score adjusted for distance
        // Closer mates are better (more negative for the losing side)
        return -MATE_SCORE + ply;
    }
    
    if (board.isStalemate() || board.isDraw()) {
        return 0; // Draw
    }
    
    // Material evaluation (most important)
    int ourMaterial = evaluateMaterial(board, currentPlayer);
    int theirMaterial = evaluateMaterial(board, opponent);
    int materialScore = ourMaterial - theirMaterial;
    
    // Piece activity evaluation (attacks and defense)
    int ourActivity = evaluatePieceActivity(board, currentPlayer);
    int theirActivity = evaluatePieceActivity(board, opponent);
    int activityScore = (ourActivity - theirActivity) * ACTIVITY_WEIGHT;
    
    // King safety evaluation
    int ourKingSafety = evaluateKingSafety(board, currentPlayer);
    int theirKingSafety = evaluateKingSafety(board, opponent);
    int kingSafetyScore = ourKingSafety - theirKingSafety;
    
    // Position bonus (centralization - small weight)
    int ourPosition = evaluatePositionBonus(board, currentPlayer);
    int theirPosition = evaluatePositionBonus(board, opponent);
    int positionScore = (ourPosition - theirPosition) * POSITION_WEIGHT;
    
    // Mobility evaluation (less weight than other factors)
    int ourMobility = evaluateMobility(board, currentPlayer);
    int theirMobility = evaluateMobility(board, opponent);
    int mobilityScore = (ourMobility - theirMobility) * 2;
    
    return materialScore + activityScore + kingSafetyScore + positionScore + mobilityScore;
}

int Evaluation::evaluate(const Board& board) {
    // Default overload calls the ply version with 0
    return evaluate(board, 0);
}

} // namespace V1
} // namespace Chess
