cmake_minimum_required(VERSION 3.15)
project(ChessEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
endif()

# Source files
set(CORE_SOURCES
    Core/Board.cpp
    Core/Move.cpp
    Core/ChessEngine.cpp
    v1/Search.cpp
    v1/Evaluation.cpp
    v2/Search.cpp
    v2/Evaluation.cpp
)

set(CORE_HEADERS
    Core/Board.h
    Core/Move.h
    Core/ChessEngine.h
    Core/Types.h
    v1/Search.h
    v1/Evaluation.h
    v2/Search.h
    v2/Evaluation.h
)

# For WebAssembly build
if(EMSCRIPTEN)
    add_executable(chess_engine
        ${CORE_SOURCES}
        bindings/emscripten_bindings.cpp
    )
    
    target_include_directories(chess_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    set_target_properties(chess_engine PROPERTIES
        LINK_FLAGS "--bind -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME='createChessEngine' -s ENVIRONMENT='web' -O3"
    )
else()
    # Native build - create static library
    add_library(chess_engine STATIC ${CORE_SOURCES} ${CORE_HEADERS})

    target_include_directories(chess_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# Optional: Build test executable (for development/testing)
option(BUILD_TESTS "Build test executable" OFF)
if(BUILD_TESTS)
    add_executable(chess_test test/main.cpp)
    target_link_libraries(chess_test PRIVATE chess_engine)
endif()
