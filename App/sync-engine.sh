#!/bin/bash

# Sync C++ engine files from root /engine to module directories
# This ensures iOS and Android modules stay in sync with the main engine

set -e  # Exit on error

ENGINE_ROOT="../engine"
IOS_TARGET="./modules/chess-engine/ios"
CPP_TARGET="./modules/chess-engine/cpp/engine"

echo "🔄 Syncing C++ engine files..."
echo ""

# Function to sync a directory
sync_dir() {
    local source_dir=$1
    local target_ios=$2
    local target_cpp=$3
    local dir_name=$4
    
    if [ -d "$source_dir" ]; then
        echo "📁 Syncing $dir_name..."
        
        # Sync to iOS
        if [ -d "$target_ios" ]; then
            rsync -av --delete \
                --include='*.h' \
                --include='*.cpp' \
                --include='*/' \
                --exclude='*' \
                "$source_dir/" "$target_ios/$dir_name/"
            echo "  ✅ iOS: $target_ios/$dir_name/"
        fi
        
        # Sync to cpp/engine (Android)
        if [ -d "$target_cpp" ]; then
            rsync -av --delete \
                --include='*.h' \
                --include='*.cpp' \
                --include='*/' \
                --exclude='*' \
                "$source_dir/" "$target_cpp/$dir_name/"
            echo "  ✅ Android: $target_cpp/$dir_name/"
        fi
        
        echo ""
    else
        echo "⚠️  Warning: Source directory $source_dir not found"
        echo ""
    fi
}

# Sync Core files to iOS root (for podspec compatibility)
# iOS podspec includes *.cpp from root, so we sync Core files there
echo "📁 Syncing Core to iOS root..."
if [ -d "$ENGINE_ROOT/Core" ]; then
    rsync -av \
        --include='*.h' \
        --include='*.cpp' \
        --exclude='*' \
        "$ENGINE_ROOT/Core/" "$IOS_TARGET/"
    echo "  ✅ iOS Root: $IOS_TARGET/"
    echo ""
fi

# Sync Core files to Android (maintaining Core subdirectory)
sync_dir "$ENGINE_ROOT/Core" "$IOS_TARGET" "$CPP_TARGET" "Core"

# Sync v1 files
sync_dir "$ENGINE_ROOT/v1" "$IOS_TARGET" "$CPP_TARGET" "v1"

# Sync v2 files
sync_dir "$ENGINE_ROOT/v2" "$IOS_TARGET" "$CPP_TARGET" "v2"

echo "✨ Engine sync complete!"
echo ""
echo "Files synced:"
echo "  Source: ../engine/{Core,v1,v2}"
echo "  → iOS Root: ./modules/chess-engine/ios/*.{h,cpp} (Core files)"
echo "  → iOS: ./modules/chess-engine/ios/{Core,v1,v2}"
echo "  → Android: ./modules/chess-engine/cpp/engine/{Core,v1,v2}"
