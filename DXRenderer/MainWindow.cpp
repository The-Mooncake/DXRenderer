#include "MainWindow.h"

#include <cstdio>
#include <tchar.h>


// Default Message class, forwards to main app class.
LRESULT CALLBACK WndProcForward(_In_ HWND hWnd, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return  G_MainWindow->MessageLoop(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

MainWindow::MainWindow(HINSTANCE InHInstance)
{
    hInstance = InHInstance; 
}

LRESULT MainWindow::MessageLoop(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, DXRenderer!");
    
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

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
        
    }

    return 0;
}


int MainWindow::Run()
{
    static TCHAR szWindowClass[] = _T("DXRenderer");
    static TCHAR szTitle[] = _T("Win D3D Renderer - Mooncake");
    
    // Window Info
    WNDCLASSEX wcex;
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProcForward;
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
           _T("DXRenderer Failed to register window class!"),
           NULL);

        return 1;
    }
    
    // Create the base window
    hWnd = CreateWindowEx(
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
        printf("Error: %d\n", GetLastError());
        MessageBox(NULL,
           _T("Call to CreateWindowEx failed!"),
           _T("DXRenderer Failed to make a window!"),
           NULL);


        return 1;
    }
    
    // Show, its hidden by default.
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    // Message loop
    MSG msg;
    bool bMsg = false;
    msg.message = WM_NULL;
    PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE);
    
    while (msg.message != WM_QUIT)
    {
        bMsg = (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE) != 0);
    
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
