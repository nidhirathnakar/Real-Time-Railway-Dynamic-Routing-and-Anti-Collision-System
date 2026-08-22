cmake_minimum_required(VERSION 3.20)

project(RailwayAntiCollision
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(railway_simulator
    src/main.cpp
    src/RailwayGraph.cpp
    src/Train.cpp
)

target_include_directories(railway_simulator
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)