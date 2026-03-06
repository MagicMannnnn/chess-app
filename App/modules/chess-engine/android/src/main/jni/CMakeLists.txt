cmake_minimum_required(VERSION 3.10)
project(chess-engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the chess engine source
set(ENGINE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../engine")

# Add the chess engine source files
add_library(chess-engine SHARED
    chess-engine.cpp
    ${ENGINE_SRC_DIR}/Core/Board.cpp
    ${ENGINE_SRC_DIR}/Core/ChessEngine.cpp
    ${ENGINE_SRC_DIR}/Core/Move.cpp
    ${ENGINE_SRC_DIR}/Core/MoveGenerator.cpp
)

# Include directories
target_include_directories(chess-engine PRIVATE
    ${ENGINE_SRC_DIR}
    ${ENGINE_SRC_DIR}/Core
)

# Find required libraries
find_library(log-lib log)

# Link libraries
target_link_libraries(chess-engine ${log-lib})
