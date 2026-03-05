# Chess Engine

A high-performance C++ chess engine with complete chess rules implementation, designed to be compiled to WebAssembly for use in React Native applications.

## Building

### Native Build (for testing)

```bash
mkdir build
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
```

### WebAssembly Build (for React Native app)

Install Emscripten SDK first: https://emscripten.org/docs/getting_started/downloads.html

```bash
mkdir build-wasm
cd build-wasm
emcmake cmake ..
emmake make
```

This will generate `chess_engine.wasm` that can be used in the React Native app.

## License

See LICENSE file in project root.
