#include "MainWindow.h"

// Windows
#include <cstdio>
#include <tchar.h>
#include <wrl.h> // ComPtr

// D3D
#include <d3d12.h>
#include <d3dcommon.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgi1_3.h>
#include <vector>

#pragma comment(lib, "dxgi") // For CreateDXGIFactory2 linker error.

// Namespaces
using namespace Microsoft::WRL;
using namespace DirectX;

// Useful documentation examples:
// https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12HelloWorld/src/HelloGenericPrograms
// https://learn.microsoft.com/en-us/windows/win32/direct3dgetstarted/work-with-dxgi
// https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide
// https://www.rastertek.com/tutdx11win10.html


MainWindow::MainWindow(HINSTANCE InHInstance)
{
    hInstance = InHInstance;


    UINT dxgiFactoryFlags = 0;
    ComPtr<IDXGIFactory7> Factory;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&Factory));

    
    UINT i = 0;
    ComPtr<IDXGIAdapter1> Adapter;
    DXGI_ADAPTER_DESC1 AdapterDesc;
    while (Factory->EnumAdapters1(i, &Adapter) != DXGI_ERROR_NOT_FOUND)
    {
        Adapter.Get()->GetDesc1(&AdapterDesc);
        if (AdapterDesc.VendorId == 4318) // NVIDIA Vendor ID !VERY HACKY! 
        {
            break;
        }
        ++i;
    }

    ComPtr<ID3D12Device14> Device;
    //HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device));
    
}

LRESULT CALLBACK MainWindow::WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, DXRenderer!");
    
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOut(hdc,
           5, 5,
           greeting, static_cast<int>(_tcslen(greeting)));
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
        
    }

    return 0;
}


int MainWindow::Run()
{
    static TCHAR szWindowClass[] = _T("DXRenderer");
    static TCHAR szTitle[] = _T("Win D3D Renderer - Mooncake");
    
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

        return 1;
    }
    
    // Create the base window
    hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
        NULL,
        NULL,
        hInstance,
       NULL
    );
    if (!hWnd)
    {
        printf("Error: %d\n", GetLastError());
        MessageBox(NULL,
           _T("Call to CreateWindowEx failed!"),
           _T("DXRenderer Failed to make a window!"),
           NULL);


        return 1;
    }
    
    // Show, its hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    // Message loop
    MSG msg;
    bool bMsg = false;
    msg.message = WM_NULL;
    PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE);
    
    while (msg.message != WM_QUIT)
    {
        bMsg = (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0);
    
        if (bMsg)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Update renderer.
            
            
        }
    
    }

    return static_cast<int>(msg.wParam);
}
