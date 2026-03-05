#include "ChessEngine.h"
#include <sstream>
#include <iomanip>

namespace Chess {

ChessEngine::ChessEngine() {
    newGame();
}

void ChessEngine::newGame() {
    board_.reset();
}

bool ChessEngine::loadFromFEN(const std::string& fen) {
    return board_.fromFEN(fen);
}

std::string ChessEngine::getFEN() const {
    return board_.toFEN();
}

bool ChessEngine::makeMove(const std::string& algebraic) {
    Move move = Move::fromAlgebraic(algebraic);
    return makeMove(move);
}

bool ChessEngine::makeMove(const Move& move) {
    return board_.makeMove(move);
}

void ChessEngine::undoMove() {
    board_.unmakeMove();
}

std::vector<std::string> ChessEngine::getLegalMoves() const {
    std::vector<Move> moves = board_.generateLegalMoves();
    std::vector<std::string> result;
    
    for (const Move& move : moves) {
        result.push_back(move.toAlgebraic());
    }
    
    return result;
}

std::vector<std::string> ChessEngine::getLegalMovesFrom(const std::string& square) const {
    Square sq = parseSquare(square);
    if (sq < 0 || sq >= 64) return {};
    
    std::vector<Move> moves = board_.generateLegalMoves(sq);
    std::vector<std::string> result;
    
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
    if (sq < 0 || sq >= 64) return "none";
    
    Piece piece = board_.getPiece(sq);
    return pieceToString(piece);
}

std::string ChessEngine::getPieceAt(int row, int col) const {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return "none";
    
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
            return rights & CastlingRights::WHITE_KINGSIDE;
        } else {
            return rights & CastlingRights::BLACK_KINGSIDE;
        }
    } else if (side == "queenside" || side == "q") {
        if (player == Color::WHITE) {
            return rights & CastlingRights::WHITE_QUEENSIDE;
        } else {
            return rights & CastlingRights::BLACK_QUEENSIDE;
        }
    }
    
    return false;
}

std::string ChessEngine::getEnPassantSquare() const {
    Square sq = board_.getEnPassantTarget();
    if (sq < 0) return "-";
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
                    c = std::tolower(c);
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
    return board_.getFullmoveNumber() - 1;
}

std::string ChessEngine::getLastMove() const {
    // This would require tracking move history in the engine
    // For now, return empty string
    return "";
}

Square ChessEngine::parseSquare(const std::string& square) const {
    if (square.length() != 2) return -1;
    
    int col = square[0] - 'a';
    int row = square[1] - '1';
    
    if (col < 0 || col >= 8 || row < 0 || row >= 8) return -1;
    
    return row * 8 + col;
}

std::string ChessEngine::squareToString(Square sq) const {
    if (sq < 0 || sq >= 64) return "-";
    
    int row = sq / 8;
    int col = sq % 8;
    
    std::string result;
    result += static_cast<char>('a' + col);
    result += static_cast<char>('1' + row);
    
    return result;
}

std::string ChessEngine::pieceToString(const Piece& piece) const {
    if (piece.isEmpty()) return "none";
    
    std::string result;
    
    // Color prefix
    result += (piece.color == Color::WHITE) ? "white_" : "black_";
    
    // Piece type
    switch (piece.type) {
        case PieceType::PAWN:   result += "pawn"; break;
        case PieceType::KNIGHT: result += "knight"; break;
        case PieceType::BISHOP: result += "bishop"; break;
        case PieceType::ROOK:   result += "rook"; break;
        case PieceType::QUEEN:  result += "queen"; break;
        case PieceType::KING:   result += "king"; break;
        default: result += "unknown"; break;
    }
    
    return result;
}

} // namespace Chess
