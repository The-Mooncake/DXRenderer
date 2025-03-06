#include "MainWindow.h"

// Windows
#include <cstdio>
#include <tchar.h>
#include <wrl.h> // ComPtr

// D3D
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
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
// https://learn.microsoft.com/en-us/windows/win32/direct3d12/creating-a-basic-direct3d-12-component
// https://alain.xyz/blog/raw-directx12
// https://github.com/alaingalvan/directx12-seed/tree/master/src
// https://whoisryosuke.com/blog/2023/learning-directx-12-in-2023 - Lots of links to other tutorial series.

MainWindow::MainWindow(HINSTANCE InHInstance)
{
    hInstance = InHInstance;
    G_MainWindow = this;
    
    SetupDevice();
    SetupWindow();
    SetupRendering();
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
        G_MainWindow->Render();
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
        
    }

    return 0;
}

void MainWindow::SetupDevice()
{
    HRESULT HR;

    // Setup DX Factory
    HR = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
    if (FAILED(HR))
    {
        printf("Failed to create D3D12 Factory\n");
        PostQuitMessage(1);
        return;
    }

    // Get Adapter
    DXGI_ADAPTER_DESC1 AdapterDesc;
    while (Factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&Adapter)) != DXGI_ERROR_NOT_FOUND)
    {
        HR = Adapter.Get()->GetDesc1(&AdapterDesc);
        break;
    }
    if (FAILED(HR))
    {
        printf("Failed to create D3D12 Factory\n");
        PostQuitMessage(1);
        return;
    }

    // Get the device.
    HR = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
    if (FAILED(HR))
    {
        printf("Failed to create D3D12 device\n");
        PostQuitMessage(1);
        return;
    }
    
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
    HR = Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&CmdQueue));
    if (FAILED(HR))
    {
        printf("Failed to create command queue\n");
        PostQuitMessage(1);
        return;
    }

    HR = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdAllocator));
    if (FAILED(HR))
    {
        printf("Failed to create command allocator\n");
        PostQuitMessage(1);
        return;
    }

    HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAllocator.Get(), 0, IID_PPV_ARGS(&CmdList));
    if (FAILED(HR))
    {
        printf("Failed to create command list.\n");
        PostQuitMessage(1);
        return;
    }

    // Setup the events.
    HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    if (FAILED(HR))
    {
        printf("Failed to create DX Fence\n");
        PostQuitMessage(1);
        return;
    }
    FenceEvent = CreateEvent(nullptr, false, false, nullptr);
    
}

void MainWindow::SetupWindow()
{
    const TCHAR szWindowClass[] = _T("DXRenderer");
    const TCHAR szTitle[] = _T("Win D3D Renderer - Mooncake");

    const UINT Width = 600;
    const UINT Height = 400;
    
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

        return;
    }
    
    // Create the base window
    hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        static_cast<int>(Width), static_cast<int>(Height),
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


        return;
    }

    // DX Viewport params
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<float>(Width);
    Viewport.Height = static_cast<float>(Height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 100.0f;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC MainWindow::CreatePipelineDesc()
{
    D3D12_INPUT_ELEMENT_DESC InElementDesc[] =  // Define the vertex input layout.
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    
    D3D12_RASTERIZER_DESC Raster_Desc{};
    Raster_Desc.FillMode = D3D12_FILL_MODE_SOLID;
    Raster_Desc.CullMode = D3D12_CULL_MODE_NONE;
    Raster_Desc.FrontCounterClockwise = false;
    Raster_Desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    Raster_Desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    Raster_Desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    Raster_Desc.DepthClipEnable = true;
    Raster_Desc.MultisampleEnable = false;
    Raster_Desc.AntialiasedLineEnable = false;
    Raster_Desc.ForcedSampleCount = 0;
    Raster_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    
    D3D12_BLEND_DESC Blend_Desc{};
    Blend_Desc.AlphaToCoverageEnable = false;
    Blend_Desc.IndependentBlendEnable = false;
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipeStateDesc = {};
    PipeStateDesc.InputLayout = { InElementDesc, _countof(InElementDesc) }; // Array of shader intrinsics
    PipeStateDesc.pRootSignature = RootSig.Get();
    PipeStateDesc.VS = {reinterpret_cast<UINT8*>(VS->GetBufferPointer()), VS->GetBufferSize()};
    PipeStateDesc.PS = {reinterpret_cast<UINT8*>(PS->GetBufferPointer()), PS->GetBufferSize()};
    PipeStateDesc.RasterizerState = Raster_Desc;
    PipeStateDesc.BlendState = Blend_Desc;
    PipeStateDesc.DepthStencilState.DepthEnable = FALSE;
    PipeStateDesc.DepthStencilState.StencilEnable = FALSE;
    PipeStateDesc.SampleMask = UINT_MAX;
    PipeStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PipeStateDesc.NumRenderTargets = FrameBufferCount;
    PipeStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Probably should iterate through.
    PipeStateDesc.SampleDesc.Count = 1;

    return PipeStateDesc;
}


void MainWindow::SetupRootSignature()
{
    D3D12_DESCRIPTOR_RANGE1 RtvDescRanges[1];
    RtvDescRanges[0].NumDescriptors = 1;
    RtvDescRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    RtvDescRanges[0].BaseShaderRegister = 0;
    RtvDescRanges[0].RegisterSpace = 0;
    RtvDescRanges[0].OffsetInDescriptorsFromTableStart = 0;
    RtvDescRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    D3D12_ROOT_PARAMETER1 RootParam[1] = {};
    RootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    RootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    RootParam[0].DescriptorTable.NumDescriptorRanges = 1;
    RootParam[0].DescriptorTable.pDescriptorRanges = RtvDescRanges;


    D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    RootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    RootSignatureDesc.Desc_1_1.NumParameters = 1;
    RootSignatureDesc.Desc_1_1.pParameters = RootParam; 
    RootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
    RootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
    
    ID3DBlob* ErrorBlob = nullptr;
    ID3DBlob* SigBlob = nullptr;
    D3D12SerializeVersionedRootSignature(&RootSignatureDesc, &SigBlob, &ErrorBlob);
    HRESULT HR = Device->CreateRootSignature(0, SigBlob->GetBufferPointer(), SigBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));
    if (FAILED(HR))
    {
        printf("Failed to create root signature.\n");
        PostQuitMessage(1);
        ErrorBlob->Release();
        return;
    }
    RootSig->SetName(L"Main Render Sig");
    SigBlob->Release();
}

void MainWindow::SetupRendering()
{
    HRESULT HR;

    // SwapChain
    DXGI_SWAP_CHAIN_DESC1 desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
    desc.Width = 600;
    desc.Height = 400;
    desc.BufferCount = 2;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;      //multisampling setting
    desc.SampleDesc.Quality = 0;    //vendor-specific flag
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // Might need to use CreateSwapChainForCoreWindow || CreateSwapchainForComposition...
    ComPtr<IDXGISwapChain1> BaseSwapChain;
    HR = Factory->CreateSwapChainForHwnd(CmdQueue.Get(), hWnd, &desc, nullptr, nullptr, &BaseSwapChain);
    if (FAILED(HR))
    {
        printf("Failed to create swap chain.\n");
        PostQuitMessage(1);
        return;
    }
    BaseSwapChain.As(&SwapChain); // To SwapChain4.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();
    
    SwapChain->ResizeBuffers(2, 600, 400, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // RTV Heaps
    D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
    RtvHeapDesc.NumDescriptors = FrameBufferCount;
    RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR = Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&FrameBufferHeap));
    if (FAILED(HR))
    {
        printf("Failed to create rtv heap.\n");
        PostQuitMessage(1);
        return;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE BufferHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHeapOffsetSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    // Create Buffer Resources.
    D3D12_RENDER_TARGET_VIEW_DESC RtvDesc{};
    RtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // Swap chain rtv setup.
    FrameBuffers.resize(FrameBufferCount);
    for (UINT Idx = 0; Idx < FrameBufferCount; Idx++)
    {
        ComPtr<ID3D12Resource>& Buffer = FrameBuffers.at(Idx);
        SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&Buffer));
        Device->CreateRenderTargetView(Buffer.Get(), &RtvDesc, BufferHandle);

        // Offset Buffer Handle
        BufferHandle.ptr += RtvHeapOffsetSize;
    }
    
    // Load and Compile shaders...
    UINT CompileFlags = 0; // Can use D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &VS, nullptr);
    if (FAILED(HR))
    {
        printf("Failed to compile vertex shaders.\n");
        PostQuitMessage(1);
        return;
    }
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &PS, nullptr);
    if (FAILED(HR))
    {
        printf("Failed to compile pixel shaders.\n");
        PostQuitMessage(1);
        return;
    }
    
    // Root Signature
    SetupRootSignature();
    
    // Create Pipeline State
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PipeStateDesc = CreatePipelineDesc();
    HR = Device->CreateGraphicsPipelineState(&PipeStateDesc, IID_PPV_ARGS(&PipelineState));
    if (FAILED(HR))
    {
        printf("Failed to create graphics pipeline.\n");
        PostQuitMessage(1);
        return;
    }
    
    // DX Setup correctly.
    bDXReady = true;
}

void MainWindow::UpdateRender()
{
    
}

void MainWindow::Render()
{
    HRESULT HR;
    
    // Reset render queue.
    CmdAllocator->Reset();
    CmdList->Reset(CmdAllocator.Get(), nullptr);

    // Clear RTs
    FLOAT ClearColour[4] = { 0.6f, 0.6f, 0.6f, 1.0f }; // Base grey...
    
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHandle.ptr = RtvHandle.ptr + static_cast<SIZE_T>(CurrentBackBuffer * RtvHeapOffsetSize); 
    CmdList->ClearRenderTargetView(RtvHandle, ClearColour, 0, nullptr);

    // Finalise command list and queues.
    CmdList->Close();

    ID3D12CommandList* const CmdLists[] = {
        CmdList.Get()
    };

    CmdQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
    
    DXGI_PRESENT_PARAMETERS dxgiParams = {};
    HR = SwapChain->Present(1, 0); //, &dxgiParams);
    if (FAILED(HR))
    {
        assert(false && "Failed to present swap chain!");
        //printf("Swap chain failed to present!\n");
    }
    
    // Not the best practice, however works for this example...
    const UINT64 CurrentFenceValue = FenceValue;
    CmdQueue->Signal(Fence.Get(), CurrentFenceValue);
    FenceValue++;

    if (Fence->GetCompletedValue() < CurrentFenceValue)
    {
        Fence->SetEventOnCompletion(CurrentFenceValue, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }

    // Update index.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();
}


int MainWindow::Run()
{
    if (!bDXReady) { return 1; }
    
    // Show, its hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
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
            // Currently done in the Paint message.

            // Render
            //Render();

        }
    
    }

    return static_cast<int>(msg.wParam);
}
