#pragma once

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

private:
    // UI Functions
    void WindowMenuBar();
    void ShowInfoOverlay();

    // UI Helpers
    const bool HasWindowFlag(UIWindowFlags Flag) const;
    
private:
    int WindowFlags = 0;
    
};
