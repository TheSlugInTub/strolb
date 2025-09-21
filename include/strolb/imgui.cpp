#include <strolb/imgui.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

#define SLB_FRAMES_IN_FLIGHT (2)

struct slb_ImGuiPayload_t
{
    const ImGuiPayload* payload;
};

ImFont* mainfont = nullptr;

extern "C"
{

void slb_ImGui_Init(struct GLFWwindow* window, VkInstance instance,
                  VkDescriptorPool descriptorPool,
                  VkRenderPass     renderPass,
                  VkPhysicalDevice physicalDevice, VkDevice device,
                  VkCommandPool pool, VkQueue graphicsQueue)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo info = {0};
    info.Instance = instance;
    info.DescriptorPool = descriptorPool;
    info.Device = device;
    info.RenderPass = renderPass;
    info.PhysicalDevice = physicalDevice;
    info.ImageCount = SLB_FRAMES_IN_FLIGHT;
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.Subpass = 0;
    info.QueueFamily =
        ImGui_ImplVulkanH_SelectQueueFamilyIndex(physicalDevice);
    info.Queue = graphicsQueue;
    info.MinImageCount = 2;

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_0,
        [](const char* function_name,
           void*       user_data) -> PFN_vkVoidFunction
        {
            return vkGetInstanceProcAddr(*(VkInstance*)user_data,
                                         function_name);
        },
        &instance);

    ImGui_ImplVulkan_Init(&info);
}

void slb_ImGui_NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    float menu_bar_height = ImGui::GetFrameHeight();
    
    // Adjust position and size to account for menu bar
    ImVec2 pos = viewport->Pos;
    pos.y += menu_bar_height;
    
    ImVec2 size = viewport->Size;
    size.y -= menu_bar_height;
    
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;

    ImGuiDockNodeFlags dockspace_flags =
        ImGuiDockNodeFlags_PassthruCentralNode;

    // Begin dockspace
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                     dockspace_flags);

    ImGui::End();
}

void slb_ImGui_EndFrame(VkCommandBuffer commandBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    commandBuffer);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void slb_ImGui_Terminate()
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void slb_ImGui_Theme1()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] =
        ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] =
        ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] =
        ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] =
        ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] =
        ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] =
        ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] =
        ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] =
        ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] =
        ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] =
        ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 0.71f);
    colors[ImGuiCol_HeaderHovered] =
        ImVec4(0.34f, 0.34f, 0.34f, 0.36f);
    colors[ImGuiCol_HeaderActive] =
        ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] =
        ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] =
        ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] =
        ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] =
        ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] =
        ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] =
        ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] =
        ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] =
        ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] =
        ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] =
        ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] =
        ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] =
        ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] =
        ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] =
        ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] =
        ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] =
        ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] =
        ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] =
        ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
}

bool slb_ImGui_Begin(const char* name)
{
    return ImGui::Begin(name);
}

bool slb_ImGui_BeginFlag(const char* name, bool* open)
{
    return ImGui::Begin(name, open);
}

void slb_ImGui_End()
{
    ImGui::End();
}

void slb_ImGui_DebugWindow()
{
    ImGui::ShowStyleEditor();
}

void slb_ImGui_DemoWindow()
{
    ImGui::ShowDemoWindow();
}

bool slb_ImGui_DragFloat(const char* name, float* val, float speed)
{
    return ImGui::DragFloat(name, val, speed);
}

bool slb_ImGui_DragFloat2(const char* name, float* val, float speed)
{
    return ImGui::DragFloat2(name, val, speed);
}

bool slb_ImGui_DragFloat3(const char* name, float* val, float speed)
{
    return ImGui::DragFloat3(name, val, speed);
}

bool slb_ImGui_DragFloat4(const char* name, float* val, float speed)
{
    return ImGui::DragFloat4(name, val, speed);
}

bool slb_ImGui_DragFloat16(const char* name, float* val, float speed)
{
    bool valueChanged = false;

    if (ImGui::TreeNode(name))
    {
        valueChanged |= ImGui::DragFloat4("##0", &val[0], speed);
        valueChanged |= ImGui::DragFloat4("##1", &val[4], speed);
        valueChanged |= ImGui::DragFloat4("##2", &val[8], speed);
        valueChanged |= ImGui::DragFloat4("##3", &val[12], speed);
        ImGui::TreePop();
    }

    return valueChanged;
}

bool slb_ImGui_InputInt(const char* name, int* val)
{
    return ImGui::InputInt(name, val);
}

bool slb_ImGui_InputHex(const char* name, unsigned int* val)
{
    return ImGui::InputScalar(name, ImGuiDataType_U32, val, NULL,
                              NULL, "%08X",
                              ImGuiInputTextFlags_CharsHexadecimal);
}

bool slb_ImGui_ComboBox(const char* name, const char** types,
                      int* currentType, int typeSize)
{
    return ImGui::Combo(name, currentType, types, typeSize);
}

bool slb_ImGui_Checkbox(const char* name, bool* val)
{
    return ImGui::Checkbox(name, val);
}

bool slb_ImGui_SliderInt(const char* name, int* currentType, int min,
                       int max)
{
    return ImGui::SliderInt(name, currentType, min, max);
}

bool slb_ImGui_Button(const char* name)
{
    return ImGui::Button(name);
}

// bool slb_ImGui_ImageButton(slb_ImGuiTextureID tex, vec2 size)
// {
//     return ImGui::ImageButton(tex, ImVec2(size[0], size[1]));
// }

bool slb_ImGui_InputText(const char* name, char* buffer, size_t size,
                       int flags)
{
    return ImGui::InputText(name, buffer, size, flags);
}

bool slb_ImGui_InputTextMultiline(const char* name, char* buffer,
                                size_t size, int flags)
{
    return ImGui::InputTextMultiline(
        name, buffer, size,
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
        ImGuiInputTextFlags_AllowTabInput);
}

bool slb_ImGui_ColorEdit4(const char* name, float* val)
{
    return ImGui::ColorEdit4(name, val);
}

bool slb_ImGui_IsWindowHovered()
{
    return ImGui::IsWindowHovered();
}

bool slb_ImGui_CollapsingHeader(const char* name)
{
    return ImGui::CollapsingHeader(name);
}

bool slb_ImGui_ColorPicker(const char* name, vec4 color)
{
    return ImGui::ColorPicker4(name, color);
}

void slb_ImGui_TextLong(const char* val)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    windowSize.y -= 30;
    ImGui::BeginChild("Documentation", windowSize, true);
    ImGui::Text("%s", val);
    ImGui::EndChild();
}

void slb_ImGui_Text(const char* val)
{
    ImGui::Text("%s", val);
}

bool slb_ImGui_MenuItem(const char* name)
{
    return ImGui::MenuItem(name);
}

void slb_ImGui_PushID(int id)
{
    ImGui::PushID(id);
}

void slb_ImGui_PopID()
{
    ImGui::PopID();
}

bool slb_ImGui_Selectable(const char* name, bool selected)
{
    return ImGui::Selectable(name, selected);
}

bool slb_ImGui_BeginDragDropSource(int flags)
{
    return ImGui::BeginDragDropSource(flags);
}

void slb_ImGui_SetDragDropPayload(const char* name, const void* data,
                                size_t size)
{
    ImGui::SetDragDropPayload(name, data, size);
}

void slb_ImGui_EndDragDropSource()
{
    ImGui::EndDragDropSource();
}

void slb_ImGui_EndDragDropTarget()
{
    ImGui::EndDragDropTarget();
}

bool slb_ImGui_BeginDragDropTarget()
{
    return ImGui::BeginDragDropTarget();
}

void slb_ImGui_Separator()
{
    ImGui::Separator();
}

slb_ImGuiPayload slb_ImGui_AcceptDragDropPayload(const char* name)
{
    return slb_ImGuiPayload(ImGui::AcceptDragDropPayload(name));
}

void* slb_ImGuiPayload_GetData(slb_ImGuiPayload payload)
{
    return payload->payload->Data;
}

int slb_ImGuiPayload_GetDataSize(slb_ImGuiPayload payload)
{
    return payload->payload->DataSize;
}

bool slb_ImGui_BeginPopupContextWindow()
{
    return ImGui::BeginPopupContextWindow();
}

void slb_ImGui_EndPopup()
{
    ImGui::EndPopup();
}

bool slb_ImGui_BeginMainMenuBar()
{
    return ImGui::BeginMainMenuBar();
}

bool slb_ImGui_BeginMenu(const char* name)
{
    return ImGui::BeginMenu(name);
}

bool slb_ImGui_MenuItemShortcut(const char* name, 
        const char* shortcut)
{
    return ImGui::MenuItem(name, shortcut);
}

void slb_ImGui_EndMenu()
{
    ImGui::EndMenu();
}

void slb_ImGui_EndMainMenuBar()
{
    ImGui::EndMainMenuBar();
}

bool slb_ImGui_IsHovering()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

}
