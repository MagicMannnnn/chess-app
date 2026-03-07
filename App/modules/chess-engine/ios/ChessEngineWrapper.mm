#import "ChessEngineWrapper.h"
#include "ChessEngine.h"
#include "Move.h"
#include <string>
#include <vector>

using namespace Chess;

@interface ChessEngineWrapper() {
    ChessEngine *_engine;
}
@end

@implementation ChessEngineWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        @try {
            _engine = new ChessEngine();
            NSLog(@"ChessEngineWrapper: Successfully initialized C++ engine");
        } @catch (NSException *exception) {
            NSLog(@"ChessEngineWrapper: Failed to initialize - %@", exception);
            _engine = nullptr;
        }
    }
    return self;
}

- (void)dealloc {
    if (_engine) {
        delete _engine;
        _engine = nullptr;
    }
}

- (void)newGame {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in newGame");
        return;
    }
    _engine->newGame();
}

- (BOOL)makeMove:(NSString *)move {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in makeMove");
        return NO;
    }
    NSLog(@"ChessEngineWrapper: makeMove called with: %@", move);
    std::string moveStr = [move UTF8String];
    
    // Parse and log the move details
    Chess::Move parsedMove = Chess::Move::fromAlgebraic(moveStr);
    NSLog(@"ChessEngineWrapper: Parsed move - from=%d to=%d isValid=%d", 
          parsedMove.getFrom(), parsedMove.getTo(), parsedMove.isValid());
    
    // Check current state
    std::string currentPlayer = _engine->getCurrentPlayerString();
    NSLog(@"ChessEngineWrapper: Current player: %s", currentPlayer.c_str());
    
    // Check piece at from square
    int fromRow = parsedMove.getFrom() / 8;
    int fromCol = parsedMove.getFrom() % 8;
    std::string pieceAtFrom = _engine->getPieceAt(fromRow, fromCol);
    NSLog(@"ChessEngineWrapper: Piece at from square (%d,%d): %s", fromRow, fromCol, pieceAtFrom.c_str());
    
    // Check if move is legal
    bool isLegal = _engine->isMoveLegal(moveStr);
    NSLog(@"ChessEngineWrapper: isMoveLegal result: %d", isLegal);
    
    // Get legal moves from this square specifically
    std::string fromSquare = std::string(1, 'a' + fromCol) + std::to_string(fromRow + 1);
    NSLog(@"ChessEngineWrapper: Getting legal moves from square: %s", fromSquare.c_str());
    std::vector<std::string> movesFromSquare = _engine->getLegalMovesFrom(fromSquare);
    NSLog(@"ChessEngineWrapper: Legal moves from square %s: %zu moves", fromSquare.c_str(), movesFromSquare.size());
    for (const auto& m : movesFromSquare) {
        NSLog(@"ChessEngineWrapper:   - %s", m.c_str());
    }
    
    // Get all legal moves to see what's available
    std::vector<std::string> legalMoves = _engine->getLegalMoves();
    NSLog(@"ChessEngineWrapper: Total legal moves available: %zu", legalMoves.size());
    
    bool result = _engine->makeMove(moveStr);
    NSLog(@"ChessEngineWrapper: makeMove result: %d", result);
    
    // Log board state after move for debugging
    if (result) {
        std::string piece0 = _engine->getPieceAt(0, 0);
        std::string piece4 = _engine->getPieceAt(0, 4);
        NSLog(@"ChessEngineWrapper: After move - piece at (0,0): %s, piece at (0,4): %s", piece0.c_str(), piece4.c_str());
    }
    
    return result;
}

- (void)undoMove {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in undoMove");
        return;
    }
    _engine->undoMove();
}

- (NSArray<NSString *> *)getLegalMoves {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getLegalMoves");
        return @[];
    }
    std::vector<std::string> moves = _engine->getLegalMoves();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:moves.size()];
    for (const auto& move : moves) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (NSArray<NSString *> *)getLegalMovesFrom:(NSString *)square {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getLegalMovesFrom");
        return @[];
    }
    std::string squareStr = [square UTF8String];
    std::vector<std::string> moves = _engine->getLegalMovesFrom(squareStr);
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:moves.size()];
    for (const auto& move : moves) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (BOOL)isMoveLegal:(NSString *)move {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in isMoveLegal");
        return NO;
    }
    std::string moveStr = [move UTF8String];
    return _engine->isMoveLegal(moveStr);
}

- (NSString *)getCurrentPlayer {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getCurrentPlayer");
        return @"white";
    }
    std::string player = _engine->getCurrentPlayerString();
    return [NSString stringWithUTF8String:player.c_str()];
}

- (BOOL)isInCheck {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in isInCheck");
        return NO;
    }
    return _engine->isInCheck();
}

- (BOOL)isCheckmate {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in isCheckmate");
        return NO;
    }
    return _engine->isCheckmate();
}

- (BOOL)isStalemate {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in isStalemate");
        return NO;
    }
    return _engine->isStalemate();
}

- (BOOL)isDraw {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in isDraw");
        return NO;
    }
    return _engine->isDraw();
}

- (NSArray<NSDictionary *> *)getBoard {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getBoard");
        return @[];
    }
    
    NSLog(@"ChessEngineWrapper: getBoard starting");
    
    // Log some pieces for debugging
    std::string piece0 = _engine->getPieceAt(0, 0);
    std::string piece4 = _engine->getPieceAt(0, 4);
    std::string piece60 = _engine->getPieceAt(7, 4);
    NSLog(@"ChessEngineWrapper: Sample pieces - (0,0): %s, (0,4): %s, (7,4): %s", piece0.c_str(), piece4.c_str(), piece60.c_str());
    
    NSMutableArray *board = [NSMutableArray arrayWithCapacity:64];
    
    try {
        for (int i = 0; i < 64; i++) {
            int row = i / 8;
            int col = i % 8;
            
            try {
                std::string piece = _engine->getPieceAt(row, col);
                
                if (piece.empty() || piece == "-" || piece == "none") {
                    // Use empty dictionary instead of NSNull for empty squares
                    [board addObject:@{}];
                } else {
                    // Parse piece string (e.g., "white_pawn", "black_king")
                    NSString *pieceStr = [NSString stringWithUTF8String:piece.c_str()];
                    NSArray *parts = [pieceStr componentsSeparatedByString:@"_"];
                    
                    if (parts.count == 2) {
                        NSString *color = parts[0];  // "white" or "black"
                        NSString *type = parts[1];   // "pawn", "knight", etc.
                        [board addObject:@{@"type": type, @"color": color}];
                    } else {
                        // Use empty dictionary instead of NSNull
                        [board addObject:@{}];
                    }
                }
            } catch (const std::exception& e) {
                NSLog(@"ChessEngineWrapper: C++ exception getting piece at (%d,%d): %s", row, col, e.what());
                // Use empty dictionary instead of NSNull
                [board addObject:@{}];
            } catch (...) {
                NSLog(@"ChessEngineWrapper: Unknown exception getting piece at (%d,%d)", row, col);
                // Use empty dictionary instead of NSNull
                [board addObject:@{}];
            }
        }
    } catch (const std::exception& e) {
        NSLog(@"ChessEngineWrapper: C++ exception in getBoard: %s", e.what());
        return @[];
    } catch (...) {
        NSLog(@"ChessEngineWrapper: Unknown exception in getBoard");
        return @[];
    }
    
    NSLog(@"ChessEngineWrapper: getBoard completed successfully");
    return board;
}

- (NSString *)getFEN {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getFEN");
        return @"";
    }
    std::string fen = _engine->getFEN();
    return [NSString stringWithUTF8String:fen.c_str()];
}

- (BOOL)loadFromFEN:(NSString *)fen {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in loadFromFEN");
        return NO;
    }
    std::string fenStr = [fen UTF8String];
    return _engine->loadFromFEN(fenStr);
}

- (NSString *)getBestMove:(int)depth {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getBestMove");
        return @"";
    }
    std::string bestMove = _engine->getBestMove(depth);
    return [NSString stringWithUTF8String:bestMove.c_str()];
}

- (NSArray<NSString *> *)getMoveHistory {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in getMoveHistory");
        return @[];
    }
    std::vector<std::string> history = _engine->getMoveHistory();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:history.size()];
    for (const auto& move : history) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (BOOL)canUndo {
    if (!_engine) {
        return NO;
    }
    return _engine->canUndo();
}

- (int)evaluatePosition {
    if (!_engine) {
        NSLog(@"ChessEngineWrapper: Engine is null in evaluatePosition");
        return 0;
    }
    return _engine->evaluatePosition();
}

@end
