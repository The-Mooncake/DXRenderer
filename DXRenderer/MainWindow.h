#pragma once

#include <windows.h>
#include <wrl/client.h>

// DX
#include <d3dcommon.h>
#include <d3d12.h>
#include <dxgi1_6.h>

// Global ptr to the main window instance.
//static class MainWindow* G_MainWindow = nullptr;

using  Microsoft::WRL::ComPtr;

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);

    int Run();
    static LRESULT CALLBACK WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

    void SetupDX();
    void SetupSwapChain();
    
    bool bDXReady = false;

    // DX resources.
    ComPtr<IDXGIFactory7> Factory;
    ComPtr<IDXGIAdapter1> Adapter;
    ComPtr<ID3D12Device7> Device;
    ComPtr<ID3D12CommandQueue> CmdQueue;
    ComPtr<IDXGISwapChain1> SwapChain;

    // Buffers
    ComPtr<ID3D12DescriptorHeap> RtvHeap;
    UINT FrameCount = 2; // Currently only two buffers
    ComPtr<ID3D12Resource> BackBuffer;
    ComPtr<ID3D12Resource> FrontBuffer;
    
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
};
