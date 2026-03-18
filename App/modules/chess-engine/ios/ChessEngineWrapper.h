#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface ChessEngineWrapper : NSObject

- (instancetype)init;
- (void)newGame;
- (BOOL)makeMove:(NSString *)move;
- (void)undoMove;
- (NSArray<NSString *> *)getLegalMoves;
- (NSArray<NSString *> *)getLegalMovesFrom:(NSString *)square;
- (BOOL)isMoveLegal:(NSString *)move;
- (NSString *)getCurrentPlayer;
- (BOOL)isInCheck;
- (BOOL)isCheckmate;
- (BOOL)isStalemate;
- (BOOL)isDraw;
- (NSArray<NSDictionary *> *)getBoard;
- (NSString *)getFEN;
- (BOOL)loadFromFEN:(NSString *)fen;
- (NSString *)getBestMove:(int)depth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion;
- (NSString *)getBestMoveAtDepth:(int)depth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion;
- (NSDictionary *)searchBestMove:(int)maxDepth maxTimeMs:(int)maxTimeMs aiVersion:(NSString *)aiVersion;
- (NSArray<NSString *> *)getMoveHistory;
- (BOOL)canUndo;
- (int)evaluatePosition;
- (void)clearSearchCaches;

@end

NS_ASSUME_NONNULL_END
