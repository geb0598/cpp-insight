#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "viewer.h"

static insights::viewer::Viewer* g_viewer = nullptr;

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (g_viewer) {
        return g_viewer->HandleMessage(hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc        = {};
    wc.cbSize            = sizeof(WNDCLASSEXW);
    wc.style             = CS_CLASSDC;
    wc.lpfnWndProc       = WndProc;
    wc.hInstance         = hinstance;
    wc.hIcon             = LoadIcon(hinstance, MAKEINTRESOURCE(101));
    wc.hIconSm           = LoadIcon(hinstance, MAKEINTRESOURCE(101));
    wc.lpszClassName     = L"cpp-insights-viewer";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"cpp-insights-viewer",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 960,
        nullptr, nullptr,
        hinstance, nullptr);

    insights::viewer::Viewer viewer;
    g_viewer = &viewer;

    if (!viewer.Init(hwnd)) {
        return -1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    viewer.Run();

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hinstance);

    return 0;
}