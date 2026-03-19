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
        src/pipe_transport.cpp
)

target_include_directories(cpp-insight-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cpp-insight-core PUBLIC cxx_std_17)

# -----------------------------------------------
# cpp-insight-gpu (static library)
# -----------------------------------------------
if (WIN32)
    add_library(cpp-insight-gpu STATIC
        src/gpu/gpu_profiler.cpp
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
        tests/src/test_archive.cpp
        tests/src/test_pipe_transport.cpp
        tests/src/test_scope_profiler.cpp
        tests/src/test_stat_registry.cpp
    )

    add_executable(insight-test ${TEST_SOURCES})

    target_link_libraries(insight-test
        PRIVATE
            cpp-insight-core
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(insight-test)
endif()