#include "ChessEngine.h"
#include "../v2/Search.h"
#include "../v2/Evaluation.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <cctype>

namespace Chess {

ChessEngine::ChessEngine() {
    newGame();
}

void ChessEngine::newGame() {
    board_.reset();
    moveHistory_.clear();
    V2::Search::clearCaches();
}

bool ChessEngine::loadFromFEN(const std::string& fen) {
    const bool loaded = board_.fromFEN(fen);

    if (loaded) {
        moveHistory_.clear();
        V2::Search::clearCaches();
    }

    return loaded;
}

std::string ChessEngine::getFEN() const {
    return board_.toFEN();
}

bool ChessEngine::makeMove(const std::string& algebraic) {
    Move move = Move::fromAlgebraic(algebraic);
    return makeMove(move);
}

bool ChessEngine::makeMove(const Move& move) {
    const bool result = board_.makeMove(move);

    if (result) {
        moveHistory_.push_back(move.toAlgebraic());

        // Position changed: invalidate all search state
        V2::Search::clearCaches();
    }

    return result;
}

void ChessEngine::undoMove() {
    board_.unmakeMove();

    if (!moveHistory_.empty()) {
        moveHistory_.pop_back();
    }

    // Position changed: invalidate all search state
    V2::Search::clearCaches();
}

std::vector<std::string> ChessEngine::getLegalMoves() const {
    std::vector<Move> moves = board_.generateLegalMoves();
    std::vector<std::string> result;
    result.reserve(moves.size());

    for (const Move& move : moves) {
        result.push_back(move.toAlgebraic());
    }

    return result;
}

std::vector<std::string> ChessEngine::getLegalMovesFrom(const std::string& square) const {
    Square sq = parseSquare(square);
    if (sq < 0 || sq >= 64) {
        return {};
    }

    std::vector<Move> moves = board_.generateLegalMoves(sq);
    std::vector<std::string> result;
    result.reserve(moves.size());

    for (const Move& move : moves) {
        result.push_back(move.toAlgebraic());
    }

    return result;
}

bool ChessEngine::isMoveLegal(const std::string& algebraic) const {
    Move move = Move::fromAlgebraic(algebraic);
    return board_.isLegalMove(move);
}

Color ChessEngine::getCurrentPlayer() const {
    return board_.getCurrentPlayer();
}

std::string ChessEngine::getCurrentPlayerString() const {
    return board_.getCurrentPlayer() == Color::WHITE ? "white" : "black";
}

bool ChessEngine::isInCheck() const {
    return board_.isInCheck(board_.getCurrentPlayer());
}

bool ChessEngine::isCheckmate() const {
    return board_.isCheckmate();
}

bool ChessEngine::isStalemate() const {
    return board_.isStalemate();
}

bool ChessEngine::isDraw() const {
    return board_.isDraw();
}

std::string ChessEngine::getGameResult() const {
    GameResult result = board_.getGameResult();

    switch (result) {
        case GameResult::IN_PROGRESS:
            return "in_progress";
        case GameResult::WHITE_WINS:
            return "white_wins";
        case GameResult::BLACK_WINS:
            return "black_wins";
        case GameResult::DRAW_STALEMATE:
            return "draw_stalemate";
        case GameResult::DRAW_INSUFFICIENT_MATERIAL:
            return "draw_insufficient_material";
        case GameResult::DRAW_FIFTY_MOVE:
            return "draw_fifty_move";
        case GameResult::DRAW_THREEFOLD_REPETITION:
            return "draw_threefold_repetition";
        default:
            return "unknown";
    }
}

std::string ChessEngine::getPieceAt(const std::string& square) const {
    Square sq = parseSquare(square);
    if (sq < 0 || sq >= 64) {
        return "none";
    }

    Piece piece = board_.getPiece(sq);
    return pieceToString(piece);
}

std::string ChessEngine::getPieceAt(int row, int col) const {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        return "none";
    }

    Piece piece = board_.getPiece(row, col);
    return pieceToString(piece);
}

int ChessEngine::getHalfmoveClock() const {
    return board_.getHalfmoveClock();
}

int ChessEngine::getFullmoveNumber() const {
    return board_.getFullmoveNumber();
}

bool ChessEngine::canCastle(const std::string& side) const {
    uint8_t rights = board_.getCastlingRights();
    Color player = board_.getCurrentPlayer();

    if (side == "kingside" || side == "k") {
        if (player == Color::WHITE) {
            return (rights & CastlingRights::WHITE_KINGSIDE) != 0;
        } else {
            return (rights & CastlingRights::BLACK_KINGSIDE) != 0;
        }
    }

    if (side == "queenside" || side == "q") {
        if (player == Color::WHITE) {
            return (rights & CastlingRights::WHITE_QUEENSIDE) != 0;
        } else {
            return (rights & CastlingRights::BLACK_QUEENSIDE) != 0;
        }
    }

    return false;
}

std::string ChessEngine::getEnPassantSquare() const {
    Square sq = board_.getEnPassantTarget();
    if (sq < 0) {
        return "-";
    }
    return squareToString(sq);
}

std::string ChessEngine::getBoardString() const {
    std::ostringstream result;

    result << "  +---+---+---+---+---+---+---+---+\n";

    for (int row = 7; row >= 0; row--) {
        result << (row + 1) << " |";

        for (int col = 0; col < 8; col++) {
            Piece piece = board_.getPiece(row, col);
            char c = ' ';

            if (!piece.isEmpty()) {
                switch (piece.type) {
                    case PieceType::PAWN:   c = 'P'; break;
                    case PieceType::KNIGHT: c = 'N'; break;
                    case PieceType::BISHOP: c = 'B'; break;
                    case PieceType::ROOK:   c = 'R'; break;
                    case PieceType::QUEEN:  c = 'Q'; break;
                    case PieceType::KING:   c = 'K'; break;
                    default: break;
                }

                if (piece.color == Color::BLACK) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
            }

            result << ' ' << c << " |";
        }

        result << '\n';
        result << "  +---+---+---+---+---+---+---+---+\n";
    }

    result << "    a   b   c   d   e   f   g   h\n";
    return result.str();
}

int ChessEngine::getMoveCount() const {
    return static_cast<int>(moveHistory_.size());
}

std::string ChessEngine::getLastMove() const {
    if (moveHistory_.empty()) {
        return "";
    }
    return moveHistory_.back();
}

std::vector<std::string> ChessEngine::getMoveHistory() const {
    return moveHistory_;
}

bool ChessEngine::canUndo() const {
    return !moveHistory_.empty();
}

Square ChessEngine::parseSquare(const std::string& square) const {
    if (square.length() != 2) {
        return -1;
    }

    int col = square[0] - 'a';
    int row = square[1] - '1';

    if (col < 0 || col >= 8 || row < 0 || row >= 8) {
        return -1;
    }

    return row * 8 + col;
}

std::string ChessEngine::squareToString(Square sq) const {
    if (sq < 0 || sq >= 64) {
        return "-";
    }

    int row = sq / 8;
    int col = sq % 8;

    std::string result;
    result += static_cast<char>('a' + col);
    result += static_cast<char>('1' + row);
    return result;
}

std::string ChessEngine::pieceToString(const Piece& piece) const {
    if (piece.isEmpty()) {
        return "none";
    }

    std::string result;
    result += (piece.color == Color::WHITE) ? "white_" : "black_";

    switch (piece.type) {
        case PieceType::PAWN:   result += "pawn"; break;
        case PieceType::KNIGHT: result += "knight"; break;
        case PieceType::BISHOP: result += "bishop"; break;
        case PieceType::ROOK:   result += "rook"; break;
        case PieceType::QUEEN:  result += "queen"; break;
        case PieceType::KING:   result += "king"; break;
        default:                result += "unknown"; break;
    }

    return result;
}

// AI Search functions

std::string ChessEngine::getBestMove(int depth, int maxTimeMs, const std::string& aiVersion) const {
    (void)aiVersion; // API preserved, V2 only internally

    // Fresh top-level search: clear stale TT / killer / history / PV caches
    V2::Search::clearCaches();

    Board boardCopy = board_;
    V2::SearchResult result = V2::Search::findBestMove(boardCopy, depth, nullptr, maxTimeMs);

    if (!result.bestMove.isValid()) {
        return "";
    }

    return result.bestMove.toAlgebraic();
}

std::string ChessEngine::getBestMoveAtDepth(int depth, int maxTimeMs, const std::string& aiVersion) const {
    (void)aiVersion; // API preserved, V2 only internally

    // Fresh top-level search: clear stale TT / killer / history / PV caches
    V2::Search::clearCaches();

    Board boardCopy = board_;
    V2::SearchResult result = V2::Search::findBestMoveAtDepth(boardCopy, depth, maxTimeMs);

    if (!result.bestMove.isValid()) {
        return "";
    }

    return result.bestMove.toAlgebraic();
}

ChessEngine::SearchResultData ChessEngine::searchBestMove(
    int maxDepth,
    int maxTimeMs,
    const std::string& aiVersion
) const {
    (void)aiVersion; // API preserved, V2 only internally

    // Fresh top-level search: avoid previous-position move leakage at shallow depths
    V2::Search::clearCaches();

    Board boardCopy = board_;

    SearchResultData finalResult;
    finalResult.bestMove = "";
    finalResult.score = 0;
    finalResult.depthCompleted = 0;
    finalResult.nodesSearched = 0;
    finalResult.timedOut = false;
    finalResult.totalTimeMs = 0;
    finalResult.progressHistory.clear();

    const auto startTime = std::chrono::steady_clock::now();

    for (int depth = 1; depth <= maxDepth; depth++) {
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        ).count();

        if (maxTimeMs > 0 && elapsed >= maxTimeMs) {
            finalResult.timedOut = true;
            break;
        }

        int remainingTime = 0;
        if (maxTimeMs > 0) {
            remainingTime = static_cast<int>(maxTimeMs - elapsed);
            if (remainingTime < 0) {
                remainingTime = 0;
            }
        }

        V2::SearchResult result = V2::Search::findBestMoveAtDepth(boardCopy, depth, remainingTime);

        if (result.bestMove.isValid()) {
            finalResult.bestMove = result.bestMove.toAlgebraic();
            finalResult.score = result.score;
            finalResult.depthCompleted = depth;
            finalResult.nodesSearched += result.nodesSearched;

            SearchProgressData progress;
            progress.depth = depth;
            progress.bestMove = result.bestMove.toAlgebraic();
            progress.score = result.score;
            progress.nodesSearched = result.nodesSearched;
            progress.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            ).count();

            finalResult.progressHistory.push_back(progress);
        }

        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        ).count();

        if (maxTimeMs > 0 && elapsed >= maxTimeMs) {
            finalResult.timedOut = true;
            break;
        }
    }

    finalResult.totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime
    ).count();

    return finalResult;
}

int ChessEngine::evaluatePosition(const std::string& aiVersion) const {
    (void)aiVersion; // API preserved, V2 only internally

    // Keep evaluation isolated from any stale search state
    V2::Search::clearCaches();

    V2::CastlingRights rights;
    rights.whiteKingSide  = (board_.getCastlingRights() & CastlingRights::WHITE_KINGSIDE) != 0;
    rights.whiteQueenSide = (board_.getCastlingRights() & CastlingRights::WHITE_QUEENSIDE) != 0;
    rights.blackKingSide  = (board_.getCastlingRights() & CastlingRights::BLACK_KINGSIDE) != 0;
    rights.blackQueenSide = (board_.getCastlingRights() & CastlingRights::BLACK_QUEENSIDE) != 0;

    return V2::EvaluatorV2::evaluate(
        board_,
        board_.getCurrentPlayer(),
        rights,
        false,
        false,
        board_.getFullmoveNumber()
    );
}

} // namespace Chess