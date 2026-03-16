#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <chrono>

namespace Chess {
namespace V1 {

// Initialize killer moves array
std::array<std::array<Move, 2>, Search::MAX_DEPTH> Search::killerMoves = {};

// Initialize transposition table
std::unordered_map<uint64_t, TTEntry> Search::transpositionTable;

// Initialize time control variables
long long Search::startTimeMs = 0;
int Search::maxSearchTimeMs = 0;

bool Search::isTimeUp() {
    if (maxSearchTimeMs <= 0) return false;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count() - startTimeMs;
    
    return elapsed >= maxSearchTimeMs;
}

void Search::clearCaches() {
    transpositionTable.clear();
    killerMoves = {};
}

// Simple Zobrist-like hash for board positions
uint64_t Search::computeZobristKey(const Board& board) {
    uint64_t key = 0;
    
    for (int sq = 0; sq < 64; sq++) {
        Piece piece = board.getPiece(sq);
        if (!piece.isEmpty()) {
            // Simple hash: piece type * color * square
            uint64_t pieceHash = (static_cast<uint64_t>(piece.type) + 1) * 13 +
                                 (static_cast<uint64_t>(piece.color) + 1) * 7;
            key ^= (pieceHash * (sq + 1) * 31);
        }
    }
    
    // Include current player
    key ^= (board.getCurrentPlayer() == Color::WHITE) ? 0x123456789ABCDEF0ULL : 0x0FEDCBA987654321ULL;
    
    return key;
}

bool Search::probeTranspositionTable(uint64_t key, int depth, int alpha, int beta, TTEntry& entry) {
    auto it = transpositionTable.find(key);
    if (it == transpositionTable.end()) {
        return false;
    }
    
    entry = it->second;
    
    // Only use entry if it was searched to at least the same depth
    if (entry.depth < depth) {
        return false;
    }
    
    // Check if we can use this score
    if (entry.flag == TTEntry::EXACT) {
        return true;
    } else if (entry.flag == TTEntry::LOWER_BOUND && entry.score >= beta) {
        return true;
    } else if (entry.flag == TTEntry::UPPER_BOUND && entry.score <= alpha) {
        return true;
    }
    
    return false;
}

void Search::storeTranspositionTable(uint64_t key, int score, int depth, TTEntry::Flag flag, const Move& bestMove) {
    // Limit table size
    if (transpositionTable.size() >= MAX_TT_SIZE && transpositionTable.find(key) == transpositionTable.end()) {
        // Table full and this is a new entry - skip it
        return;
    }
    
    TTEntry entry;
    entry.zobristKey = key;
    entry.score = score;
    entry.depth = depth;
    entry.flag = flag;
    entry.bestMove = bestMove;
    
    transpositionTable[key] = entry;
}

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

int Search::getMoveScore(const Board& board, const Move& move, int ply, const Move& ttMove) {
    int score = 0;
    
    // Transposition table move gets highest priority
    if (ttMove.isValid() && move == ttMove) {
        return 1000000;
    }
    
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

void Search::orderMoves(Board& board, std::vector<Move>& moves, int ply, const Move& ttMove) {
    // Score and sort moves
    std::vector<std::pair<int, Move>> scoredMoves;
    scoredMoves.reserve(moves.size());
    
    for (const Move& move : moves) {
        int score = getMoveScore(board, move, ply, ttMove);
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

int Search::quiescence(Board& board, int alpha, int beta, int& nodesSearched, int ply, bool& timeUp) {
    nodesSearched++;
    
    // Check time limit
    if (isTimeUp()) {
        timeUp = true;
        return Evaluation::evaluateMaterialOnly(board);
    }
    
    // Limit quiescence depth to prevent explosion
    const int MAX_QUIESCENCE_DEPTH = 8;
    if (ply >= MAX_QUIESCENCE_DEPTH) {
        return Evaluation::evaluateMaterialOnly(board);
    }
    
    // Stand pat score - use fast material-only evaluation
    int standPat = Evaluation::evaluateMaterialOnly(board);
    
    // Beta cutoff
    if (standPat >= beta) {
        return beta;
    }
    
    // Update alpha
    if (standPat > alpha) {
        alpha = standPat;
    }
    
    // Generate and search only captures
    std::vector<Move> captures;
    std::vector<Move> allMoves = board.generateLegalMoves();
    
    for (const Move& move : allMoves) {
        if (move.isCapture()) {
            captures.push_back(move);
        }
    }
    
    // If no captures, return stand pat
    if (captures.empty()) {
        return standPat;
    }
    
    // Order captures by MVV-LVA
    Move emptyMove;
    orderMoves(board, captures, ply, emptyMove);
    
    // Delta pruning - if we're so far behind that even capturing queen won't help, prune
    const int QUEEN_VALUE = 900;
    const int DELTA_MARGIN = QUEEN_VALUE + 200; // Queen value + safety margin
    if (standPat + DELTA_MARGIN < alpha) {
        // Even capturing the most valuable piece won't improve alpha
        return alpha;
    }
    
    // Search captures
    for (const Move& move : captures) {
        board.makeMove(move);
        int score = -quiescence(board, -beta, -alpha, nodesSearched, ply + 1, timeUp);
        board.unmakeMove();
        
        if (timeUp) {
            return alpha; // Time up, return current alpha
        }
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

int Search::minimax(Board& board, int depth, int alpha, int beta, bool maximizing, int& nodesSearched, int ply, bool& timeUp) {
    nodesSearched++;
    
    // Check time limit
    if (isTimeUp()) {
        timeUp = true;
        return Evaluation::evaluateMaterialOnly(board);
    }
    
    // Compute position key for transposition table
    uint64_t positionKey = computeZobristKey(board);
    
    // Probe transposition table
    TTEntry ttEntry;
    if (probeTranspositionTable(positionKey, depth, alpha, beta, ttEntry)) {
        return ttEntry.score;
    }
    
    // Terminal node or depth reached - use quiescence search
    if (depth == 0) {
        int score = quiescence(board, alpha, beta, nodesSearched, ply, timeUp);
        if (!timeUp) {
            storeTranspositionTable(positionKey, score, 0, TTEntry::EXACT, Move());
        }
        return score;
    }
    
    // Check for game over
    if (board.isCheckmate() || board.isStalemate() || board.isDraw()) {
        int score = Evaluation::evaluate(board, ply);
        storeTranspositionTable(positionKey, score, depth, TTEntry::EXACT, Move());
        return score;
    }
    
    std::vector<Move> moves = board.generateLegalMoves();
    
    if (moves.empty()) {
        // No legal moves - either checkmate or stalemate
        int score = Evaluation::evaluate(board, ply);
        storeTranspositionTable(positionKey, score, depth, TTEntry::EXACT, Move());
        return score;
    }
    
    // Order moves for better pruning
    orderMoves(board, moves, ply, ttEntry.bestMove);
    
    if (maximizing) {
        int maxEval = -INFINITY_SCORE;
        Move bestMove;
        TTEntry::Flag flag = TTEntry::UPPER_BOUND;
        int moveCount = 0;
        
        for (const Move& move : moves) {
            moveCount++;
            
            board.makeMove(move);
            int eval;
            
            // Late move reductions - search less promising moves at reduced depth
            // Only apply after we've searched a few moves and at sufficient depth
            bool doLMR = moveCount > 4 && depth >= 3 && !move.isCapture() && 
                         !board.isInCheck(board.getCurrentPlayer()) && !move.isPromotion();
            
            if (doLMR) {
                // Search at reduced depth first
                eval = minimax(board, depth - 2, alpha, beta, false, nodesSearched, ply + 1, timeUp);
                
                // If it looks good, re-search at full depth
                if (eval > alpha && !timeUp) {
                    eval = minimax(board, depth - 1, alpha, beta, false, nodesSearched, ply + 1, timeUp);
                }
            } else {
                // Normal search
                eval = minimax(board, depth - 1, alpha, beta, false, nodesSearched, ply + 1, timeUp);
            }
            
            board.unmakeMove();
            
            if (timeUp) {
                return maxEval; // Time up, return best found so far
            }
            
            if (eval > maxEval) {
                maxEval = eval;
                bestMove = move;
                
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
                flag = TTEntry::LOWER_BOUND;
                break; // Beta cutoff
            }
        }
        
        // Determine flag for TT entry
        if (maxEval > alpha) {
            flag = TTEntry::EXACT;
        }
        
        storeTranspositionTable(positionKey, maxEval, depth, flag, bestMove);
        return maxEval;
    } else {
        int minEval = INFINITY_SCORE;
        Move bestMove;
        TTEntry::Flag flag = TTEntry::UPPER_BOUND;
        int moveCount = 0;
        
        for (const Move& move : moves) {
            moveCount++;
            
            board.makeMove(move);
            int eval;
            
            // Late move reductions
            bool doLMR = moveCount > 4 && depth >= 3 && !move.isCapture() && 
                         !board.isInCheck(board.getCurrentPlayer()) && !move.isPromotion();
            
            if (doLMR) {
                // Search at reduced depth first
                eval = minimax(board, depth - 2, alpha, beta, true, nodesSearched, ply + 1, timeUp);
                
                // If it looks good, re-search at full depth
                if (eval < beta && !timeUp) {
                    eval = minimax(board, depth - 1, alpha, beta, true, nodesSearched, ply + 1, timeUp);
                }
            } else {
                // Normal search
                eval = minimax(board, depth - 1, alpha, beta, true, nodesSearched, ply + 1, timeUp);
            }
            
            board.unmakeMove();
            
            if (timeUp) {
                return minEval; // Time up, return best found so far
            }
            
            if (eval < minEval) {
                minEval = eval;
                bestMove = move;
                
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
                flag = TTEntry::LOWER_BOUND;
                break; // Alpha cutoff
            }
        }
        
        // Determine flag for TT entry
        if (minEval < beta) {
            flag = TTEntry::EXACT;
        }
        
        storeTranspositionTable(positionKey, minEval, depth, flag, bestMove);
        return minEval;
    }
}

SearchResult Search::findBestMove(
    Board& board,
    int maxDepth,
    std::function<void(const SearchResult&)> progressCallback,
    int maxTimeMs
) {
    SearchResult finalResult;
    
    // Initialize time control
    if (maxTimeMs > 0) {
        auto now = std::chrono::steady_clock::now();
        startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        maxSearchTimeMs = maxTimeMs;
    } else {
        maxSearchTimeMs = 0;
    }
    
    if (maxDepth < 1) maxDepth = 1;
    if (maxDepth > 20) maxDepth = 20;
    
    std::vector<Move> legalMoves = board.generateLegalMoves();
    
    if (legalMoves.empty()) {
        return finalResult; // No legal moves
    }
    
    bool timeUp = false;
    
    // Iterative deepening: search depth 1, 2, 3, ... up to maxDepth
    for (int currentDepth = 1; currentDepth <= maxDepth && !timeUp; currentDepth++) {
        SearchResult result;
        result.depth = currentDepth;
        result.nodesSearched = 0;
        
        int bestScore = -INFINITY_SCORE;
        Move bestMove;
        
        // Aspiration windows - use narrower search window based on previous score
        int alpha, beta;
        if (currentDepth >= 3 && finalResult.bestMove.isValid()) {
            // Use narrow window around previous score
            const int ASPIRATION_WINDOW = 50;
            alpha = finalResult.score - ASPIRATION_WINDOW;
            beta = finalResult.score + ASPIRATION_WINDOW;
        } else {
            alpha = -INFINITY_SCORE;
            beta = INFINITY_SCORE;
        }
        
        // Aspiration window re-search loop
        bool needResearch = false;
        do {
            bestScore = -INFINITY_SCORE;
            needResearch = false;
            
            // Order moves at root (use previous iteration's best move first)
            Move emptyMove;
            if (currentDepth > 1 && finalResult.bestMove.isValid()) {
                // Move the previous best move to the front
                auto it = std::find(legalMoves.begin(), legalMoves.end(), finalResult.bestMove);
                if (it != legalMoves.end()) {
                    std::rotate(legalMoves.begin(), it, it + 1);
                }
            } else {
                // First iteration - order moves
                orderMoves(board, legalMoves, 0, emptyMove);
            }
            
            for (const Move& move : legalMoves) {
                board.makeMove(move);
                int score = minimax(board, currentDepth - 1, alpha, beta, false, result.nodesSearched, 1, timeUp);
                board.unmakeMove();
                
                if (timeUp) {
                    // Time is up, stop searching and use the best result we have
                    // from the previous completed depth
                    break;
                }
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
                
                alpha = std::max(alpha, score);
            }
            
            // If time ran out during this depth, don't update results
            // Keep using the previous depth's results
            if (timeUp) {
                needResearch = false;
                break;
            }
            
            // Check if we need to re-search with wider window
            if (currentDepth >= 3 && finalResult.bestMove.isValid()) {
                if (bestScore <= alpha - (currentDepth >= 3 ? 50 : 0)) {
                    // Score dropped below window - widen downward
                    alpha = -INFINITY_SCORE;
                    needResearch = true;
                } else if (bestScore >= beta) {
                    // Score rose above window - widen upward
                    beta = INFINITY_SCORE;
                    needResearch = true;
                }
            }
        } while (needResearch && !timeUp);
        
        // Only update finalResult if we completed this depth
        if (!timeUp) {
            result.bestMove = bestMove;
            result.score = bestScore;
            finalResult = result;
            
            // Call progress callback with this depth's result
            if (progressCallback) {
                progressCallback(result);
            }
        }
    }
    
    return finalResult;
}

} // namespace V1
} // namespace Chess
