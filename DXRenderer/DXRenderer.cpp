
// Windows
#include <Windows.h>
#include <tchar.h>
#include <shellapi.h>

// D3D
#include <d3d12.h>

// Renderer
#include "MainWindow.h"

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    G_MainWindow = new MainWindow(hInstance);
    const int ExitCode = G_MainWindow->Run();

    delete G_MainWindow;
    G_MainWindow = nullptr;

    return ExitCode;
}

