import ExpoModulesCore

public class ChessEngineModule: Module {
  private var engine: ChessEngineWrapper?
  private let initLock = NSLock()
  
  public func definition() -> ModuleDefinition {
    Name("ChessEngine")
    
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
      engine.newGame()
    }
    
    Function("makeMove") { (move: String) -> Bool in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in makeMove")
        return false
      }
      return engine.makeMove(move)
    }
    
    Function("undoMove") {
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in undoMove")
        return
      }
      engine.undoMove()
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
      return engine.load(fromFEN: fen)
    }
    
    AsyncFunction("getBestMove") { (depth: Int, maxTimeMs: Int, promise: Promise) in
      self.ensureInitialized()
      guard let engine = self.engine else {
        NSLog("ChessEngine: Engine not initialized in getBestMove")
        promise.resolve("")
        return
      }
      
      // Run on background thread to avoid blocking UI
      DispatchQueue.global(qos: .userInitiated).async {
        let move = engine.getBestMove(Int32(depth), maxTimeMs: Int32(maxTimeMs))
        promise.resolve(move)
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
