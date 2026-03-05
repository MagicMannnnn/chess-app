# Chess Engine

A high-performance C++ chess engine with complete chess rules implementation, designed to be compiled to WebAssembly for use in React Native applications.

## Features

- **Complete Chess Rules**: All standard chess moves including:
  - Piece movement (Pawn, Knight, Bishop, Rook, Queen, King)
  - Castling (both kingside and queenside)
  - En passant captures
  - Pawn promotion
  - Check, checkmate, and stalemate detection
  - Draw conditions (fifty-move rule, insufficient material)

- **Efficient Implementation**:
  - Fast move generation
  - Legal move validation
  - Move make/unmake for easy integration with AI
  - FEN string support for position import/export

- **Clean API**: High-level ChessEngine interface for easy integration

## Project Structure

```
engine/
├── CMakeLists.txt          # Build configuration
├── .gitignore              # Git ignore rules
├── README.md               # This file
└── Core/                   # Core chess engine
    ├── Types.h             # Basic types and enums
    ├── Move.h/cpp          # Move representation
    ├── Board.h/cpp         # Board state and move generation
    └── ChessEngine.h/cpp   # Main engine interface
```

## Building

### Native Build (for testing)

```bash
mkdir build
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

### WebAssembly Build (for React Native app)

Install Emscripten SDK first: https://emscripten.org/docs/getting_started/downloads.html

```bash
mkdir build-wasm
cd build-wasm
emcmake cmake ..
emmake make
```

This will generate `chess_engine.wasm` that can be used in the React Native app.

## Usage Example

```cpp
#include "Core/ChessEngine.h"

using namespace Chess;

int main() {
    ChessEngine engine;

    // Start a new game
    engine.newGame();

    // Make moves using algebraic notation
    engine.makeMove("e2e4");  // e4
    engine.makeMove("e7e5");  // e5
    engine.makeMove("g1f3");  // Nf3

    // Get legal moves
    auto moves = engine.getLegalMoves();

    // Get legal moves from a specific square
    auto knightMoves = engine.getLegalMovesFrom("b8");

    // Check game state
    if (engine.isInCheck()) {
        std::cout << "Check!" << std::endl;
    }

    if (engine.isCheckmate()) {
        std::cout << "Checkmate!" << std::endl;
    }

    // Get FEN representation
    std::string fen = engine.getFEN();

    // Load from FEN
    engine.loadFromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

    // Undo move
    engine.undoMove();

    return 0;
}
```

## API Reference

### ChessEngine Class

#### Game Initialization

- `void newGame()` - Start a new game with standard starting position
- `bool loadFromFEN(const std::string& fen)` - Load position from FEN string
- `std::string getFEN()` - Get current position as FEN string

#### Move Operations

- `bool makeMove(const std::string& algebraic)` - Make a move (e.g., "e2e4", "e7e8q")
- `void undoMove()` - Undo the last move

#### Move Queries

- `std::vector<std::string> getLegalMoves()` - Get all legal moves
- `std::vector<std::string> getLegalMovesFrom(const std::string& square)` - Get legal moves from square
- `bool isMoveLegal(const std::string& algebraic)` - Check if move is legal

#### Game State

- `std::string getCurrentPlayerString()` - Get current player ("white" or "black")
- `bool isInCheck()` - Check if current player is in check
- `bool isCheckmate()` - Check if current player is checkmated
- `bool isStalemate()` - Check if position is stalemate
- `bool isDraw()` - Check if position is drawn
- `std::string getGameResult()` - Get game result

#### Board Queries

- `std::string getPieceAt(const std::string& square)` - Get piece at square (e.g., "white_pawn")
- `std::string getBoardString()` - Get ASCII representation of board

## Integration with React Native

Once compiled to WASM, the engine can be integrated into the React Native app:

1. Compile engine to WASM using Emscripten
2. Load WASM module in React Native using a WASM runtime
3. Call engine functions from JavaScript/TypeScript
4. Use in the sandbox screen for chess gameplay

## Performance Considerations

- Move generation is optimized for speed
- Board representation uses simple array for fast access
- Bitflags used for game state (castling rights, etc.)
- Legal move validation done efficiently

## Future Enhancements

- AI/Search (minimax with alpha-beta pruning)
- Position evaluation function
- Opening book
- Endgame tablebase support
- Threefold repetition detection
- Move history tracking
- PGN support

## License

See LICENSE file in project root.
