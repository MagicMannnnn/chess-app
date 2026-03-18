#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <array>
#include <cstdint>
#include <atomic>
#include <iostream>

namespace Chess {
namespace V2 {

namespace {
constexpr int MATE_SCORE = 30000;
constexpr int TIME_CHECK_MASK = 255; // check time/cancel every 256 nodes
constexpr int ASPIRATION_WINDOW = 50;

inline CastlingRights buildCastlingRights(const Board& board) {
    uint8_t castlingRights = board.getCastlingRights();

    CastlingRights rights;
    rights.whiteKingSide  = (castlingRights & Chess::CastlingRights::WHITE_KINGSIDE) != 0;
    rights.whiteQueenSide = (castlingRights & Chess::CastlingRights::WHITE_QUEENSIDE) != 0;
    rights.blackKingSide  = (castlingRights & Chess::CastlingRights::BLACK_KINGSIDE) != 0;
    rights.blackQueenSide = (castlingRights & Chess::CastlingRights::BLACK_QUEENSIDE) != 0;
    return rights;
}

inline int colorIndex(Color color) {
    return color == Color::WHITE ? 0 : 1;
}

inline int squareIndex(const Position& pos) {
    return pos.row * 8 + pos.col;
}

inline int evaluatePosition(const Board& board) {
    return EvaluatorV2::evaluate(
        board,
        board.getCurrentPlayer(),
        buildCastlingRights(board),
        false,
        false,
        board.getFullmoveNumber()
    );
}

inline int terminalScore(Board& board, int ply) {
    if (board.isCheckmate()) {
        return -MATE_SCORE + ply;
    }

    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    return evaluatePosition(board);
}
} // namespace

// Initialize killer moves array
std::array<std::array<Move, 2>, Search::MAX_DEPTH> Search::killerMoves = {};

// Initialize history heuristic
std::array<std::array<std::array<int, 64>, 64>, 2> Search::historyHeuristic = {};

// Track last best move and score for move ordering
Move Search::lastBestMove = Move();
int Search::lastBestScore = 0;

// Transposition table
std::unordered_map<uint64_t, TranspositionEntry> Search::transpositionTable;

// Initialize time control variables
long long Search::startTimeMs = 0;
int Search::maxSearchTimeMs = 0;
std::atomic<uint64_t> Search::activeSearchToken {0};
thread_local uint64_t Search::localSearchToken = 0;

uint64_t Search::hashBoard(const Board& board) {
    // Much cheaper fallback than hashing the FEN string every node.
    // This is not as strong as proper Zobrist hashing, but is significantly faster.
    uint64_t hash = 1469598103934665603ULL; // FNV offset basis

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = board.getPiece(r, c);
            if (p.isEmpty()) {
                continue;
            }

            uint64_t pieceCode = static_cast<uint64_t>(static_cast<int>(p.type) + 1);
            uint64_t colorCode = (p.color == Color::WHITE) ? 1ULL : 2ULL;
            uint64_t squareCode = static_cast<uint64_t>(r * 8 + c + 1);

            uint64_t value = pieceCode;
            value = value * 3ULL + colorCode;
            value = value * 67ULL + squareCode;

            hash ^= value;
            hash *= 1099511628211ULL; // FNV prime
        }
    }

    hash ^= static_cast<uint64_t>(board.getCurrentPlayer() == Color::WHITE ? 1ULL : 2ULL);
    hash *= 1099511628211ULL;

    hash ^= static_cast<uint64_t>(board.getCastlingRights());
    hash *= 1099511628211ULL;

    return hash;
}

void Search::clearCaches() {
    transpositionTable.clear();

    for (auto& depthMoves : killerMoves) {
        depthMoves[0] = Move();
        depthMoves[1] = Move();
    }

    for (auto& colorTable : historyHeuristic) {
        for (auto& fromTable : colorTable) {
            fromTable.fill(0);
        }
    }

    lastBestMove = Move();
    lastBestScore = 0;
}

void Search::cancelActiveSearches() {
    activeSearchToken.fetch_add(1, std::memory_order_relaxed);
}

bool Search::isTimeUp() {
    if (localSearchToken != activeSearchToken.load(std::memory_order_relaxed)) {
        return true;
    }

    if (maxSearchTimeMs <= 0) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count() - startTimeMs;

    return elapsed >= maxSearchTimeMs;
}

int Search::getPieceValue(PieceType type) {
    switch (type) {
        case PieceType::PAWN:   return 100;
        case PieceType::KNIGHT: return 320;
        case PieceType::BISHOP: return 330;
        case PieceType::ROOK:   return 500;
        case PieceType::QUEEN:  return 900;
        case PieceType::KING:   return 20000;
        default:                return 0;
    }
}

int Search::getMoveScore(const Board& board, const Move& move, int ply, const Move& ttMove) {
    // TT move first
    if (ttMove.isValid() && move == ttMove) {
        return 2000000;
    }

    int score = 0;

    if (move.isPromotion()) {
        score += 900000 + getPieceValue(move.getPromotion());
    }

    if (move.isCapture()) {
        Piece fromPiece = board.getPiece(move.getFrom());
        Piece toPiece = board.getPiece(move.getTo());

        score += 1000000;
        score += getPieceValue(toPiece.type) * 10 - getPieceValue(fromPiece.type);
        return score;
    }

    if (ply < MAX_DEPTH) {
        if (move == killerMoves[ply][0]) {
            score += 800000;
        } else if (move == killerMoves[ply][1]) {
            score += 700000;
        }
    }

    Position from = move.getFrom();
    Position to = move.getTo();
    if (from.isValid() && to.isValid()) {
        const int side = colorIndex(board.getCurrentPlayer());
        score += historyHeuristic[side][squareIndex(from)][squareIndex(to)];
    }

    return score;
}

void Search::orderMoves(Board& board, std::vector<Move>& moves, int ply, const Move& ttMove) {
    struct ScoredMove {
        int score;
        Move move;
    };

    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(moves.size());

    for (const Move& move : moves) {
        scoredMoves.push_back({getMoveScore(board, move, ply, ttMove), move});
    }

    std::sort(
        scoredMoves.begin(),
        scoredMoves.end(),
        [](const ScoredMove& a, const ScoredMove& b) {
            return a.score > b.score;
        }
    );

    for (size_t i = 0; i < moves.size(); ++i) {
        moves[i] = scoredMoves[i].move;
    }
}

int Search::minimax(
    Board& board,
    int depth,
    int alpha,
    int beta,
    bool maximizing,
    int& nodesSearched,
    int ply,
    bool& timeUp
) {
    nodesSearched++;

    const int alphaOriginal = alpha;
    const int betaOriginal = beta;

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
    }

    const uint64_t hash = hashBoard(board);

    Move ttMove;
    auto ttIt = transpositionTable.find(hash);
    if (ttIt != transpositionTable.end()) {
        const TranspositionEntry& entry = ttIt->second;
        ttMove = entry.bestMove;

        if (entry.depth >= depth) {
            if (entry.type == TranspositionEntry::EXACT) {
                return entry.score;
            } else if (entry.type == TranspositionEntry::LOWER_BOUND) {
                alpha = std::max(alpha, entry.score);
            } else if (entry.type == TranspositionEntry::UPPER_BOUND) {
                beta = std::min(beta, entry.score);
            }

            if (alpha >= beta) {
                return entry.score;
            }
        }
    }

    if (depth == 0) {
        return evaluatePosition(board);
    }

    if (board.isDraw()) {
        return 0;
    }

    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        return terminalScore(board, ply);
    }

    orderMoves(board, moves, ply, ttMove);

    Move bestMove;
    int bestScore = maximizing ? -INFINITY_SCORE : INFINITY_SCORE;

    if (maximizing) {
        for (const Move& move : moves) {
            board.makeMove(move);
            const int score = minimax(
                board,
                depth - 1,
                alpha,
                beta,
                false,
                nodesSearched,
                ply + 1,
                timeUp
            );
            board.unmakeMove();

            if (timeUp) {
                return bestScore == -INFINITY_SCORE ? evaluatePosition(board) : bestScore;
            }

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }

            if (score > alpha) {
                alpha = score;
            }

            if (alpha >= beta) {
                if (!move.isCapture() && ply < MAX_DEPTH && move.isValid()) {
                    if (killerMoves[ply][0] != move) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }

                    Position from = move.getFrom();
                    Position to = move.getTo();
                    if (from.isValid() && to.isValid()) {
                        const int side = colorIndex(board.getCurrentPlayer());
                        historyHeuristic[side][squareIndex(from)][squareIndex(to)] += depth * depth;
                    }
                }
                break;
            }
        }
    } else {
        for (const Move& move : moves) {
            board.makeMove(move);
            const int score = minimax(
                board,
                depth - 1,
                alpha,
                beta,
                true,
                nodesSearched,
                ply + 1,
                timeUp
            );
            board.unmakeMove();

            if (timeUp) {
                return bestScore == INFINITY_SCORE ? evaluatePosition(board) : bestScore;
            }

            if (score < bestScore) {
                bestScore = score;
                bestMove = move;
            }

            if (score < beta) {
                beta = score;
            }

            if (alpha >= beta) {
                if (!move.isCapture() && ply < MAX_DEPTH && move.isValid()) {
                    if (killerMoves[ply][0] != move) {
                        killerMoves[ply][1] = killerMoves[ply][0];
                        killerMoves[ply][0] = move;
                    }

                    Position from = move.getFrom();
                    Position to = move.getTo();
                    if (from.isValid() && to.isValid()) {
                        const int side = colorIndex(board.getCurrentPlayer());
                        historyHeuristic[side][squareIndex(from)][squareIndex(to)] += depth * depth;
                    }
                }
                break;
            }
        }
    }

    if (transpositionTable.size() >= MAX_TT_SIZE) {
        transpositionTable.clear();
    }

    TranspositionEntry entry;
    entry.hash = hash;
    entry.bestMove = bestMove;
    entry.score = bestScore;
    entry.depth = depth;

    if (bestScore <= alphaOriginal) {
        entry.type = TranspositionEntry::UPPER_BOUND;
    } else if (bestScore >= betaOriginal) {
        entry.type = TranspositionEntry::LOWER_BOUND;
    } else {
        entry.type = TranspositionEntry::EXACT;
    }

    transpositionTable[hash] = entry;

    return bestScore;
}

SearchResult Search::findBestMove(
    Board& board,
    int maxDepth,
    std::function<void(const SearchResult&)> progressCallback,
    int maxTimeMs
) {
    SearchResult finalResult;

    localSearchToken = activeSearchToken.fetch_add(1, std::memory_order_relaxed) + 1;

    if (maxTimeMs > 0) {
        auto now = std::chrono::steady_clock::now();
        startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        maxSearchTimeMs = maxTimeMs;
    } else {
        maxSearchTimeMs = 0;
    }

    if (maxDepth < 1) {
        maxDepth = 1;
    }
    if (maxDepth > 20) {
        maxDepth = 20;
    }

    std::vector<Move> legalMoves = board.generateLegalMoves();

    if (legalMoves.empty()) {
        finalResult.depth = 0;
        finalResult.score = terminalScore(board, 0);
        finalResult.nodesSearched = 0;
        return finalResult;
    }

    finalResult.bestMove = legalMoves.front();
    finalResult.score = evaluatePosition(board);
    finalResult.depth = 0;
    finalResult.nodesSearched = 0;

    bool timeUp = false;

    for (int currentDepth = 1; currentDepth <= maxDepth && !timeUp; ++currentDepth) {
        SearchResult result;
        result.depth = currentDepth;
        result.nodesSearched = 0;

        int bestScore = -INFINITY_SCORE;
        Move bestMove = legalMoves.front();

        int alpha = -INFINITY_SCORE;
        int beta = INFINITY_SCORE;
        bool usingAspirationWindow = false;

        if (currentDepth >= 3 && finalResult.bestMove.isValid()) {
            alpha = finalResult.score - ASPIRATION_WINDOW;
            beta = finalResult.score + ASPIRATION_WINDOW;
            usingAspirationWindow = true;
        }

        bool needResearch = false;
        do {
            needResearch = false;
            timeUp = false;
            bestScore = -INFINITY_SCORE;
            bestMove = legalMoves.front();

            // First order by heuristics/TT
            Move rootTtMove;
            const uint64_t rootHash = hashBoard(board);
            auto ttIt = transpositionTable.find(rootHash);
            if (ttIt != transpositionTable.end()) {
                rootTtMove = ttIt->second.bestMove;
            }
            orderMoves(board, legalMoves, 0, rootTtMove);

            // Then strongly prioritize previous iteration PV move
            if (finalResult.bestMove.isValid()) {
                auto it = std::find(legalMoves.begin(), legalMoves.end(), finalResult.bestMove);
                if (it != legalMoves.end()) {
                    std::rotate(legalMoves.begin(), it, it + 1);
                }
            }

            for (const Move& move : legalMoves) {
                board.makeMove(move);
                const int score = minimax(
                    board,
                    currentDepth - 1,
                    alpha,
                    beta,
                    false,
                    result.nodesSearched,
                    1,
                    timeUp
                );
                board.unmakeMove();

                if (timeUp) {
                    break;
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }

                if (score > alpha) {
                    alpha = score;
                }
            }

            if (
                !timeUp &&
                usingAspirationWindow &&
                currentDepth >= 3 &&
                finalResult.bestMove.isValid()
            ) {
                if (bestScore <= finalResult.score - ASPIRATION_WINDOW) {
                    alpha = -INFINITY_SCORE;
                    beta = INFINITY_SCORE;
                    usingAspirationWindow = false;
                    needResearch = true;
                } else if (bestScore >= finalResult.score + ASPIRATION_WINDOW) {
                    alpha = -INFINITY_SCORE;
                    beta = INFINITY_SCORE;
                    usingAspirationWindow = false;
                    needResearch = true;
                }
            }
        } while (needResearch && !timeUp);

        if (!timeUp) {
            result.bestMove = bestMove;
            result.score = bestScore;
            finalResult = result;

            lastBestMove = bestMove;
            lastBestScore = bestScore;

            if (progressCallback) {
                progressCallback(result);
            }
        }
    }

    return finalResult;
}

SearchResult Search::findBestMoveAtDepth(
    Board& board,
    int depth,
    int maxTimeMs
) {
    SearchResult result;

    localSearchToken = activeSearchToken.fetch_add(1, std::memory_order_relaxed) + 1;

    result.depth = depth;
    result.nodesSearched = 0;

    if (maxTimeMs > 0) {
        auto now = std::chrono::steady_clock::now();
        startTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        maxSearchTimeMs = maxTimeMs;
    } else {
        maxSearchTimeMs = 0;
    }

    if (depth < 1) {
        depth = 1;
    }
    if (depth > 20) {
        depth = 20;
    }

    // Depth 1 marks the beginning of a new JS-managed iterative search.
    // Reset caches here so each new board position starts from a clean state.
    if (depth == 1) {
        clearCaches();
    }

    if (depth == 1) {
        std::cout
            << "V2::Search depth=1 position FEN=" << board.toFEN()
            << " currentPlayer=" << (board.getCurrentPlayer() == Color::WHITE ? "white" : "black")
            << std::endl;
    }

    std::vector<Move> legalMoves = board.generateLegalMoves();

    if (legalMoves.empty()) {
        result.score = terminalScore(board, 0);
        return result;
    }

    result.bestMove = legalMoves.front();
    result.score = evaluatePosition(board);

    int bestScore = -INFINITY_SCORE;
    Move bestMove = legalMoves.front();

    int alpha;
    int beta;
    // Only use aspiration windows if we have a valid lastBestMove from THIS position
    // Check if lastBestMove is actually a legal move in current position
    bool canUseAspiration = false;
    if (depth >= 3 && lastBestMove.isValid()) {
        // Verify lastBestMove is actually legal in this position
        for (const Move& move : legalMoves) {
            if (move == lastBestMove) {
                canUseAspiration = true;
                break;
            }
        }
    }
    
    bool usingAspirationWindow = false;
    if (canUseAspiration) {
        alpha = lastBestScore - ASPIRATION_WINDOW;
        beta = lastBestScore + ASPIRATION_WINDOW;
        usingAspirationWindow = true;
    } else {
        alpha = -INFINITY_SCORE;
        beta = INFINITY_SCORE;
    }

    Move rootTtMove;
    const uint64_t rootHash = hashBoard(board);
    auto ttIt = transpositionTable.find(rootHash);
    if (ttIt != transpositionTable.end()) {
        rootTtMove = ttIt->second.bestMove;
    }

    orderMoves(board, legalMoves, 0, rootTtMove);

    // Only use lastBestMove for move ordering if it's valid for this position
    if (canUseAspiration) {
        auto it = std::find(legalMoves.begin(), legalMoves.end(), lastBestMove);
        if (it != legalMoves.end()) {
            std::rotate(legalMoves.begin(), it, it + 1);
        }
    }

    bool needResearch = false;
    bool timeUp = false;

    do {
        needResearch = false;
        timeUp = false;
        bestScore = -INFINITY_SCORE;

        for (const Move& move : legalMoves) {
            board.makeMove(move);
            const int score = minimax(
                board,
                depth - 1,
                alpha,
                beta,
                false,
                result.nodesSearched,
                1,
                timeUp
            );
            board.unmakeMove();

            if (timeUp) {
                break;
            }

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }

            if (score > alpha) {
                alpha = score;
            }
        }

        if (!timeUp && usingAspirationWindow && depth >= 3 && lastBestMove.isValid()) {
            if (bestScore <= lastBestScore - ASPIRATION_WINDOW) {
                alpha = -INFINITY_SCORE;
                beta = INFINITY_SCORE;
                usingAspirationWindow = false;
                needResearch = true;
            } else if (bestScore >= lastBestScore + ASPIRATION_WINDOW) {
                alpha = -INFINITY_SCORE;
                beta = INFINITY_SCORE;
                usingAspirationWindow = false;
                needResearch = true;
            }
        }
    } while (needResearch && !timeUp);

    result.bestMove = bestMove;
    result.score = bestScore;

    lastBestMove = bestMove;
    lastBestScore = bestScore;

    return result;
}

} // namespace V2
} // namespace Chess