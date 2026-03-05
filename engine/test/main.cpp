#include <iostream>
#include <iomanip>
#include "../Core/ChessEngine.h"

using namespace Chess;

void printMoves(const std::vector<std::string>& moves) {
    std::cout << "Legal moves (" << moves.size() << "): ";
    for (size_t i = 0; i < moves.size(); i++) {
        std::cout << moves[i];
        if (i < moves.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
}

void printGameState(const ChessEngine& engine) {
    std::cout << "\n" << engine.getBoardString() << std::endl;
    std::cout << "Turn: " << engine.getCurrentPlayerString() << std::endl;
    std::cout << "FEN: " << engine.getFEN() << std::endl;
    
    if (engine.isInCheck()) {
        std::cout << "Status: CHECK" << std::endl;
    }
    
    if (engine.isCheckmate()) {
        std::cout << "Status: CHECKMATE" << std::endl;
    }
    
    if (engine.isStalemate()) {
        std::cout << "Status: STALEMATE" << std::endl;
    }
    
    if (engine.isDraw()) {
        std::cout << "Status: DRAW" << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "=== Chess Engine Test ===" << std::endl;
    
    ChessEngine engine;
    
    // Test 1: Initial position
    std::cout << "\n--- Test 1: Initial Position ---" << std::endl;
    engine.newGame();
    printGameState(engine);
    
    auto moves = engine.getLegalMoves();
    std::cout << "Number of legal moves in starting position: " << moves.size() << std::endl;
    
    // Test 2: Make some moves (Scholar's Mate)
    std::cout << "\n--- Test 2: Scholar's Mate ---" << std::endl;
    engine.newGame();
    
    std::cout << "1. e4..." << std::endl;
    engine.makeMove("e2e4");
    
    std::cout << "1... e5" << std::endl;
    engine.makeMove("e7e5");
    printGameState(engine);
    
    std::cout << "2. Bc4..." << std::endl;
    engine.makeMove("f1c4");
    
    std::cout << "2... Nc6" << std::endl;
    engine.makeMove("b8c6");
    
    std::cout << "3. Qh5..." << std::endl;
    engine.makeMove("d1h5");
    
    std::cout << "3... Nf6??" << std::endl;
    engine.makeMove("g8f6");
    
    std::cout << "4. Qxf7#" << std::endl;
    engine.makeMove("h5f7");
    
    printGameState(engine);
    std::cout << "Result: " << engine.getGameResult() << std::endl;
    
    // Test 3: Castling
    std::cout << "\n--- Test 3: Castling ---" << std::endl;
    engine.newGame();
    
    // Clear the way for kingside castling
    engine.makeMove("e2e4");
    engine.makeMove("e7e5");
    engine.makeMove("g1f3");
    engine.makeMove("b8c6");
    engine.makeMove("f1e2");
    engine.makeMove("g8f6");
    
    std::cout << "White can castle kingside: " 
              << (engine.canCastle("kingside") ? "Yes" : "No") << std::endl;
    
    auto kingMoves = engine.getLegalMovesFrom("e1");
    std::cout << "King moves from e1: ";
    printMoves(kingMoves);
    
    if (engine.canCastle("kingside")) {
        std::cout << "\nCastling kingside..." << std::endl;
        engine.makeMove("e1g1");
        printGameState(engine);
    }
    
    // Test 4: En Passant
    std::cout << "\n--- Test 4: En Passant ---" << std::endl;
    engine.loadFromFEN("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1");
    printGameState(engine);
    
    std::cout << "En passant target: " << engine.getEnPassantSquare() << std::endl;
    
    auto pawnMoves = engine.getLegalMovesFrom("e5");
    std::cout << "Pawn moves from e5: ";
    printMoves(pawnMoves);
    
    if (engine.isMoveLegal("e5d6")) {
        std::cout << "\nCapturing en passant..." << std::endl;
        engine.makeMove("e5d6");
        printGameState(engine);
    }
    
    // Test 5: Promotion
    std::cout << "\n--- Test 5: Pawn Promotion ---" << std::endl;
    engine.loadFromFEN("8/4P3/8/8/8/8/8/4k2K w - - 0 1");
    printGameState(engine);
    
    std::cout << "Promoting pawn to queen..." << std::endl;
    engine.makeMove("e7e8q");
    printGameState(engine);
    
    // Test 6: FEN import/export
    std::cout << "\n--- Test 6: FEN Import/Export ---" << std::endl;
    std::string testFEN = "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4";
    std::cout << "Loading FEN: " << testFEN << std::endl;
    engine.loadFromFEN(testFEN);
    printGameState(engine);
    
    std::cout << "Exported FEN: " << engine.getFEN() << std::endl;
    
    // Test 7: Move validation
    std::cout << "\n--- Test 7: Move Validation ---" << std::endl;
    engine.newGame();
    
    std::cout << "Is 'e2e4' legal? " << (engine.isMoveLegal("e2e4") ? "Yes" : "No") << std::endl;
    std::cout << "Is 'e2e5' legal? " << (engine.isMoveLegal("e2e5") ? "Yes" : "No") << std::endl;
    std::cout << "Is 'a1a8' legal? " << (engine.isMoveLegal("a1a8") ? "Yes" : "No") << std::endl;
    
    // Test 8: Undo move
    std::cout << "\n--- Test 8: Undo Move ---" << std::endl;
    engine.newGame();
    
    std::cout << "Making moves e2e4, e7e5..." << std::endl;
    engine.makeMove("e2e4");
    engine.makeMove("e7e5");
    printGameState(engine);
    
    std::cout << "Undoing last move..." << std::endl;
    engine.undoMove();
    printGameState(engine);
    
    std::cout << "\n=== All Tests Complete ===" << std::endl;
    
    return 0;
}
