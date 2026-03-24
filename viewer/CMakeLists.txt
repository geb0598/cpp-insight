include(FetchContent)

# -----------------------------------------------
# ImGui
# -----------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC d3d11 dxgi)

# -----------------------------------------------
# ImPlot
# -----------------------------------------------
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        v0.16 
)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
)

target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})

target_link_libraries(implot PUBLIC imgui)

# -----------------------------------------------
# cpp-insight-viewer
# -----------------------------------------------
add_executable(cpp-insight-viewer WIN32
    src/main.cpp
    src/viewer.cpp
    src/toolbar_panel.cpp
    src/realtime_panel.cpp
    src/timeline_panel.cpp
    src/stack_panel.cpp
)

target_include_directories(cpp-insight-viewer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cpp-insight-viewer
    PRIVATE
        cpp-insight-core
        implot
        imgui
)