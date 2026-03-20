#import "ChessEngineWrapper.h"
#include "ChessEngine.h"
#include "Move.h"
#include "v2/Search.h"
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
    if (!_engine) return;
    _engine->newGame();
}

- (BOOL)makeMove:(NSString *)move {
    if (!_engine) return NO;
    std::string moveStr = [move UTF8String];
    return _engine->makeMove(moveStr);
}

- (void)undoMove {
    if (!_engine) return;
    _engine->undoMove();
}

- (NSArray<NSString *> *)getLegalMoves {
    if (!_engine) return @[];
    std::vector<std::string> moves = _engine->getLegalMoves();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:moves.size()];
    for (const auto& move : moves) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (NSArray<NSString *> *)getLegalMovesFrom:(NSString *)square {
    if (!_engine) return @[];
    std::string squareStr = [square UTF8String];
    std::vector<std::string> moves = _engine->getLegalMovesFrom(squareStr);
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:moves.size()];
    for (const auto& move : moves) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (BOOL)isMoveLegal:(NSString *)move {
    if (!_engine) return NO;
    std::string moveStr = [move UTF8String];
    return _engine->isMoveLegal(moveStr);
}

- (NSString *)getCurrentPlayer {
    if (!_engine) return @"white";
    std::string player = _engine->getCurrentPlayerString();
    return [NSString stringWithUTF8String:player.c_str()];
}

- (BOOL)isInCheck {
    if (!_engine) return NO;
    return _engine->isInCheck();
}

- (BOOL)isCheckmate {
    if (!_engine) return NO;
    return _engine->isCheckmate();
}

- (BOOL)isStalemate {
    if (!_engine) return NO;
    return _engine->isStalemate();
}

- (BOOL)isDraw {
    if (!_engine) return NO;
    return _engine->isDraw();
}

- (NSArray<NSDictionary *> *)getBoard {
    if (!_engine) return @[];

    NSMutableArray *board = [NSMutableArray arrayWithCapacity:64];
    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        std::string piece = _engine->getPieceAt(row, col);

        if (piece.empty() || piece == "-" || piece == "none") {
            [board addObject:@{}];
        } else {
            NSString *pieceStr = [NSString stringWithUTF8String:piece.c_str()];
            NSArray *parts = [pieceStr componentsSeparatedByString:@"_"];
            if (parts.count == 2) {
                [board addObject:@{ @"type": parts[1], @"color": parts[0] }];
            } else {
                [board addObject:@{}];
            }
        }
    }

    return board;
}

- (NSString *)getFEN {
    if (!_engine) return @"";
    std::string fen = _engine->getFEN();
    return [NSString stringWithUTF8String:fen.c_str()];
}

- (BOOL)loadFromFEN:(NSString *)fen {
    if (!_engine) return NO;
    std::string fenStr = [fen UTF8String];
    return _engine->loadFromFEN(fenStr);
}

- (NSString *)getBestMove:(int)depth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion {
    if (!_engine) return @"";
    std::string versionStr = aiVersion ? [aiVersion UTF8String] : "v1";
    std::string bestMove = _engine->getBestMove(depth, maxTimeMs, versionStr);
    return [NSString stringWithUTF8String:bestMove.c_str()];
}

- (NSString *)getBestMoveAtDepth:(int)depth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion {
    if (!_engine) return @"";
    std::string versionStr = aiVersion ? [aiVersion UTF8String] : "v1";
    std::string bestMove = _engine->getBestMoveAtDepth(depth, maxTimeMs, versionStr);
    return [NSString stringWithUTF8String:bestMove.c_str()];
}

- (NSDictionary *)searchBestMove:(int)maxDepth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion {
    return [self searchBestMove:0 maxDepth:maxDepth maxTimeMs:maxTimeMs aiVersion:aiVersion progress:nil];
}

- (NSDictionary *)searchBestMove:(int)searchId
                        maxDepth:(int)maxDepth
                       maxTimeMs:(int)maxTimeMs
                       aiVersion:(NSString *)aiVersion
                        progress:(ChessSearchProgressBlock)progress {
    if (!_engine) {
        return @{
            @"searchId": @(searchId),
            @"bestMove": @"",
            @"score": @0,
            @"depthCompleted": @0,
            @"nodesSearched": @0,
            @"timedOut": @NO,
            @"cancelled": @NO,
            @"totalTimeMs": @0,
            @"progressHistory": @[]
        };
    }

    std::string versionStr = aiVersion ? [aiVersion UTF8String] : "v1";
    Chess::ChessEngine::SearchResultData result = _engine->searchBestMove(
        maxDepth,
        maxTimeMs,
        versionStr,
        [&](const Chess::ChessEngine::SearchProgressData& progressData) {
            if (!progress) {
                return;
            }

            NSDictionary *progressDict = @{
                @"searchId": @(searchId),
                @"depth": @(progressData.depth),
                @"bestMove": [NSString stringWithUTF8String:progressData.bestMove.c_str()],
                @"score": @(progressData.score),
                @"nodesSearched": @(progressData.nodesSearched),
                @"timeMs": @(progressData.timeMs)
            };
            progress(progressDict);
        }
    );

    NSMutableArray *progressArray = [NSMutableArray arrayWithCapacity:result.progressHistory.size()];
    for (const auto& progressData : result.progressHistory) {
        NSDictionary *progressDict = @{
            @"searchId": @(searchId),
            @"depth": @(progressData.depth),
            @"bestMove": [NSString stringWithUTF8String:progressData.bestMove.c_str()],
            @"score": @(progressData.score),
            @"nodesSearched": @(progressData.nodesSearched),
            @"timeMs": @(progressData.timeMs)
        };
        [progressArray addObject:progressDict];
    }

    return @{
        @"searchId": @(searchId),
        @"bestMove": [NSString stringWithUTF8String:result.bestMove.c_str()],
        @"score": @(result.score),
        @"depthCompleted": @(result.depthCompleted),
        @"nodesSearched": @(result.nodesSearched),
        @"timedOut": @(result.timedOut),
        @"cancelled": @(result.cancelled),
        @"totalTimeMs": @(result.totalTimeMs),
        @"progressHistory": progressArray
    };
}

- (NSArray<NSString *> *)getMoveHistory {
    if (!_engine) return @[];
    std::vector<std::string> history = _engine->getMoveHistory();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:history.size()];
    for (const auto& move : history) {
        [result addObject:[NSString stringWithUTF8String:move.c_str()]];
    }
    return result;
}

- (BOOL)canUndo {
    if (!_engine) return NO;
    return _engine->canUndo();
}

- (int)evaluatePosition {
    if (!_engine) return 0;
    return _engine->evaluatePosition();
}

- (void)clearSearchCaches {
    if (!_engine) return;
    _engine->clearSearchCaches();
}

@end
