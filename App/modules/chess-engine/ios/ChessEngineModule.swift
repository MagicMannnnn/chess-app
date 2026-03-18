import ExpoModulesCore

public class ChessEngineModule: Module {
  private var engine: ChessEngineWrapper?
  private let initLock = NSLock()
  private let engineQueue = DispatchQueue(label: "com.chessapp.engine.queue", qos: .userInitiated)
  
  public func definition() -> ModuleDefinition {
    Name("ChessEngine")
    
    // Event emitter for search progress updates
    Events("onSearchProgress")
    
    OnCreate {
      self.initLock.lock()
      defer { self.initLock.unlock() }
      if self.engine == nil {
        do {
          self.engine = ChessEngineWrapper()
          NSLog("ChessEngine: Module initialized successfully")
        } catch {
          NSLog("Error initializing ChessEngineWrapper: \(error)")
        }
      }
    }
    
    OnDestroy {
      self.initLock.lock()
      defer { self.initLock.unlock() }
      self.engine = nil
    }
    
    Function("newGame") {
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in newGame")
        return
      }
      engine.clearSearchCaches()
      self.engineQueue.sync {
        engine.newGame()
      }
    }
    
    Function("makeMove") { (move: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in makeMove")
        return false
      }
      engine.clearSearchCaches()
      return self.engineQueue.sync {
        engine.makeMove(move)
      }
    }
    
    Function("undoMove") {
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in undoMove")
        return
      }
      engine.clearSearchCaches()
      self.engineQueue.sync {
        engine.undoMove()
      }
    }
    
    Function("getLegalMoves") { () -> [String] in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getLegalMoves")
        return []
      }
      return engine.getLegalMoves()
    }
    
    Function("getLegalMovesFrom") { (square: String) -> [String] in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getLegalMovesFrom")
        return []
      }
      return engine.getLegalMoves(from: square)
    }
    
    Function("isMoveLegal") { (move: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in isMoveLegal")
        return false
      }
      return engine.isMoveLegal(move)
    }
    
    Function("getCurrentPlayer") { () -> String in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getCurrentPlayer")
        return "white"
      }
      return engine.getCurrentPlayer()
    }
    
    Function("isInCheck") { () -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in isInCheck")
        return false
      }
      return engine.isInCheck()
    }
    
    Function("isCheckmate") { () -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in isCheckmate")
        return false
      }
      return engine.isCheckmate()
    }
    
    Function("isStalemate") { () -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in isStalemate")
        return false
      }
      return engine.isStalemate()
    }
    
    Function("isDraw") { () -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in isDraw")
        return false
      }
      return engine.isDraw()
    }
    
    Function("getBoard") { () -> [[String: Any]] in
      NSLog("===== SWIFT GETBOARD START =====")
      self.ensureInitialized()
      
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getBoard")
        return []
      }
      
      NSLog("ChessEngine: Calling wrapper getBoard")
      let board = engine.getBoard()
      NSLog("ChessEngine: Got board with \(board.count) squares")
      
      // Convert NSArray to Swift array of dictionaries
      let result: [[String: Any]] = board.compactMap { item in
        if let dict = item as? [String: Any] {
          return dict
        }
        return [:]  // Empty dict for anything unexpected
      }
      
      NSLog("ChessEngine: Returning board with \(result.count) squares")
      return result
    }
    
    Function("getFEN") { () -> String in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getFEN")
        return ""
      }
      return engine.getFEN()
    }
    
    Function("loadFromFEN") { (fen: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in loadFromFEN")
        return false
      }
      engine.clearSearchCaches()
      return self.engineQueue.sync {
        engine.load(fromFEN: fen)
      }
    }
    
    AsyncFunction("getBestMove") { (depth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getBestMove")
        promise.resolve("")
        return
      }
      
      // Run on background thread to avoid blocking UI
      self.engineQueue.async {
        let move = engine.getBestMove(Int32(depth), maxTimeMs: Int32(maxTimeMs), aiVersion: aiVersion)
        promise.resolve(move)
      }
    }
    
    AsyncFunction("getBestMoveAtDepth") { (depth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getBestMoveAtDepth")
        promise.resolve("")
        return
      }
      
      // Run on background thread to avoid blocking UI
      self.engineQueue.async {
        let move = engine.getBestMove(atDepth: Int32(depth), maxTimeMs: Int32(maxTimeMs), aiVersion: aiVersion)
        promise.resolve(move)
      }
    }
    
    AsyncFunction("searchBestMove") { (maxDepth: Int, maxTimeMs: Int, aiVersion: String, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in searchBestMove")
        promise.resolve([
          "bestMove": "",
          "score": 0,
          "depthCompleted": 0,
          "nodesSearched": 0,
          "timedOut": false,
          "totalTimeMs": 0,
          "progressHistory": []
        ])
        return
      }
      
      // Run on background thread to avoid blocking UI
      self.engineQueue.async {
        NSLog("ChessEngine: Starting searchBestMove with maxDepth=\(maxDepth), maxTime=\(maxTimeMs)ms, version=\(aiVersion)")
        let result = engine.searchBestMove(Int32(maxDepth), maxTimeMs: Int32(maxTimeMs), aiVersion: aiVersion)
        NSLog("ChessEngine: Search completed - bestMove=\(result["bestMove"] ?? "none"), depth=\(result["depthCompleted"] ?? 0)")
        
        // Emit progress events for each completed depth BEFORE resolving promise
        if let progressHistory = result["progressHistory"] as? [[String: Any]] {
          for progress in progressHistory {
            self.sendEvent("onSearchProgress", progress)
            // Small delay to ensure events are processed in order
            Thread.sleep(forTimeInterval: 0.01)
          }
        }
        
        promise.resolve(result)
      }
    }
    
    Function("getMoveHistory") { () -> [String] in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getMoveHistory")
        return []
      }
      return engine.getMoveHistory()
    }
    
    Function("canUndo") { () -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in canUndo")
        return false
      }
      return engine.canUndo()
    }
    
    Function("evaluatePosition") { () -> Int in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in evaluatePosition")
        return 0
      }
      return Int(engine.evaluatePosition())
    }

    Function("clearSearchCaches") {
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in clearSearchCaches")
        return
      }
      engine.clearSearchCaches()
    }
  }
  
  private func ensureInitialized() {
    initLock.lock()
    defer { initLock.unlock() }
    if engine == nil {
      engine = ChessEngineWrapper()
      NSLog("ChessEngine: Lazy initialized in ensureInitialized")
    }
  }
}
