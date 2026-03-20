#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include "Board.h"
#include "Move.h"
#include "Types.h"
#include <functional>
#include <string>
#include <vector>

namespace Chess {

class ChessEngine {
public:
    ChessEngine();

    void newGame();
    bool loadFromFEN(const std::string& fen);
    std::string getFEN() const;

    bool makeMove(const std::string& algebraic);
    bool makeMove(const Move& move);
    void undoMove();

    std::vector<std::string> getLegalMoves() const;
    std::vector<std::string> getLegalMovesFrom(const std::string& square) const;
    bool isMoveLegal(const std::string& algebraic) const;

    Color getCurrentPlayer() const;
    std::string getCurrentPlayerString() const;
    bool isInCheck() const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isDraw() const;
    std::string getGameResult() const;

    std::string getPieceAt(const std::string& square) const;
    std::string getPieceAt(int row, int col) const;

    int getHalfmoveClock() const;
    int getFullmoveNumber() const;
    bool canCastle(const std::string& side) const;
    std::string getEnPassantSquare() const;

    std::string getBoardString() const;

    int getMoveCount() const;
    std::string getLastMove() const;
    std::vector<std::string> getMoveHistory() const;
    bool canUndo() const;

    std::string getBestMove(int depth, int maxTimeMs = 0, const std::string& aiVersion = "v1") const;
    std::string getBestMoveAtDepth(int depth, int maxTimeMs = 0, const std::string& aiVersion = "v1") const;
    int evaluatePosition(const std::string& aiVersion = "v1") const;

    struct SearchProgressData {
        int depth;
        std::string bestMove;
        int score;
        int nodesSearched;
        long long timeMs;
    };

    struct SearchResultData {
        std::string bestMove;
        int score;
        int depthCompleted;
        int nodesSearched;
        bool timedOut;
        bool cancelled;
        long long totalTimeMs;
        std::vector<SearchProgressData> progressHistory;
    };

    using SearchProgressCallback = std::function<void(const SearchProgressData&)>;

    SearchResultData searchBestMove(int maxDepth, int maxTimeMs, const std::string& aiVersion) const;
    SearchResultData searchBestMove(
        int maxDepth,
        int maxTimeMs,
        const std::string& aiVersion,
        SearchProgressCallback progressCallback
    ) const;

    void clearSearchCaches();

private:
    Board board_;
    std::vector<std::string> moveHistory_;

    Square parseSquare(const std::string& square) const;
    std::string squareToString(Square sq) const;
    std::string pieceToString(const Piece& piece) const;
};

} // namespace Chess

#endif // CHESS_ENGINE_H
