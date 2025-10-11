#include "MainWindow.h"

// Windows
#include <cstdio>
#include <tchar.h>
#include <wrl.h> // ComPtr

// App
#include "USDScene.h"
#include "Renderer.h"
#include "UIBase.h"

// D3D
#include <d3dcommon.h>
#include <d3dcompiler.h>

// NVTX
#include <nvtx3/nvtx3.hpp>

// Imgui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"


MainWindow::MainWindow(HINSTANCE InHInstance)
{
    nvtx3::scoped_range r{ "Init MainWindow" };

    G_MainWindow = this;
    hInstance = InHInstance;

    Scene = std::make_unique<USDScene>();
    RendererDX = std::make_unique<Renderer>();
    UI = std::make_unique<UIBase>();
}

MainWindow::~MainWindow()
{
    // Shutdown imgui
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Clear ptr
    G_MainWindow = nullptr;
}

// Imgui WinProc Implementation
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK MainWindow::WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    nvtx3::scoped_range r{ "WinProcedure" };

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if (G_MainWindow->RendererDX->Device != nullptr && wParam != SIZE_MINIMIZED)
        {
            const UINT Width = LOWORD(lParam);
            const UINT Height = HIWORD(lParam);
            G_MainWindow->RendererDX->QueueResize(Width, Height);
            
            G_MainWindow->Tick();
            break;
        }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
        
    }

    return 0;
}

bool MainWindow::SetupWindow()
{
    bool bResult = false;

    const TCHAR szWindowClass[] = _T("DXRenderer");
    const TCHAR szTitle[] = _T("Win D3D Renderer - Mooncake");

    // Window Info
    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = MainWindow::WinProcedure;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    
    // Register Window
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
           _T("Call to RegisterClassEx failed!"),
           _T("DXRenderer Failed to register window class!"),
           NULL);

        return bResult;
    }

    // Adjust the windows window to be the correct size for the framebuffer resolution.
    RECT WindowRect{0, 0, static_cast<long>(RendererDX->Width), static_cast<long>(RendererDX->Height)};
    AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);
    UINT AdjustedWidth = static_cast<UINT>(WindowRect.right - WindowRect.left);
    UINT AdjustedHeight = static_cast<UINT>(WindowRect.bottom - WindowRect.top);
    
    // Create the base window
    hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(AdjustedWidth), static_cast<int>(AdjustedHeight),
        nullptr,
        nullptr,
        hInstance,
       nullptr
    );
    if (!hWnd)
    {
        printf("Error: %d\n", GetLastError());
        MessageBox(NULL,
           _T("Call to CreateWindowEx failed!"),
           _T("DXRenderer Failed to make a window!"),
           NULL);

        return bResult;
    }

    bResult = true;
    return bResult;
}

void MainWindow::Tick()
{
    nvtx3::scoped_range loop{ "Tick" };
    
    Time += TimeStep;

    // Update the UI
    UI->RenderUI();

    // Render
    RendererDX->Update();
    RendererDX->Render();
}

int MainWindow::Run()
{
    // Load the scene.
    Scene->LoadExampleCube();
    RendererDX->Setup();

    // Init UI.
    if (!UI->InitImgui()) { return 1;}
    
    // Show, the window hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    
    // Message loop
    MSG Msg;
    Msg.message = WM_NULL;
    PeekMessage(&Msg, nullptr, 0, 0, PM_NOREMOVE);
    
    while (Msg.message != WM_QUIT)
    {
        bool bMsg = (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE) != 0);
        if (bMsg)
        {
            nvtx3::scoped_range loop{ "Handle Windows Messages" };

            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
        else
        {
            // Tick when no messages received...
            Tick();
        }
    }

    return static_cast<int>(Msg.wParam);
}
