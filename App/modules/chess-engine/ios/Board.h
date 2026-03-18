#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include "Types.h"
#include "Move.h"
#include <vector>
#include <array>

namespace Chess {

class Board {
public:
    Board();
    
    // Initialize board to standard starting position
    void reset();
    
    // Get piece at square
    Piece getPiece(Square sq) const;
    Piece getPiece(int row, int col) const;
    
    // Make/unmake moves
    bool makeMove(const Move& move);
    void unmakeMove();
    
    // Move generation
    std::vector<Move> generateLegalMoves() const;
    std::vector<Move> generateLegalMoves(Square from) const;
    bool isLegalMove(const Move& move) const;
    
    // Game state
    Color getCurrentPlayer() const { return currentPlayer_; }
    bool isInCheck(Color color) const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isDraw() const;
    GameResult getGameResult() const;
    
    // Castle rights and en passant
    uint8_t getCastlingRights() const { return castlingRights_; }
    Square getEnPassantTarget() const { return enPassantTarget_; }
    int getHalfmoveClock() const { return halfmoveClock_; }
    int getFullmoveNumber() const { return fullmoveNumber_; }
    
    // FEN support
    std::string toFEN() const;
    bool fromFEN(const std::string& fen);
    
private:
    // Board state
    std::array<Piece, 64> board_;
    Color currentPlayer_;
    uint8_t castlingRights_;
    Square enPassantTarget_;
    int halfmoveClock_;  // For fifty-move rule
    int fullmoveNumber_;
    
    // Move history for unmake
    struct MoveRecord {
        Move move;
        Piece capturedPiece;
        uint8_t castlingRights;
        Square enPassantTarget;
        int halfmoveClock;
    };
    std::vector<MoveRecord> moveHistory_;
    
    // Helper methods
    void setPiece(Square sq, const Piece& piece);
    bool isSquareAttacked(Square sq, Color attackerColor) const;
    std::vector<Move> generatePseudoLegalMoves(Color color) const;
    std::vector<Move> generatePieceMoves(Square from) const;
    bool wouldBeInCheck(const Move& move, Color color) const;
    
    // Piece-specific move generation
    void generatePawnMoves(Square from, std::vector<Move>& moves) const;
    void generateKnightMoves(Square from, std::vector<Move>& moves) const;
    void generateBishopMoves(Square from, std::vector<Move>& moves) const;
    void generateRookMoves(Square from, std::vector<Move>& moves) const;
    void generateQueenMoves(Square from, std::vector<Move>& moves) const;
    void generateKingMoves(Square from, std::vector<Move>& moves) const;
    
    // Sliding piece helper
    void generateSlidingMoves(Square from, const std::vector<std::pair<int, int>>& directions, 
                             std::vector<Move>& moves) const;
    
    // Find king position
    Square findKing(Color color) const;
};

} // namespace Chess

#endif // CHESS_BOARD_H
