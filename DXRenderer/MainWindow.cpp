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

// Define SDK version.
// Requires the Microsoft.Direct3D.D3D12 package (from nuget), version is the middle number of the version: '1.615.1'
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 615; } 
extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\"; }

// Namespaces and Imports.
using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Useful documentation examples:
// https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/Samples/Desktop/D3D12HelloWorld/src/HelloGenericPrograms
// https://learn.microsoft.com/en-us/windows/win32/direct3dgetstarted/work-with-dxgi
// https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide
// https://www.rastertek.com/tutdx11win10.html
// https://walbourn.github.io/anatomy-of-direct3d-12-create-device/


MainWindow::MainWindow(HINSTANCE InHInstance)
{
    hInstance = InHInstance;

    SetupDX();
}

LRESULT CALLBACK MainWindow::WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, DXRenderer!");
    
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
        
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

void MainWindow::SetupDX()
{
    HRESULT hr;

    // Setup DX Factory
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
    if (FAILED(hr))
    {
        printf("Failed to create D3D12 Factory\n");
        PostQuitMessage(1);
        return;
    }

    // Get Adapter
    DXGI_ADAPTER_DESC1 AdapterDesc;
    while (Factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&Adapter)) != DXGI_ERROR_NOT_FOUND)
    {
        hr = Adapter.Get()->GetDesc1(&AdapterDesc);
        break;
    }
    if (FAILED(hr))
    {
        printf("Failed to create D3D12 Factory\n");
        PostQuitMessage(1);
        return;
    }

    // Get the device.
    hr = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
    if (FAILED(hr))
    {
        printf("Failed to create D3D12 device\n");
        PostQuitMessage(1);
        return;
    }

    // GFX Pipeline or Context in DX11.
    // D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc{};
    // hr = Device->CreateGraphicsPipelineState(&Desc ,IID_PPV_ARGS(&Device));
    // if (FAILED(hr))
    // {
    //     printf("Failed to create graphics pipeline\n");
    //     PostQuitMessage(1);
    //     return;
    // }
    
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
    hr = Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&CmdQueue));
    if (FAILED(hr))
    {
        printf("Failed to create command queue\n");
        PostQuitMessage(1);
        return;
    }
    
    // DX Setup correctly.
    bDXReady = true;
}

void MainWindow::SetupSwapChain()
{
    HRESULT HR;
    
    DXGI_SWAP_CHAIN_DESC1 desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
    desc.BufferCount = 2;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;      //multisampling setting
    desc.SampleDesc.Quality = 0;    //vendor-specific flag
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    

     // Might need to use CreateSwapChainForCoreWindow || CreateSwapchainForComposition...
     HR = Factory->CreateSwapChainForHwnd(CmdQueue.Get(), hWnd, &desc, nullptr, nullptr, &SwapChain);
     if (FAILED(HR))
     {
         printf("Failed to create swap chain.\n");
         PostQuitMessage(1);
         return;
     }

    // RT Heaps
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR = Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvHeap));
    if (FAILED(HR))
    {
        printf("Failed to create rtv heap.\n");
        PostQuitMessage(1);
        return;
    }
    
    D3D12_CPU_DESCRIPTOR_HANDLE BufferHandle(RtvHeap->GetCPUDescriptorHandleForHeapStart());

    //Device->CreateRenderTargetView()
    
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    Device->CreateRenderTargetView(BackBuffer.Get(), &rtvDesc, BufferHandle);
    BufferHandle.ptr += sizeof(D3D12_CPU_DESCRIPTOR_HANDLE); // Offset by one. // Likely this is where its failing as the buffer is not being offset or assigned correctly. 
    //Device->CreateRenderTargetView(FrontBuffer.Get(), &rtvDesc, BufferHandle);

    
}


int MainWindow::Run()
{
    if (!bDXReady) { return 1; }
    
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

    // Start DX.
    SetupSwapChain();
    
    // Message loop
    MSG msg;
    msg.message = WM_NULL;
    PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
    
    while (msg.message != WM_QUIT)
    {
        bool bMsg = (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0);
    
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
