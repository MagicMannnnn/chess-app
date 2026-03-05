#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../Core/ChessEngine.h"
#include <vector>
#include <string>

using namespace emscripten;
using namespace Chess;

// Wrapper class to expose ChessEngine to JavaScript
class ChessEngineWrapper {
private:
    ChessEngine engine_;
    
public:
    ChessEngineWrapper() {}
    
    void newGame() {
        engine_.newGame();
    }
    
    bool loadFromFEN(const std::string& fen) {
        return engine_.loadFromFEN(fen);
    }
    
    std::string getFEN() const {
        return engine_.getFEN();
    }
    
    bool makeMove(const std::string& move) {
        return engine_.makeMove(move);
    }
    
    void undoMove() {
        engine_.undoMove();
    }
    
    val getLegalMoves() const {
        std::vector<std::string> moves = engine_.getLegalMoves();
        val jsArray = val::array();
        for (size_t i = 0; i < moves.size(); i++) {
            jsArray.set(i, moves[i]);
        }
        return jsArray;
    }
    
    val getLegalMovesFrom(const std::string& square) const {
        std::vector<std::string> moves = engine_.getLegalMovesFrom(square);
        val jsArray = val::array();
        for (size_t i = 0; i < moves.size(); i++) {
            jsArray.set(i, moves[i]);
        }
        return jsArray;
    }
    
    bool isMoveLegal(const std::string& move) const {
        return engine_.isMoveLegal(move);
    }
    
    std::string getCurrentPlayer() const {
        return engine_.getCurrentPlayerString();
    }
    
    bool isInCheck() const {
        return engine_.isInCheck();
    }
    
    bool isCheckmate() const {
        return engine_.isCheckmate();
    }
    
    bool isStalemate() const {
        return engine_.isStalemate();
    }
    
    bool isDraw() const {
        return engine_.isDraw();
    }
    
    std::string getGameResult() const {
        return engine_.getGameResult();
    }
    
    std::string getPieceAt(const std::string& square) const {
        return engine_.getPieceAt(square);
    }
    
    int getHalfmoveClock() const {
        return engine_.getHalfmoveClock();
    }
    
    int getFullmoveNumber() const {
        return engine_.getFullmoveNumber();
    }
    
    bool canCastleKingside() const {
        return engine_.canCastle("kingside");
    }
    
    bool canCastleQueenside() const {
        return engine_.canCastle("queenside");
    }
    
    std::string getEnPassantSquare() const {
        return engine_.getEnPassantSquare();
    }
};

EMSCRIPTEN_BINDINGS(chess_engine) {
    class_<ChessEngineWrapper>("ChessEngine")
        .constructor<>()
        .function("newGame", &ChessEngineWrapper::newGame)
        .function("loadFromFEN", &ChessEngineWrapper::loadFromFEN)
        .function("getFEN", &ChessEngineWrapper::getFEN)
        .function("makeMove", &ChessEngineWrapper::makeMove)
        .function("undoMove", &ChessEngineWrapper::undoMove)
        .function("getLegalMoves", &ChessEngineWrapper::getLegalMoves)
        .function("getLegalMovesFrom", &ChessEngineWrapper::getLegalMovesFrom)
        .function("isMoveLegal", &ChessEngineWrapper::isMoveLegal)
        .function("getCurrentPlayer", &ChessEngineWrapper::getCurrentPlayer)
        .function("isInCheck", &ChessEngineWrapper::isInCheck)
        .function("isCheckmate", &ChessEngineWrapper::isCheckmate)
        .function("isStalemate", &ChessEngineWrapper::isStalemate)
        .function("isDraw", &ChessEngineWrapper::isDraw)
        .function("getGameResult", &ChessEngineWrapper::getGameResult)
        .function("getPieceAt", &ChessEngineWrapper::getPieceAt)
        .function("getHalfmoveClock", &ChessEngineWrapper::getHalfmoveClock)
        .function("getFullmoveNumber", &ChessEngineWrapper::getFullmoveNumber)
        .function("canCastleKingside", &ChessEngineWrapper::canCastleKingside)
        .function("canCastleQueenside", &ChessEngineWrapper::canCastleQueenside)
        .function("getEnPassantSquare", &ChessEngineWrapper::getEnPassantSquare);
}
