#pragma once


// Common
#include <wrl/client.h>
#include <stdlib.h>
#include <vector>

#include "pch.h"
#include "StaticMeshPipeline.h"

// DX
#include <d3dcommon.h>
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>


using Microsoft::WRL::ComPtr; // Import only the ComPtr

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normals;
    DirectX::XMFLOAT4 Colour;
};

class Renderer
{
public:
    ~Renderer();
    
    // Rendering Setup
    bool Setup();

    // Main render loop.
    void Update();
    void Render();

    // Timing
    void WaitForPreviousFrame();

    // Utilities
    void SetBackBufferOM(ComPtr<ID3D12GraphicsCommandList>& InCmdList) const;

private:
    // Setup Helpers
    bool SetupDevice();
    bool SetupSwapChain();
    bool SetupMeshRootSignature();

    // Frame Stages
    // From https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12Multithreading/src/D3D12Multithreading.cpp
    void BeginFrame();
    void MidFrame();
    void EndFrame();


public:
    // Pipelines
    std::unique_ptr<class StaticMeshPipeline> SMPipe;

    // Renderer Properties
    bool VSyncEnabled = true;
    
    // Window and Viewport Properties
    UINT Width = 800;
    UINT Height = 600;
    float NearPlane = 0.01f; // Can't be zero for depth buffer.
    float FarPlane = 100.0f;

    float FieldOfView = 45.0;
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

    float MaxDepth = 1.0f;
    
    // Render State
    bool bDXReady = false;

    // DX resources.
    ComPtr<ID3D12Debug> DebugController;
    ComPtr<IDXGIDebug1> DebugDXGI;
    ComPtr<ID3D12Device7> Device;
    ComPtr<IDXGIFactory7> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    ComPtr<ID3D12CommandQueue> CmdQueue;
    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<IDXGISwapChain4> SwapChain;
    ComPtr<ID3D12RootSignature> RootSig;
    D3D12_VIEWPORT Viewport;

    // Frame Cmd Lists
    ComPtr<ID3D12GraphicsCommandList> CmdListBeginFrame;
    ComPtr<ID3D12GraphicsCommandList> CmdListMidFrame;
    ComPtr<ID3D12GraphicsCommandList> CmdListEndFrame;

    std::vector<ID3D12CommandList*> Cmds; // The list in order of rendering commands. 
    
    // Frame Buffers
    std::vector<ComPtr<ID3D12Resource>> FrameBuffers;
    UINT CurrentBackBuffer = 0;
    DXGI_FORMAT FrameBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    ComPtr<ID3D12DescriptorHeap> FrameBufferHeap;
    UINT RtvHeapOffsetSize = 0;
    UINT FrameBufferCount = 2; // Currently only two buffers
    // And Depth/Stencil buffer
    ComPtr<ID3D12Resource> DepthBuffer; 
    ComPtr<ID3D12DescriptorHeap> DepthBufferHeap;
    DXGI_FORMAT DepthSampleFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; 

    // Synchronisation
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue = 0;
    HANDLE FenceEvent;
    
    // World Constants
    CB_WVP WVP; // World View Projection buffer.

};
