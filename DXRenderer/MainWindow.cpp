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
    SetupSwapChain();
    SetupMeshPipeline();
       
    // DX Setup correctly.
    bDXReady = true;
}



MainWindow::~MainWindow()
{
    WaitForPreviousFrame();
    CloseHandle(FenceEvent);
    G_MainWindow = nullptr;
    FrameBuffers.clear();
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
        G_MainWindow->UpdateRender();
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

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
    {
        DebugController->EnableDebugLayer();
    }

    HR = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DebugDXGI));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create IDXGIDebug1!.", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    // Setup DX Factory
    HR = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create D3D12 Factory!.", L"Error", MB_OK);
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
        MessageBoxW(nullptr, L"Failed to create D3D12 Factory!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    // Get the device.
    HR = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create D3D12 device!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
    HR = Device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&CmdQueue));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command queue!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    HR = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdAllocator));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command allocator!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAllocator.Get(), 0, IID_PPV_ARGS(&CmdList));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command list!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    CmdList->Close();

    // Setup the events.
    HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create DX Fence!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    FenceEvent = CreateEvent(nullptr, false, false, nullptr);
    
}

void MainWindow::SetupWindow()
{
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
    Viewport.MaxDepth = 1.0f;
}

void MainWindow::SetupSwapChain()
{
    HRESULT HR;

    // SwapChain
    DXGI_SWAP_CHAIN_DESC1 desc;
    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC1));
    desc.Width = Width;
    desc.Height = Height;
    desc.BufferCount = FrameBufferCount;
    desc.Format = FrameBufferFormat;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;      //multisampling setting
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Might need to use CreateSwapChainForCoreWindow || CreateSwapchainForComposition...
    ComPtr<IDXGISwapChain1> BaseSwapChain;
    HR = Factory->CreateSwapChainForHwnd(CmdQueue.Get(), hWnd, &desc, nullptr, nullptr, &BaseSwapChain);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create swap chain!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    BaseSwapChain.As(&SwapChain); // To SwapChain4.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();

    Factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    SwapChain->ResizeBuffers(2, Width, Height, FrameBufferFormat, 0);

    // RTV Heaps
    D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
    RtvHeapDesc.NumDescriptors = FrameBufferCount;
    RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR = Device->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&FrameBufferHeap));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create rtv heap!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE BufferHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHeapOffsetSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    FrameBufferHeap->SetName(L"Frame Buffer Heap");

    // Create Buffer Resources.
    D3D12_RENDER_TARGET_VIEW_DESC RtvDesc{};
    RtvDesc.Format = FrameBufferFormat;
    RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // Swap chain rtv setup.
    FrameBuffers.resize(FrameBufferCount);
    for (UINT Idx = 0; Idx < FrameBufferCount; Idx++)
    {
        ComPtr<ID3D12Resource>& Buffer = FrameBuffers.at(Idx);
        HR = SwapChain->GetBuffer(Idx, IID_PPV_ARGS(&Buffer));
        if (FAILED(HR))
        {
            MessageBoxW(nullptr, L"Failed to get buffer!", L"Error", MB_OK);
            printf("Failed to get buffer at idx: %d", Idx);
            PostQuitMessage(1);
        }
        Device->CreateRenderTargetView(Buffer.Get(), &RtvDesc, BufferHandle); // Buffers are null ptr after creating RTs, not normal...

        // Offset Buffer Handle
        BufferHandle.ptr += RtvHeapOffsetSize;
    }
}

void MainWindow::SetupMeshPipeline()
{
    HRESULT HR;

    // Load and Compile shaders...
#if defined(_DEBUG)
    UINT CompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT CompileFlags = 0;
#endif
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", CompileFlags, 0, &VS, nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to compile vertex shaders!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    HR = D3DCompileFromFile(L"./Shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", CompileFlags, 0, &PS, nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to compile pixel shaders!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    // Root Signature
    SetupRootSignature();

    // Create Pipeline State
    CreateMeshPipeline();

    // Setup buffers.
    MeshConstantBuffer();
    MeshIndexBuffer();
    MeshVertexBuffer();
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
    RootParam[0].DescriptorTable.NumDescriptorRanges = _countof(RtvDescRanges);
    RootParam[0].DescriptorTable.pDescriptorRanges = RtvDescRanges;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    RootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    RootSignatureDesc.Desc_1_1.NumParameters = _countof(RootParam);
    RootSignatureDesc.Desc_1_1.pParameters = RootParam;
    RootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
    RootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

    ComPtr<ID3DBlob> ErrorBlob = nullptr;
    ComPtr<ID3DBlob> SigBlob;
    HRESULT HR = D3D12SerializeVersionedRootSignature(&RootSignatureDesc, &SigBlob, &ErrorBlob);
    if FAILED(HR)
    {
        MessageBoxW(nullptr, L"Failed to serialize root signature!", L"Error", MB_OK);
        if (ErrorBlob) { ErrorBlob->Release(); }
        PostQuitMessage(1);
        return;
    }

    HR = Device->CreateRootSignature(0, SigBlob->GetBufferPointer(), SigBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create root signature!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    RootSig->SetName(L"Main Render Root Signature");
    SigBlob->Release();
    if (ErrorBlob) { ErrorBlob->Release(); }
}

void MainWindow::CreateMeshPipeline()
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
    Raster_Desc.DepthClipEnable = false;
    Raster_Desc.MultisampleEnable = false;
    Raster_Desc.AntialiasedLineEnable = false;
    Raster_Desc.ForcedSampleCount = 0;
    Raster_Desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    
    D3D12_BLEND_DESC Blend_Desc{};
    Blend_Desc.AlphaToCoverageEnable = false;
    Blend_Desc.IndependentBlendEnable = false;
    const D3D12_RENDER_TARGET_BLEND_DESC DefaultRenderTargetBlendDesc =
    {
        false, false,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    Blend_Desc.RenderTarget[0] = DefaultRenderTargetBlendDesc;
    
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
    PipeStateDesc.NumRenderTargets = 1;
    PipeStateDesc.RTVFormats[0] = FrameBufferFormat; // Probably should iterate through.
    PipeStateDesc.SampleDesc.Count = 1;

    HRESULT HR = Device->CreateGraphicsPipelineState(&PipeStateDesc, IID_PPV_ARGS(&PipelineState));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create graphics pipeline!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    PipelineState->SetName(L"Pipeline State - Mesh");
}

void MainWindow::MeshConstantBuffer()
{
    HRESULT HR;

    // Create the Constant Buffer
    D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
    HeapDesc.NodeMask = 0;
    HeapDesc.NumDescriptors = 1;
    HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    HR = Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&ConstantBufferHeap));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create constant buffer descriptor heap!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 0;
    HeapProps.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC CbResourceDesc;
    CbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    CbResourceDesc.Alignment = 0;
    CbResourceDesc.Width = (sizeof(WVP) + 255) & ~255;
    CbResourceDesc.Height = 1;
    CbResourceDesc.DepthOrArraySize = 1;
    CbResourceDesc.MipLevels = 1;
    CbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    CbResourceDesc.SampleDesc.Count = 1;
    CbResourceDesc.SampleDesc.Quality = 0;
    CbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    CbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &CbResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ConstantBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create constant buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }


    // Create our Constant Buffer View
    D3D12_CONSTANT_BUFFER_VIEW_DESC CbvDesc = {};
    CbvDesc.BufferLocation = ConstantBuffer->GetGPUVirtualAddress();
    CbvDesc.SizeInBytes = (sizeof(WVP) + 255) & ~255; // CB size is required to be 256-byte aligned.

    D3D12_CPU_DESCRIPTOR_HANDLE CbvHandle(ConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
    CbvHandle.ptr = CbvHandle.ptr + Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;
    Device->CreateConstantBufferView(&CbvDesc, CbvHandle);

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE ReadRange;
    ReadRange.Begin = 0;
    ReadRange.End = 0;

    UINT8* MappedConstantBuffer;

    HR = ConstantBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&MappedConstantBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to map constant buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    memcpy(MappedConstantBuffer, &WVP, sizeof(WVP));
    ConstantBuffer->Unmap(0, &ReadRange);

    ConstantBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");
    ConstantBuffer->SetName(L"Constant Buffer");
}

void MainWindow::MeshVertexBuffer()
{
    HRESULT HR;
    
    const UINT VertexBufferSize = sizeof(VertexBufferData);

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC VertexBufferResourceDesc;
    VertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    VertexBufferResourceDesc.Alignment = 0;
    VertexBufferResourceDesc.Width = VertexBufferSize;
    VertexBufferResourceDesc.Height = 1;
    VertexBufferResourceDesc.DepthOrArraySize = 1;
    VertexBufferResourceDesc.MipLevels = 1;
    VertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    VertexBufferResourceDesc.SampleDesc.Count = 1;
    VertexBufferResourceDesc.SampleDesc.Quality = 0;
    VertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    VertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
     
    HR = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &VertexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&VertexBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create vertex buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    // Copy the triangle data to the vertex buffer.
    UINT8* VertexDataBegin;

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE ReadRange;
    ReadRange.Begin = 0;
    ReadRange.End = 0;

    HR = VertexBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&VertexDataBegin));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to map vertex buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    memcpy(VertexDataBegin, VertexBufferData, sizeof(VertexBufferData));
    VertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.StrideInBytes = sizeof(Vertex);
    VertexBufferView.SizeInBytes = VertexBufferSize;

    VertexBuffer->SetName(L"Vertex Buffer");

    Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    FenceValue = 1;
    FenceEvent = CreateEvent(nullptr, false, false, nullptr);
    WaitForPreviousFrame();
}

void MainWindow::MeshIndexBuffer()
{
    HRESULT HR;
    
    // Declare Handles
    const UINT IndexBufferSize = sizeof(TriIndexBufferData);

    D3D12_HEAP_PROPERTIES HeapIndexProps;
    HeapIndexProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapIndexProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapIndexProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapIndexProps.CreationNodeMask = 1;
    HeapIndexProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC IndexBufferResourceDesc;
    IndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    IndexBufferResourceDesc.Alignment = 0;
    IndexBufferResourceDesc.Width = IndexBufferSize;
    IndexBufferResourceDesc.Height = 1;
    IndexBufferResourceDesc.DepthOrArraySize = 1;
    IndexBufferResourceDesc.MipLevels = 1;
    IndexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    IndexBufferResourceDesc.SampleDesc.Count = 1;
    IndexBufferResourceDesc.SampleDesc.Quality = 0;
    IndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    IndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR = Device->CreateCommittedResource(
        &HeapIndexProps, D3D12_HEAP_FLAG_NONE, &IndexBufferResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&IndexBuffer));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create index buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    // Copy data to DirectX 12 driver memory:
    UINT8* pVertexDataBegin;

    // We do not intend to read from this resource on the CPU.
    D3D12_RANGE IdxReadRange;
    IdxReadRange.Begin = 0;
    IdxReadRange.End = 0;
    
    HR = IndexBuffer->Map(0, &IdxReadRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to Map index buffer!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    memcpy(pVertexDataBegin, TriIndexBufferData, sizeof(TriIndexBufferData));
    IndexBuffer->Unmap(0, nullptr);

    IndexBuffer->SetName(L"Mesh Index Buffer");

    // 👀 Initialize the index buffer view.
    IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    IndexBufferView.SizeInBytes = IndexBufferSize;
}

void MainWindow::UpdateRender()
{
    // Update things like camera pos and view, etc...
     
    XMFLOAT4X4 Default = XMFLOAT4X4(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
    WVP.ProjectionMatrix = Default;
    WVP.ModelMatrix = Default;
    WVP.ProjectionMatrix= Default;
}

void MainWindow::Render()
{
    HRESULT HR;
    
    // Reset render queue.
    CmdAllocator->Reset();
    HR = CmdList->Reset(CmdAllocator.Get(), PipelineState.Get());
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to reset the command list!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }

    CmdList->SetGraphicsRootSignature(RootSig.Get());
    CmdList->RSSetViewports(1, &Viewport);
    D3D12_RECT ScissorRect{};
    ScissorRect.right = Width;
    ScissorRect.bottom = Height;
    ScissorRect.left = 0;
    ScissorRect.top = 0;
    CmdList->RSSetScissorRects(1, &ScissorRect);
    CmdList->SetPipelineState(PipelineState.Get());

    // Upload const buffers
    ComPtr<ID3D12DescriptorHeap> DescriptorHeaps[] = {ConstantBufferHeap};
    CmdList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps->GetAddressOf());  
    D3D12_GPU_DESCRIPTOR_HANDLE CbHandle(ConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());
    CmdList->SetGraphicsRootDescriptorTable(0, CbHandle);
    
    // Clear Backbuffer...
    // Indicate that the back buffer will be used as a render target.
    D3D12_RESOURCE_BARRIER RtBarrier;
    RtBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    RtBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    RtBarrier.Transition.pResource = FrameBuffers.at(CurrentBackBuffer).Get();
    RtBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    RtBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    RtBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CmdList->ResourceBarrier(1, &RtBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHandle.ptr = RtvHandle.ptr + static_cast<SIZE_T>(CurrentBackBuffer * RtvHeapOffsetSize); 
    CmdList->OMSetRenderTargets(1, &RtvHandle, false, nullptr);

    // Clear Backbuffer
    FLOAT ClearColour[4] = { 0.6f, 0.6f, 0.6f, 1.0f }; // Base grey...
    CmdList->ClearRenderTargetView(RtvHandle, ClearColour, 0, nullptr);
    
    // Mesh rendering
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->IASetVertexBuffers(0, 1, &VertexBufferView);
    CmdList->IASetIndexBuffer(&IndexBufferView);

    CmdList->DrawIndexedInstanced(3, 1, 0 ,0, 0);
    
    D3D12_RESOURCE_BARRIER PresentBarrier;
    PresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    PresentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    PresentBarrier.Transition.pResource = FrameBuffers.at(CurrentBackBuffer).Get();
    PresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    PresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    PresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CmdList->ResourceBarrier(1, &PresentBarrier);

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
        MessageBoxW(nullptr, L"Failed to present swap chain!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    
    // Not the best practice, however works for this example...
    WaitForPreviousFrame();

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

            //Update();
            //Render();

        }
    
    }

    return static_cast<int>(msg.wParam);
}

void MainWindow::WaitForPreviousFrame()
{
    // Signal and increment the fence value.
    const UINT64 CurrentFence = FenceValue;
    (CmdQueue->Signal(Fence.Get(), CurrentFence));
    FenceValue++;

    // Wait until the previous frame is finished.
    if (Fence->GetCompletedValue() < CurrentFence)
    {
        Fence->SetEventOnCompletion(CurrentFence, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }

    CurrentBackBuffer= SwapChain->GetCurrentBackBufferIndex();
}