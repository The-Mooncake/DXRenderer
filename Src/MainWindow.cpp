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
            // UINT Width = LOWORD(lParam) * G_MainWindow->DpiScaling;
            // UINT Height = HIWORD(lParam) * G_MainWindow->DpiScaling;
            //
            // std::wstring Title = _T("Win D3D Renderer - Mooncake | ");
            // Title.append(std::to_wstring(Width));
            // Title.append(_T("x"));
            // Title.append(std::to_wstring(Height));
            //
            // SetWindowText(hWnd, Title.c_str());
            //
            // G_MainWindow->RendererDX->WaitForPreviousFrame();
            //
            // G_MainWindow->RendererDX->AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
            // G_MainWindow->RendererDX->Width = Width;
            // G_MainWindow->RendererDX->Height = Height;
            //
            // G_MainWindow->RendererDX->Viewport.Height = Height;
            // G_MainWindow->RendererDX->Viewport.Width = Width;
            //
            // DXGI_SWAP_CHAIN_DESC desc = {};
            // G_MainWindow->RendererDX->SwapChain->GetDesc(&desc);
            //
            // G_MainWindow->RendererDX->CleanupFrameBuffers();
            // HRESULT result = G_MainWindow->RendererDX->SwapChain->ResizeBuffers(desc.BufferCount, Width, Height, desc.BufferDesc.Format, desc.Flags);
            // assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            // G_MainWindow->RendererDX->CreateFrameBuffers();
        }        
        
    // case WM_PAINT:
    //     //G_MainWindow->RendererDX->Update();
    //     //G_MainWindow->RendererDX->Render();
    //
    //     break;

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
        static_cast<int>(AdjustedWidth * DpiScaling), static_cast<int>(AdjustedHeight * DpiScaling),
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

bool MainWindow::InitImgui()
{
    IMGUI_CHECKVERSION();
    
    ImGui_ImplWin32_EnableDpiAwareness();
    DpiScaling = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    
    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(DpiScaling * ImguiUIScaling); // Only for scaling ui not window mappings.
    
    // Setup Platform/Renderer backends
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = RendererDX.get()->Device.Get();
    init_info.CommandQueue = RendererDX.get()->CmdQueue.Get();
    init_info.NumFramesInFlight = 1;
    init_info.RTVFormat = RendererDX.get()->FrameBufferFormat; 
    init_info.SrvDescriptorHeap = RendererDX.get()->SrvBufferHeap.Get();

    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return ImguiHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return ImguiHeapAlloc.Free(cpu_handle, gpu_handle); };
    if (!ImGui_ImplWin32_Init(hWnd))
    {
        MessageBoxW(nullptr, L"Failed to initialise ImGui Win32!", L"Error", MB_OK);
        PostQuitMessage(1);
        return false;
    }
    if (!ImGui_ImplDX12_Init(&init_info))
    {
        MessageBoxW(nullptr, L"Failed to Initialise ImGui DX12!", L"Error", MB_OK);
        PostQuitMessage(1);
        return false;
    }
    
    return true;
}

int MainWindow::Run()
{
    // Load the scene.
    Scene->LoadExampleCube();
    RendererDX->Setup();

    if (!InitImgui()) { return 1;}
    
    // Show, the window hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    
    // Message loop
    MSG Msg;
    Msg.message = WM_NULL;
    PeekMessage(&Msg, nullptr, 0, 0, PM_NOREMOVE);
    
    while (Msg.message != WM_QUIT)
    {
        nvtx3::scoped_range loop{ "Tick" };

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
            RendererDX->Update();
            RendererDX->Render();
        }
    
    }

    return static_cast<int>(Msg.wParam);
}
