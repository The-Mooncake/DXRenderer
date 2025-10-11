#pragma once

// Common
#include <windows.h>
#include <wrl/client.h>
#include <memory>

// DX
#include <d3dcommon.h>
#include <dxgidebug.h>

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
    bool SetupWindow();

public:
    // App Global Classes, can be accessed through G_MainWindow
    std::unique_ptr<class Renderer> RendererDX;
    std::unique_ptr<class USDScene> Scene;
    std::unique_ptr<class UIBase> UI;
    
private:
    // Windows Ptrs
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;

    // Timing
    double Time = 0.0;
    float TimeStep = 0.01f;
    

};
