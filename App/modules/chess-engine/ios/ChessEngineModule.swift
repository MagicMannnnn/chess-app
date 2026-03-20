import ExpoModulesCore

public class ChessEngineModule: Module {
  private var engine: ChessEngineWrapper?
  private let initLock = NSLock()
  private let engineQueue = DispatchQueue(label: "com.chessapp.engine.queue", qos: .userInitiated)

  public func definition() -> ModuleDefinition {
    Name("ChessEngine")

    Events("onSearchProgress")

    OnCreate {
      self.initLock.lock()
      defer { self.initLock.unlock() }
      if self.engine == nil {
        self.engine = ChessEngineWrapper()
      }
    }

    OnDestroy {
      self.initLock.lock()
      defer { self.initLock.unlock() }
      self.engine = nil
    }

    Function("newGame") {
      self.ensureInitialized()
      guard let engine = self.engine else { return }
      engine.clearSearchCaches()
      self.engineQueue.sync {
        engine.newGame()
      }
    }

    Function("makeMove") { (move: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else { return false }
      engine.clearSearchCaches()
      return self.engineQueue.sync {
        engine.makeMove(move)
      }
    }

    Function("undoMove") {
      self.ensureInitialized()
      guard let engine = self.engine else { return }
      engine.clearSearchCaches()
      self.engineQueue.sync {
        engine.undoMove()
      }
    }

    Function("getLegalMoves") { () -> [String] in
      self.ensureInitialized()
      return self.engine?.getLegalMoves() ?? []
    }

    Function("getLegalMovesFrom") { (square: String) -> [String] in
      self.ensureInitialized()
      return self.engine?.getLegalMoves(from: square) ?? []
    }

    Function("isMoveLegal") { (move: String) -> Bool in
      self.ensureInitialized()
      return self.engine?.isMoveLegal(move) ?? false
    }

    Function("getCurrentPlayer") { () -> String in
      self.ensureInitialized()
      return self.engine?.getCurrentPlayer() ?? "white"
    }

    Function("isInCheck") { () -> Bool in
      self.ensureInitialized()
      return self.engine?.isInCheck() ?? false
    }

    Function("isCheckmate") { () -> Bool in
      self.ensureInitialized()
      return self.engine?.isCheckmate() ?? false
    }

    Function("isStalemate") { () -> Bool in
      self.ensureInitialized()
      return self.engine?.isStalemate() ?? false
    }

    Function("isDraw") { () -> Bool in
      self.ensureInitialized()
      return self.engine?.isDraw() ?? false
    }

    Function("getBoard") { () -> [[String: Any]] in
      self.ensureInitialized()
      let board = self.engine?.getBoard() ?? []
      return board.compactMap { item in
        if let dict = item as? [String: Any] {
          return dict
        }
        return [:]
      }
    }

    Function("getFEN") { () -> String in
      self.ensureInitialized()
      return self.engine?.getFEN() ?? ""
    }

    Function("loadFromFEN") { (fen: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else { return false }
      engine.clearSearchCaches()
      return self.engineQueue.sync {
        engine.load(fromFEN: fen)
      }
    }

    AsyncFunction("getBestMove") { (depth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        promise.resolve("")
        return
      }

      self.engineQueue.async {
        let move = engine.getBestMove(Int32(depth), maxTimeMs: Int32(maxTimeMs), aiVersion: aiVersion)
        promise.resolve(move)
      }
    }

    AsyncFunction("getBestMoveAtDepth") { (depth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        promise.resolve("")
        return
      }

      self.engineQueue.async {
        let move = engine.getBestMove(atDepth: Int32(depth), maxTimeMs: Int32(maxTimeMs), aiVersion: aiVersion)
        promise.resolve(move)
      }
    }

    AsyncFunction("searchBestMove") { (searchId: Int, maxDepth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        promise.resolve([
          "searchId": searchId,
          "bestMove": "",
          "score": 0,
          "depthCompleted": 0,
          "nodesSearched": 0,
          "timedOut": false,
          "cancelled": false,
          "totalTimeMs": 0,
          "progressHistory": []
        ])
        return
      }

      self.engineQueue.async {
        let result = engine.searchBestMove(
          Int32(searchId),
          maxDepth: Int32(maxDepth),
          maxTimeMs: Int32(maxTimeMs),
          aiVersion: aiVersion
        ) { progress in
          DispatchQueue.main.async {
            // Convert [AnyHashable : Any] to [String : Any?] for sendEvent
            let progressDict = progress.reduce(into: [String: Any?]()) { result, pair in
              if let key = pair.key as? String {
                result[key] = pair.value
              }
            }
            self.sendEvent("onSearchProgress", progressDict)
          }
        }
        promise.resolve(result)
      }
    }

    Function("getMoveHistory") { () -> [String] in
      self.ensureInitialized()
      return self.engine?.getMoveHistory() ?? []
    }

    Function("canUndo") { () -> Bool in
      self.ensureInitialized()
      return self.engine?.canUndo() ?? false
    }

    Function("evaluatePosition") { () -> Int in
      self.ensureInitialized()
      return Int(self.engine?.evaluatePosition() ?? 0)
    }

    Function("clearSearchCaches") {
      self.ensureInitialized()
      self.engine?.clearSearchCaches()
    }
  }

  private func ensureInitialized() {
    initLock.lock()
    defer { initLock.unlock() }
    if engine == nil {
      engine = ChessEngineWrapper()
    }
  }
}
