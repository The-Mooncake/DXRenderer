#pragma once

// Common
#include <windows.h>
#include <wrl/client.h>
#include <stdlib.h>
#include <vector>

#include "pch.h"

// DX
#include <d3dcommon.h>
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>

using Microsoft::WRL::ComPtr; // Import only the ComPtr

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);
    ~MainWindow();

    int Run();
    static LRESULT CALLBACK WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE& GetHInstance() { return hInstance; }
    HWND& GetHWND() { return hWnd; }

    const double& GetTime() const { return Time; } 
    bool SetupWindow(const UINT& DefaultWidth, const UINT& DefaultHeight);

public:
    // App Global Classes, can be accessed through G_MainWindow
    std::unique_ptr<class Renderer> RendererDX;
    std::unique_ptr<class USDScene> Scene;
    
private:

    // Windows Ptrs
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;

    // Timing
    double Time = 0.0;
    float TimeStep = 0.01f;
    

};
