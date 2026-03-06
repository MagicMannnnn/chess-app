#import "ChessEngineWrapper.h"
#include "ChessEngine.h"
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
    std::string moveStr = [move UTF8String];
    return _engine->makeMove(moveStr);
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

@end
