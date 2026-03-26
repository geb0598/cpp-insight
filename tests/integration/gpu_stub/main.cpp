#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "insight/insight.h"
#include "insight/gpu/gpu_profiler.h"
#include "insight/gpu/gpu_profiler_backend.h"
#include "insight/gpu/d3d11_gpu_profiler_backend.h"

INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);

INSIGHT_DECLARE_STAT("CPU_MainLoop", STAT_CPU_MAIN,  STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("CPU_Render",   STAT_CPU_DRAW,  STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("CPU_Present",  STAT_CPU_PRES,  STATGROUP_RENDERING);

INSIGHT_DECLARE_STAT("GPU_Clear",    STAT_GPU_CLEAR, STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("GPU_Present",  STAT_GPU_PRES,  STATGROUP_RENDERING);

INSIGHT_DECLARE_STAT("CPU_Triangle", STAT_CPU_TRI,   STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("GPU_Triangle", STAT_GPU_TRI,   STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("CPU_Cube",     STAT_CPU_CUBE,  STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("GPU_Cube",     STAT_GPU_CUBE,  STATGROUP_RENDERING);

static bool g_is_running = true;

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        g_is_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct ConstantBuffer {
    DirectX::XMMATRIX mvp;
};

const char* g_shader_code = R"(
cbuffer CB : register(b0) {
    matrix MVP;
};
struct VS_IN {
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct PS_IN {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
PS_IN VSMain(VS_IN input) {
    PS_IN output;
    output.pos = mul(float4(input.pos, 1.0f), MVP);
    output.col = input.col;
    return output;
}
float4 PSMain(PS_IN input) : SV_Target {
    return input.col;
}
)";

int main() {
    std::cout << "Starting DX11 GPU Stub...\n";

    HINSTANCE hinstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinstance;
    wc.lpszClassName = L"insight-gpu-stub";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"DX11 GPU Stub",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 720,
        nullptr, nullptr, hinstance, nullptr);

    Microsoft::WRL::ComPtr<ID3D11Device>           device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         swap_chain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 1280;
    sd.BufferDesc.Height                  = 720;
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
    const D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, feature_levels, 1, D3D11_SDK_VERSION,
        &sd, swap_chain.GetAddressOf(), device.GetAddressOf(),
        &feature_level, context.GetAddressOf());

    if (FAILED(hr)) { return -1; }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    device->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv.GetAddressOf());

    D3D11_TEXTURE2D_DESC ds_desc = {};
    ds_desc.Width              = 1280;
    ds_desc.Height             = 720;
    ds_desc.MipLevels          = 1;
    ds_desc.ArraySize          = 1;
    ds_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ds_desc.SampleDesc.Count   = 1;
    ds_desc.Usage              = D3D11_USAGE_DEFAULT;
    ds_desc.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_stencil;
    device->CreateTexture2D(&ds_desc, nullptr, depth_stencil.GetAddressOf());
    device->CreateDepthStencilView(depth_stencil.Get(), nullptr, dsv.GetAddressOf());

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  pixel_shader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  input_layout;
    Microsoft::WRL::ComPtr<ID3D10Blob>         vs_blob;
    Microsoft::WRL::ComPtr<ID3D10Blob>         ps_blob;

    D3DCompile(g_shader_code, strlen(g_shader_code), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, vs_blob.GetAddressOf(), nullptr);
    D3DCompile(g_shader_code, strlen(g_shader_code), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, ps_blob.GetAddressOf(), nullptr);

    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, vertex_shader.GetAddressOf());
    device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, pixel_shader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    device->CreateInputLayout(layout, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), input_layout.GetAddressOf());

    Vertex tri_vertices[] = {
        { {  0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    Vertex cube_vertices[] = {
        { { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } }
    };
    WORD cube_indices[] = {
        3,1,0, 2,1,3, 0,5,4, 1,5,0, 3,4,7, 0,4,3,
        1,6,5, 2,6,1, 2,7,6, 3,7,2, 6,4,5, 7,4,6
    };

    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.Usage          = D3D11_USAGE_DEFAULT;
    vb_desc.ByteWidth      = sizeof(tri_vertices);
    vb_desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vb_data = { tri_vertices, 0, 0 };
    Microsoft::WRL::ComPtr<ID3D11Buffer> tri_vb;
    device->CreateBuffer(&vb_desc, &vb_data, tri_vb.GetAddressOf());

    vb_desc.ByteWidth = sizeof(cube_vertices);
    vb_data.pSysMem   = cube_vertices;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cube_vb;
    device->CreateBuffer(&vb_desc, &vb_data, cube_vb.GetAddressOf());

    D3D11_BUFFER_DESC ib_desc = {};
    ib_desc.Usage          = D3D11_USAGE_DEFAULT;
    ib_desc.ByteWidth      = sizeof(cube_indices);
    ib_desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ib_data = { cube_indices, 0, 0 };
    Microsoft::WRL::ComPtr<ID3D11Buffer> cube_ib;
    device->CreateBuffer(&ib_desc, &ib_data, cube_ib.GetAddressOf());

    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage          = D3D11_USAGE_DEFAULT;
    cb_desc.ByteWidth      = sizeof(ConstantBuffer);
    cb_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
    device->CreateBuffer(&cb_desc, nullptr, cb.GetAddressOf());

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    insight::GpuProfiler::GetInstance().Init(
        std::make_unique<insight::D3D11GpuProfilerBackend>(device.Get(), context.Get()));

    INSIGHT_INITIALIZE();
    std::cout << "Insight connected. Running DX11 loop...\n";

    DirectX::XMMATRIX proj_matrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.01f, 100.0f);
    float angle = 0.0f;

    MSG msg = {};
    while (g_is_running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!g_is_running) {
            break;
        }

        angle += 0.01f;

        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE(STAT_CPU_MAIN);

            {
                INSIGHT_SCOPE(STAT_CPU_DRAW);

                {
                    INSIGHT_GPU_SCOPE(STAT_GPU_CLEAR);
                    const float clear_color[] = { 0.2f, 0.3f, 0.4f, 1.0f };
                    context->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());
                    context->ClearRenderTargetView(rtv.Get(), clear_color);
                    context->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                }

                D3D11_VIEWPORT vp = { 0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f };
                context->RSSetViewports(1, &vp);
                context->IASetInputLayout(input_layout.Get());
                context->VSSetShader(vertex_shader.Get(), nullptr, 0);
                context->PSSetShader(pixel_shader.Get(), nullptr, 0);
                context->VSSetConstantBuffers(0, 1, cb.GetAddressOf());

                {
                    INSIGHT_SCOPE(STAT_CPU_TRI);
                    INSIGHT_GPU_SCOPE(STAT_GPU_TRI);

                    ConstantBuffer cb_data;
                    cb_data.mvp = DirectX::XMMatrixTranspose(
                        DirectX::XMMatrixTranslation(-1.5f, 0.0f, 5.0f) * proj_matrix);
                    context->UpdateSubresource(cb.Get(), 0, nullptr, &cb_data, 0, 0);

                    UINT stride = sizeof(Vertex);
                    UINT offset = 0;
                    context->IASetVertexBuffers(0, 1, tri_vb.GetAddressOf(), &stride, &offset);
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    
                    context->Draw(3, 0);
                }

                {
                    INSIGHT_SCOPE(STAT_CPU_CUBE);
                    INSIGHT_GPU_SCOPE(STAT_GPU_CUBE);

                    ConstantBuffer cb_data;
                    cb_data.mvp = DirectX::XMMatrixTranspose(
                        DirectX::XMMatrixRotationY(angle) * DirectX::XMMatrixRotationX(angle * 0.5f) *
                        DirectX::XMMatrixTranslation(1.5f, 0.0f, 5.0f) * proj_matrix);
                    context->UpdateSubresource(cb.Get(), 0, nullptr, &cb_data, 0, 0);

                    UINT stride = sizeof(Vertex);
                    UINT offset = 0;
                    context->IASetVertexBuffers(0, 1, cube_vb.GetAddressOf(), &stride, &offset);
                    context->IASetIndexBuffer(cube_ib.Get(), DXGI_FORMAT_R16_UINT, 0);
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    
                    context->DrawIndexed(36, 0, 0);
                }
            }

            {
                INSIGHT_SCOPE(STAT_CPU_PRES);
                INSIGHT_GPU_SCOPE(STAT_GPU_PRES);
                swap_chain->Present(1, 0);
            }
        }
        INSIGHT_FRAME_END();
    }

    insight::Client::GetInstance().Disconnect();
    return 0;
}