#pragma once

#include <windows.h>

// Global ptr to the main window instance.
static class MainWindow* G_MainWindow = nullptr;

// Win message processor, forward to G_MainWindow.
LRESULT CALLBACK WndProcForward(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam);

class MainWindow
{
public:

    MainWindow(HINSTANCE InHInstance);

    LRESULT MessageLoop(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    int Run();

private:
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;
};
