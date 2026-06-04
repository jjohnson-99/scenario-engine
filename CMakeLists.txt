cmake_minimum_required(VERSION 3.20)

project(
    scenario_engine
    VERSION 0.1.0
    DESCRIPTION "Decision-making under uncertainty"
    LANGUAGES CXX
)

# --------------------------------------------------
# Global settings
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# --------------------------------------------------
# Dependencies
# --------------------------------------------------

find_package(Catch2 CONFIG REQUIRED)

# --------------------------------------------------
# Source files
# --------------------------------------------------

set(
    SCENARIO_ENGINE_SOURCES

    src/data/TimeSeries.cpp
    src/data/CsvLoader.cpp
    src/config/ConfigLoader.cpp
    src/forecasting/MovingAverageForecaster.cpp
    src/forecasting/ExponentialSmoothingForecaster.cpp
    src/evaluation/Backtester.cpp
    src/evaluation/Benchmark.cpp
    src/evaluation/CsvExporter.cpp
    src/evaluation/Metrics.cpp
)

# --------------------------------------------------
# Library
# --------------------------------------------------

add_library(
    scenario_engine_lib
    ${SCENARIO_ENGINE_SOURCES}
)

target_include_directories(
    scenario_engine_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
    scenario_engine_lib
    PUBLIC
        cxx_std_23
)

# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------

if(MSVC)

    target_compile_options(
        scenario_engine_lib
        PRIVATE
            /W4
    )

else()

    target_compile_options(
        scenario_engine_lib
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
    )

endif()

# --------------------------------------------------
# Main executable
# --------------------------------------------------

add_executable(
    scenario_engine
    src/main.cpp
)

target_link_libraries(
    scenario_engine
    PRIVATE
        scenario_engine_lib
)

# --------------------------------------------------
# Tests
# --------------------------------------------------

add_executable(
    scenario_engine_tests
    tests/TimeSeriesTests.cpp
    tests/MovingAverageForecasterTests.cpp
    tests/ExponentialSmoothingForecasterTests.cpp
    tests/BacktesterTests.cpp
    tests/BenchmarkTests.cpp
    tests/CsvExporterTests.cpp
    tests/MultiHorizonBenchmarkTests.cpp
)

target_link_libraries(
    scenario_engine_tests
    PRIVATE
        scenario_engine_lib
        Catch2::Catch2WithMain
)

include(Catch)

catch_discover_tests(
    scenario_engine_tests
)

# --------------------------------------------------
# Future folders
# --------------------------------------------------

set_target_properties(
    scenario_engine_lib
    PROPERTIES
        FOLDER "Library"
)

set_target_properties(
    scenario_engine
    PROPERTIES
        FOLDER "Applications"
)

set_target_properties(
    scenario_engine_tests
    PROPERTIES
        FOLDER "Tests"
)
