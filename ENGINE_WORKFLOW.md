# Engine Development Workflow

## Overview

The C++ chess engine files are maintained in the root `/engine/` directory and automatically synced to the iOS and Android module directories.

## Directory Structure

```
/engine/                          (Source of truth - edit here!)
├── Core/                         → Board, ChessEngine, Move, Types
├── v1/                           → v1 Search & Evaluation
└── v2/                           → v2 Search & Evaluation

/App/modules/chess-engine/
├── ios/                          (Auto-synced - do not edit)
│   ├── Core/
│   ├── v1/
│   └── v2/
└── cpp/engine/                   (Auto-synced - do not edit)
    ├── Core/
    ├── v1/
    └── v2/
```

## Workflow

### 1. Edit C++ Engine Files

Make all changes in `/engine/` directory:

```bash
cd engine
# Edit files in Core/, v1/, or v2/
vim v2/Search.cpp
```

### 2. Sync to Modules

From the App directory, run:

```bash
cd App
npm run sync-engine
```

This copies all `.h` and `.cpp` files from:

- `/engine/Core` → `ios/Core` & `cpp/engine/Core`
- `/engine/v1` → `ios/v1` & `cpp/engine/v1`
- `/engine/v2` → `ios/v2` & `cpp/engine/v2`

### 3. Rebuild

After syncing, rebuild the native modules:

```bash
npm run ios      # For iOS
npm run android  # For Android
```

## Important Notes

⚠️ **Always edit in `/engine/`** - The `ios/` and `cpp/engine/` copies are automatically generated

✅ **Run `npm run sync-engine`** before building to ensure changes are propagated

🔄 **The sync script uses `rsync --delete`** - manual edits in ios/cpp directories will be overwritten

## Testing Changes

1. Edit files in `/engine/`
2. Run `npm run sync-engine`
3. Run `npm run ios` or `npm run android`
4. Test in simulator/emulator

## Files Synced

Only C++ source files are synced:

- `*.h` (headers)
- `*.cpp` (implementations)

Build files, binaries, and other files are not synced.
