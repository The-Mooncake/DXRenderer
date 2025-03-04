
// Windows
#include <Windows.h>

// Renderer
#include "MainWindow.h"

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    MainWindow App = MainWindow(hInstance);
    const int ExitCode = App.Run();

    return ExitCode;
}

