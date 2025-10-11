#pragma once

#include "ImGuiDescHeap.h"

// ImGui rendering heap desc global.
inline ImguiDescHeapAllocator ImguiHeapAlloc;

enum class UIWindowFlags : int
{
    None            = 0,
    Overlay         = 1 << 0,
    DemoUI          = 1 << 1,
};

class UIBase
{
public:
    UIBase();
    
    void RenderUI();
    bool InitImgui();
    
    const float& GetDpiScale() const { return DpiScaling; } 
    
private:
    // UI Functions
    void WindowMenuBar();
    void ShowInfoOverlay();

    // UI Helpers
    const bool HasWindowFlag(UIWindowFlags Flag) const;
    
private:
    int WindowFlags = 0;
    
    // UI Scaling
    float DpiScaling = 1.0f;
    float ImguiUIScaling = 1.0f;
};
