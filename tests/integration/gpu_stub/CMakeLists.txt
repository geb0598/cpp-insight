add_executable(gpu-stub
    main.cpp
)

target_link_libraries(gpu-stub
    PRIVATE
        cpp-insights-core
        cpp-insights-gpu
)

target_compile_definitions(gpu-stub
    PRIVATE
        INSIGHTS
        INSIGHTS_GPU
)