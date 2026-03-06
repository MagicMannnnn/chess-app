#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>

namespace Chess {
namespace V1 {

int Search::minimax(Board& board, int depth, int alpha, int beta, bool maximizing, int& nodesSearched) {
    nodesSearched++;
    
    // Terminal node
    if (depth == 0 || board.isCheckmate() || board.isStalemate() || board.isDraw()) {
        return Evaluation::evaluate(board);
    }
    
    std::vector<Move> moves = board.generateLegalMoves();
    
    if (moves.empty()) {
        // No legal moves - either checkmate or stalemate
        return Evaluation::evaluate(board);
    }
    
    if (maximizing) {
        int maxEval = -INFINITY_SCORE;
        
        for (const Move& move : moves) {
            board.makeMove(move);
            int eval = minimax(board, depth - 1, alpha, beta, false, nodesSearched);
            board.unmakeMove();
            
            maxEval = std::max(maxEval, eval);
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
            int eval = minimax(board, depth - 1, alpha, beta, true, nodesSearched);
            board.unmakeMove();
            
            minEval = std::min(minEval, eval);
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
        
        for (const Move& move : legalMoves) {
            board.makeMove(move);
            int score = minimax(board, currentDepth - 1, alpha, beta, false, result.nodesSearched);
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
