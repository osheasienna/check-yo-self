cmake_minimum_required(VERSION 3.16)

project(chessbot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(chessbot
    src/main.cpp
    1_construct_board.cpp
    2_generate_legal_moves.cpp
)

target_include_directories(chessbot
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

