#ifndef CHESS_V1_SEARCH_H
#define CHESS_V1_SEARCH_H

#include "../Types.h"
#include "../Board.h"
#include "../Move.h"
#include <string>
#include <functional>

namespace Chess {
namespace V1 {

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
    static SearchResult findBestMove(
        Board& board, 
        int maxDepth,
        std::function<void(const SearchResult&)> progressCallback = nullptr
    );
    
private:
    static int minimax(
        Board& board,
        int depth,
        int alpha,
        int beta,
        bool maximizing,
        int& nodesSearched
    );
    
    static constexpr int INFINITY_SCORE = 1000000;
};

} // namespace V1
} // namespace Chess

#endif // CHESS_V1_SEARCH_H
