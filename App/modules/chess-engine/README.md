# Chess Engine Native Module

This is a local Expo module that exposes the C++ chess engine to React Native as a native module.

## Architecture

- **C++ Engine**: Core chess logic in `/engine/Core/`
- **iOS Bridge**: Objective-C++ wrapper in `ios/ChessEngineWrapper.mm`
- **Android Bridge**: JNI wrapper in `android/src/main/jni/chess-engine.cpp`
- **TypeScript Interface**: Expo module bindings in `src/ChessEngineModule.ts`

## Setup

### Prerequisites

1. This module requires a **development build** (not Expo Go)
2. Make sure you have:
   - Xcode (for iOS)
   - Android Studio & NDK (for Android)
   - Node.js and npm

### Building the Development Build

```bash
# From App directory

# iOS
npx expo prebuild --platform ios
npx expo run:ios

# Android
npx expo prebuild --platform android
npx expo run:android
```

### Manual iOS Setup (if needed)

If the module isn't automatically linked:

1. Run `pod install` in the `ios` directory
2. Open `ios/ChessApp.xcworkspace` in Xcode
3. Build and run

### Manual Android Setup (if needed)

The module should be auto-linked. If you encounter issues:

1. Ensure NDK is installed via Android Studio
2. Clean and rebuild:
   ```bash
   cd android
   ./gradlew clean
   cd ..
   npx expo run:android
   ```

## Usage

The module is already integrated in `App/lib/chess/ChessEngineNative.ts` and exported from `App/lib/chess/index.ts`.

```typescript
import { ChessEngine } from '@/lib/chess'

const engine = new ChessEngine()
engine.newGame()
engine.makeMove('e2e4')
const board = engine.getBoard()
```

## API

All chess logic runs in native code:

- `newGame()`: Start a new game
- `makeMove(algebraic: string)`: Make a move (e.g., 'e2e4', 'e7e8q')
- `getLegalMovesFrom(square: string)`: Get legal moves from a square
- `getBoard()`: Get current board state (64-element array)
- `getCurrentPlayer()`: Get current player ('white' or 'black')
- `isInCheck()`, `isCheckmate()`, `isStalemate()`, `isDraw()`: Game state queries
- `getFEN()`, `loadFromFEN(fen)`: FEN string support

## Development

### Modifying C++ Code

1. Edit files in `/engine/Core/`
2. Rebuild the development build:

   ```bash
   # iOS
   npx expo run:ios

   # Android
   npx expo run:android
   ```

### Debugging

- **iOS**: Use Xcode debugger with breakpoints in `.mm` or `.cpp` files
- **Android**: Use Android Studio debugger with breakpoints in `.cpp` files
- **JavaScript**: Use React Native debugger as normal

## Performance

The native module offers significant performance benefits:

- All chess logic runs in compiled C++
- No JavaScript bridge overhead for computations
- Move generation and validation happen natively
- Board state maintained in native memory

## Troubleshooting

### iOS: Module not found

```bash
cd ios
pod install
cd..
npx expo run:ios
```

### Android: CMake errors

Ensure you have CMake and NDK installed via Android Studio SDK Manager.

### TypeScript errors

```bash
npm install
npx expo prebuild --clean
```

## Files Structure

```
modules/chess-engine/
├── package.json
├── expo-module.config.json
├── chess-engine.podspec (iOS)
├── index.ts (TypeScript entry)
├── src/
│   └── ChessEngineModule.ts (Native module interface)
├── ios/
│   ├── ChessEngineModule.swift (Swift module)
│   ├── ChessEngineWrapper.h (Objective-C header)
│   └── ChessEngineWrapper.mm (Objective-C++ bridge)
└── android/
    ├── build.gradle
    └── src/main/
        ├── java/expo/modules/chessengine/
        │   └── ChessEngineModule.kt (Kotlin module)
        └── jni/
            ├── CMakeLists.txt
            └── chess-engine.cpp (JNI bridge)
```
