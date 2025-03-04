#pragma once

#include <windows.h>

// Global ptr to the main window instance.
//static class MainWindow* G_MainWindow = nullptr;

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);

    int Run();
    static LRESULT CALLBACK WinProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
};
