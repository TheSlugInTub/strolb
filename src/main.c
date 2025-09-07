#include <stdio.h>
#include <strolb/vulkan.h>
#include <strolb/camera.h>
#include <strolb/input.h>
#include <stb/stb_image.h>
#include <cglm/cglm.h>

typedef struct
{
    vec3 pos;
    vec2 texCoord;
} Vertex;

typedef struct
{
    mat4 model;
    mat4 view;
    mat4 proj;
} UniformBufferObject;

static const Vertex vertices[] = {
    {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f}}, // Bottom left
    {{0.5f, 0.0f, -0.5f}, {1.0f, 1.0f}},  // Bottom right
    {{0.5f, 0.0f, 0.5f}, {1.0f, 0.0f}},   // Top right
    {{-0.5f, 0.0f, 0.5f}, {0.0f, 0.0f}}   // Top left
};

static const uint16_t indices[] = {
    0, 1, 2, // First triangle: bottom-left, bottom-right, top-right
    2, 3, 0  // Second triangle: top-right, top-left, bottom-left
};

float sensitivity = -0.01f;

void ControlCamera(slb_Camera* camera, slb_Window* window, float dt)
{
    float speed = 1.0f * dt;

    // Up/down movement (W/S)
    if (slb_Input_GetKey(window, SLB_KEY_W))
    {
        camera->position[2] += 1.0f * speed;
    }
    if (slb_Input_GetKey(window, SLB_KEY_S))
    {
        camera->position[2] -= 1.0f * speed;
    }

    // Strafe left/right (A/D)
    if (slb_Input_GetKey(window, SLB_KEY_A))
    {
        camera->position[0] += 1.0f * speed;
    }
    if (slb_Input_GetKey(window, SLB_KEY_D))
    {
        camera->position[0] -= 1.0f * speed;
    }
    
    // Forward and backwards
    if (slb_Input_GetKey(window, SLB_KEY_E))
    {
        camera->position[1] -= 1.0f * speed;
    }
    if (slb_Input_GetKey(window, SLB_KEY_Q))
    {
        camera->position[1] += 1.0f * speed;
    }
}

int main(int argc, char** argv)
{
    slb_Window window =
        slb_Window_Create("Slug's Window", 1920, 1080, false, true);

    // glfwSetInputMode(window.window, GLFW_CURSOR,
    //                  GLFW_CURSOR_DISABLED);

    slb_Camera camera = slb_Camera_Create((vec3) {0.0f, 5.0f, 1.0f},
                                          (vec3) {0.0f, 0.0f, 1.0f},
                                          0.0f, -90.0f, 80.0f);

    vec2 previousMousePosition = {0.0f, 0.0f};
    vec2 mousePosition = {0.0f, 0.0f};

    vec2 cursorPosition = {0.0f, 0.0f};

    slb_Instance instance = slb_Instance_Create("Slug's Application");
    slb_DebugMessenger debugMessenger =
        slb_DebugMessenger_Create(instance);
    slb_Surface surface = slb_Surface_Create(instance, &window);
    slb_PhysicalDevice physicalDevice =
        slb_PhysicalDevice_Create(instance, surface);
    slb_Device device =
        slb_Device_Create(instance, physicalDevice, surface);

    slb_Image depthImage = slb_Image_Create(
        &device, physicalDevice, window.width, window.height,
        VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    depthImage.imageView = slb_ImageView_Create(
        &device, depthImage.image, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    slb_Swapchain swapchain = slb_Swapchain_Create(
        &window, physicalDevice, surface, &device, &depthImage);

    slb_RenderPass renderPass =
        slb_RenderPass_Create(&swapchain, &device);

    slb_Swapchain_CreateFramebuffers(&swapchain, &device, renderPass,
                                     &depthImage);

    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[2] = {0};

    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].pImmutableSamplers = NULL;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].pImmutableSamplers = NULL;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    slb_DescriptorSetLayout descriptorSetLayout =
        slb_DescriptorSetLayout_Create(bindings, 2, &device);

    // Create graphics pipeline
    VkVertexInputBindingDescription bindingDescription = {0};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {0};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

    slb_Pipeline graphicsPipeline = slb_Pipeline_Create(
        &device, &swapchain, renderPass, "shaders/vert.spv",
        "shaders/frag.spv", &bindingDescription,
        attributeDescriptions, 2, &descriptorSetLayout, 1);

    slb_CommandPool commandPool =
        slb_CommandPool_Create(physicalDevice, &device, surface);

    VkDescriptorPoolSize poolSizes[2] = {0};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = SLB_FRAMES_IN_FLIGHT;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = SLB_FRAMES_IN_FLIGHT;

    slb_DescriptorPool descriptorPool = slb_DescriptorPool_Create(
        poolSizes, 2, SLB_FRAMES_IN_FLIGHT, &device);

    VkDeviceSize vertexBufferSize = sizeof(vertices);

    slb_Buffer stagingVertexBuffer = slb_Buffer_Create(
        vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, &device);

    void* data;
    vkMapMemory(device.device, stagingVertexBuffer.memory, 0,
                vertexBufferSize, 0, &data);
    memcpy(data, vertices, (size_t)vertexBufferSize);
    vkUnmapMemory(device.device, stagingVertexBuffer.memory);

    slb_Buffer vertexBuffer = slb_Buffer_Create(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, &device);

    slb_CopyBuffer(stagingVertexBuffer.buffer, vertexBuffer.buffer,
                   vertexBufferSize, &device, &commandPool);

    vkDestroyBuffer(device.device, stagingVertexBuffer.buffer, NULL);
    vkFreeMemory(device.device, stagingVertexBuffer.memory, NULL);

    VkDeviceSize indexBufferSize = sizeof(indices);

    slb_Buffer stagingIndexBuffer = slb_Buffer_Create(
        indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, &device);

    void* indexData;
    vkMapMemory(device.device, stagingIndexBuffer.memory, 0,
                indexBufferSize, 0, &indexData);
    memcpy(indexData, indices, (size_t)indexBufferSize);
    vkUnmapMemory(device.device, stagingIndexBuffer.memory);

    slb_Buffer indexBuffer = slb_Buffer_Create(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, &device);

    slb_CopyBuffer(stagingIndexBuffer.buffer, indexBuffer.buffer,
                   indexBufferSize, &device, &commandPool);

    vkDestroyBuffer(device.device, stagingIndexBuffer.buffer, NULL);
    vkFreeMemory(device.device, stagingIndexBuffer.memory, NULL);

    // CREATE TEXTURE

    int      texWidth, texHeight, texChannels;
    stbi_uc* pixels =
        stbi_load("res/textures/cursor.png", &texWidth, &texHeight,
                  &texChannels, STBI_rgb_alpha);

    VkDeviceSize imageSize = texWidth * texHeight * 4; // RGBA

    slb_Buffer stagingBuffer =
        slb_Buffer_Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          physicalDevice, &device);

    void* textureData;
    vkMapMemory(device.device, stagingBuffer.memory, 0, imageSize, 0,
                &textureData);
    memcpy(textureData, pixels, (size_t)imageSize);
    vkUnmapMemory(device.device, stagingBuffer.memory);

    slb_Image textureImage = slb_Image_Create(
        &device, physicalDevice, texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Transition image layout for transfer
    slb_TransitionImageLayout(
        textureImage.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &device, &commandPool);

    // Copy buffer to image
    slb_CopyBufferToImage(stagingBuffer.buffer, textureImage.image,
                          texWidth, texHeight, &device, &commandPool);

    // Transition image layout for shader access
    slb_TransitionImageLayout(
        textureImage.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &device,
        &commandPool);

    // Create image view
    textureImage.imageView = slb_ImageView_Create(
        &device, textureImage.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT);

    textureImage.sampler = slb_Sampler_Create(
        VK_FILTER_NEAREST, VK_FILTER_NEAREST, false, 1.0f, false,
        false, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_MIPMAP_MODE_NEAREST,
        &device);

    // Clean up staging buffer
    vkDestroyBuffer(device.device, stagingBuffer.buffer, NULL);
    vkFreeMemory(device.device, stagingBuffer.memory, NULL);

    // CREATE UNIFORM BUFFERS

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    slb_DescriptorSet uniformBuffers;

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        uniformBuffers.buffers[i] = slb_Buffer_Create(
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice, &device);

        vkMapMemory(device.device, uniformBuffers.buffers[i].memory,
                    0, bufferSize, 0, &uniformBuffers.buffersMap[i]);
    }

    // CREATE DESCRIPTOR SETS

    VkDescriptorSetLayout layouts[SLB_FRAMES_IN_FLIGHT];
    for (int i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        layouts[i] = descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = SLB_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(device.device, &allocInfo,
                                 uniformBuffers.descriptorSets) !=
        VK_SUCCESS)
    {
        slb_Error("Failed to allocate descriptor sets",
                  slb_ErrorType_Error);
    }

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer = uniformBuffers.buffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImage.imageView;
        imageInfo.sampler = textureImage.sampler;

        VkWriteDescriptorSet descriptorWrites[2] = {0};

        descriptorWrites[0].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = uniformBuffers.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = uniformBuffers.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device.device, 2, descriptorWrites, 0,
                               NULL);
    }

    // SEMAPHORE CREATION

    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphore imageAvailableSemaphores[SLB_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[SLB_FRAMES_IN_FLIGHT];
    VkFence     inFlightFences[SLB_FRAMES_IN_FLIGHT];

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device.device, &semaphoreInfo, NULL,
                              &imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(device.device, &semaphoreInfo, NULL,
                              &renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateFence(device.device, &fenceInfo, NULL,
                          &inFlightFences[i]) != VK_SUCCESS)
        {
            slb_Error("Failed to create synchronization objects",
                      slb_ErrorType_Error);
        }
    }

    int currentFrame = 0;

    while (!slb_Window_ShouldClose(&window))
    {
        vkWaitForFences(device.device, 1,
                        &inFlightFences[currentFrame], VK_TRUE,
                        UINT64_MAX);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device.device, swapchain.swapchain,
                              UINT64_MAX,
                              imageAvailableSemaphores[currentFrame],
                              VK_NULL_HANDLE, &imageIndex);

        vkResetFences(device.device, 1,
                      &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandPool.commandBuffers[currentFrame],
                             0);

        // RECORD COMMAND BUFFER
        // ---

        VkCommandBufferBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkCommandBuffer commandBuffer =
            commandPool.commandBuffers[currentFrame];

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) !=
            VK_SUCCESS)
        {
            slb_Error("Failed to begin recording command buffer",
                      slb_ErrorType_Error);
        }

        VkRenderPassBeginInfo renderPassInfo = {0};
        renderPassInfo.sType =
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = *(VkFramebuffer*)slb_Vector_Get(
            swapchain.swapchainFramebuffers, imageIndex);
        renderPassInfo.renderArea.offset = (VkOffset2D) {0, 0};
        renderPassInfo.renderArea.extent = swapchain.swapchainExtent;

        VkClearValue clearValues[2] = {0};
        clearValues[0].color =
            (VkClearColorValue) {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil =
            (VkClearDepthStencilValue) {1.0f, 0};

        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          graphicsPipeline.pipeline);

        VkViewport viewport = {0};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain.swapchainExtent.width;
        viewport.height = (float)swapchain.swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {0};
        scissor.offset = (VkOffset2D) {0, 0};
        scissor.extent = swapchain.swapchainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        VkBuffer     vertexBuffers[] = {vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers,
                               offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline.layout, 0, 1,
            &uniformBuffers.descriptorSets[currentFrame], 0, NULL);

        vkCmdDrawIndexed(commandBuffer,
                         sizeof(indices) / sizeof(indices[0]), 1, 0,
                         0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            slb_Error("Failed to record command buffer",
                      slb_ErrorType_Error);
        }

        // ---

        // UPDATE UNIFORM BUFFERS
        // ---

        UniformBufferObject ubo = {0};

        // Model matrix
        glm_mat4_identity(ubo.model);
        glm_translate(ubo.model, cursorPosition);
        glm_scale(ubo.model, (vec3) {0.1f, 0.0f, 0.1f});

        // View matrix
        slb_Camera_GetViewMatrix(&camera, ubo.view);

        // Projection matrix
        glm_perspective(glm_rad(45.0f),
                        swapchain.swapchainExtent.width /
                            (float)swapchain.swapchainExtent.height,
                        0.1f, 1000.0f, ubo.proj);

        // GLM uses OpenGL conventions, Vulkan has inverted Y
        ubo.proj[1][1] *= -1;

        memcpy(uniformBuffers.buffersMap[currentFrame], &ubo,
               sizeof(ubo));

        // ---

        VkSubmitInfo submitInfo = {0};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {
            imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers =
            &commandPool.commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {
            renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(device.graphicsQueue, 1, &submitInfo,
                          inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            slb_Error("Failed to submit draw command buffer",
                      slb_ErrorType_Error);
        }

        VkPresentInfoKHR presentInfo = {0};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchain.swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(device.presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % SLB_FRAMES_IN_FLIGHT;

        mousePosition[0] = slb_Input_GetMouseInputHorizontal(&window);
        mousePosition[1] = slb_Input_GetMouseInputVertical(&window);

        vec2 mouseDifference;
        glm_vec2_sub(mousePosition, previousMousePosition,
                     mouseDifference);

        glm_vec2_mul(mouseDifference, (vec2) {sensitivity, sensitivity},
                     mouseDifference);

        // vec3 cursorAdd = {mouseDifference[0], 0.0f,
        //                   mouseDifference[1]};
        // glm_vec3_add(cursorAdd, cursorPosition, cursorPosition);

        vec3 cursorPos;
        slb_Camera_CursorToWorld(&camera, mousePosition[0], mousePosition[1],
                1920, 1080, ubo.proj, ubo.view, cursorPos);
        glm_vec3_copy(cursorPos, cursorPosition);

        glm_vec2_copy(mousePosition, previousMousePosition);

        ControlCamera(&camera, &window, 0.016f);

        slb_Window_Update(&window);
    }

    return 0;
}
