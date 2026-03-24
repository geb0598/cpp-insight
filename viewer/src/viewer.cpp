#include "viewer.h"

#include "realtime_panel.h"
#include "stack_panel.h"
#include "timeline_panel.h"
#include "toolbar_panel.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <implot.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

namespace insight::viewer {

bool Viewer::Init(HWND hwnd) {
    if (!InitDX11(hwnd)) {
        return false;
    }

    if (!InitServer()) {
        // @todo
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImPlot::CreateContext();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device_.Get(), context_.Get());

    panels_.push_back(std::make_unique<ToolbarPanel>(ctx_));
    panels_.push_back(std::make_unique<RealtimePanel>(ctx_));
    panels_.push_back(std::make_unique<TimelinePanel>(ctx_));
    panels_.push_back(std::make_unique<StackPanel>(ctx_));

    is_running_ = true;
    return true;
}

void Viewer::Shutdown() {
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
    auto& server = insight::Server::GetInstance();
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
    ImGui::Begin("cpp-insight", nullptr,
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    for (auto& panel : panels_) {
        panel->Render();
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

} // namespace insight::viewer