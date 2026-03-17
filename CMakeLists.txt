# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(cpp-insight VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMKAE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------
# cpp-insight-core (header only)
# -----------------------------------------------
add_library(cpp-insight-core INTERFACE)

target_include_directories(cpp-insight-core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cpp-insight-core INTERFACE cxx_std_17)

if(WIN32)
    target_compile_definitions(cpp-insight-core 
        INTERFACE INSIGHT_PLATFORM_WINDOWS=1
    )
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(cpp-insight-core 
        INTERFACE INSIGHT_PLATFORM_LINUX=1
    )
endif()

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
    add_executable(insight-test tests/main.cpp)
    target_link_libraries(insight-test PRIVATE cpp-insight-gpu)
endif()