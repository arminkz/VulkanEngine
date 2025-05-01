#include "GUI.h"


GUI::GUI(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;     // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL3_InitForVulkan(_ctx->window);

    // ImGui_ImplVulkan_InitInfo init_info = {};
    // init_info.Instance = _ctx->instance;
    // init_info.PhysicalDevice = _ctx->physicalDevice;
    // init_info.Device = _ctx->device;
    // init_info.QueueFamily = 0; // TODO: Get the correct queue family index
    // init_info.Queue = _ctx->graphicsQueue;



}


GUI::~GUI()
{
    // Cleanup
    // ImGui_ImplVulkan_Shutdown();
    // ImGui_ImplSDL3_Shutdown();
    // ImGui::DestroyContext();
    _ctx = nullptr;
}