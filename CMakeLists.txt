cmake_minimum_required(VERSION 3.16)

project(chess-king VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(chess-king
    src/main.cpp
    1_construct_board.cpp
    2_generate_legal_moves.cpp
    3_search_moves.cpp
    4_select_best_move.cpp
    make_move.cpp
    5_generate_output.cpp
    zobrist_h.cpp
    attacks.cpp
    openings.cpp
)

target_include_directories(chess-king
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

