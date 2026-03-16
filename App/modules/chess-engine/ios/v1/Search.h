#ifndef CHESS_V1_SEARCH_H
#define CHESS_V1_SEARCH_H

#include "../Core/Types.h"
#include "../Core/Board.h"
#include "../Core/Move.h"
#include <string>
#include <functional>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>

namespace Chess {
namespace V1 {

struct SearchResult {
    Move bestMove;
    int score;
    int depth;
    int nodesSearched;
    
    SearchResult() : bestMove(), score(0), depth(0), nodesSearched(0) {}
};

// Transposition table entry
struct TTEntry {
    uint64_t zobristKey;
    int score;
    int depth;
    enum Flag { EXACT, LOWER_BOUND, UPPER_BOUND } flag;
    Move bestMove;
    
    TTEntry() : zobristKey(0), score(0), depth(0), flag(EXACT), bestMove() {}
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
    
    // Search at exactly one depth (no iterative deepening)
    // Uses existing transposition table from previous searches
    // This is for when JS manages iterative deepening
    static SearchResult findBestMoveAtDepth(
        Board& board,
        int depth,
        int maxTimeMs = 0
    );
    
    // Clear search caches (call when starting a new game)
    static void clearCaches();
    
private:
    // Time control
    static long long startTimeMs;
    static int maxSearchTimeMs;
    static bool isTimeUp();
    
    // Killer moves for move ordering (2 moves per depth)
    static constexpr int MAX_DEPTH = 64;
    static std::array<std::array<Move, 2>, MAX_DEPTH> killerMoves;
    
    // Transposition table for caching positions
    static std::unordered_map<uint64_t, TTEntry> transpositionTable;
    static constexpr size_t MAX_TT_SIZE = 1000000; // 1M entries
    
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
    
    // Quiescence search - search captures until position is quiet
    static int quiescence(
        Board& board,
        int alpha,
        int beta,
        int& nodesSearched,
        int ply,
        bool& timeUp
    );
    
    // Move ordering helpers
    static int getMoveScore(const Board& board, const Move& move, int ply, const Move& ttMove);
    static int getPieceValue(PieceType type);
    static void orderMoves(Board& board, std::vector<Move>& moves, int ply, const Move& ttMove);
    
    // Transposition table helpers
    static uint64_t computeZobristKey(const Board& board);
    static bool probeTranspositionTable(uint64_t key, int depth, int alpha, int beta, TTEntry& entry);
    static void storeTranspositionTable(uint64_t key, int score, int depth, TTEntry::Flag flag, const Move& bestMove);
    
    static constexpr int INFINITY_SCORE = 1000000;
};

} // namespace V1
} // namespace Chess

#endif // CHESS_V1_SEARCH_H
