package expo.modules.chessengine

import expo.modules.kotlin.modules.Module
import expo.modules.kotlin.modules.ModuleDefinition

class ChessEngineModule : Module() {
  private external fun nativeNewGame()
  private external fun nativeMakeMove(move: String): Boolean
  private external fun nativeUndoMove()
  private external fun nativeGetLegalMoves(): Array<String>
  private external fun nativeGetLegalMovesFrom(square: String): Array<String>
  private external fun nativeIsMoveLegal(move: String): Boolean
  private external fun nativeGetCurrentPlayer(): String
  private external fun nativeIsInCheck(): Boolean
  private external fun nativeIsCheckmate(): Boolean
  private external fun nativeIsStalemate(): Boolean
  private external fun nativeIsDraw(): Boolean
  private external fun nativeGetBoard(): Array<Map<String, String>?>
  private external fun nativeGetFEN(): String
  private external fun nativeLoadFromFEN(fen: String): Boolean
  
  companion object {
    init {
      System.loadLibrary("chess-engine")
    }
  }

  override fun definition() = ModuleDefinition {
    Name("ChessEngine")

    Function("newGame") {
      nativeNewGame()
    }

    Function("makeMove") { move: String ->
      nativeMakeMove(move)
    }

    Function("undoMove") {
      nativeUndoMove()
    }

    Function("getLegalMoves") {
      nativeGetLegalMoves().toList()
    }

    Function("getLegalMovesFrom") { square: String ->
      nativeGetLegalMovesFrom(square).toList()
    }

    Function("isMoveLegal") { move: String ->
      nativeIsMoveLegal(move)
    }

    Function("getCurrentPlayer") {
      nativeGetCurrentPlayer()
    }

    Function("isInCheck") {
      nativeIsInCheck()
    }

    Function("isCheckmate") {
      nativeIsCheckmate()
    }

    Function("isStalemate") {
      nativeIsStalemate()
    }

    Function("isDraw") {
      nativeIsDraw()
    }

    Function("getBoard") {
      nativeGetBoard().toList()
    }

    Function("getFEN") {
      nativeGetFEN()
    }

    Function("loadFromFEN") { fen: String ->
      nativeLoadFromFEN(fen)
    }
  }
}
