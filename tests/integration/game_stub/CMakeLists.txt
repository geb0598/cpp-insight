add_executable(game-stub
    main.cpp
)

target_link_libraries(game-stub
    PRIVATE
        cpp-insight-core
)