#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

#include "Types.h"
#include <string>

namespace Chess {

class Move {
public:
    Move();
    Move(Square from, Square to, uint8_t flags = MoveFlags::NORMAL, 
         PieceType promotion = PieceType::NONE);
    
    Square getFrom() const { return from_; }
    Square getTo() const { return to_; }
    uint8_t getFlags() const { return flags_; }
    PieceType getPromotion() const { return promotion_; }
    
    bool isCapture() const { return flags_ & MoveFlags::CAPTURE; }
    bool isDoublePawnPush() const { return flags_ & MoveFlags::DOUBLE_PAWN_PUSH; }
    bool isEnPassant() const { return flags_ & MoveFlags::EN_PASSANT; }
    bool isCastling() const { return flags_ & MoveFlags::CASTLING; }
    bool isPromotion() const { return flags_ & MoveFlags::PROMOTION; }
    
    bool isValid() const { return from_ >= 0 && from_ < 64 && to_ >= 0 && to_ < 64; }
    
    // Convert to algebraic notation (e.g., "e2e4", "e7e8q" for promotion)
    std::string toAlgebraic() const;
    
    // Parse from algebraic notation
    static Move fromAlgebraic(const std::string& notation);
    
    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const { return !(*this == other); }
    
private:
    Square from_;
    Square to_;
    uint8_t flags_;
    PieceType promotion_;
};

} // namespace Chess

#endif // CHESS_MOVE_H
