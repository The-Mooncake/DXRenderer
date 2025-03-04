
// Windows
#include <Windows.h>
#include <tchar.h>
#include <shellapi.h>

// D3D
#include <d3d12.h>

// Renderer
#include "MainWindow.h"

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, Windows desktop!");

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOut(hdc,
           5, 5,
           greeting, _tcslen(greeting));
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;

}

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    static TCHAR szWindowClass[] = _T("DesktopApp");
    static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");
    
    // Window Info
    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    // Register Window
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
           _T("Call to RegisterClassEx failed!"),
           _T("Windows Desktop Guided Tour"),
           NULL);

        return 1;
    }

    // Create the base window
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
        NULL,
        NULL,
        hInstance,
       NULL
    );
    if (!hWnd)
    {
        MessageBox(NULL,
           _T("Call to CreateWindowEx failed!"),
           _T("Windows Desktop Guided Tour"),
           NULL);

        return 1;
    }

    // Show, its hidden by default.
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    bool bMsg = false;
    msg.message = WM_NULL;
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

    
    while (msg.message != WM_QUIT)
    {
        bool bMsg = (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0);

        if (bMsg)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Update renderer.
            
            
        }

    }

    return static_cast<int>(msg.wParam);
}

