add_executable(gpu-stub
    main.cpp
)

target_link_libraries(gpu-stub
    PRIVATE
        cpp-insight-core
        cpp-insight-gpu
)

target_compile_definitions(gpu-stub
    PRIVATE
        INSIGHT_GPU
)