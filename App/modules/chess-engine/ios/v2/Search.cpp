#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <array>
#include <cstdint>
#include <atomic>
#include <iostream>
#include <cmath>

namespace Chess {
namespace V2 {

namespace {
constexpr int MATE_SCORE = 30000;
constexpr int TIME_CHECK_MASK = 255; // check time/cancel every 256 nodes
constexpr int QUIESCENCE_MAX_DEPTH = 8;

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

// Negamax requires evaluation from side-to-move perspective.
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

inline int mateScore(int ply) {
    return -MATE_SCORE + ply;
}

inline int terminalScore(Board& board, int ply) {
    if (board.isCheckmate()) {
        return mateScore(ply);
    }

    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    return evaluatePosition(board);
}

inline bool isTacticalMove(const Move& move, bool inCheck) {
    if (inCheck) {
        return true;
    }
    return move.isCapture() || move.isPromotion();
}

inline bool isCastleMove(const Board& board, const Move& move) {
    const Position from = move.getFrom();
    const Position to = move.getTo();

    if (!from.isValid() || !to.isValid()) {
        return false;
    }

    const Piece piece = board.getPiece(from.row, from.col);
    if (piece.isEmpty() || piece.type != PieceType::KING) {
        return false;
    }

    return from.row == to.row &&
           std::abs(static_cast<int>(from.col) - static_cast<int>(to.col)) == 2;
}

inline bool moveIsImmediateMate(Board& board, const Move& move) {
    board.makeMove(move);
    const bool mate = board.isCheckmate();
    board.unmakeMove();
    return mate;
}

} // namespace

std::array<std::array<Move, 2>, Search::MAX_DEPTH> Search::killerMoves = {};
std::array<std::array<std::array<int, 64>, 64>, 2> Search::historyHeuristic = {};
Move Search::lastBestMove = Move();
int Search::lastBestScore = 0;
std::unordered_map<uint64_t, TranspositionEntry> Search::transpositionTable;
long long Search::startTimeMs = 0;
int Search::maxSearchTimeMs = 0;
std::atomic<uint64_t> Search::activeSearchToken {0};
thread_local uint64_t Search::localSearchToken = 0;

uint64_t Search::hashBoard(const Board& board) {
    return board.positionHash_;
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
    if (ttMove.isValid() && move == ttMove) {
        return 3000000;
    }

    int score = 0;

    if (move.isPromotion()) {
        score += 1500000 + getPieceValue(move.getPromotion()) * 4;
    }

    if (move.isCapture()) {
        const Piece fromPiece = board.getPiece(move.getFrom());
        Piece toPiece = board.getPiece(move.getTo());

        int capturedValue = getPieceValue(toPiece.type);
        if (capturedValue == 0 && move.isEnPassant()) {
            capturedValue = getPieceValue(PieceType::PAWN);
        }

        const int attackerValue = getPieceValue(fromPiece.type);

        score += 1200000;
        score += capturedValue * 32 - attackerValue * 2;

        if (capturedValue > attackerValue) {
            score += 50000;
        } else if (capturedValue < attackerValue) {
            score -= 15000;
        }

        return score;
    }

    if (isCastleMove(board, move)) {
        score += 250000;
    }

    if (ply < MAX_DEPTH) {
        if (move == killerMoves[ply][0]) {
            score += 220000;
        } else if (move == killerMoves[ply][1]) {
            score += 180000;
        }
    }

    const Position from = move.getFrom();
    const Position to = move.getTo();

    if (from.isValid() && to.isValid()) {
        const int side = colorIndex(board.getCurrentPlayer());
        score += historyHeuristic[side][squareIndex(from)][squareIndex(to)];

        const int r = static_cast<int>(to.row);
        const int c = static_cast<int>(to.col);
        if ((r == 3 || r == 4) && (c == 3 || c == 4)) {
            score += 60;
        } else if (r >= 2 && r <= 5 && c >= 2 && c <= 5) {
            score += 30;
        }
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

int Search::quiescence(
    Board& board,
    int alpha,
    int beta,
    int& nodesSearched,
    int ply,
    bool& timeUp,
    int qDepth
) {
    nodesSearched++;

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && Search::isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
    }

    if (board.isCheckmate()) {
        return mateScore(ply);
    }
    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    const bool inCheck = board.isInCheck(board.getCurrentPlayer());

    if (qDepth >= QUIESCENCE_MAX_DEPTH) {
        return evaluatePosition(board);
    }

    const int standPat = evaluatePosition(board);

    if (!inCheck) {
        if (standPat >= beta) {
            return standPat;
        }
        if (standPat > alpha) {
            alpha = standPat;
        }
    }

    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        return terminalScore(board, ply);
    }

    std::vector<Move> tacticalMoves;
    tacticalMoves.reserve(moves.size());

    for (const Move& move : moves) {
        if (isTacticalMove(move, inCheck)) {
            tacticalMoves.push_back(move);
        }
    }

    if (tacticalMoves.empty()) {
        return standPat;
    }

    Search::orderMoves(board, tacticalMoves, std::min(ply, Search::MAX_DEPTH - 1), Move());

    int bestScore = standPat;

    for (const Move& move : tacticalMoves) {
        board.makeMoveUnchecked(move);
        const int score = -quiescence(board, -beta, -alpha, nodesSearched, ply + 1, timeUp, qDepth + 1);
        board.unmakeMoveUnchecked();

        if (timeUp) {
            return bestScore;
        }

        if (score > bestScore) {
            bestScore = score;
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            break;
        }
    }

    return bestScore;
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
    (void)maximizing; // preserved for API compatibility

    nodesSearched++;

    const int alphaOriginal = alpha;
    const int betaOriginal = beta;

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
    }

    if (board.isCheckmate()) {
        return mateScore(ply);
    }
    if (board.isStalemate() || board.isDraw()) {
        return 0;
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

    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        return terminalScore(board, ply);
    }

    if (depth <= 0) {
        return quiescence(board, alpha, beta, nodesSearched, ply, timeUp, 0);
    }

    orderMoves(board, moves, std::min(ply, MAX_DEPTH - 1), ttMove);

    Move bestMove;
    int bestScore = -INFINITY_SCORE;

    for (const Move& move : moves) {
        board.makeMoveUnchecked(move);

        const int score = -minimax(
            board,
            depth - 1,
            -beta,
            -alpha,
            false,
            nodesSearched,
            ply + 1,
            timeUp
        );

        board.unmakeMoveUnchecked();

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

    for (const Move& move : legalMoves) {
        if (moveIsImmediateMate(board, move)) {
            finalResult.bestMove = move;
            finalResult.score = MATE_SCORE - 1;
            finalResult.depth = 1;
            finalResult.nodesSearched = 1;
            return finalResult;
        }
    }

    finalResult.bestMove = legalMoves.front();
    finalResult.score = 0;
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

        Move rootTtMove;
        const uint64_t rootHash = hashBoard(board);
        auto ttIt = transpositionTable.find(rootHash);
        if (ttIt != transpositionTable.end()) {
            rootTtMove = ttIt->second.bestMove;
        }

        orderMoves(board, legalMoves, 0, rootTtMove);

        if (finalResult.bestMove.isValid()) {
            auto it = std::find(legalMoves.begin(), legalMoves.end(), finalResult.bestMove);
            if (it != legalMoves.end()) {
                std::rotate(legalMoves.begin(), it, it + 1);
            }
        }

        timeUp = false;

        for (const Move& move : legalMoves) {
            board.makeMoveUnchecked(move);
            const int score = -minimax(
                board,
                currentDepth - 1,
                -beta,
                -alpha,
                false,
                result.nodesSearched,
                1,
                timeUp
            );
            board.unmakeMoveUnchecked();

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

        if (!timeUp) {
            result.bestMove = bestMove;
            result.score = bestScore;
            finalResult = result;

            lastBestMove = bestMove;
            lastBestScore = bestScore;

            if (progressCallback) {
                progressCallback(result);
            }

            if (bestScore >= MATE_SCORE - 100) {
                break;
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

    if (depth == 1) {
        clearCaches();
    }

    std::vector<Move> legalMoves = board.generateLegalMoves();

    if (legalMoves.empty()) {
        result.score = terminalScore(board, 0);
        return result;
    }

    for (const Move& move : legalMoves) {
        if (moveIsImmediateMate(board, move)) {
            result.bestMove = move;
            result.score = MATE_SCORE - 1;
            return result;
        }
    }

    result.bestMove = legalMoves.front();
    result.score = 0;

    int bestScore = -INFINITY_SCORE;
    Move bestMove = legalMoves.front();

    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;

    Move rootTtMove;
    const uint64_t rootHash = hashBoard(board);
    auto ttIt = transpositionTable.find(rootHash);
    if (ttIt != transpositionTable.end()) {
        rootTtMove = ttIt->second.bestMove;
    }

    orderMoves(board, legalMoves, 0, rootTtMove);

    if (lastBestMove.isValid()) {
        auto it = std::find(legalMoves.begin(), legalMoves.end(), lastBestMove);
        if (it != legalMoves.end()) {
            std::rotate(legalMoves.begin(), it, it + 1);
        }
    }

    bool timeUp = false;

    for (const Move& move : legalMoves) {
        board.makeMoveUnchecked(move);
        const int score = -minimax(
            board,
            depth - 1,
            -beta,
            -alpha,
            false,
            result.nodesSearched,
            1,
            timeUp
        );
        board.unmakeMoveUnchecked();

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

    result.bestMove = bestMove;
    result.score = bestScore;

    lastBestMove = bestMove;
    lastBestScore = bestScore;

    return result;
}

} // namespace V2
} // namespace Chess