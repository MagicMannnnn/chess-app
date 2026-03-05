#include "Move.h"

namespace Chess {

Move::Move() 
    : from_(-1), to_(-1), flags_(MoveFlags::NORMAL), promotion_(PieceType::NONE) {}

Move::Move(Square from, Square to, uint8_t flags, PieceType promotion)
    : from_(from), to_(to), flags_(flags), promotion_(promotion) {}

std::string Move::toAlgebraic() const {
    if (!isValid()) return "invalid";
    
    std::string result;
    
    // From square
    int fromRow = from_ / 8;
    int fromCol = from_ % 8;
    result += static_cast<char>('a' + fromCol);
    result += static_cast<char>('1' + fromRow);
    
    // To square
    int toRow = to_ / 8;
    int toCol = to_ % 8;
    result += static_cast<char>('a' + toCol);
    result += static_cast<char>('1' + toRow);
    
    // Promotion piece
    if (isPromotion()) {
        switch (promotion_) {
            case PieceType::QUEEN: result += 'q'; break;
            case PieceType::ROOK: result += 'r'; break;
            case PieceType::BISHOP: result += 'b'; break;
            case PieceType::KNIGHT: result += 'n'; break;
            default: break;
        }
    }
    
    return result;
}

Move Move::fromAlgebraic(const std::string& notation) {
    if (notation.length() < 4) return Move();
    
    int fromCol = notation[0] - 'a';
    int fromRow = notation[1] - '1';
    int toCol = notation[2] - 'a';
    int toRow = notation[3] - '1';
    
    if (fromCol < 0 || fromCol > 7 || fromRow < 0 || fromRow > 7 ||
        toCol < 0 || toCol > 7 || toRow < 0 || toRow > 7) {
        return Move();
    }
    
    Square from = fromRow * 8 + fromCol;
    Square to = toRow * 8 + toCol;
    
    uint8_t flags = MoveFlags::NORMAL;
    PieceType promotion = PieceType::NONE;
    
    // Check for promotion
    if (notation.length() == 5) {
        flags |= MoveFlags::PROMOTION;
        switch (notation[4]) {
            case 'q': promotion = PieceType::QUEEN; break;
            case 'r': promotion = PieceType::ROOK; break;
            case 'b': promotion = PieceType::BISHOP; break;
            case 'n': promotion = PieceType::KNIGHT; break;
            default: return Move();
        }
    }
    
    return Move(from, to, flags, promotion);
}

bool Move::operator==(const Move& other) const {
    return from_ == other.from_ && 
           to_ == other.to_ && 
           flags_ == other.flags_ && 
           promotion_ == other.promotion_;
}

} // namespace Chess
