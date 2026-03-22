#pragma once

#include <vector>
#include <memory>

#include <d3d11.h>
#include <wrl/client.h>

#include "insight/server.h"

#include "panel.h"

namespace insight::viewer {

class Viewer {
public:
    Viewer() = default;
    ~Viewer() { Shutdown(); }

    bool Init(HWND hwnd);
    void Shutdown();
    void Run();

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    bool InitDX11(HWND hwnd);
    bool InitServer();

    void Resize(UINT width, UINT height);

    void CheckConnection();

    void BeginFrame();
    void EndFrame();

    void Render();
    void Reset();

    Microsoft::WRL::ComPtr<ID3D11Device>           device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         swap_chain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;

    std::future<TransportResult> accept_future_;

    PanelContext                        ctx_;
    std::vector<std::unique_ptr<Panel>> panels_;

    bool is_running_ = false;
};

} // namespace insight::viewer