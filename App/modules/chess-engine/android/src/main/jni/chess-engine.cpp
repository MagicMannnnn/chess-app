#include <jni.h>
#include <string>
#include <vector>
#include "../../../../../engine/Core/ChessEngine.h"

using namespace Chess;

static ChessEngine* engine = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeNewGame(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    engine->newGame();
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeMakeMove(JNIEnv* env, jobject thiz, jstring move) {
    if (!engine) {
        engine = new ChessEngine();
    }
    const char* moveStr = env->GetStringUTFChars(move, nullptr);
    bool result = engine->makeMove(std::string(moveStr));
    env->ReleaseStringUTFChars(move, moveStr);
    return result;
}

JNIEXPORT void JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeUndoMove(JNIEnv* env, jobject thiz) {
    if (engine) {
        engine->undoMove();
    }
}

JNIEXPORT jobjectArray JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeGetLegalMoves(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    std::vector<std::string> moves = engine->getLegalMoves();
    jobjectArray result = env->NewObjectArray(moves.size(), env->FindClass("java/lang/String"), nullptr);
    for (size_t i = 0; i < moves.size(); i++) {
        env->SetObjectArrayElement(result, i, env->NewStringUTF(moves[i].c_str()));
    }
    return result;
}

JNIEXPORT jobjectArray JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeGetLegalMovesFrom(JNIEnv* env, jobject thiz, jstring square) {
    if (!engine) {
        engine = new ChessEngine();
    }
    const char* squareStr = env->GetStringUTFChars(square, nullptr);
    std::vector<std::string> moves = engine->getLegalMovesFrom(std::string(squareStr));
    env->ReleaseStringUTFChars(square, squareStr);
    
    jobjectArray result = env->NewObjectArray(moves.size(), env->FindClass("java/lang/String"), nullptr);
    for (size_t i = 0; i < moves.size(); i++) {
        env->SetObjectArrayElement(result, i, env->NewStringUTF(moves[i].c_str()));
    }
    return result;
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeIsMoveLegal(JNIEnv* env, jobject thiz, jstring move) {
    if (!engine) {
        engine = new ChessEngine();
    }
    const char* moveStr = env->GetStringUTFChars(move, nullptr);
    bool result = engine->isMoveLegal(std::string(moveStr));
    env->ReleaseStringUTFChars(move, moveStr);
    return result;
}

JNIEXPORT jstring JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeGetCurrentPlayer(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    std::string player = engine->getCurrentPlayerString();
    return env->NewStringUTF(player.c_str());
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeIsInCheck(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    return engine->isInCheck();
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeIsCheckmate(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    return engine->isCheckmate();
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeIsStalemate(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    return engine->isStalemate();
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeIsDraw(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    return engine->isDraw();
}

JNIEXPORT jobjectArray JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeGetBoard(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    jmethodID hashMapPut = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    
    jobjectArray result = env->NewObjectArray(64, env->FindClass("java/util/Map"), nullptr);
    
    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        std::string piece = engine->getPieceAt(row, col);
        
        if (piece.empty() || piece == "-") {
            env->SetObjectArrayElement(result, i, nullptr);
        } else {
            jobject hashMap = env->NewObject(hashMapClass, hashMapInit);
            
            std::string color = (piece[0] == 'w') ? "white" : "black";
            std::string type;
            
            switch (piece[1]) {
                case 'P': type = "pawn"; break;
                case 'N': type = "knight"; break;
                case 'B': type = "bishop"; break;
                case 'R': type = "rook"; break;
                case 'Q': type = "queen"; break;
                case 'K': type = "king"; break;
                default: type = "unknown"; break;
            }
            
            env->CallObjectMethod(hashMap, hashMapPut, 
                env->NewStringUTF("type"), env->NewStringUTF(type.c_str()));
            env->CallObjectMethod(hashMap, hashMapPut,
                env->NewStringUTF("color"), env->NewStringUTF(color.c_str()));
            
            env->SetObjectArrayElement(result, i, hashMap);
        }
    }
    
    return result;
}

JNIEXPORT jstring JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeGetFEN(JNIEnv* env, jobject thiz) {
    if (!engine) {
        engine = new ChessEngine();
    }
    std::string fen = engine->getFEN();
    return env->NewStringUTF(fen.c_str());
}

JNIEXPORT jboolean JNICALL
Java_expo_modules_chessengine_ChessEngineModule_nativeLoadFromFEN(JNIEnv* env, jobject thiz, jstring fen) {
    if (!engine) {
        engine = new ChessEngine();
    }
    const char* fenStr = env->GetStringUTFChars(fen, nullptr);
    bool result = engine->loadFromFEN(std::string(fenStr));
    env->ReleaseStringUTFChars(fen, fenStr);
    return result;
}

} // extern "C"
