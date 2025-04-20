#include "MainWindow.h"

// Windows
#include <cstdio>
#include <tchar.h>
#include <wrl.h> // ComPtr
#include <iostream>
#include <string>
#include <format>

// App
#include "USDScene.h"
#include "Renderer.h"

// D3D
#include <d3dcommon.h>
#include <d3dcompiler.h>


MainWindow::MainWindow(HINSTANCE InHInstance)
{
    G_MainWindow = this;
    hInstance = InHInstance;

    Scene = std::make_unique<USDScene>();
    RendererDX = std::make_unique<Renderer>();
}

MainWindow::~MainWindow()
{
    G_MainWindow = nullptr;
}

LRESULT CALLBACK MainWindow::WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    case WM_PAINT:
        G_MainWindow->RendererDX->Render();
        G_MainWindow->RendererDX->Update();

        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
        
    }

    return 0;
}

bool MainWindow::SetupWindow(const UINT& DefaultWidth, const UINT& DefaultHeight)
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
    
    // Create the base window
    hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(DefaultWidth), static_cast<int>(DefaultHeight),
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

int MainWindow::Run()
{
    // Load the scene.
    Scene->LoadExampleCube(); 
    
    RendererDX->Setup();

    // Show, the window hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    
    // Message loop
    MSG Msg;
    Msg.message = WM_NULL;
    PeekMessage(&Msg, nullptr, 0, 0, PM_NOREMOVE);
    
    while (Msg.message != WM_QUIT)
    {
        Time += TimeStep;

        bool bMsg = (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE) != 0);
        if (bMsg)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
        else
        {
            // Update renderer when no messages received...
            RendererDX->Render();
            RendererDX->Update();
        }
    
    }

    return static_cast<int>(Msg.wParam);
}
