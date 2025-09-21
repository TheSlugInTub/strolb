#pragma once

#include <cglm/cglm.h>
#include <stdarg.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void*                    slb_ImGuiTextureID;
typedef struct slb_ImGuiPayload_t* slb_ImGuiPayload;

void slb_ImGui_Init(struct GLFWwindow* window, VkInstance instance, 
        VkDescriptorPool descriptorPool,
        VkRenderPass renderPass, VkPhysicalDevice physicalDevice, VkDevice device,
        VkCommandPool pool, VkQueue graphicsQueue);
void slb_ImGui_NewFrame();
void slb_ImGui_EndFrame(VkCommandBuffer commandBuffer);
void slb_ImGui_Terminate();
void slb_ImGui_Theme1();

bool slb_ImGui_Begin(const char* name);
bool slb_ImGui_BeginFlag(const char* name, bool* open);
void slb_ImGui_End();

void slb_ImGui_DebugWindow();
void slb_ImGui_DemoWindow();

bool slb_ImGui_DragFloat(const char* name, float* val, float speed);
bool slb_ImGui_DragFloat2(const char* name, float* val, float speed);
bool slb_ImGui_DragFloat3(const char* name, float* val, float speed);
bool slb_ImGui_DragFloat4(const char* name, float* val, float speed);
bool slb_ImGui_DragFloat16(const char* name, float* val, float speed);

bool slb_ImGui_InputInt(const char* name, int* val);
bool slb_ImGui_InputHex(const char* name, unsigned int* val);

bool slb_ImGui_ComboBox(const char* name, const char** types,
                      int* currentType, int typeSize);

bool slb_ImGui_Checkbox(const char* name, bool* val);
bool slb_ImGui_SliderInt(const char* name, int* currentType, int min,
                       int max);
bool slb_ImGui_Button(const char* name);
bool slb_ImGui_ImageButton(slb_ImGuiTextureID tex, vec2 size);
bool slb_ImGui_InputText(const char* name, char* buffer, size_t size,
                       int flags);
bool slb_ImGui_InputTextMultiline(const char* name, char* buffer, size_t size,
                       int flags);

bool slb_ImGui_ColorEdit4(const char* name, float* val);

bool slb_ImGui_IsWindowHovered();
bool slb_ImGui_CollapsingHeader(const char* name);
bool slb_ImGui_ColorPicker(const char* name, vec4 color);
void slb_ImGui_TextLong(const char* val);
void slb_ImGui_Text(const char* val);

bool slb_ImGui_MenuItem(const char* name);
void slb_ImGui_PushID(int id);
void slb_ImGui_PopID();
bool slb_ImGui_Selectable(const char* name, bool selected);
bool slb_ImGui_BeginDragDropSource(int flags);
void slb_ImGui_SetDragDropPayload(const char* name, const void* data,
                                size_t size);
void slb_ImGui_EndDragDropSource();

bool slb_ImGui_BeginDragDropTarget();
void slb_ImGui_EndDragDropTarget();

void slb_ImGui_Separator();

slb_ImGuiPayload slb_ImGui_AcceptDragDropPayload(const char* name);
void*          slb_ImGuiPayload_GetData(slb_ImGuiPayload payload);
int            slb_ImGuiPayload_GetDataSize(slb_ImGuiPayload payload);

bool slb_ImGui_BeginPopupContextWindow();
void slb_ImGui_EndPopup();

bool slb_ImGui_BeginMainMenuBar();
bool slb_ImGui_BeginMenu(const char* name);
bool slb_ImGui_MenuItemShortcut(const char* name, 
        const char* shortcut);
void slb_ImGui_EndMenu();
void slb_ImGui_EndMainMenuBar();

bool slb_ImGui_IsHovering();

#ifdef __cplusplus
}
#endif
