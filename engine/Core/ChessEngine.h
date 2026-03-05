#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include "Board.h"
#include "Move.h"
#include "Types.h"
#include <string>
#include <vector>

namespace Chess {

/**
 * Main chess engine interface
 * Provides high-level API for chess game management
 */
class ChessEngine {
public:
    ChessEngine();
    
    // Game initialization
    void newGame();
    bool loadFromFEN(const std::string& fen);
    std::string getFEN() const;
    
    // Move operations
    bool makeMove(const std::string& algebraic);
    bool makeMove(const Move& move);
    void undoMove();
    
    // Get available moves
    std::vector<std::string> getLegalMoves() const;
    std::vector<std::string> getLegalMovesFrom(const std::string& square) const;
    bool isMoveLegal(const std::string& algebraic) const;
    
    // Game state queries
    Color getCurrentPlayer() const;
    std::string getCurrentPlayerString() const;
    bool isInCheck() const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isDraw() const;
    std::string getGameResult() const;
    
    // Board queries
    std::string getPieceAt(const std::string& square) const;
    std::string getPieceAt(int row, int col) const;
    
    // Game information
    int getHalfmoveClock() const;
    int getFullmoveNumber() const;
    bool canCastle(const std::string& side) const;
    std::string getEnPassantSquare() const;
    
    // Board representation for display
    std::string getBoardString() const;
    
    // Move history
    int getMoveCount() const;
    std::string getLastMove() const;
    
private:
    Board board_;
    
    // Helper methods
    Square parseSquare(const std::string& square) const;
    std::string squareToString(Square sq) const;
    std::string pieceToString(const Piece& piece) const;
};

} // namespace Chess

#endif // CHESS_ENGINE_H
