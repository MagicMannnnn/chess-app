#include "Search.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <array>

namespace Chess {
namespace V2 {

namespace {
constexpr int MATE_SCORE = 30000;
constexpr int TIME_CHECK_MASK = 2047; // check time every 2048 nodes

inline CastlingRights buildCastlingRights(const Board& board) {
    uint8_t castlingRights = board.getCastlingRights();

    CastlingRights rights;
    rights.whiteKingSide  = (castlingRights & Chess::CastlingRights::WHITE_KINGSIDE) != 0;
    rights.whiteQueenSide = (castlingRights & Chess::CastlingRights::WHITE_QUEENSIDE) != 0;
    rights.blackKingSide  = (castlingRights & Chess::CastlingRights::BLACK_KINGSIDE) != 0;
    rights.blackQueenSide = (castlingRights & Chess::CastlingRights::BLACK_QUEENSIDE) != 0;
    return rights;
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

// Initialize time control variables
long long Search::startTimeMs = 0;
int Search::maxSearchTimeMs = 0;

bool Search::isTimeUp() {
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

int Search::getMoveScore(const Board& board, const Move& move, int ply) {
    int score = 0;

    if (move.isPromotion()) {
        score += 9000 + getPieceValue(move.getPromotion());
    }

    if (move.isCapture()) {
        Piece fromPiece = board.getPiece(move.getFrom());
        Piece toPiece = board.getPiece(move.getTo());

        score += 10000;
        score += getPieceValue(toPiece.type) * 10 - getPieceValue(fromPiece.type);
    } else if (ply < MAX_DEPTH) {
        if (move == killerMoves[ply][0]) {
            score += 8000;
        } else if (move == killerMoves[ply][1]) {
            score += 7000;
        }
    }

    return score;
}

void Search::orderMoves(Board& board, std::vector<Move>& moves, int ply) {
    struct ScoredMove {
        int score;
        Move move;
    };

    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(moves.size());

    for (const Move& move : moves) {
        scoredMoves.push_back({getMoveScore(board, move, ply), move});
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

    if ((nodesSearched & TIME_CHECK_MASK) == 0 && isTimeUp()) {
        timeUp = true;
        return evaluatePosition(board);
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

    orderMoves(board, moves, ply);

    if (maximizing) {
        int bestScore = -INFINITY_SCORE;

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
                }
                break;
            }
        }

        return bestScore;
    }

    int bestScore = INFINITY_SCORE;

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
            }
            break;
        }
    }

    return bestScore;
}

SearchResult Search::findBestMove(
    Board& board,
    int maxDepth,
    std::function<void(const SearchResult&)> progressCallback,
    int maxTimeMs
) {
    SearchResult finalResult;

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

        if (currentDepth > 1 && finalResult.bestMove.isValid()) {
            auto it = std::find(legalMoves.begin(), legalMoves.end(), finalResult.bestMove);
            if (it != legalMoves.end()) {
                std::rotate(legalMoves.begin(), it, it + 1);
            }
        } else {
            orderMoves(board, legalMoves, 0);
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

        if (!timeUp) {
            result.bestMove = bestMove;
            result.score = bestScore;
            finalResult = result;

            if (progressCallback) {
                progressCallback(result);
            }
        }
    }

    return finalResult;
}

} // namespace V2
} // namespace Chess