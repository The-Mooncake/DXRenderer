#include "Renderer.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <string>

#include "MainWindow.h"
#include "StaticMeshPipeline.h"

#include <nvtx3/nvtx3.hpp>

// Imgui
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "ImGuiDescHeap.h"



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
    nvtx3::scoped_range r{ "Setup Renderer" };

    if (!SetupDevice())                             { return false; } // return without setting bDXReady to true...
    if (!G_MainWindow->SetupWindow())               { return false; }
    if (!SetupSwapChain())                          { return false; }
    if (!SetupMeshRootSignature())                  { return false; }
    if (!SetupImguiRendering())                     { return false; }
    
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
    
    // Create as closed.
    std::string MsgCmdList;
    HR = Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListBeginFrame));
    if (FAILED(HR)) { MsgCmdList += "Failed to create command begin frame cmd list!\n"; }
    HR = HR | Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListMidFrame));
    if (FAILED(HR)) { MsgCmdList += "Failed to create command mid frame cmd list!\n"; }
    HR = HR | Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CmdListEndFrame));
    if (FAILED(HR)) { MsgCmdList += "Failed to create command end frame cmd list!"; }
    if (MsgCmdList.length() > 0)
    {
        LPCWSTR msg = reinterpret_cast<LPCWSTR>(MsgCmdList.c_str());
        MessageBoxW(nullptr, msg, L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    
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
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = MaxDepth;
    
    bResult = true;
    return bResult;
}

bool Renderer::SetupSwapChain()
{
    HRESULT HR;
    bool bResult = false;

    // SwapChain
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
    ZeroMemory(&SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
    SwapChainDesc.Width = Width;
    SwapChainDesc.Height = Height;
    SwapChainDesc.BufferCount = FrameBufferCount;
    SwapChainDesc.Format = FrameBufferFormat;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SampleDesc.Count = 1;      //multisampling setting
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    // SWAP CHAIN WAS SCALING BACK BUFFERS TO FIT WINDOW SIZE - CAUSING IMGUI TO NOT LINE UP.
    SwapChainDesc.Scaling = DXGI_SCALING_NONE; // Disabling scaling.
    if (!VSyncEnabled) { SwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; }
    
    ComPtr<IDXGISwapChain1> BaseSwapChain;
    HR = Factory->CreateSwapChainForHwnd(CmdQueue.Get(), G_MainWindow->GetHWND(), &SwapChainDesc, nullptr, nullptr, &BaseSwapChain);
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create swap chain!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    BaseSwapChain.As(&SwapChain); // To SwapChain4.
    CurrentBackBuffer = SwapChain->GetCurrentBackBufferIndex();

    // Set the background colour to be red to indicate an error.
    const DXGI_RGBA ErrorColor{1.0f, 0.0f, 0.0f, 1.0f};
    SwapChain->SetBackgroundColor(&ErrorColor);

    Factory->MakeWindowAssociation(G_MainWindow->GetHWND(), DXGI_MWA_NO_ALT_ENTER);
    SwapChain->ResizeBuffers(FrameBufferCount, Width, Height, FrameBufferFormat, 0);

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

    if (bResult = CreateFrameBuffers(); !bResult) { return bResult; } 
    
    // Create Depth/Stencil Buffer
    D3D12_DESCRIPTOR_HEAP_DESC DepthHeapDesc = {};
    DepthHeapDesc.NumDescriptors = 1;
    DepthHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DepthHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR = Device->CreateDescriptorHeap(&DepthHeapDesc, IID_PPV_ARGS(&DepthBufferHeap));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create depth/stencil heap!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    DepthBufferHeap->SetName(L"Depth/Stencil Resource Heap");

    // Depth Heap Resource
    D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilDesc = {};
    DepthStencilDesc.Format = DepthSampleFormat;
    DepthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    DepthStencilDesc.Texture2D.MipSlice = 0;

    D3D12_CLEAR_VALUE DepthOptimizedClearValue = {};
    DepthOptimizedClearValue.Format = DepthSampleFormat;
    DepthOptimizedClearValue.DepthStencil.Depth = MaxDepth;
    DepthOptimizedClearValue.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_DESC DepthResourceDesc = {};
    DepthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthResourceDesc.Alignment = 0;
    DepthResourceDesc.Width = Width;
    DepthResourceDesc.Height = Height;
    DepthResourceDesc.DepthOrArraySize = 1;
    DepthResourceDesc.MipLevels = 1;
    DepthResourceDesc.Format = DepthSampleFormat;
    DepthResourceDesc.SampleDesc.Count = 1;
    DepthResourceDesc.SampleDesc.Quality = 0;
    DepthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES DepthHeapProps;
    DepthHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    DepthHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    DepthHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    DepthHeapProps.CreationNodeMask = 0;
    DepthHeapProps.VisibleNodeMask = 0;

    HR = Device->CreateCommittedResource(
    &DepthHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &DepthResourceDesc,
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &DepthOptimizedClearValue,
    IID_PPV_ARGS(&DepthBuffer)
    );
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to create depth/stencil committed resource!", L"Error", MB_OK);
        PostQuitMessage(1);
        return bResult;
    }
    DepthBuffer->SetName(L"Depth Buffer Resource");

    Device->CreateDepthStencilView(DepthBuffer.Get(), &DepthStencilDesc, DepthBufferHeap->GetCPUDescriptorHandleForHeapStart());
    
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
    nvtx3::scoped_range r("BeginFrame");

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
    CmdListBeginFrame->BeginEvent(1, "BeginFrame", sizeof("BeginFrame"));
    
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
    
    // Clear render targets and depth|stencil.
    FLOAT ClearColour[4] = { 0.6f, 0.6f, 0.6f, 1.0f }; // Base grey...
    CmdListBeginFrame->ClearRenderTargetView(RtvHandle, ClearColour, 0, nullptr);
    CmdListBeginFrame->ClearDepthStencilView(DepthBufferHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, MaxDepth, 0, 0, nullptr);
    
    CmdListBeginFrame->EndEvent();
    HR = CmdListBeginFrame->Close();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"Failed to close 'CmdListBeginFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
}

void Renderer::MidFrame()
{
    nvtx3::scoped_range r("MidFrame");

    // Transition shadow map from the shadow pass to the readable scene pass.
    // Not necessary with this current renderer.
}

void Renderer::EndFrame()
{
    nvtx3::scoped_range r("EndFrame");

    HRESULT HR;
    
    HR = CmdListEndFrame->Reset(CmdAllocator.Get(), nullptr);
    CmdListEndFrame->BeginEvent(1, "EndFrame", sizeof("EndFrame"));
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"EndFrame: Failed to reset 'CmdListEndFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
    CmdListEndFrame->SetName(L"CmdList-EndFrame");

    // Imgui Rendering
    SetBackBufferOM(CmdListEndFrame); // Imgui requires the output merger.
    CmdListEndFrame->SetDescriptorHeaps(1, SrvBufferHeap.GetAddressOf());
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdListEndFrame.Get());    

    // Indicate that the back buffer will be used to present.
    D3D12_RESOURCE_BARRIER PresentBarrier;
    PresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    PresentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    PresentBarrier.Transition.pResource = FrameBuffers.at(CurrentBackBuffer).Get();
    PresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    PresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    PresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    CmdListEndFrame->ResourceBarrier(1, &PresentBarrier);
    
    // Finish frame.
    CmdListEndFrame->EndEvent();
    HR = CmdListEndFrame->Close();
    if (FAILED(HR))
    {
        MessageBoxW(nullptr, L"EndFrame: Failed to close 'CmdListEndFrame'!", L"Error", MB_OK);
        PostQuitMessage(1);
    }
}

bool Renderer::SetupImguiRendering()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 64;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&SrvBufferHeap)) != S_OK)
        return false;
    ImguiHeapAlloc.Create(Device.Get(), SrvBufferHeap.Get());
    SrvBufferHeap->SetName(L"Imgui-SrvBufferHeap");
    return true;
}

bool Renderer::CreateFrameBuffers()
{
    HRESULT HR;

    D3D12_CPU_DESCRIPTOR_HANDLE BufferHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHeapOffsetSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    FrameBufferHeap->SetName(L"Frame Buffer Heap");
    
    D3D12_RENDER_TARGET_VIEW_DESC RtvDesc{};
    RtvDesc.Format = FrameBufferFormat;
    RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    
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
            return false;
        }
        Device->CreateRenderTargetView(Buffer.Get(), &RtvDesc, BufferHandle); // Buffers are null ptr after creating RTs, not normal...

        // Offset Buffer Handle
        BufferHandle.ptr += RtvHeapOffsetSize;
    }
    return true;
}

void Renderer::CleanupFrameBuffers()
{
    WaitForPreviousFrame();
    FrameBuffers.clear();
}

void Renderer::WaitForPreviousFrame()
{
    nvtx3::scoped_range r{ "WaitForPreviousFrame" };

    // Signal and increment the fence value.
    const UINT64 CurrentFence = FenceValue;
    CmdQueue->Signal(Fence.Get(), CurrentFence);
    FenceValue++;

    // Wait until the previous frame is finished.
    const UINT64 CompletedValue = Fence->GetCompletedValue();
    if (CompletedValue < CurrentFence)
    {
        Fence->SetEventOnCompletion(CurrentFence, FenceEvent);
        WaitForSingleObject(FenceEvent, INFINITE);
    }
}

void Renderer::SetBackBufferOM(ComPtr<ID3D12GraphicsCommandList>& InCmdList) const 
{
    D3D12_CPU_DESCRIPTOR_HANDLE RtvHandle(FrameBufferHeap->GetCPUDescriptorHandleForHeapStart());
    RtvHandle.ptr = RtvHandle.ptr + static_cast<SIZE_T>(CurrentBackBuffer * RtvHeapOffsetSize);

    D3D12_CPU_DESCRIPTOR_HANDLE DepthHandle(DepthBufferHeap->GetCPUDescriptorHandleForHeapStart());
    
    InCmdList->OMSetRenderTargets(1, &RtvHandle, false, &DepthHandle);
}

void Renderer::Update()
{
    nvtx3::scoped_range r("Update Tick");

    // Using Left handed coordinate systems, but matrices need to be transposed for hlsl.
    XMMATRIX Model = DirectX::XMMatrixIdentity();
    XMMATRIX Rot = DirectX::XMMatrixRotationY(static_cast<float>(G_MainWindow->GetTime()));
    WVP.ModelMatrix = DirectX::XMMatrixMultiply(Model, Rot);
    WVP.ModelMatrix = DirectX::XMMatrixTranspose(WVP.ModelMatrix);
    
    WVP.ViewMatrix = DirectX::XMMatrixLookAtRH({0, 0, -2, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}); // Y is up.
    WVP.ViewMatrix = DirectX::XMMatrixTranspose(WVP.ViewMatrix);

    WVP.ProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(FieldOfView), AspectRatio, NearPlane, FarPlane);
    WVP.ProjectionMatrix = DirectX::XMMatrixTranspose(WVP.ProjectionMatrix);

    // Update constant buffer.
    SMPipe->Update(WVP);
    
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(); // Show demo window! :)
    
}

void Renderer::Render()
{
    nvtx3::scoped_range r("Render Tick");

    HRESULT HR;
    
    // Build and execute the command list.
    Cmds.clear();

    BeginFrame();
    Cmds.emplace_back(CmdListBeginFrame.Get());

    Cmds.emplace_back(SMPipe->PopulateCmdList().Get());

    EndFrame();
    Cmds.emplace_back(CmdListEndFrame.Get());
    
    CmdQueue->ExecuteCommandLists(static_cast<UINT>(Cmds.size()), Cmds.data());

    // Render to screen
    UINT PresentFlags = 0;
    UINT PresentInterval = 1;
    if (!VSyncEnabled)
    {
        PresentInterval = 0;
        PresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }
    
    HR = SwapChain->Present(PresentInterval, PresentFlags); 
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
