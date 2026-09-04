cmake_minimum_required(VERSION 3.20)

project(RailwayAntiCollision
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RAILWAY_SOURCES
    src/RailwayGraph.cpp
    src/RouteEngine.cpp
    src/TrackBlock.cpp
    src/BlockManager.cpp
    src/Switch.cpp
    src/BrakingModel.cpp
    src/CollisionDetector.cpp
    src/InterlockingController.cpp
    src/TelemetryQueue.cpp
    src/Metrics.cpp
    src/Logger.cpp
    src/SimulationEngine.cpp
    src/ReservationManager.cpp
    src/Train.cpp
)

add_executable(railway_simulator src/main.cpp ${RAILWAY_SOURCES})

target_include_directories(railway_simulator
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

include(CTest)

if(BUILD_TESTING)
    foreach(test_name IN ITEMS core graph route_engine blocks reservations switches braking interlocking collision telemetry simulation)
        add_executable(railway_test_${test_name} tests/test_${test_name}.cpp ${RAILWAY_SOURCES})
        target_include_directories(railway_test_${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
        add_test(NAME railway_${test_name} COMMAND railway_test_${test_name})
    endforeach()
endif()