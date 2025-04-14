#include "Renderer.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>

#include "MainWindow.h"
#include "StaticMeshPipeline.h"

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


Renderer::~Renderer()
{
    if (CmdQueue) { WaitForPreviousFrame(); }
    CloseHandle(FenceEvent);
    FrameBuffers.clear();
}

bool Renderer::Setup()
{
    if (!SetupDevice())                             { return false; } // return without setting bDXReady to true...
    if (!G_MainWindow->SetupWindow(Width, Height))  { return false; }
    if (!SetupSwapChain())                          { return false; }
    if (!SetupMeshRootSignature())                  { return false; }
    
    SMPipe = std::make_unique<StaticMeshPipeline>(this);
       
    // DX Setup correctly.
    bDXReady = true;
    return bDXReady;
}

bool Renderer::SetupDevice()
{
    HRESULT HR;
    bool bResult = false;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
    {
        DebugController->EnableDebugLayer();
    }

    HR = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DebugDXGI));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create IDXGIDebug1!.", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Setup DX Factory
    HR = CreateDXGIFactory1(IID_PPV_ARGS(&Factory));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create D3D12 Factory!.", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Get Adapter
    DXGI_ADAPTER_DESC1 AdapterDesc;
    while (Factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&Adapter)) != DXGI_ERROR_NOT_FOUND)
    {
        HR = Adapter.Get()->GetDesc1(&AdapterDesc);
        WCHAR wstring[128] = L"Using adapter: ";
        wcscat_s(wstring, AdapterDesc.Description);
        wcscat_s(wstring, L"\n");
        OutputDebugStringW(wstring);
        break;
    }
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create D3D12 Factory!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Get the device.
    HR = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create D3D12 device!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    
    D3D12_COMMAND_QUEUE_DESC CmdQueueDesc{};
    HR = Device->CreateCommandQueue(&CmdQueueDesc, IID_PPV_ARGS(&CmdQueue));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command queue!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    HR = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdAllocator));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command allocator!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }

    // Setup the command lists.
    HR = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAllocator.Get(), 0, IID_PPV_ARGS(&CmdList));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create command list!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    CmdList->Close();

    // Create as closed.
    HR = Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListBeginFrame));
    HR = Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListMidFrame));
    HR = Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListEndFrame));

    // Setup the events.
    HR = Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create DX Fence!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    FenceEvent = CreateEvent(nullptr, false, false, nullptr);
    
    // DX Viewport params
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<float>(Width);
    Viewport.Height = static_cast<float>(Height);
    Viewport.MinDepth = NearPlane;
    Viewport.MaxDepth = FarPlane;
    
    bResult = true;
    return bResult;
}

bool Renderer::SetupSwapChain()
{
    HRESULT HR;
    bool bResult = false;

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

    ComPtr<IDXGISwapChain1> BaseSwapChain;
    HR = Factory->CreateSwapChainForHwnd(CmdQueue.Get(), G_MainWindow->GetHWND(), &desc, nullptr, nullptr, &BaseSwapChain);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create swap chain!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    BaseSwapChain.As(&SwapChain); // To SwapChain4.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();

    Factory->MakeWindowAssociation(G_MainWindow->GetHWND(), DXGI_MWA_NO_ALT_ENTER);
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
        return bResult;
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
            return  bResult;
        }
        Device->CreateRenderTargetView(Buffer.Get(), &RtvDesc, BufferHandle); // Buffers are null ptr after creating RTs, not normal...

        // Offset Buffer Handle
        BufferHandle.ptr += RtvHeapOffsetSize;
    }

    bResult = true;
    return bResult;
}

bool Renderer::SetupMeshRootSignature()
{
    bool bResult = false;

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
        return bResult;
    }

    HR = Device->CreateRootSignature(0, SigBlob->GetBufferPointer(), SigBlob->GetBufferSize(), IID_PPV_ARGS(&RootSig));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create root signature!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    RootSig->SetName(L"Main Mesh Root Signature");
    SigBlob->Release();
    if (ErrorBlob) { ErrorBlob->Release(); }

    bResult = true;
    return bResult;
}

void Renderer::BeginFrame()
{
    HRESULT HR;
    
    // Reset the allocator.
    HR = CmdAllocator->Reset();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"BeginFrame: Failed to reset CmdAllocator!", L"Error", MB_OK);
        PostQuitMessage(1);;
    }
    
    // Reset Cmd list
    HR = CmdListBeginFrame->Reset(CmdAllocator.Get(), nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to reset 'CmdListBeginFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    CmdListBeginFrame->SetName(L"CmdList-BeginFrame");
    
    // Indicate which back buffer to use
    D3D12_RESOURCE_BARRIER RtBarrier;
    RtBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    RtBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    RtBarrier.Transition.pResource = FrameBuffers.at(CurrentBackBuffer).Get();
    RtBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    RtBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    RtBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CmdListBeginFrame->ResourceBarrier(1, &RtBarrier);

    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHandle.ptr = RtvHandle.ptr + static_cast<SIZE_T>(CurrentBackBuffer * RtvHeapOffsetSize); 
    
    // Clear render targets (and depth stencil if we have it).
    FLOAT ClearColour[4] = { 0.6f, 0.6f, 0.6f, 1.0f }; // Base grey...
    CmdListBeginFrame->ClearRenderTargetView(RtvHandle, ClearColour, 0, nullptr);
    
    HR = CmdListBeginFrame->Close();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to close 'CmdListBeginFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
}

void Renderer::MidFrame()
{
    // Transition shadow map from the shadow pass to the readable scene pass.
    // Not necessary with this current renderer.
}

void Renderer::EndFrame()
{
    HRESULT HR;
    
    HR = CmdListEndFrame->Reset(CmdAllocator.Get(), nullptr);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"EndFrame: Failed to reset 'CmdListEndFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    CmdListEndFrame->SetName(L"CmdList-EndFrame");
    
    // Set the RT for the Output merger...?
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHandle.ptr = RtvHandle.ptr + static_cast<SIZE_T>(CurrentBackBuffer * RtvHeapOffsetSize); 
    CmdListEndFrame->OMSetRenderTargets(1, &RtvHandle, false, nullptr);
    
    // Indicate that the back buffer will be used to present.
    D3D12_RESOURCE_BARRIER PresentBarrier;
    PresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    PresentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    PresentBarrier.Transition.pResource = FrameBuffers.at(CurrentBackBuffer).Get();
    PresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    PresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    PresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CmdListEndFrame->ResourceBarrier(1, &PresentBarrier);

    HR = CmdListEndFrame->Close();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"EndFrame: Failed to close 'CmdListEndFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
}

void Renderer::WaitForPreviousFrame()
{
    // Signal and increment the fence value.
    const UINT64 CurrentFence = FenceValue;
    CmdQueue->Signal(Fence.Get(), CurrentFence);
    FenceValue++;

    // Wait until the previous frame is finished.
    if (Fence->GetCompletedValue() < CurrentFence)
    {
        Fence->SetEventOnCompletion(CurrentFence, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }

    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();
}

void Renderer::Update()
{
    // Using Left handed coordinate systems, but matrices need to be transposed for hlsl.
    XMMATRIX Model = DirectX::XMMatrixIdentity();
    XMMATRIX Rot = DirectX::XMMatrixRotationY(static_cast<float>(G_MainWindow->GetTime()));
    WVP.ModelMatrix = DirectX::XMMatrixMultiply(Model, Rot);
    WVP.ModelMatrix = DirectX::XMMatrixTranspose(WVP.ModelMatrix);
    
    WVP.ViewMatrix = DirectX::XMMatrixLookAtLH({0, 0, -2, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}); // Y is up.
    WVP.ViewMatrix = DirectX::XMMatrixTranspose(WVP.ViewMatrix);

    WVP.ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(FieldOfView), AspectRatio, NearPlane, FarPlane);
    WVP.ProjectionMatrix = DirectX::XMMatrixTranspose(WVP.ProjectionMatrix);

    // Update constant buffer.
    SMPipe->Update(WVP);
}

void Renderer::Render()
{
    HRESULT HR;

    BeginFrame();
    MidFrame();
    EndFrame();
    
    // Populate pipeline specific command lists.
    // E.g: 1 for backbuffer, 1 for mesh rendering.
    ID3D12CommandList* const CmdLists[3] = {
        CmdListBeginFrame.Get(),
        SMPipe->PopulateCmdList().Get(),
        CmdListEndFrame.Get()
    };

    CmdQueue->ExecuteCommandLists(_countof(CmdLists), CmdLists);
    
    HR = SwapChain->Present(1, 0);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to present swap chain!", L"Error", MB_OK);
        PostQuitMessage(1);
        return;
    }
    
    // Not the best practice, however works for this example...
    WaitForPreviousFrame();

    // Update index.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();

}
