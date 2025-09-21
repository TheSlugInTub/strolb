#include <strolb/vulkan.h>

slb_Buffer slb_Buffer_Create(uint32_t size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             slb_PhysicalDevice    physicalDevice,
                             slb_Device*           device)
{
    slb_Buffer buffer;

    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->device, &bufferInfo, NULL,
                       &buffer.buffer) != VK_SUCCESS)
    {
        slb_Error("Failed to create buffer", slb_ErrorType_Error);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device->device, buffer.buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = slb_FindMemoryType(
        physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, NULL,
                         &buffer.memory) != VK_SUCCESS)
    {
        slb_Error("Failed to allocate buffer memory",
                  slb_ErrorType_Error);
    }

    vkBindBufferMemory(device->device, buffer.buffer, buffer.memory,
                       0);

    return buffer;
}

void slb_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                    VkDeviceSize size, slb_Device* device,
                    slb_CommandPool* commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->device, &allocInfo,
                             &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {0};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1,
                    &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(device->graphicsQueue);

    vkFreeCommandBuffers(device->device, commandPool->commandPool, 1,
                         &commandBuffer);
}

static const char* validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"};

static const uint32_t validationLayerCount =
    sizeof(validationLayers) / sizeof(validationLayers[0]);

bool slb_CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties* availableLayers = (VkLayerProperties*)malloc(
        layerCount * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (uint32_t i = 0; i < validationLayerCount; i++)
    {
        bool layerFound = false;

        for (uint32_t j = 0; j < layerCount; j++)
        {
            if (strcmp(validationLayers[i],
                       availableLayers[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            free(availableLayers);
            return false;
        }
    }

    free(availableLayers);
    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL slb_DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{

    printf("Vulkan validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

void slb_Error(const char* message, slb_ErrorType type)
{
    printf("SLB ERROR: \n%s\n\n", message);
}

void slb_PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT* createInfo)
{
    createInfo->sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = slb_DebugCallback;
    createInfo->pUserData = NULL;
}

slb_Instance slb_Instance_Create(const char* icationName)
{
    slb_Instance instance;

    if (SLB_USE_VALIDATION_LAYERS &&
        !slb_CheckValidationLayerSupport())
    {
        slb_Error("Validation layers requested but not available",
                  slb_ErrorType_Warning);
    }

    // Send information about our ication to the Vulkan API
    VkApplicationInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    info.pApplicationName = icationName;
    info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    info.pEngineName = icationName;
    info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &info;

    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions;

    // Get required vulkan extensions from GLFW
    glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Create extensions array with space for other extensions
    uint32_t     extensionCount = glfwExtensionCount;
    const char** extensions = (const char**)malloc(
        (glfwExtensionCount + 1) * sizeof(const char*));

    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
        extensions[i] = glfwExtensions[i];
    }

    if (SLB_USE_VALIDATION_LAYERS)
    {
        // Enable the validation layer extension
        extensions[extensionCount++] =
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    if (SLB_USE_VALIDATION_LAYERS)
    {
        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;

        slb_PopulateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS)
    {
        slb_Error("Failed to create Vulkan instance",
                  slb_ErrorType_Error);
    }

    free(extensions);

    return instance;
}

slb_Surface slb_Surface_Create(slb_Instance instance,
                               slb_Window*  window)
{
    slb_Surface surface;

    if (glfwCreateWindowSurface(instance, window->window, NULL,
                                &surface) != VK_SUCCESS)
    {
        slb_Error("Failed to create window surface",
                  slb_ErrorType_Error);
    }

    return surface;
}

VkResult slb_CreateDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks*              pAllocator,
    VkDebugUtilsMessengerEXT*                 pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL)
    {
        return func(instance, pCreateInfo, pAllocator,
                    pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

slb_DebugMessenger slb_DebugMessenger_Create(slb_Instance instance)
{
    slb_DebugMessenger messenger;

    if (SLB_USE_VALIDATION_LAYERS)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo =
            {0};
        slb_PopulateDebugMessengerCreateInfo(
            &debugMessengerCreateInfo);

        if (slb_CreateDebugUtilsMessengerEXT(
                instance, &debugMessengerCreateInfo, NULL,
                &messenger) != VK_SUCCESS)
        {
            slb_Error("Failed to set up debug messenger",
                      slb_ErrorType_Warning);
        }
    }

    return messenger;
}

slb_QueueFamilyIndices
slb_FindQueueFamilies(slb_PhysicalDevice device, slb_Surface surface)
{
    slb_QueueFamilyIndices indices = {0};

    indices.isValid = false;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device,
                                             &queueFamilyCount, NULL);

    slb_Vector* queueFamilies = slb_Vector_Create(
        sizeof(VkQueueFamilyProperties), queueFamilyCount);
    queueFamilies->size = queueFamilyCount;

    vkGetPhysicalDeviceQueueFamilyProperties(
        device, &queueFamilyCount,
        (VkQueueFamilyProperties*)queueFamilies->data);

    for (int i = 0; i < queueFamilies->size; i++)
    {
        VkQueueFamilyProperties* prop =
            (VkQueueFamilyProperties*)slb_Vector_Get(queueFamilies,
                                                     i);

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);

        if ((prop->queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            presentSupport)
        {
            indices.graphicsFamily = i;
            indices.presentFamily = i;
            indices.isValid = true;
        }
    }

    return indices;
}

bool slb_CheckExtensionsSupported(slb_PhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL,
                                         &extensionCount, NULL);

    VkExtensionProperties* availableExtensions =
        (VkExtensionProperties*)malloc(extensionCount *
                                       sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(
        device, NULL, &extensionCount, availableExtensions);

    const char* requiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    uint32_t requiredExtensionCount = 1;

    for (uint32_t i = 0; i < requiredExtensionCount; i++)
    {
        bool found = false;
        for (uint32_t j = 0; j < extensionCount; j++)
        {
            if (strcmp(requiredExtensions[i],
                       availableExtensions[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            free(availableExtensions);
            return false;
        }
    }

    free(availableExtensions);
    return true;
}

bool slb_IsDeviceSuitable(slb_PhysicalDevice device,
                          slb_Surface        surface)
{
    slb_QueueFamilyIndices indices =
        slb_FindQueueFamilies(device, surface);
    return indices.isValid && slb_CheckExtensionsSupported(device);
}

slb_PhysicalDevice slb_PhysicalDevice_Create(slb_Instance instance,
                                             slb_Surface  surface)
{
    slb_PhysicalDevice physicalDevice;

    uint32_t deviceCount = 0;

    // First call: get the number of devices
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);

    if (deviceCount == 0)
    {
        slb_Error("No physical device found", slb_ErrorType_Error);
    }

    // Create vector with the correct size
    slb_Vector* physicalDevices =
        slb_Vector_Create(sizeof(VkPhysicalDevice), deviceCount);

    // Second call: get the actual devices
    vkEnumeratePhysicalDevices(
        instance, &deviceCount,
        (VkPhysicalDevice*)physicalDevices->data);

    // Update the vector's size to reflect the actual number of
    // devices
    physicalDevices->size = deviceCount;

    for (int i = 0; i < physicalDevices->size; i++)
    {
        VkPhysicalDevice* devicePtr =
            (VkPhysicalDevice*)slb_Vector_Get(physicalDevices, i);
        VkPhysicalDevice device = *devicePtr;

        if (slb_IsDeviceSuitable(device, surface))
        {
            physicalDevice = device;
            break; // Found a suitable device, no need to continue
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        slb_Error("Failed to find a suitable physical device",
                  slb_ErrorType_Error);
    }

    return physicalDevice;
}

slb_Device slb_Device_Create(slb_Instance       instance,
                             slb_PhysicalDevice physicalDevice,
                             slb_Surface        surface)
{
    slb_Device device;

    slb_QueueFamilyIndices indices =
        slb_FindQueueFamilies(physicalDevice, surface);

    // Create queue create infos for unique queue families
    slb_Vector* queueCreateInfos =
        slb_Vector_Create(sizeof(VkDeviceQueueCreateInfo), 2);

    uint32_t uniqueQueueFamilies[2];
    int      uniqueCount = 0;

    // Add graphics family
    uniqueQueueFamilies[uniqueCount++] = indices.graphicsFamily;

    // Add present family only if it's different from graphics family
    if (indices.presentFamily != indices.graphicsFamily)
    {
        uniqueQueueFamilies[uniqueCount++] = indices.presentFamily;
    }

    float queuePriority = 1.0f;
    for (int i = 0; i < uniqueCount; i++)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {0};
        queueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        slb_Vector_PushBack(queueCreateInfos, &queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {0};

    // Define required device extensions
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME};

    deviceFeatures.wideLines = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos =
        (VkDeviceQueueCreateInfo*)queueCreateInfos->data;
    deviceCreateInfo.queueCreateInfoCount =
        (uint32_t)queueCreateInfos->size;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // Enable swapchain extension
    deviceCreateInfo.enabledExtensionCount = 4;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (SLB_USE_VALIDATION_LAYERS)
    {
        deviceCreateInfo.enabledLayerCount = validationLayerCount;
        deviceCreateInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL,
                       &device.device) != VK_SUCCESS)
    {
        printf("SK ERROR: Failed to create logical device.");
    }

    // Get the queue handles
    vkGetDeviceQueue(device.device, indices.graphicsFamily, 0,
                     &device.graphicsQueue);
    vkGetDeviceQueue(device.device, indices.presentFamily, 0,
                     &device.presentQueue);

    slb_Vector_Free(queueCreateInfos);

    return device;
}

slb_SwapchainDetails
slb_QuerySwapchainSupport(slb_PhysicalDevice physicalDevice,
                          slb_Surface        surface)
{
    slb_SwapchainDetails details = {0};

    details.formats =
        slb_Vector_Create(sizeof(VkSurfaceFormatKHR), 1);
    details.presentModes =
        slb_Vector_Create(sizeof(VkPresentModeKHR), 1);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                         &formatCount, NULL);

    if (formatCount != 0)
    {
        slb_Vector_Resize(details.formats, formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &formatCount,
            (VkSurfaceFormatKHR*)details.formats->data);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, NULL);

    if (presentModeCount != 0)
    {
        slb_Vector_Resize(details.presentModes, presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &presentModeCount,
            (VkPresentModeKHR*)details.presentModes->data);
    }

    return details;
}

VkSurfaceFormatKHR
slb_ChooseSwapSurfaceFormat(slb_Vector* availableFormats)
{
    for (int i = 0; i < availableFormats->size; i++)
    {
        VkSurfaceFormatKHR* formatPtr =
            (VkSurfaceFormatKHR*)slb_Vector_Get(availableFormats, i);
        VkSurfaceFormatKHR availableFormat = *formatPtr;

        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return *(
        (VkSurfaceFormatKHR*)slb_Vector_Get(availableFormats, 0));
}

VkPresentModeKHR
slb_ChooseSwapPresentMode(slb_Vector* availablePresentModes)
{
    for (int i = 0; i < availablePresentModes->size; i++)
    {
        VkPresentModeKHR* formatPtr =
            (VkPresentModeKHR*)slb_Vector_Get(availablePresentModes,
                                              i);
        VkPresentModeKHR availableMode = *formatPtr;

        if (availableMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            return availableMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t slb_ClampU32(uint32_t val, uint32_t low, uint32_t high)
{
    if (val < low)
    {
        return low;
    }
    else if (val > high)
    {
        return high;
    }

    return val;
}

VkExtent2D
slb_ChooseSwapExtent(VkSurfaceCapabilitiesKHR* capabilities,
                     slb_Window*               window)
{
    if (capabilities->currentExtent.width != UINT32_MAX)
    {
        return capabilities->currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window->window, &width, &height);

        VkExtent2D actualExtent = {(uint32_t)(width),
                                   (uint32_t)(height)};
        actualExtent.width = slb_ClampU32(
            actualExtent.width, capabilities->minImageExtent.width,
            capabilities->maxImageExtent.width);
        actualExtent.height = slb_ClampU32(
            actualExtent.height, capabilities->minImageExtent.height,
            capabilities->maxImageExtent.height);

        return actualExtent;
    }
}

slb_Swapchain slb_Swapchain_Create(slb_Window*        window,
                                   slb_PhysicalDevice physicalDevice,
                                   slb_Surface        surface,
                                   slb_Device*        device,
                                   slb_Image*         depthImage)
{
    slb_Swapchain swapchain;

    slb_SwapchainDetails details =
        slb_QuerySwapchainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR format =
        slb_ChooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR mode =
        slb_ChooseSwapPresentMode(details.presentModes);
    VkExtent2D extent =
        slb_ChooseSwapExtent(&details.capabilities, window);

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 &&
        imageCount > details.capabilities.maxImageCount)
    {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    slb_QueueFamilyIndices indices =
        slb_FindQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily,
                                     indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;  // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device->device, &createInfo, NULL,
                             &swapchain.swapchain) != VK_SUCCESS)
    {
        slb_Error("Failed to create swapchain", slb_ErrorType_Error);
    }

    swapchain.swapchainImageFormat = format.format;
    swapchain.swapchainExtent = extent;

    swapchain.swapchainImages = slb_Vector_Create(sizeof(VkImage), 3);

    vkGetSwapchainImagesKHR(device->device, swapchain.swapchain,
                            &imageCount, NULL);
    slb_Vector_Resize(swapchain.swapchainImages, imageCount);
    vkGetSwapchainImagesKHR(
        device->device, swapchain.swapchain, &imageCount,
        (VkImage*)swapchain.swapchainImages->data);

    swapchain.swapchainImageViews = slb_Vector_Create(
        sizeof(VkImageView), swapchain.swapchainImages->size);

    VkImageView imageView = {0};

    for (int i = 0; i < swapchain.swapchainImages->size; i++)
    {
        slb_Vector_PushBack(swapchain.swapchainImageViews,
                            &imageView);

        VkImage* swapchainImage =
            (VkImage*)slb_Vector_Get(swapchain.swapchainImages, i);

        VkImageView* swapchainImageView =
            (VkImageView*)slb_Vector_Get(
                swapchain.swapchainImageViews, i);

        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = *swapchainImage;

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain.swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device->device, &createInfo, NULL,
                              swapchainImageView) != VK_SUCCESS)
        {
            slb_Error("Failed to create image views",
                      slb_ErrorType_Error);
        }
    }

    return swapchain;
}

void slb_Swapchain_CreateFramebuffers(slb_Swapchain* swapchain,
                                      slb_Device*    device,
                                      slb_RenderPass renderPass,
                                      slb_Image*     depthImage)
{
    swapchain->swapchainFramebuffers =
        slb_Vector_Create(sizeof(VkFramebuffer), 1);

    for (size_t i = 0; i < swapchain->swapchainImageViews->size; i++)
    {
        VkFramebuffer framebuf = {0};

        VkImageView attachments[2] = {0};
        attachments[0] = *(VkImageView*)slb_Vector_Get(
            swapchain->swapchainImageViews, i);
        attachments[1] = depthImage->imageView;

        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType =
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchain->swapchainExtent.width;
        framebufferInfo.height = swapchain->swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->device, &framebufferInfo,
                                NULL, &framebuf) != VK_SUCCESS)
        {
            slb_Error("Failed to create framebuffer",
                      slb_ErrorType_Error);
        }

        slb_Vector_PushBack(swapchain->swapchainFramebuffers,
                            &framebuf);
    }
}

slb_RenderPass slb_RenderPass_Create(slb_Swapchain* swapchain,
                                     slb_Device*    device)
{
    slb_RenderPass renderPass;

    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = swapchain->swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {0};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {0};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[2] = {colorAttachment,
                                              depthAttachment};

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device->device, &renderPassInfo, NULL,
                           &renderPass) != VK_SUCCESS)
    {
        slb_Error("Failed to create render pass",
                  slb_ErrorType_Error);
    }

    return renderPass;
}

slb_DescriptorSetLayout
slb_DescriptorSetLayout_Create(VkDescriptorSetLayoutBinding* bindings,
                               uint16_t    bindingCount,
                               slb_Device* device)
{
    slb_DescriptorSetLayout layout;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
    layoutInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindingCount;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(device->device, &layoutInfo, NULL,
                                    &layout) != VK_SUCCESS)
    {
        slb_Error("Failed to create descriptor set layout",
                  slb_ErrorType_Error);
    }

    return layout;
}

char* skReadFile(const char* filePath, uint32_t* len)
{
    FILE* shaderStream = fopen(filePath, "rb");
    if (shaderStream == NULL)
    {
        slb_Error("Failed to open file", slb_ErrorType_Warning);
    }

    fseek(shaderStream, 0, SEEK_END);
    size_t length = ftell(shaderStream);
    fseek(shaderStream, 0, SEEK_SET);

    char* shaderCode = (char*)malloc(length);
    fread(shaderCode, sizeof(char), length, shaderStream);
    fclose(shaderStream);

    *len = length;
    return shaderCode;
}

VkShaderModule slb_CreateShaderModule(slb_Device* device,
                                      char* buffer, uint32_t len)
{
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = len;
    createInfo.pCode = (const uint32_t*)buffer;

    if (len % 4 != 0)
    {
        printf("SK ERROR: Shader code size must be multiple of 4 "
               "bytes for SPIR-V\n");
        return VK_NULL_HANDLE;
    }

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device->device, &createInfo, NULL,
                             &shaderModule) != VK_SUCCESS)
    {
        printf("SK ERROR: Failed to create shader module.");
    }

    return shaderModule;
}

slb_Pipeline slb_Pipeline_Create(
    slb_Device* device, slb_Swapchain* swapchain,
    slb_RenderPass renderPass, const char* vsPath, const char* fsPath,
    VkVertexInputBindingDescription*   bindingDescription,
    VkVertexInputAttributeDescription* attributeDescriptions,
    uint32_t                           attributeDescriptionCount,
    slb_DescriptorSetLayout* layouts, uint32_t layoutCount,
    VkPrimitiveTopology topology)
{
    slb_Pipeline pipeline;

    uint32_t vertLen, fragLen;
    char*    vertShaderCode = skReadFile(vsPath, &vertLen);
    char*    fragShaderCode = skReadFile(fsPath, &fragLen);

    VkShaderModule vertMod =
        slb_CreateShaderModule(device, vertShaderCode, vertLen);
    VkShaderModule fragMod =
        slb_CreateShaderModule(device, fragShaderCode, fragLen);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    vertShaderStageInfo.module = vertMod;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragMod;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount =
        attributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions =
        attributeDescriptions;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescription;

    VkDynamicState dynamicStates[3] = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR,
                                       VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 3;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain->swapchainExtent.width;
    viewport.height = (float)swapchain->swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D) {0, 0};
    scissor.extent = swapchain->swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor =
        VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layoutCount;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(device->device, &pipelineLayoutInfo,
                               NULL, &pipeline.layout) != VK_SUCCESS)
    {
        slb_Error("Failed to create pipeline layout",
                  slb_ErrorType_Error);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;

    pipelineInfo.layout = pipeline.layout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = (VkStencilOpState) {0};
    depthStencil.back = (VkStencilOpState) {0};

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1,
                                  &pipelineInfo, NULL,
                                  &pipeline.pipeline) != VK_SUCCESS)
    {
        slb_Error("Failed to create pipeline", slb_ErrorType_Error);
    }

    return pipeline;
}

uint32_t slb_FindMemoryType(slb_PhysicalDevice    physicalDevice,
                            uint32_t              typeFilter,
                            VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                        &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags &
             properties) == properties)
        {
            return i;
        }
    }

    slb_Error("Failed to find suitable memory type",
              slb_ErrorType_Warning);
    return -1;
}

slb_Image slb_Image_Create(slb_Device*        device,
                           slb_PhysicalDevice physicalDevice,
                           uint32_t width, uint32_t height,
                           VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags     usage,
                           VkMemoryPropertyFlags properties)
{
    slb_Image image;

    VkImageCreateInfo imageInfo = {0};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->device, &imageInfo, NULL,
                      &image.image) != VK_SUCCESS)
    {
        slb_Error("Failed to create image", slb_ErrorType_Warning);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device->device, image.image,
                                 &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = slb_FindMemoryType(
        physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, NULL,
                         &image.memory) != VK_SUCCESS)
    {
        slb_Error("Failed to allocate image memory",
                  slb_ErrorType_Warning);
    }

    vkBindImageMemory(device->device, image.image, image.memory, 0);

    return image;
}

VkImageView slb_ImageView_Create(slb_Device* device, VkImage image,
                                 VkFormat           format,
                                 VkImageAspectFlags flags)
{
    VkImageViewCreateInfo viewInfo = {0};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = flags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device->device, &viewInfo, NULL,
                          &imageView) != VK_SUCCESS)
    {
        slb_Error("Failed to create image view for image",
                  slb_ErrorType_Warning);
    }

    return imageView;
}

slb_CommandPool
slb_CommandPool_Create(slb_PhysicalDevice physicalDevice,
                       slb_Device* device, slb_Surface surface)
{
    slb_CommandPool commandPool;

    slb_QueueFamilyIndices indices =
        slb_FindQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily;

    if (vkCreateCommandPool(device->device, &poolInfo, NULL,
                            &commandPool.commandPool) != VK_SUCCESS)
    {
        slb_Error("Failed to create command pool",
                  slb_ErrorType_Error);
    }

    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = SLB_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(
            device->device, &allocInfo,
            (VkCommandBuffer*)commandPool.commandBuffers) !=
        VK_SUCCESS)
    {
        slb_Error("Failed to allocate command buffers",
                  slb_ErrorType_Error);
    }

    return commandPool;
}

slb_DescriptorPool
slb_DescriptorPool_Create(VkDescriptorPoolSize* poolSizes,
                          int numPoolSizes, int maxSets,
                          slb_Device* device)
{
    slb_DescriptorPool pool;

    VkDescriptorPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = numPoolSizes;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = maxSets;

    if (vkCreateDescriptorPool(device->device, &poolInfo, NULL,
                               &pool) != VK_SUCCESS)
    {
        slb_Error("Failed to create descriptor pool",
                  slb_ErrorType_Error);
    }

    return pool;
}

void slb_TransitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout    oldLayout,
                               VkImageLayout    newLayout,
                               slb_Device*      device,
                               slb_CommandPool* commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->device, &allocInfo,
                             &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {0};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        slb_Error("Unsupported layout transition",
                  slb_ErrorType_Error);
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
                         0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(device->graphicsQueue);

    vkFreeCommandBuffers(device->device, commandPool->commandPool, 1,
                         &commandBuffer);
}

void slb_CopyBufferToImage(VkBuffer buffer, VkImage image,
                           uint32_t width, uint32_t height,
                           slb_Device*      device,
                           slb_CommandPool* commandPool)
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->device, &allocInfo,
                             &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D) {0, 0, 0};
    region.imageExtent = (VkExtent3D) {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(device->graphicsQueue);

    vkFreeCommandBuffers(device->device, commandPool->commandPool, 1,
                         &commandBuffer);
}

slb_Sampler
slb_Sampler_Create(VkFilter magFilter, VkFilter minFilter,
                   bool anistoropyEnable, float maxAnisotropy,
                   bool unnormalizedCoordinates, bool compareEnable,
                   VkCompareOp         compareOp,
                   VkSamplerMipmapMode mipmapMode, slb_Device* device)
{
    slb_Sampler sampler;

    VkSamplerCreateInfo samplerInfo = {0};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = anistoropyEnable;
    samplerInfo.maxAnisotropy = maxAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates;
    samplerInfo.compareEnable = compareEnable;
    samplerInfo.compareOp = compareOp;
    samplerInfo.mipmapMode = mipmapMode;

    if (vkCreateSampler(device->device, &samplerInfo, NULL,
                        &sampler) != VK_SUCCESS)
    {
        slb_Error("Failed to create texture sampler",
                  slb_ErrorType_Error);
    }

    return sampler;
}
