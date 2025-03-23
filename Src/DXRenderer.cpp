
// Windows
#include <Windows.h>

// Renderer
#include "MainWindow.h"

// USD
#include <pxr/usd/usd/stage.h>


// Force Nvidia GPU - e.g in laptop cases.
extern "C" { __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001; }

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateInMemory();
    


    MainWindow App = MainWindow(hInstance);
    const int ExitCode = App.Run();

    return ExitCode;
}

