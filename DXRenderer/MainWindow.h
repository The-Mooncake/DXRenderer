#pragma once

#include <windows.h>
#include <wrl/client.h>
#include <stdlib.h>
#include <vector>

// DX
#include <d3dcommon.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// Global ptr to the main window instance.
static class MainWindow* G_MainWindow = nullptr;

using  Microsoft::WRL::ComPtr;

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);

    int Run();
    static LRESULT CALLBACK WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

    void SetupDevice();
    void SetupWindow();
    void SetupRendering();

    void Render();
    
    bool bDXReady = false;

    // DX resources.
    ComPtr<IDXGIFactory7> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    ComPtr<ID3D12Device7> Device;
    ComPtr<ID3D12CommandQueue> CmdQueue;
    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<IDXGISwapChain4> SwapChain;
    ComPtr<ID3D12PipelineState> PipelineState;
    D3D12_VIEWPORT Viewport;
    
    ComPtr<ID3DBlob> VS;
    ComPtr<ID3DBlob> PS;
    
    
    // Buffers
    ComPtr<ID3D12DescriptorHeap> FrameBufferHeap;
    UINT RtvHeapOffsetSize = 0;
    UINT FrameBufferCount = 2; // Currently only two buffers
    std::vector<ComPtr<ID3D12Resource>> FrameBuffers;
    UINT CurrentBackBuffer = 0;

    // Synchronisation
    ComPtr<ID3D12Fence> Fence;
    UINT64 FenceValue;
    // FenceEnvent;


    // Windows Ptrs
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
};
