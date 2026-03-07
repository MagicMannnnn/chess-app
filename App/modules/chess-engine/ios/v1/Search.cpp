#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>

namespace Chess {
namespace V1 {

// Initialize killer moves array
std::array<std::array<Move, 2>, Search::MAX_DEPTH> Search::killerMoves = {};

int Search::getPieceValue(PieceType type) {
    switch (type) {
        case PieceType::PAWN:   return 100;
        case PieceType::KNIGHT: return 320;
        case PieceType::BISHOP: return 330;
        case PieceType::ROOK:   return 500;
        case PieceType::QUEEN:  return 900;
        case PieceType::KING:   return 20000;
        default: return 0;
    }
}

int Search::getMoveScore(const Board& board, const Move& move, int ply) {
    int score = 0;
    
    // MVV-LVA: Most Valuable Victim - Least Valuable Attacker
    if (move.isCapture()) {
        Piece fromPiece = board.getPiece(move.getFrom());
        Piece toPiece = board.getPiece(move.getTo());
        
        // Victim value * 10 - attacker value (prioritize capturing valuable pieces with cheap pieces)
        score = getPieceValue(toPiece.type) * 10 - getPieceValue(fromPiece.type);
        score += 10000; // Base score for captures
    }
    // Promotions are very valuable
    else if (move.isPromotion()) {
        score = 9000 + getPieceValue(move.getPromotion());
    }
    // Killer moves (moves that caused cutoffs at this depth)
    else if (ply < MAX_DEPTH) {
        if (move == killerMoves[ply][0]) {
            score = 8000;
        } else if (move == killerMoves[ply][1]) {
            score = 7000;
        }
    }
    
    return score;
}

void Search::orderMoves(Board& board, std::vector<Move>& moves, int ply) {
    // Score and sort moves
    std::vector<std::pair<int, Move>> scoredMoves;
    scoredMoves.reserve(moves.size());
    
    for (const Move& move : moves) {
        int score = getMoveScore(board, move, ply);
        scoredMoves.emplace_back(score, move);
    }
    
    // Sort by score (highest first)
    std::sort(scoredMoves.begin(), scoredMoves.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Update moves vector
    for (size_t i = 0; i < moves.size(); i++) {
        moves[i] = scoredMoves[i].second;
    }
}

int Search::minimax(Board& board, int depth, int alpha, int beta, bool maximizing, int& nodesSearched, int ply) {
    nodesSearched++;
    
    // Terminal node
    if (depth == 0 || board.isCheckmate() || board.isStalemate() || board.isDraw()) {
        return Evaluation::evaluate(board, ply);
    }
    
    std::vector<Move> moves = board.generateLegalMoves();
    
    if (moves.empty()) {
        // No legal moves - either checkmate or stalemate
        return Evaluation::evaluate(board, ply);
    }
    
    // Order moves for better pruning
    orderMoves(board, moves, ply);
    
    if (maximizing) {
        int maxEval = -INFINITY_SCORE;
        
        for (const Move& move : moves) {
            board.makeMove(move);
            int eval = minimax(board, depth - 1, alpha, beta, false, nodesSearched, ply + 1);
            board.unmakeMove();
            
            if (eval > maxEval) {
                maxEval = eval;
                
                // Store killer move if it caused a cutoff and isn't a capture
                if (eval >= beta && !move.isCapture() && ply < MAX_DEPTH) {
                    // Shift killer moves down
                    if (killerMoves[ply][0] != move) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }
                }
            }
            
            alpha = std::max(alpha, eval);
            
            if (beta <= alpha) {
                break; // Beta cutoff
            }
        }
        
        return maxEval;
    } else {
        int minEval = INFINITY_SCORE;
        
        for (const Move& move : moves) {
            board.makeMove(move);
            int eval = minimax(board, depth - 1, alpha, beta, true, nodesSearched, ply + 1);
            board.unmakeMove();
            
            if (eval < minEval) {
                minEval = eval;
                
                // Store killer move if it caused a cutoff and isn't a capture
                if (eval <= alpha && !move.isCapture() && ply < MAX_DEPTH) {
                    // Shift killer moves down
                    if (killerMoves[ply][0] != move) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }
                }
            }
            
            beta = std::min(beta, eval);
            
            if (beta <= alpha) {
                break; // Alpha cutoff
            }
        }
        
        return minEval;
    }
}

SearchResult Search::findBestMove(
    Board& board,
    int maxDepth,
    std::function<void(const SearchResult&)> progressCallback
) {
    SearchResult finalResult;
    
    if (maxDepth < 1) maxDepth = 1;
    if (maxDepth > 20) maxDepth = 20;
    
    std::vector<Move> legalMoves = board.generateLegalMoves();
    
    if (legalMoves.empty()) {
        return finalResult; // No legal moves
    }
    
    // Iterative deepening: search depth 1, 2, 3, ... up to maxDepth
    for (int currentDepth = 1; currentDepth <= maxDepth; currentDepth++) {
        SearchResult result;
        result.depth = currentDepth;
        result.nodesSearched = 0;
        
        int bestScore = -INFINITY_SCORE;
        Move bestMove;
        
        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;
        
        // Order moves at root (use previous iteration's best move first)
        if (currentDepth > 1 && finalResult.bestMove.isValid()) {
            // Move the previous best move to the front
            auto it = std::find(legalMoves.begin(), legalMoves.end(), finalResult.bestMove);
            if (it != legalMoves.end()) {
                std::rotate(legalMoves.begin(), it, it + 1);
            }
        } else {
            // First iteration - order moves
            orderMoves(board, legalMoves, 0);
        }
        
        for (const Move& move : legalMoves) {
            board.makeMove(move);
            int score = minimax(board, currentDepth - 1, alpha, beta, false, result.nodesSearched, 1);
            board.unmakeMove();
            
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            
            alpha = std::max(alpha, score);
        }
        
        result.bestMove = bestMove;
        result.score = bestScore;
        finalResult = result;
        
        // Call progress callback with this depth's result
        if (progressCallback) {
            progressCallback(result);
        }
    }
    
    return finalResult;
}

} // namespace V1
} // namespace Chess
