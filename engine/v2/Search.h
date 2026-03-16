#ifndef CHESS_V2_SEARCH_H
#define CHESS_V2_SEARCH_H

#include "../Types.h"
#include "../Board.h"
#include "../Move.h"
#include <string>
#include <functional>
#include <vector>
#include <array>

namespace Chess {
namespace V2 {

struct SearchResult {
    Move bestMove;
    int score;
    int depth;
    int nodesSearched;
    
    SearchResult() : bestMove(), score(0), depth(0), nodesSearched(0) {}
};

class Search {
public:
    // Search for the best move using iterative deepening
    // Calls progressCallback with each completed depth level
    // maxTimeMs: maximum time in milliseconds (0 = no time limit)
    static SearchResult findBestMove(
        Board& board, 
        int maxDepth,
        std::function<void(const SearchResult&)> progressCallback = nullptr,
        int maxTimeMs = 0
    );
    
private:
    // Time control
    static long long startTimeMs;
    static int maxSearchTimeMs;
    static bool isTimeUp();
    
    // Killer moves for move ordering (2 moves per depth)
    static constexpr int MAX_DEPTH = 64;
    static std::array<std::array<Move, 2>, MAX_DEPTH> killerMoves;
    
    static int minimax(
        Board& board,
        int depth,
        int alpha,
        int beta,
        bool maximizing,
        int& nodesSearched,
        int ply,
        bool& timeUp
    );
    
    // Move ordering helpers
    static int getMoveScore(const Board& board, const Move& move, int ply);
    static int getPieceValue(PieceType type);
    static void orderMoves(Board& board, std::vector<Move>& moves, int ply);
    
    static constexpr int INFINITY_SCORE = 1000000;
};

} // namespace V2
} // namespace Chess

#endif // CHESS_V2_SEARCH_H
