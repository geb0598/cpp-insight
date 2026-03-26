add_executable(game-stub
    main.cpp
)

target_link_libraries(game-stub
    PRIVATE
        cpp-insights-core
        cpp-insights-gpu
)

target_compile_definitions(game-stub
    PRIVATE
        INSIGHTS_GPU
)