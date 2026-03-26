cmake_minimum_required(VERSION 3.16)
project(cpp-insight VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------
# cpp-insight-core (header only)
# -----------------------------------------------
add_library(cpp-insight-core
    STATIC
        src/insight/client.cpp
        src/insight/connection.cpp
        src/insight/pipe_transport.cpp
        src/insight/reporter.cpp
        src/insight/server.cpp
)

target_include_directories(cpp-insight-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(cpp-insight-core PUBLIC cxx_std_17)

# -----------------------------------------------
# cpp-insight-gpu (static library)
# -----------------------------------------------
if (WIN32)
    add_library(cpp-insight-gpu STATIC
        src/insight/gpu/gpu_profiler.cpp
        src/insight/gpu/d3d11_gpu_profiler_backend.cpp
    )

    target_include_directories(cpp-insight-gpu
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(cpp-insight-gpu
        PUBLIC cpp-insight-core
        PRIVATE d3d11
    )
else()
    message(STATUS "cpp-insight-gpu: Windows only, skipping on this platform")
endif()

# -----------------------------------------------
# cpp-insight-viewer 
# -----------------------------------------------

option(INSIGHT_BUILD_VIEWER "Build viewer" ON)
if(INSIGHT_BUILD_VIEWER)
    add_subdirectory(viewer)
endif()

# -----------------------------------------------
# Tests
# -----------------------------------------------
option(INSIGHT_BUILD_TESTS "Build tests" OFF)
if(INSIGHT_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    set(CMAKE_CTEST_ARGUMENTS "--no-compress-output")

    set(TEST_SOURCES
        tests/unit/test_archive.cpp
        tests/unit/test_pipe_transport.cpp
        tests/unit/test_registry.cpp
        tests/unit/test_reporter.cpp
        tests/unit/test_scope_profiler.cpp
    )

    add_executable(insight-test ${TEST_SOURCES})

    target_include_directories(insight-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    target_link_libraries(insight-test
        PRIVATE
            cpp-insight-core
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(insight-test)

    add_subdirectory(tests/integration/game_stub)
    add_subdirectory(tests/integration/gpu_stub)
endif()