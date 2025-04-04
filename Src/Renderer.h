#pragma once


// Common
#include <wrl/client.h>
#include <stdlib.h>
#include <vector>

#include "pch.h"

// DX
#include <d3dcommon.h>
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr; // Import only the ComPtr

// ConstBuffer
struct CB_WVP
{
    DirectX::XMMATRIX ModelMatrix = DirectX::XMMatrixIdentity(); // Model to World
    DirectX::XMMATRIX ViewMatrix = DirectX::XMMatrixIdentity(); // World to View / Camera 
    DirectX::XMMATRIX ProjectionMatrix = DirectX::XMMatrixIdentity(); // View to 2D Projection
};

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Colour;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();
    
    // Rendering Setup
    bool SetupRenderer();
    
    bool SetupDevice();
    bool SetupSwapChain();
    bool SetupMeshPipeline();
    
    // Main render loop.
    void Update();
    void Render();

    // Setup Helpers
    bool SetupRootSignature();
    bool CreateMeshPipeline();
    bool MeshConstantBuffer();
    bool MeshVertexBuffer();
    bool MeshIndexBuffer();

    // Timing
    void WaitForPreviousFrame();
    
    // Window and Viewport
    UINT Width = 800;
    UINT Height = 600;
    float NearPlane = 0.1f;
    float FarPlane = 100.0f;

    float FieldOfView = 45.0;
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    
    // Tri Data...
    uint32_t TriIndexBufferData[3] = {0, 1, 2};
    Vertex VertexBufferData[3] = {{{0.0f, 0.25f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                                  {{0.25f, -0.25f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                                  {{-0.25f, -0.25f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};
    
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
    ComPtr<ID3D12PipelineState> PipelineState;
    D3D12_VIEWPORT Viewport;

    // Shaders and object resources.
    ComPtr<ID3DBlob> VS;
    ComPtr<ID3DBlob> PS;
    
    // Frame Buffers
    std::vector<ComPtr<ID3D12Resource>> FrameBuffers;
    UINT CurrentBackBuffer = 0;
    DXGI_FORMAT FrameBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    ComPtr<ID3D12DescriptorHeap> FrameBufferHeap;
    UINT RtvHeapOffsetSize = 0;
    UINT FrameBufferCount = 2; // Currently only two buffers

    // Synchronisation
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue = 0;
    HANDLE FenceEvent;

    // Mesh Buffers
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    ComPtr<ID3D12Resource> VertexBuffer;
    ComPtr<ID3D12Resource> IndexBuffer;    
    ComPtr<ID3D12Resource> ConstantBuffer;
    ComPtr<ID3D12DescriptorHeap> ConstantBufferHeap;

    // World Constants
    CB_WVP WVP; // World View Projection buffer.

};
