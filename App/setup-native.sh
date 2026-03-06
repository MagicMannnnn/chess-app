#!/bin/bash

set -e

echo "🚀 Setting up Chess Engine Native Module"
echo ""

cd "$(dirname "$0")"

echo "📦 Installing dependencies..."
npm install

echo ""
echo "🔧 Prebuilding for native development..."
npx expo prebuild --clean

echo ""
echo "📱 Choose platform to build:"
echo "1) iOS"
echo "2) Android"
echo "3) Both"
read -p "Enter choice (1-3): " choice

case $choice in
  1)
    echo ""
    echo "🍎 Building for iOS..."
    cd ios
    pod install
    cd ..
    echo ""
    echo "✅ iOS setup complete!"
    echo "Run: npx expo run:ios"
    ;;
  2)
    echo ""
    echo "🤖 Building for Android..."
    echo ""
    echo "✅ Android setup complete!"
    echo "Run: npx expo run:android"
    ;;
  3)
    echo ""
    echo "🍎 Setting up iOS..."
    cd ios
    pod install
    cd ..
    echo ""
    echo "✅ Setup complete!"
    echo "Run: npx expo run:ios (for iOS)"
    echo "Run: npx expo run:android (for Android)"
    ;;
  *)
    echo "Invalid choice"
    exit 1
    ;;
esac

echo ""
echo "📝 The chess engine now runs natively using C++!"
echo "All game logic is executed in native code for maximum performance."
