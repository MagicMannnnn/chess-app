#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <array>
#include <cstdint>
#include <atomic>

namespace Chess {
namespace V2 {

namespace {
constexpr int MATE_SCORE = 30000;
constexpr int TIME_CHECK_MASK = 2047; // check time/cancel every 2048 nodes
constexpr int QUIESCENCE_MAX_DEPTH = 6;

thread_local Color rootSearchColor = Color::WHITE;

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
    return static_cast<int>(pos.row) * 8 + static_cast<int>(pos.col);
}

inline int evaluatePosition(const Board& board) {
    return EvaluatorV2::evaluate(
        board,
        rootSearchColor,
        buildCastlingRights(board),
        false,
        false,
        board.getFullmoveNumber()
    );
}

inline int terminalScore(Board& board, int ply) {
    if (board.isCheckmate()) {
        return board.getCurrentPlayer() == rootSearchColor
            ? (-MATE_SCORE + ply)
            : (MATE_SCORE - ply);
    }

    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    return evaluatePosition(board);
}

inline int localPieceValue(PieceType type) {
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

inline bool moveIsImmediateMate(Board& board, const Move& move) {
    board.makeMove(move);
    const bool mate = board.isCheckmate();
    board.unmakeMove();
    return mate;
}

inline bool isTacticalMove(const Move& move, bool inCheck) {
    if (inCheck) {
        return true;
    }
    return move.isCapture() || move.isPromotion();
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
    uint64_t hash = 1469598103934665603ULL;

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
            hash *= 1099511628211ULL;
        }
    }

    hash ^= static_cast<uint64_t>(board.getCurrentPlayer() == Color::WHITE ? 1ULL : 2ULL);
    hash *= 1099511628211ULL;

    hash ^= static_cast<uint64_t>(board.getCastlingRights());
    hash *= 1099511628211ULL;

    const Square enPassant = board.getEnPassantTarget();
    hash ^= static_cast<uint64_t>(enPassant + 2);
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
    return localPieceValue(type);
}

int Search::getMoveScore(const Board& board, const Move& move, int ply, const Move& ttMove) {
    if (ttMove.isValid() && move == ttMove) {
        return 3000000;
    }

    int score = 0;

    if (move.isPromotion()) {
        score += 2000000 + getPieceValue(move.getPromotion()) * 16;
    }

    if (move.isCapture()) {
        const Piece fromPiece = board.getPiece(move.getFrom());
        Piece toPiece = board.getPiece(move.getTo());

        int capturedValue = getPieceValue(toPiece.type);
        if (capturedValue == 0 && move.isEnPassant()) {
            capturedValue = getPieceValue(PieceType::PAWN);
        }

        const int attackerValue = getPieceValue(fromPiece.type);
        score += 1000000 + capturedValue * 32 - attackerValue;
        return score;
    }

    if (ply < MAX_DEPTH) {
        if (move == killerMoves[ply][0]) {
            return 900000;
        }
        if (move == killerMoves[ply][1]) {
            return 800000;
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
            score += 32;
        } else if (r >= 2 && r <= 5 && c >= 2 && c <= 5) {
            score += 16;
        }
    }

    return score;
}

void Search::orderMoves(Board& board, std::vector<Move>& moves, int ply, const Move& ttMove) {
    std::stable_sort(
        moves.begin(),
        moves.end(),
        [&](const Move& a, const Move& b) {
            return getMoveScore(board, a, ply, ttMove) > getMoveScore(board, b, ply, ttMove);
        }
    );
}

static int quiescence(
    Board& board,
    int alpha,
    int beta,
    int& nodesSearched,
    int ply,
    bool& timeUp,
    int qDepth
) {
    ++nodesSearched;

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && Search::isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
    }

    if (board.isCheckmate()) {
        return terminalScore(board, ply);
    }
    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    if (qDepth >= QUIESCENCE_MAX_DEPTH) {
        return evaluatePosition(board);
    }

    const bool maximizing = board.getCurrentPlayer() == rootSearchColor;
    const bool inCheck = board.isInCheck(board.getCurrentPlayer());
    const int standPat = evaluatePosition(board);

    if (!inCheck) {
        if (maximizing) {
            if (standPat >= beta) {
                return standPat;
            }
            if (standPat > alpha) {
                alpha = standPat;
            }
        } else {
            if (standPat <= alpha) {
                return standPat;
            }
            if (standPat < beta) {
                beta = standPat;
            }
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

    if (maximizing) {
        int bestScore = inCheck ? -Search::INFINITY_SCORE : standPat;
        for (const Move& move : tacticalMoves) {
            board.makeMove(move);
            const int score = quiescence(board, alpha, beta, nodesSearched, ply + 1, timeUp, qDepth + 1);
            board.unmakeMove();

            if (timeUp) {
                return bestScore == -Search::INFINITY_SCORE ? standPat : bestScore;
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

    int bestScore = inCheck ? Search::INFINITY_SCORE : standPat;
    for (const Move& move : tacticalMoves) {
        board.makeMove(move);
        const int score = quiescence(board, alpha, beta, nodesSearched, ply + 1, timeUp, qDepth + 1);
        board.unmakeMove();

        if (timeUp) {
            return bestScore == Search::INFINITY_SCORE ? standPat : bestScore;
        }

        if (score < bestScore) {
            bestScore = score;
        }
        if (score < beta) {
            beta = score;
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
    ++nodesSearched;

    const int alphaOriginal = alpha;
    const int betaOriginal = beta;

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
    }

    if (board.isCheckmate()) {
        return terminalScore(board, ply);
    }
    if (board.isStalemate() || board.isDraw()) {
        return 0;
    }

    if (depth <= 0) {
        return quiescence(board, alpha, beta, nodesSearched, ply, timeUp, 0);
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
            }
            if (entry.type == TranspositionEntry::LOWER_BOUND) {
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

    orderMoves(board, moves, std::min(ply, MAX_DEPTH - 1), ttMove);

    Move bestMove;
    int bestScore = maximizing ? -INFINITY_SCORE : INFINITY_SCORE;

    if (maximizing) {
        for (const Move& move : moves) {
            board.makeMove(move);
            const int score = minimax(board, depth - 1, alpha, beta, false, nodesSearched, ply + 1, timeUp);
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
            const int score = minimax(board, depth - 1, alpha, beta, true, nodesSearched, ply + 1, timeUp);
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
    rootSearchColor = board.getCurrentPlayer();

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
    finalResult.score = evaluatePosition(board);
    finalResult.depth = 0;
    finalResult.nodesSearched = 0;

    bool timeUp = false;

    for (int currentDepth = 1; currentDepth <= maxDepth && !timeUp; ++currentDepth) {
        SearchResult result;
        result.depth = currentDepth;
        result.nodesSearched = 0;

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

        int bestScore = -INFINITY_SCORE;
        Move bestMove = legalMoves.front();
        int alpha = -INFINITY_SCORE;
        const int beta = INFINITY_SCORE;

        for (const Move& move : legalMoves) {
            board.makeMove(move);
            const int score = minimax(board, currentDepth - 1, alpha, beta, false, result.nodesSearched, 1, timeUp);
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
    rootSearchColor = board.getCurrentPlayer();

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
    result.score = evaluatePosition(board);

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

    int bestScore = -INFINITY_SCORE;
    Move bestMove = legalMoves.front();
    int alpha = -INFINITY_SCORE;
    const int beta = INFINITY_SCORE;
    bool timeUp = false;

    for (const Move& move : legalMoves) {
        board.makeMove(move);
        const int score = minimax(board, depth - 1, alpha, beta, false, result.nodesSearched, 1, timeUp);
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

    result.bestMove = bestMove;
    result.score = bestScore;
    lastBestMove = bestMove;
    lastBestScore = bestScore;
    return result;
}

} // namespace V2
} // namespace Chess
