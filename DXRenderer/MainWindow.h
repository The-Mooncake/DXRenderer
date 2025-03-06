#pragma once

#include <windows.h>
#include <wrl/client.h>
#include <stdlib.h>
#include <vector>

// DX
#include <d3dcommon.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// Global ptr to the main window instance.
static class MainWindow* G_MainWindow = nullptr;

using Microsoft::WRL::ComPtr; // Import only the ComPtr


// ConstBuffer
struct CB_WVP
{
    DirectX::XMFLOAT4X4 ProjectionMatrix;
    DirectX::XMFLOAT4X4 ModelMatrix;
    DirectX::XMFLOAT4X4 ViewMatrix;
};

struct Vertex
{
    float Position[3];
    float Colour[3];
};

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);

    int Run();
    static LRESULT CALLBACK WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // Main Setup Steps
    void SetupDevice();
    void SetupWindow();
    void SetupSwapChain();
    void SetupMeshPipeline();

    void MeshConstantBuffer();
    void MeshVertexBuffer();
    void MeshIndexBuffer();
    
    // Main render loop.
    void UpdateRender();
    void Render();

    // Setup Helpers
    D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateMeshPipelineDesc();
    void SetupRootSignature();

    
    
    // Tri Data...
    uint32_t TriIndexBufferData[3] = {0, 1, 2};
    Vertex VertexBufferData[3] = {{{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                                  {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                  {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    
    // Render State
    bool bDXReady = false;

    // DX resources.
    ComPtr<IDXGIFactory7> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    ComPtr<ID3D12Device7> Device;
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
    ComPtr<ID3D12DescriptorHeap> FrameBufferHeap;
    UINT RtvHeapOffsetSize = 0;
    UINT FrameBufferCount = 2; // Currently only two buffers
    std::vector<ComPtr<ID3D12Resource>> FrameBuffers;
    UINT CurrentBackBuffer = 0;

    // Synchronisation
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue;
    HANDLE FenceEvent;

    // Mesh Buffers
    CB_WVP WVP; // World View Projection buffer.
    D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
    ID3D12DescriptorHeap* ConstantBufferHeap;
    
    // Windows Ptrs
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
};
