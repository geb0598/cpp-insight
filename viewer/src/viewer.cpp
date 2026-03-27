#include "viewer.h"

#include <filesystem>
#include <windows.h>

#include "flame_panel.h"
#include "realtime_panel.h"
#include "stack_panel.h"
#include "timeline_panel.h"
#include "toolbar_panel.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <implot.h>

namespace {

void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding     = 6.0f;
    s.ChildRounding      = 4.0f;
    s.FrameRounding      = 4.0f;
    s.PopupRounding      = 4.0f;
    s.ScrollbarRounding  = 6.0f;
    s.GrabRounding       = 4.0f;
    s.TabRounding        = 4.0f;

    s.WindowPadding      = ImVec2(10.0f, 10.0f);
    s.FramePadding       = ImVec2(8.0f,   4.0f);
    s.ItemSpacing        = ImVec2(8.0f,   6.0f);
    s.ScrollbarSize      = 12.0f;
    s.GrabMinSize        = 10.0f;
    s.WindowBorderSize   = 1.0f;
    s.FrameBorderSize    = 0.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    c[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
    c[ImGuiCol_WindowBg]              = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    c[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.15f, 0.98f);
    c[ImGuiCol_Border]                = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    c[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_FrameBg]               = ImVec4(0.17f, 0.17f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]         = ImVec4(0.27f, 0.27f, 0.32f, 1.00f);
    c[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.10f, 0.12f, 0.75f);
    c[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.42f, 0.42f, 0.48f, 1.00f);
    c[ImGuiCol_CheckMark]             = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = ImVec4(0.55f, 0.78f, 1.00f, 1.00f);
    c[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_ButtonHovered]         = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    c[ImGuiCol_ButtonActive]          = ImVec4(0.38f, 0.38f, 0.45f, 1.00f);
    c[ImGuiCol_Header]                = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.35f, 0.35f, 0.42f, 1.00f);
    c[ImGuiCol_Separator]             = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    c[ImGuiCol_SeparatorHovered]      = ImVec4(0.38f, 0.38f, 0.45f, 1.00f);
    c[ImGuiCol_SeparatorActive]       = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.28f, 0.28f, 0.34f, 0.50f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(0.38f, 0.38f, 0.45f, 0.75f);
    c[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_Tab]                   = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    c[ImGuiCol_TabHovered]            = ImVec4(0.28f, 0.28f, 0.34f, 1.00f);
    c[ImGuiCol_TabActive]             = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    c[ImGuiCol_TabUnfocused]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    c[ImGuiCol_TableHeaderBg]         = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    c[ImGuiCol_TableBorderStrong]     = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    c[ImGuiCol_TableBorderLight]      = ImVec4(0.19f, 0.19f, 0.22f, 1.00f);
    c[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    c[ImGuiCol_TextSelectedBg]        = ImVec4(0.45f, 0.75f, 1.00f, 0.35f);
    c[ImGuiCol_NavHighlight]          = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

} // anonymous namespace

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

namespace insights::viewer {

bool Viewer::Init(HWND hwnd) {
    if (!InitDX11(hwnd)) {
        return false;
    }

    if (!InitServer()) {
        // @todo
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    {
        ImGuiIO& io = ImGui::GetIO();
        char exe_buf[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_buf, MAX_PATH);
        auto font_path = std::filesystem::path(exe_buf).parent_path()
                         / "JetBrainsMonoNerdFont-Regular.ttf";
        static const ImWchar ranges[] = { 0x0020, 0x00FF, 0xE000, 0xF8FF, 0 };
        io.Fonts->AddFontFromFileTTF(font_path.string().c_str(), 16.0f, nullptr, ranges);
    }

    ApplyStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device_.Get(), context_.Get());

    panels_.push_back(std::make_unique<ToolbarPanel>(ctx_));
    panels_.push_back(std::make_unique<RealtimePanel>(ctx_));
    panels_.push_back(std::make_unique<TimelinePanel>(ctx_));
    panels_.push_back(std::make_unique<FlamePanel>(ctx_));
    panels_.push_back(std::make_unique<StackPanel>(ctx_));

    is_running_ = true;
    return true;
}

void Viewer::Shutdown() {
    Server::GetInstance().Stop();

    ImPlot::DestroyContext();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Viewer::Run() {
    MSG msg = {};
    while (is_running_) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                is_running_ = false;
            }
        }

        if (!is_running_) {
            break;
        }

        if (ctx_.needs_reset) {
            Reset();
            ctx_.needs_reset = false;
        }

        BeginFrame();
        Render();
        EndFrame();
    }
}

bool Viewer::InitDX11(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    UINT width  = static_cast<UINT>(rect.right  - rect.left);
    UINT height = static_cast<UINT>(rect.bottom - rect.top);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = width;
    sd.BufferDesc.Height                  = height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, feature_levels, 1,
        D3D11_SDK_VERSION,
        &sd,
        swap_chain_.GetAddressOf(),
        device_.GetAddressOf(),
        &feature_level,
        context_.GetAddressOf());

    if (FAILED(hr)) { return false; }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    device_->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_.GetAddressOf());

    return true;
}

bool Viewer::InitServer() {
    auto& server = insights::Server::GetInstance();
    server.SetOnConnected([this]() {
        ctx_.server_state = ServerState::CONNECTED;
    });
    server.SetOnDisconnected([this](){
        ctx_.server_state = ServerState::OFFLINE;
    });
    return true;
}

void Viewer::Resize(UINT width, UINT height) {
    rtv_.Reset();
    swap_chain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    device_->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv_.GetAddressOf());
}

void Viewer::BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Viewer::EndFrame() {
    ImGui::Render();

    const float clear_color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    context_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(rtv_.Get(), clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swap_chain_->Present(1, 0);
}

void Viewer::Render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("cpp-insights", nullptr,
        ImGuiWindowFlags_NoTitleBar        |
        ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoMove            |
        ImGuiWindowFlags_NoScrollbar       |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_MenuBar);

    // for (auto& panel : panels_) {
    //     panel->Render();
    // }

    // panels_[0] = ToolbarPanel
    // panels_[1] = RealtimePanel
    // panels_[2] = TimelinePanel
    // panels_[3] = FlamePanel
    // panels_[4] = StackPanel

    panels_[0]->Render();
    panels_[1]->Render();
    panels_[2]->Render();

    if (ImGui::BeginTable("##layout", 2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
            ImVec2(-1, -1))) {
        ImGui::TableSetupColumn("Left",  ImGuiTableColumnFlags_WidthStretch, 0.50f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.50f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        panels_[3]->Render();

        ImGui::TableSetColumnIndex(1);
        panels_[4]->Render();

        ImGui::EndTable();
    }

    ImGui::End();
}

void Viewer::Reset() {
    for (auto& panel : panels_) {
        panel->Reset();
    }
}

LRESULT Viewer::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) {
        return true;
    }

    switch (msg) {
    case WM_SIZE:
        if (device_ && wp != SIZE_MINIMIZED) {
            Resize(LOWORD(lp), HIWORD(lp));
        }
        return 0;
    case WM_DESTROY:
        is_running_ = false;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

} // namespace insightss::viewer