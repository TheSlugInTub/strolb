#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <strolb/vector.h>
#include <strolb/window.h>

#define SLB_FRAMES_IN_FLIGHT 2
#define SLB_USE_VALIDATION_LAYERS true

typedef struct
{
    uint32_t  graphicsFamily;
    uint32_t  presentFamily;
    bool      isValid;
} slb_QueueFamilyIndices;

typedef struct
{
    VkSurfaceCapabilitiesKHR capabilities;
    slb_Vector*              formats;      // VkSurfaceFormatKHR
    slb_Vector*              presentModes; // VkPresentModeKHR
} slb_SwapchainDetails;

typedef VkInstance slb_Instance;

slb_Instance slb_Instance_Create(const char* applicationName);

typedef VkDebugUtilsMessengerEXT slb_DebugMessenger;

slb_DebugMessenger slb_DebugMessenger_Create(slb_Instance instance);

typedef VkSurfaceKHR slb_Surface;

slb_Surface slb_Surface_Create(slb_Instance instance, slb_Window* window);

typedef VkPhysicalDevice slb_PhysicalDevice;

slb_PhysicalDevice slb_PhysicalDevice_Create(slb_Instance instance, slb_Surface surface);

typedef struct
{
    VkDevice device;
    VkQueue  graphicsQueue;
    VkQueue  presentQueue;
} slb_Device;

slb_Device slb_Device_Create(slb_Instance instance, slb_PhysicalDevice physicalDevice, 
        slb_Surface surface);

typedef struct
{
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    imageView;
    VkSampler      sampler;
} slb_Image;

typedef struct
{
    VkSwapchainKHR swapchain;
    VkFormat       swapchainImageFormat;
    VkExtent2D     swapchainExtent;
    slb_Vector*    swapchainImages;       // VkImage
    slb_Vector*    swapchainImageViews;   // VkImageView
    slb_Vector*    swapchainFramebuffers; // VkFramebuffer
} slb_Swapchain;

slb_Swapchain slb_Swapchain_Create(slb_Window* window, slb_PhysicalDevice physicalDevice,
        slb_Surface surface, slb_Device* device, slb_Image* depthImage);

typedef VkRenderPass slb_RenderPass;

void slb_Swapchain_CreateFramebuffers(slb_Swapchain* swapchain, slb_Device* device,
        slb_RenderPass renderPass,
        slb_Image* depthImage);

typedef struct
{
    VkCommandPool   commandPool;
    VkCommandBuffer commandBuffers[SLB_FRAMES_IN_FLIGHT];
} slb_CommandPool;

typedef VkDescriptorSetLayout slb_DescriptorSetLayout;

slb_DescriptorSetLayout slb_DescriptorSetLayout_Create(
    VkDescriptorSetLayoutBinding* bindings,
    uint16_t bindingCount, slb_Device* device);

typedef struct
{
    VkBuffer buffer;
    VkDeviceMemory memory;
} slb_Buffer;

typedef struct
{
    VkDescriptorSet descriptorSets[SLB_FRAMES_IN_FLIGHT];
    slb_Buffer      buffers[SLB_FRAMES_IN_FLIGHT];
    void*           buffersMap[SLB_FRAMES_IN_FLIGHT];
} slb_DescriptorSet;

typedef VkRenderPass slb_RenderPass;

slb_RenderPass slb_RenderPass_Create(slb_Swapchain* swapchain, slb_Device* device);

typedef struct
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
} slb_Pipeline;

slb_Pipeline slb_Pipeline_Create(
    slb_Device* device, slb_Swapchain* swapchain,
    slb_RenderPass renderPass, const char* vsPath, const char* fsPath,
    VkVertexInputBindingDescription*   bindingDescription,
    VkVertexInputAttributeDescription* attributeDescriptions,
    uint32_t                           attributeDescriptionCount,
    slb_DescriptorSetLayout* layouts, uint32_t layoutCount,
    VkPrimitiveTopology topology);

slb_CommandPool slb_CommandPool_Create(slb_PhysicalDevice physicalDevice, 
        slb_Device* device,
        slb_Surface surface);

typedef VkDescriptorPool slb_DescriptorPool;

slb_DescriptorPool slb_DescriptorPool_Create(VkDescriptorPoolSize* poolSizes, 
        int numPoolSizes, int maxSets, slb_Device* device);

typedef VkFence slb_Fence;
typedef VkSemaphore slb_Semaphore;

typedef enum
{
    slb_ErrorType_Error,
    slb_ErrorType_Warning,
    slb_ErrorType_Info,
} slb_ErrorType;

void slb_Error(const char* str, slb_ErrorType errorType);

uint32_t slb_FindMemoryType(slb_PhysicalDevice physicalDevice, uint32_t typeFilter,
                              VkMemoryPropertyFlags properties);

slb_Buffer slb_Buffer_Create(uint32_t size, VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, slb_PhysicalDevice physicalDevice,
        slb_Device* device);

void slb_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, 
        slb_Device* device, slb_CommandPool* commandPool);

slb_Image slb_Image_Create(slb_Device* device, 
        slb_PhysicalDevice physicalDevice, 
        uint32_t width,
        uint32_t height, VkFormat format,
        VkImageTiling         tiling,
        VkImageUsageFlags     usage,
        VkMemoryPropertyFlags properties);

VkImageView slb_ImageView_Create(slb_Device* device,
    VkImage image, VkFormat format,
    VkImageAspectFlags flags);

void slb_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, 
        VkImageLayout newLayout, slb_Device* device, slb_CommandPool* commandPool);

void slb_CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
        slb_Device* device, slb_CommandPool* commandPool);

typedef VkSampler slb_Sampler;

slb_Sampler slb_Sampler_Create(VkFilter magFilter, VkFilter minFilter, 
        bool anistoropyEnable, float maxAnisotropy, bool unnormalizedCoordinates, 
        bool compareEnable, VkCompareOp compareOp, VkSamplerMipmapMode mipmapMode,
        slb_Device* device);
