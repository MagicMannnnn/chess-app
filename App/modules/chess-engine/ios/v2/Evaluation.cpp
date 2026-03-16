#include "Evaluation.h"
#include <cmath>
#include <algorithm>

namespace Chess {
namespace V2 {

const int EvaluatorV2::PIECE_VALUES[7] = {
    100,  // PAWN
    320,  // KNIGHT
    330,  // BISHOP
    500,  // ROOK
    900,  // QUEEN
    20000,// KING
    0     // NONE
};

// Material-first evaluation: material is KING, positional factors are minimal
int EvaluatorV2::evaluate(const Board& board, Color aiColor, const CastlingRights& castling, 
                          bool whiteHasCastled, bool blackHasCastled, int moveCount) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int score = 0;
    
    // MATERIAL IS EVERYTHING - this is the primary score
    int aiMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, oppColor);
    int materialDiff = aiMaterial - oppMaterial;
    
    // Base material score - multiply by 10 to ensure it dominates
    score = materialDiff * 10;
    
    // Extra penalty for material deficit to prevent blunders
    if (materialDiff < -50) { // Down more than half a pawn
        score += materialDiff * 20; // Massive additional penalty
    }
    
    // PIECE COORDINATION: Small bonus for pieces attacking/defending other pieces
    // This encourages active, coordinated play without overriding material
    int aiCoordination = evaluatePieceCoordination(board, aiColor);
    int oppCoordination = evaluatePieceCoordination(board, oppColor);
    score += (aiCoordination - oppCoordination); // Small ~2-5 point bonuses per piece
    
    // Piece development - TINY bonus/penalty (can't override material)
    if (moveCount < 18 && materialDiff >= -100) { // Only if not badly down material
        int aiDevelopment = evaluatePieceDevelopment(board, aiColor, moveCount);
        int oppDevelopment = evaluatePieceDevelopment(board, oppColor, moveCount);
        // Heavily scaled down - development matters but material matters WAY more
        score += (aiDevelopment - oppDevelopment) / 5; // Divide by 5 to make tiny
    }
    
    // Mobility bonus - MINIMAL (piece activity) 
    if (materialDiff >= -50) { // Only consider mobility if not down material
        int aiMobility = evaluateMobility(board, aiColor, castling);
        int oppMobility = evaluateMobility(board, oppColor, castling);
        score += (aiMobility - oppMobility) / 50; // Heavily scaled down (was /15)
    }
    
    // Castling bonus - small encouragement (can't override material)
    if (materialDiff >= -100) {
        const int CASTLING_BONUS = 15; // Reduced from 25
        
        if (aiColor == Color::WHITE) {
            if (whiteHasCastled) {
                score += CASTLING_BONUS;
            }
            if (blackHasCastled) {
                score -= CASTLING_BONUS;
            }
        } else {
            if (blackHasCastled) {
                score += CASTLING_BONUS;
            }
            if (whiteHasCastled) {
                score -= CASTLING_BONUS;
            }
        }
        
        // Tiny bonus for keeping castling rights
        const int CASTLING_RIGHTS_BONUS = 5; // Reduced from 8
        
        if (aiColor == Color::WHITE && !whiteHasCastled && moveCount < 15) {
            if (castling.whiteKingSide || castling.whiteQueenSide) {
                score += CASTLING_RIGHTS_BONUS;
            }
        } else if (aiColor == Color::BLACK && !blackHasCastled && moveCount < 15) {
            if (castling.blackKingSide || castling.blackQueenSide) {
                score += CASTLING_RIGHTS_BONUS;
            }
        }
        
        if (oppColor == Color::WHITE && !whiteHasCastled && moveCount < 15) {
            if (castling.whiteKingSide || castling.whiteQueenSide) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        } else if (oppColor == Color::BLACK && !blackHasCastled && moveCount < 15) {
            if (castling.blackKingSide || castling.blackQueenSide) {
                score -= CASTLING_RIGHTS_BONUS;
            }
        }
    }
    
    // Minimal positional bonus: center control (tiny values)
    if (materialDiff >= -50) {
        const int CENTER_BONUS[8][8] = {
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 2, 2, 2, 2, 0, 0},  // Reduced from 5
            {0, 0, 2, 3, 3, 2, 0, 0},  // Reduced from 5,8,8,5
            {0, 0, 2, 3, 3, 2, 0, 0},  // Material must dominate
            {0, 0, 2, 2, 2, 2, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0}
        };
        
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece p = board.getPiece(r, c);
                if (p.isEmpty()) continue;
                
                int bonus = 0;
                
                // Center control, but not for kings in opening/middlegame
                if (p.type != PieceType::KING || moveCount > 30) {
                    bonus += CENTER_BONUS[r][c];
                }
                
                if (p.color == aiColor) {
                    score += bonus;
                } else {
                    score -= bonus;
                }
            }
        }
    }
    
    // Endgame evaluation - only when material is very low
    if (isEndgame(board)) {
        int endgameBonus = evaluateEndgame(board, aiColor, castling);
        score += endgameBonus;
    }
    
    return score;
}

// Check if we're in endgame (few pieces left)
bool EvaluatorV2::isEndgame(const Board& board) {
    int totalPieces = 0;
    int totalQueensRooks = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.type != PieceType::KING) {
                totalPieces++;
                if (p.type == PieceType::QUEEN || p.type == PieceType::ROOK) {
                    totalQueensRooks++;
                }
            }
        }
    }
    
    // Endgame if: very few pieces OR few heavy pieces
    return totalPieces <= 10 || totalQueensRooks <= 2;
}

Position EvaluatorV2::findKing(const Board& board, Color color) {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.type == PieceType::KING && p.color == color) {
                return Position(r, c);
            }
        }
    }
    return Position(-1, -1);
}

// Endgame evaluation for checkmate patterns
int EvaluatorV2::evaluateEndgame(const Board& board, Color aiColor, const CastlingRights& castling) {
    Color oppColor = (aiColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    int aiMaterial = countMaterial(board, aiColor);
    int oppMaterial = countMaterial(board, oppColor);
    
    int score = 0;
    
    // PREVENT opponent pawn promotion - this is critical!
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != oppColor || p.type != PieceType::PAWN) continue;
            
            // Check how close opponent pawn is to promotion
            int oppAdvancedRank = (oppColor == Color::WHITE) ? (7 - r) : r;
            if (oppAdvancedRank >= 4) {
                // HEAVILY penalize opponent's advanced pawns
                score -= oppAdvancedRank * 50;
                
                // MASSIVE penalty if opponent pawn is about to promote
                if (oppAdvancedRank >= 6) {
                    score -= 200;
                }
            }
        }
    }
    
    // If we have significantly more material, push for checkmate
    if (aiMaterial - oppMaterial > 300) { // Up by more than a minor piece
        Position oppKing = findKing(board, oppColor);
        Position aiKing = findKing(board, aiColor);
        
        if (oppKing.isValid()) {
            // Push enemy king to edge of board
            int oppKingFile = oppKing.col;
            int oppKingRank = oppKing.row;
            
            int distToEdge = std::min({oppKingFile, 7 - oppKingFile, oppKingRank, 7 - oppKingRank});
            score += (7 - distToEdge) * 30; // Reward pushing king to edge
            
            // Bring our king closer for checkmate
            if (aiKing.isValid()) {
                int kingDistance = std::abs(aiKing.row - oppKing.row) + std::abs(aiKing.col - oppKing.col);
                score += (14 - kingDistance) * 20; // Reward king proximity
            }
            
            // Strongly encourage pawn promotion when winning
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(r, c);
                    if (p.isEmpty() || p.color != aiColor || p.type != PieceType::PAWN) continue;
                    
                    // Check if pawn is advanced and relatively safe
                    int advancedRank = (aiColor == Color::WHITE) ? (7 - r) : r;
                    if (advancedRank >= 4) {
                        // Heavily reward advanced pawns in endgame
                        score += advancedRank * 40;
                        
                        // Extra bonus if close to promotion
                        if (advancedRank >= 6) {
                            score += 150;
                        }
                        
                        // Check if pawn is protected
                        bool protected_pawn = false;
                        
                        // Check diagonal protection
                        int pawnDirection = (aiColor == Color::WHITE) ? -1 : 1;
                        for (int dc = -1; dc <= 1; dc += 2) {
                            Position protectorPos(r + pawnDirection, c + dc);
                            if (protectorPos.isValid()) {
                                Piece protector = board.getPiece(protectorPos.row, protectorPos.col);
                                if (!protector.isEmpty() && protector.color == aiColor && 
                                    protector.type == PieceType::PAWN) {
                                    protected_pawn = true;
                                    break;
                                }
                            }
                        }
                        
                        // Check if king protects pawn
                        if (aiKing.isValid()) {
                            int distKingToPawn = std::max(std::abs(aiKing.row - r), std::abs(aiKing.col - c));
                            if (distKingToPawn <= 1) {
                                protected_pawn = true;
                            }
                        }
                        
                        if (protected_pawn) {
                            score += 50; // Bonus for protected advanced pawn
                        }
                    }
                }
            }
            
            // Count our queens and rooks to determine mating potential
            int queens = 0, rooks = 0;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    Piece p = board.getPiece(r, c);
                    if (p.isEmpty() || p.color != aiColor) continue;
                    if (p.type == PieceType::QUEEN) queens++;
                    if (p.type == PieceType::ROOK) rooks++;
                }
            }
            
            // If we have sufficient mating material, aggressively pursue checkmate
            if (queens >= 1 || rooks >= 2 || (queens >= 1 && rooks >= 1)) {
                // Extra pressure - coordinate pieces to corner the king
                score += 100;
                
                // Simplified: just reward having mating material
                // Proper king mobility evaluation would require move generation
            }
        }
    }
    
    return score;
}

int EvaluatorV2::countMaterial(const Board& board, Color color) {
    int material = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (!p.isEmpty() && p.color == color) {
                int typeIdx = static_cast<int>(p.type);
                // Bounds check to prevent memory access errors
                if (typeIdx >= 0 && typeIdx < 7) {
                    material += PIECE_VALUES[typeIdx];
                }
            }
        }
    }
    
    return material;
}

// Check if a piece is on its starting square
bool EvaluatorV2::isPieceOnStartingSquare(PieceType type, Color color, Position pos) {
    if (color == Color::WHITE) {
        switch (type) {
            case PieceType::KNIGHT:
                return pos.row == 0 && (pos.col == 1 || pos.col == 6);
            case PieceType::BISHOP:
                return pos.row == 0 && (pos.col == 2 || pos.col == 5);
            case PieceType::ROOK:
                return pos.row == 0 && (pos.col == 0 || pos.col == 7);
            case PieceType::QUEEN:
                return pos.row == 0 && pos.col == 3;
            default:
                return false;
        }
    } else {
        switch (type) {
            case PieceType::KNIGHT:
                return pos.row == 7 && (pos.col == 1 || pos.col == 6);
            case PieceType::BISHOP:
                return pos.row == 7 && (pos.col == 2 || pos.col == 5);
            case PieceType::ROOK:
                return pos.row == 7 && (pos.col == 0 || pos.col == 7);
            case PieceType::QUEEN:
                return pos.row == 7 && pos.col == 3;
            default:
                return false;
        }
    }
}

// Count undeveloped pieces (knights, bishops still on starting squares)
int EvaluatorV2::countUndevelopedPieces(const Board& board, Color color) {
    int undeveloped = 0;
    
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) continue;
            
            Position pos(r, c);
            
            // Count knights and bishops on starting squares
            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (isPieceOnStartingSquare(p.type, color, pos)) {
                    undeveloped++;
                }
            }
        }
    }
    
    return undeveloped;
}

// Check if rooks have moved from starting position
bool EvaluatorV2::isRookDeveloped(const Board& board, Color color) {
    int startRow = (color == Color::WHITE) ? 0 : 7;
    
    // Check if either rook is off its starting square
    Piece leftRook = board.getPiece(startRow, 0);
    Piece rightRook = board.getPiece(startRow, 7);
    
    bool leftRookMoved = leftRook.isEmpty() || leftRook.type != PieceType::ROOK || leftRook.color != color;
    bool rightRookMoved = rightRook.isEmpty() || rightRook.type != PieceType::ROOK || rightRook.color != color;
    
    return leftRookMoved || rightRookMoved;
}

// Evaluate piece development - reward getting pieces off back rank
int EvaluatorV2::evaluatePieceDevelopment(const Board& board, Color color, int moveCount) {
    int development = 0;
    int undevelopedCount = countUndevelopedPieces(board, color);
    bool hasUndevelopedMinors = undevelopedCount > 0;
    bool rooksDeveloped = isRookDeveloped(board, color);
    
    // SMALL penalty for undeveloped pieces (can't override material)
    if (moveCount > 5) {
        development -= undevelopedCount * 10; // Reduced from 30
    }
    
    // SMALL penalty for moving rooks before minor development
    if (hasUndevelopedMinors && rooksDeveloped && moveCount < 15) {
        development -= 15; // Reduced from 40
    }
    
    // Early game development bonuses
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty() || p.color != color) continue;
            
            Position pos(r, c);
            
            // Small reward for developing knights and bishops
            if (p.type == PieceType::KNIGHT || p.type == PieceType::BISHOP) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development += 10; // Reduced from 25
                } else if (moveCount > 8) {
                    development -= 8; // Reduced from 20
                }
            }
            
            // Tiny encouragement for central pawn advancement
            if (p.type == PieceType::PAWN && moveCount < 10) {
                if (color == Color::WHITE) {
                    if ((c == 3 || c == 4) && r >= 3) {
                        development += 6; // Reduced from 15
                    }
                } else {
                    if ((c == 3 || c == 4) && r <= 4) {
                        development += 6; // Reduced from 15
                    }
                }
            }
            
            // Small penalty for early queen development
            if (p.type == PieceType::QUEEN && moveCount < 10) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 10; // Reduced from 25
                }
            }
            
            // Small penalty for moving rooks too early
            if (p.type == PieceType::ROOK && moveCount < 10 && hasUndevelopedMinors) {
                if (!isPieceOnStartingSquare(p.type, color, pos)) {
                    development -= 12; // Reduced from 35
                }
            }
        }
    }
    
    return development;
}

// Evaluate piece coordination: pieces that attack/defend other pieces
// Small bonuses to favor coordinated, active play without overriding material
int EvaluatorV2::evaluatePieceCoordination(const Board& board, Color color) {
    int coordination = 0;
    Color oppColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    
    // For each piece of our color, check what it attacks/defends
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            Piece piece = board.getPiece(r, c);
            if (piece.isEmpty() || piece.color != color) continue;
            
            Position from(r, c);
            
            // Check all squares this piece can attack/defend
            for (int tr = 0; tr < 8; tr++) {
                for (int tc = 0; tc < 8; tc++) {
                    Position to(tr, tc);
                    if (r == tr && c == tc) continue;
                    
                    Piece target = board.getPiece(tr, tc);
                    if (target.isEmpty()) continue;
                    
                    // Check if this piece can attack/defend that square
                    // Simple check: would moving there be legal in terms of piece movement?
                    bool canReach = false;
                    
                    switch (piece.type) {
                        case PieceType::PAWN: {
                            // Pawns attack diagonally
                            int direction = (color == Color::WHITE) ? 1 : -1;
                            if (to.row == from.row + direction && abs(to.col - from.col) == 1) {
                                canReach = true;
                            }
                            break;
                        }
                        case PieceType::KNIGHT: {
                            int dr = abs(to.row - from.row);
                            int dc = abs(to.col - from.col);
                            if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
                                canReach = true;
                            }
                            break;
                        }
                        case PieceType::BISHOP: {
                            if (abs(to.row - from.row) == abs(to.col - from.col)) {
                                // Check diagonal is clear
                                int dr = (to.row > from.row) ? 1 : -1;
                                int dc = (to.col > from.col) ? 1 : -1;
                                bool clear = true;
                                int steps = abs(to.row - from.row) - 1;
                                for (int i = 1; i <= steps; i++) {
                                    if (!board.getPiece(from.row + i*dr, from.col + i*dc).isEmpty()) {
                                        clear = false;
                                        break;
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::ROOK: {
                            if (to.row == from.row || to.col == from.col) {
                                // Check line is clear
                                bool clear = true;
                                if (to.row == from.row) {
                                    int start = std::min(from.col, to.col) + 1;
                                    int end = std::max(from.col, to.col);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(from.row, i).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                } else {
                                    int start = std::min(from.row, to.row) + 1;
                                    int end = std::max(from.row, to.row);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(i, from.col).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::QUEEN: {
                            // Queen = rook + bishop
                            if (to.row == from.row || to.col == from.col) {
                                // Rook-like movement
                                bool clear = true;
                                if (to.row == from.row) {
                                    int start = std::min(from.col, to.col) + 1;
                                    int end = std::max(from.col, to.col);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(from.row, i).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                } else {
                                    int start = std::min(from.row, to.row) + 1;
                                    int end = std::max(from.row, to.row);
                                    for (int i = start; i < end; i++) {
                                        if (!board.getPiece(i, from.col).isEmpty()) {
                                            clear = false;
                                            break;
                                        }
                                    }
                                }
                                canReach = clear;
                            } else if (abs(to.row - from.row) == abs(to.col - from.col)) {
                                // Bishop-like movement
                                int dr = (to.row > from.row) ? 1 : -1;
                                int dc = (to.col > from.col) ? 1 : -1;
                                bool clear = true;
                                int steps = abs(to.row - from.row) - 1;
                                for (int i = 1; i <= steps; i++) {
                                    if (!board.getPiece(from.row + i*dr, from.col + i*dc).isEmpty()) {
                                        clear = false;
                                        break;
                                    }
                                }
                                canReach = clear;
                            }
                            break;
                        }
                        case PieceType::KING: {
                            if (abs(to.row - from.row) <= 1 && abs(to.col - from.col) <= 1) {
                                canReach = true;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    
                    if (canReach) {
                        if (target.color == color) {
                            // Defending a friendly piece: small bonus
                            // More valuable pieces defended = bigger bonus
                            int defendValue = PIECE_VALUES[static_cast<int>(target.type)] / 200;
                            coordination += std::min(5, defendValue); // Cap at 5 points
                        } else {
                            // Attacking an enemy piece: tiny bonus for activity
                            coordination += 2;
                        }
                    }
                }
            }
        }
    }
    
    return coordination;
}

// Evaluate piece mobility - count number of legal moves
int EvaluatorV2::evaluateMobility(const Board& board, Color color, const CastlingRights& castling) {
    // Simplified mobility - just return 0 since full move generation is complex
    // and mobility is heavily scaled down anyway (/ 50)
    // TODO: Implement proper mobility evaluation if needed
    return 0;
}

} // namespace V2
} // namespace Chess
