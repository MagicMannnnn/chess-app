#ifndef CHESS_V2_SEARCH_H
#define CHESS_V2_SEARCH_H

#include "../Types.h"
#include "../Board.h"
#include "../Move.h"
#include <string>
#include <functional>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <atomic>

namespace Chess {
namespace V2 {

struct SearchResult {
    Move bestMove;
    int score;
    int depth;
    int nodesSearched;

    SearchResult() : bestMove(), score(0), depth(0), nodesSearched(0) {}
};

// Transposition table entry
struct TranspositionEntry {
    uint64_t hash;
    Move bestMove;
    int score;
    int depth;
    enum NodeType { EXACT, LOWER_BOUND, UPPER_BOUND } type;

    TranspositionEntry() : hash(0), bestMove(), score(0), depth(0), type(EXACT) {}
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
    // Uses existing killer moves / history / TT from previous searches
    static SearchResult findBestMoveAtDepth(
        Board& board,
        int depth,
        int maxTimeMs = 0
    );

    // Clear all caches (transposition table, killer moves, etc.)
    static void clearCaches();

    // Request cancellation for any currently running search.
    // Running searches will observe this and terminate quickly.
    static void cancelActiveSearches();

private:
    // Time control
    static long long startTimeMs;
    static int maxSearchTimeMs;
    static bool isTimeUp();
    static std::atomic<uint64_t> activeSearchToken;
    static thread_local uint64_t localSearchToken;

    // Killer moves for move ordering (2 moves per depth)
    static constexpr int MAX_DEPTH = 64;
    static std::array<std::array<Move, 2>, MAX_DEPTH> killerMoves;

    // History heuristic [color][from][to]
    static std::array<std::array<std::array<int, 64>, 64>, 2> historyHeuristic;

    // Track last best move and score for move ordering across depths
    static Move lastBestMove;
    static int lastBestScore;

    // Transposition table for caching position evaluations
    static std::unordered_map<uint64_t, TranspositionEntry> transpositionTable;
    static constexpr size_t MAX_TT_SIZE = 1000000; // ~1M entries

    // Hash function for board positions
    static uint64_t hashBoard(const Board& board);

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
    static int getMoveScore(const Board& board, const Move& move, int ply, const Move& ttMove);
    static int getPieceValue(PieceType type);
    static void orderMoves(Board& board, std::vector<Move>& moves, int ply, const Move& ttMove = Move());

    static constexpr int INFINITY_SCORE = 1000000;
};

} // namespace V2
} // namespace Chess

#endif // CHESS_V2_SEARCH_H