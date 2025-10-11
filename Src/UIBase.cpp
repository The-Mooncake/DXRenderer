#include "UIBase.h"
#include "imgui.h"
#include "ImGuiDescHeap.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "MainWindow.h"
#include "Renderer.h"
#include "pch.h"

UIBase::UIBase()
{
    WindowFlags |= static_cast<int>(UIWindowFlags::Overlay);
}

bool UIBase::InitImgui()
{
    IMGUI_CHECKVERSION();
    
    ImGui_ImplWin32_EnableDpiAwareness();
    DpiScaling = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking support.
    
    ImGui::StyleColorsDark();
    
    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(DpiScaling * ImguiUIScaling); // Only for scaling ui not window mappings.
    
    // Setup Platform/Renderer backends
    std::unique_ptr<Renderer>& R = G_MainWindow->RendererDX;
    
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = R.get()->Device.Get();
    init_info.CommandQueue = R.get()->CmdQueue.Get();
    init_info.NumFramesInFlight = 1;
    init_info.RTVFormat = R.get()->FrameBufferFormat; 
    init_info.SrvDescriptorHeap = R.get()->ImguiSrvBufferHeap.Get();

    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return ImguiHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return ImguiHeapAlloc.Free(cpu_handle, gpu_handle); };
    if (!ImGui_ImplWin32_Init(G_MainWindow->GetHWND()))
    {
        MessageBoxW(nullptr, L"Failed to initialise ImGui Win32!", L"Error", MB_OK);
        PostQuitMessage(1);
        return false;
    }
    if (!ImGui_ImplDX12_Init(&init_info))
    {
        MessageBoxW(nullptr, L"Failed to Initialise ImGui DX12!", L"Error", MB_OK);
        PostQuitMessage(1);
        return false;
    }
    
    return true;
}

void UIBase::RenderUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();

    // Set the display size to the window size.
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(
        static_cast<float>(G_MainWindow->RendererDX->Width),
        static_cast<float>(G_MainWindow->RendererDX->Height) );

    ImGui::NewFrame();

    // Start drawing UI
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
    WindowMenuBar();
    
    if (HasWindowFlag(UIWindowFlags::Overlay)) { ShowInfoOverlay(); }
    if (HasWindowFlag(UIWindowFlags::DemoUI)) { ImGui::ShowDemoWindow(); }

}

void UIBase::WindowMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // Open work here...
            }
            if (ImGui::MenuItem("Show Info Overlay", nullptr, HasWindowFlag(UIWindowFlags::Overlay)))
            {
                WindowFlags ^= static_cast<int>(UIWindowFlags::Overlay);
            }
            if (ImGui::MenuItem("Show Demo Window", nullptr, HasWindowFlag(UIWindowFlags::DemoUI)))
            {
                WindowFlags ^= static_cast<int>(UIWindowFlags::DemoUI);
            }
            if (ImGui::MenuItem("Exit")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UIBase::ShowInfoOverlay()
{
    bool OpenValue = true;
    bool* p_open = &OpenValue;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_size;

    static int location = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (location >= 0)
    {
        const float PAD = 10.0f;
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    else if (location == -2)
    {
        // Center window
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
    {
        ImGui::Text("Simple overlay\n" "(right-click to change position)");
        ImGui::Separator();
        ImGui::Text("Work Size: (%.1f,%.1f)", work_size.x, work_size.y);
        ImGui::Text("Dpi Scale: %.3f", viewport->DpiScale);
        if (ImGui::IsMousePosValid())
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Custom",       NULL, location == -1)) location = -1;
            if (ImGui::MenuItem("Center",       NULL, location == -2)) location = -2;
            if (ImGui::MenuItem("Top-left",     NULL, location == 0)) location = 0;
            if (ImGui::MenuItem("Top-right",    NULL, location == 1)) location = 1;
            if (ImGui::MenuItem("Bottom-left",  NULL, location == 2)) location = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, location == 3)) location = 3;
            //if (p_open && ImGui::MenuItem("Close")) *p_open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

const bool UIBase::HasWindowFlag(UIWindowFlags Flag) const
{
    const int FlagValue = static_cast<int>(Flag);
    return (WindowFlags & FlagValue) == FlagValue;
}

