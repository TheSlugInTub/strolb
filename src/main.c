#include <stdio.h>
#include <strolb/vulkan.h>
#include <strolb/camera.h>
#include <strolb/input.h>
#include <strolb/imgui.h>
#include <strolb/json.h>
#include <stb/stb_image.h>
#include <cglm/cglm.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define MAX_RENDER_OBJECTS 1000
#define MAX_TEXT_OBJECTS   100
#define ATLAS_WIDTH        512
#define ATLAS_HEIGHT       512

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

// Character info for font atlas
typedef struct
{
    float ax; // advance.x
    float ay; // advance.y

    float bw; // bitmap.width
    float bh; // bitmap.rows

    float bl; // bitmap_left
    float bt; // bitmap_top

    float tx; // x offset in texture atlas
    float ty; // y offset in texture atlas
} Character;

typedef struct
{
    slb_Image         texture;
    slb_DescriptorSet descriptorSet;
    slb_Buffer        vertexBuffer;
    slb_Buffer        indexBuffer;
    vec2              position;
    vec2              scale;
} RenderObject;

typedef struct
{
    char              text[256];
    vec2              position;
    vec3              color;
    float             scale;
    slb_DescriptorSet descriptorSet;
    slb_Buffer        vertexBuffer;
    slb_Buffer        indexBuffer;
    int               vertexCount;
} TextObject;

typedef struct
{
    slb_Buffer vertexBuffer;
    slb_Buffer indexBuffer;

    uint32_t vertexCount;
    uint32_t indexCount;

    vec3  color;
    float lineWidth;

    slb_DescriptorSet descriptorSet;

    mat4 transform;

    int firstBoxIndex;
    int secondBoxIndex;
} LineObject;

// Global font data
Character characters[128];
slb_Image fontAtlas;

typedef struct
{
    char        text[1024];
    char        event[1024];
    slb_Vector* connections; // int
    int         numTextObjects;
    int         beginningTextIndex;
} DialogueBox;

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

void CreateDialogueBox(const char* text, vec2 pos, float textScale,
                       slb_Vector*             renderObjects,
                       slb_Vector*             textObjects,
                       slb_Vector*             dialogueBoxes,
                       slb_PhysicalDevice      physicalDevice,
                       slb_Device*             device,
                       slb_CommandPool*        commandPool,
                       slb_DescriptorSetLayout descriptorSetLayout,
                       slb_DescriptorPool      descriptorPool);

void CreateDialogueBoxAtIndex(
    const char* text, vec2 pos, float textScale,
    slb_Vector* renderObjects, slb_Vector* textObjects,
    slb_Vector* dialogueBoxes, int insertIndex,
    slb_PhysicalDevice physicalDevice, slb_Device* device,
    slb_CommandPool*        commandPool,
    slb_DescriptorSetLayout descriptorSetLayout,
    slb_DescriptorPool      descriptorPool);

void UpdateDialogueBox(int dialogueIndex, slb_Vector* renderObjects,
                       slb_Vector*             textObjects,
                       slb_Vector*             dialogueBoxes,
                       slb_Vector*             lineObjects,
                       slb_PhysicalDevice      physicalDevice,
                       slb_Device*             device,
                       slb_CommandPool*        commandPool,
                       slb_DescriptorSetLayout descriptorSetLayout,
                       slb_DescriptorPool      descriptorPool);

void ControlCamera(slb_Camera* camera, slb_Window* window, float dt)
{
    float speed = 0.4f * dt;

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
        camera->position[0] -= 1.0f * speed;
    }
    if (slb_Input_GetKey(window, SLB_KEY_D))
    {
        camera->position[0] += 1.0f * speed;
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

// Initialize FreeType and create font atlas
int InitializeFreeType(const char* fontPath, int fontSize,
                       slb_PhysicalDevice physicalDevice,
                       slb_Device*        device,
                       slb_CommandPool*   commandPool)
{
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        fprintf(stderr, "Could not init FreeType Library\n");
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face))
    {
        fprintf(stderr, "Failed to load font\n");
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Create atlas texture
    unsigned char* atlasData = calloc(ATLAS_WIDTH * ATLAS_HEIGHT, 1);
    int            pen_x = 0, pen_y = 0;
    int            row_height = 0;

    // Load first 128 ASCII characters
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            fprintf(stderr, "Failed to load Glyph\n");
            continue;
        }

        FT_GlyphSlot g = face->glyph;

        // If we've run out of room, go to next row
        if (pen_x + g->bitmap.width >= ATLAS_WIDTH)
        {
            pen_x = 0;
            pen_y += row_height;
            row_height = 0;
        }

        // Check if we have enough vertical space
        if (pen_y + g->bitmap.rows >= ATLAS_HEIGHT)
        {
            fprintf(stderr, "Font atlas too small\n");
            break;
        }

        // Copy glyph bitmap to atlas
        for (int y = 0; y < g->bitmap.rows; y++)
        {
            for (int x = 0; x < g->bitmap.width; x++)
            {
                int atlas_x = pen_x + x;
                int atlas_y = pen_y + y;
                atlasData[atlas_y * ATLAS_WIDTH + atlas_x] =
                    g->bitmap.buffer[y * g->bitmap.width + x];
            }
        }

        // Store character info
        characters[c].ax = g->advance.x >> 6;
        characters[c].ay = g->advance.y >> 6;
        characters[c].bw = g->bitmap.width;
        characters[c].bh = g->bitmap.rows;
        characters[c].bl = g->bitmap_left;
        characters[c].bt = g->bitmap_top;
        characters[c].tx = (float)pen_x / ATLAS_WIDTH;
        characters[c].ty = (float)pen_y / ATLAS_HEIGHT;

        row_height = (g->bitmap.rows > row_height) ? g->bitmap.rows
                                                   : row_height;
        pen_x += g->bitmap.width + 20; // padding
    }

    // Create Vulkan texture from atlas
    VkDeviceSize imageSize = ATLAS_WIDTH * ATLAS_HEIGHT;

    slb_Buffer stagingBuffer =
        slb_Buffer_Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          physicalDevice, device);

    void* data;
    vkMapMemory(device->device, stagingBuffer.memory, 0, imageSize, 0,
                &data);
    memcpy(data, atlasData, (size_t)imageSize);
    vkUnmapMemory(device->device, stagingBuffer.memory);

    fontAtlas = slb_Image_Create(
        device, physicalDevice, ATLAS_WIDTH, ATLAS_HEIGHT,
        VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Transition image layout for transfer
    slb_TransitionImageLayout(fontAtlas.image, VK_FORMAT_R8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              device, commandPool);

    // Copy buffer to image
    slb_CopyBufferToImage(stagingBuffer.buffer, fontAtlas.image,
                          ATLAS_WIDTH, ATLAS_HEIGHT, device,
                          commandPool);

    // Transition image layout for shader access
    slb_TransitionImageLayout(
        fontAtlas.image, VK_FORMAT_R8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, device,
        commandPool);

    // Create image view
    fontAtlas.imageView = slb_ImageView_Create(
        device, fontAtlas.image, VK_FORMAT_R8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT);

    fontAtlas.sampler = slb_Sampler_Create(
        VK_FILTER_LINEAR, VK_FILTER_LINEAR, false, 1.0f, false, false,
        VK_COMPARE_OP_ALWAYS, VK_SAMPLER_MIPMAP_MODE_LINEAR, device);

    // Clean up
    vkDestroyBuffer(device->device, stagingBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingBuffer.memory, NULL);
    free(atlasData);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return 0;
}

TextObject CreateTextObject(const char* text, vec2 position,
                            vec3 color, float scale,
                            slb_PhysicalDevice      physicalDevice,
                            slb_Device*             device,
                            slb_CommandPool*        commandPool,
                            slb_DescriptorSetLayout layout,
                            slb_DescriptorPool      pool)
{
    TextObject textObj = {0};
    strcpy(textObj.text, text);
    glm_vec2_copy(position, textObj.position);
    glm_vec3_copy(color, textObj.color);
    textObj.scale = scale;

    // Generate vertex data for text
    float x = 0.0f;
    int   len = strlen(text);
    textObj.vertexCount =
        len * 6; // 6 vertices per character (2 triangles)

    Vertex* textVertices =
        malloc(textObj.vertexCount * sizeof(Vertex));

    for (int i = 0; i < len; i++)
    {
        char      c = text[i];
        Character ch = characters[(int)c];

        float xpos = x + ch.bl * scale;
        float ypos = -(ch.bh - ch.bt) * scale;
        float w = ch.bw * scale;
        float h = ch.bh * scale;

        // Texture coordinates
        float tx = ch.tx;
        float ty = ch.ty;
        float tw = ch.bw / (float)ATLAS_WIDTH;
        float th = ch.bh / (float)ATLAS_HEIGHT;

        // First triangle
        textVertices[i * 6 + 0] =
            (Vertex) {{xpos, 0.0f, ypos + h}, {tx, ty}};
        textVertices[i * 6 + 1] =
            (Vertex) {{xpos, 0.0f, ypos}, {tx, ty + th}};
        textVertices[i * 6 + 2] =
            (Vertex) {{xpos + w, 0.0f, ypos}, {tx + tw, ty + th}};

        // Second triangle
        textVertices[i * 6 + 3] =
            (Vertex) {{xpos, 0.0f, ypos + h}, {tx, ty}};
        textVertices[i * 6 + 4] =
            (Vertex) {{xpos + w, 0.0f, ypos}, {tx + tw, ty + th}};
        textVertices[i * 6 + 5] =
            (Vertex) {{xpos + w, 0.0f, ypos + h}, {tx + tw, ty}};

        x += ch.ax * scale;
    }

    // Create vertex buffer
    VkDeviceSize bufferSize = textObj.vertexCount * sizeof(Vertex);

    slb_Buffer stagingBuffer = slb_Buffer_Create(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, device);

    void* data;
    vkMapMemory(device->device, stagingBuffer.memory, 0, bufferSize,
                0, &data);
    memcpy(data, textVertices, (size_t)bufferSize);
    vkUnmapMemory(device->device, stagingBuffer.memory);

    textObj.vertexBuffer = slb_Buffer_Create(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, device);

    slb_CopyBuffer(stagingBuffer.buffer, textObj.vertexBuffer.buffer,
                   bufferSize, device, commandPool);

    vkDestroyBuffer(device->device, stagingBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingBuffer.memory, NULL);

    // Create uniform buffers
    VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        textObj.descriptorSet.buffers[i] = slb_Buffer_Create(
            uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice, device);

        vkMapMemory(device->device,
                    textObj.descriptorSet.buffers[i].memory, 0,
                    uniformBufferSize, 0,
                    &textObj.descriptorSet.buffersMap[i]);
    }

    // Create descriptor sets
    VkDescriptorSetLayout layouts[SLB_FRAMES_IN_FLIGHT];
    for (int i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        layouts[i] = layout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = SLB_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(
            device->device, &allocInfo,
            textObj.descriptorSet.descriptorSets) != VK_SUCCESS)
    {
        slb_Error("Failed to allocate text descriptor sets",
                  slb_ErrorType_Error);
    }

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer = textObj.descriptorSet.buffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = fontAtlas.imageView;
        imageInfo.sampler = fontAtlas.sampler;

        VkWriteDescriptorSet descriptorWrites[2] = {0};

        descriptorWrites[0].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet =
            textObj.descriptorSet.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet =
            textObj.descriptorSet.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device->device, 2, descriptorWrites, 0,
                               NULL);
    }

    free(textVertices);
    return textObj;
}

RenderObject CreateRenderObject(const char* texturePath,
                                vec2 position, vec2 scale,
                                slb_PhysicalDevice physicalDevice,
                                slb_Device*        device,
                                slb_CommandPool*   commandPool,
                                slb_DescriptorSetLayout layout,
                                slb_DescriptorPool      pool)
{
    RenderObject renderObject;
    glm_vec2_copy(position, renderObject.position);
    glm_vec2_copy(scale, renderObject.scale);

    VkDeviceSize vertexBufferSize = sizeof(vertices);

    slb_Buffer stagingVertexBuffer = slb_Buffer_Create(
        vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, device);

    void* data;
    vkMapMemory(device->device, stagingVertexBuffer.memory, 0,
                vertexBufferSize, 0, &data);
    memcpy(data, vertices, (size_t)vertexBufferSize);
    vkUnmapMemory(device->device, stagingVertexBuffer.memory);

    renderObject.vertexBuffer = slb_Buffer_Create(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, device);

    slb_CopyBuffer(stagingVertexBuffer.buffer,
                   renderObject.vertexBuffer.buffer, vertexBufferSize,
                   device, commandPool);

    vkDestroyBuffer(device->device, stagingVertexBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingVertexBuffer.memory, NULL);

    VkDeviceSize indexBufferSize = sizeof(indices);

    slb_Buffer stagingIndexBuffer = slb_Buffer_Create(
        indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, device);

    void* indexData;
    vkMapMemory(device->device, stagingIndexBuffer.memory, 0,
                indexBufferSize, 0, &indexData);
    memcpy(indexData, indices, (size_t)indexBufferSize);
    vkUnmapMemory(device->device, stagingIndexBuffer.memory);

    renderObject.indexBuffer = slb_Buffer_Create(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, device);

    slb_CopyBuffer(stagingIndexBuffer.buffer,
                   renderObject.indexBuffer.buffer, indexBufferSize,
                   device, commandPool);

    vkDestroyBuffer(device->device, stagingIndexBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingIndexBuffer.memory, NULL);

    // CREATE TEXTURE

    int      texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight,
                                &texChannels, STBI_rgb_alpha);

    VkDeviceSize imageSize = texWidth * texHeight * 4; // RGBA

    slb_Buffer stagingBuffer =
        slb_Buffer_Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          physicalDevice, device);

    void* textureData;
    vkMapMemory(device->device, stagingBuffer.memory, 0, imageSize, 0,
                &textureData);
    memcpy(textureData, pixels, (size_t)imageSize);
    vkUnmapMemory(device->device, stagingBuffer.memory);

    renderObject.texture = slb_Image_Create(
        device, physicalDevice, texWidth, texHeight,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Transition image layout for transfer
    slb_TransitionImageLayout(
        renderObject.texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device, commandPool);

    // Copy buffer to image
    slb_CopyBufferToImage(stagingBuffer.buffer,
                          renderObject.texture.image, texWidth,
                          texHeight, device, commandPool);

    // Transition image layout for shader access
    slb_TransitionImageLayout(
        renderObject.texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, device,
        commandPool);

    // Create image view
    renderObject.texture.imageView = slb_ImageView_Create(
        device, renderObject.texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT);

    renderObject.texture.sampler = slb_Sampler_Create(
        VK_FILTER_NEAREST, VK_FILTER_NEAREST, false, 1.0f, false,
        false, VK_COMPARE_OP_ALWAYS, VK_SAMPLER_MIPMAP_MODE_NEAREST,
        device);

    // Clean up staging buffer
    vkDestroyBuffer(device->device, stagingBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingBuffer.memory, NULL);

    // CREATE UNIFORM BUFFERS

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        renderObject.descriptorSet.buffers[i] = slb_Buffer_Create(
            bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice, device);

        vkMapMemory(device->device,
                    renderObject.descriptorSet.buffers[i].memory, 0,
                    bufferSize, 0,
                    &renderObject.descriptorSet.buffersMap[i]);
    }

    // CREATE DESCRIPTOR SETS

    VkDescriptorSetLayout layouts[SLB_FRAMES_IN_FLIGHT];
    for (int i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        layouts[i] = layout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = SLB_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(
            device->device, &allocInfo,
            renderObject.descriptorSet.descriptorSets) != VK_SUCCESS)
    {
        slb_Error("Failed to allocate descriptor sets",
                  slb_ErrorType_Error);
    }

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer =
            renderObject.descriptorSet.buffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = renderObject.texture.imageView;
        imageInfo.sampler = renderObject.texture.sampler;

        VkWriteDescriptorSet descriptorWrites[2] = {0};

        descriptorWrites[0].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet =
            renderObject.descriptorSet.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet =
            renderObject.descriptorSet.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device->device, 2, descriptorWrites, 0,
                               NULL);
    }

    return renderObject;
}

void DestroyTextObject(TextObject* textObj, slb_Device* device)
{
    vkDestroyBuffer(device->device, textObj->vertexBuffer.buffer,
                    NULL);
    vkFreeMemory(device->device, textObj->vertexBuffer.memory, NULL);

    for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT; j++)
    {
        vkUnmapMemory(device->device,
                      textObj->descriptorSet.buffers[j].memory);
        vkDestroyBuffer(device->device,
                        textObj->descriptorSet.buffers[j].buffer,
                        NULL);
        vkFreeMemory(device->device,
                     textObj->descriptorSet.buffers[j].memory, NULL);
    }
}

int currentDialogueBox = -1;

void DestroyRenderObject(RenderObject* obj, slb_Device* device)
{
    vkDestroyBuffer(device->device, obj->vertexBuffer.buffer, NULL);
    vkFreeMemory(device->device, obj->vertexBuffer.memory, NULL);
    vkDestroyBuffer(device->device, obj->indexBuffer.buffer, NULL);
    vkFreeMemory(device->device, obj->indexBuffer.memory, NULL);

    vkDestroyImageView(device->device, obj->texture.imageView, NULL);
    vkDestroySampler(device->device, obj->texture.sampler, NULL);
    vkDestroyImage(device->device, obj->texture.image, NULL);
    vkFreeMemory(device->device, obj->texture.memory, NULL);

    for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT; j++)
    {
        vkUnmapMemory(device->device,
                      obj->descriptorSet.buffers[j].memory);
        vkDestroyBuffer(device->device,
                        obj->descriptorSet.buffers[j].buffer, NULL);
        vkFreeMemory(device->device,
                     obj->descriptorSet.buffers[j].memory, NULL);
    }
}

void CreateDialogueBoxAtIndex(
    const char* textOriginal, vec2 pos, float textScale,
    slb_Vector* renderObjects, slb_Vector* textObjects,
    slb_Vector* dialogueBoxes, int insertIndex,
    slb_PhysicalDevice physicalDevice, slb_Device* device,
    slb_CommandPool*        commandPool,
    slb_DescriptorSetLayout descriptorSetLayout,
    slb_DescriptorPool      descriptorPool)
{
    char text[1024];

    char buffer[1024];
    strcpy(buffer, textOriginal);
    
    // Count lines and store pointers
    char* lines[100];
    int line_count = 0;
    
    char* token = strtok(buffer, "\n");
    while (token != NULL && line_count < 100) {
        lines[line_count++] = token;
        token = strtok(NULL, "\n");
    }
    
    // Build reversed string
    text[0] = '\0';
    for (int i = line_count - 1; i >= 0; i--) {
        strcat(text, lines[i]);
        if (i > 0) {
            strcat(text, "\n");
        }
    }

    DialogueBox box = {};
    strcpy(box.text, textOriginal);

    // Calculate box dimensions (same as before)
    const char* delimiter = "\n";
    char*       textCopy = strdup(text);
    char*       line = strtok(textCopy, delimiter);
    int         lineCount = 0;
    float       maxLineWidth = 0.0f;

    while (line != NULL)
    {
        lineCount++;
        float lineWidth = 0.0f;
        for (int i = 0; i < strlen(line); i++)
        {
            Character ch = characters[(int)line[i]];
            lineWidth += ch.ax * textScale;
        }
        if (lineWidth > maxLineWidth)
        {
            maxLineWidth = lineWidth;
        }
        line = strtok(NULL, delimiter);
    }
    free(textCopy);

    const float padding = 0.4f;
    float       boxWidth = maxLineWidth + 2 * padding;
    float       boxHeight =
        (characters['A'].bh * textScale * lineCount) + 2 * padding;

    RenderObject boxObj = CreateRenderObject(
        "res/textures/grey.png", (vec2) {pos[0], pos[1]},
        (vec2) {boxWidth, boxHeight}, physicalDevice, device,
        commandPool, descriptorSetLayout, descriptorPool);

    slb_Vector_Insert(renderObjects, insertIndex + 1, &boxObj);

    textCopy = strdup(text);
    line = strtok(textCopy, delimiter);
    float yOffset = padding;
    int   textInsertIndex = 0;

    // Calculate where to insert text objects
    for (int i = 0; i < insertIndex; i++)
    {
        DialogueBox* prevBox = slb_Vector_Get(dialogueBoxes, i);
        textInsertIndex += prevBox->numTextObjects;
    }

    printf("InsertIndex: %d\n", insertIndex);

    box.beginningTextIndex = textInsertIndex;
    box.numTextObjects = lineCount;

    for (int i = 0; i < lineCount; i++)
    {
        vec2 textPos = {pos[0] - boxWidth / 2 + padding,
                        pos[1] - boxHeight / 2 + yOffset};

        TextObject textObj = CreateTextObject(
            line, textPos, (vec3) {0.0f, 0.0f, 0.0f}, textScale,
            physicalDevice, device, commandPool, descriptorSetLayout,
            descriptorPool);

        slb_Vector_Insert(textObjects, textInsertIndex + i, &textObj);

        yOffset += characters['A'].bh * textScale * 1.2f;
        line = strtok(NULL, delimiter);
    }
    free(textCopy);

    // Update text indices for dialogue boxes that come after this one
    for (int i = insertIndex; i < dialogueBoxes->size; i++)
    {
        DialogueBox* laterBox = slb_Vector_Get(dialogueBoxes, i);
        laterBox->beginningTextIndex += lineCount;
    }

    box.connections = slb_Vector_Create(sizeof(int), 1);

    slb_Vector_Insert(dialogueBoxes, insertIndex, &box);
}

void UpdateDialogueBox(int dialogueIndex, slb_Vector* renderObjects,
                       slb_Vector*             textObjects,
                       slb_Vector*             dialogueBoxes,
                       slb_Vector*             lineObjects,
                       slb_PhysicalDevice      physicalDevice,
                       slb_Device*             device,
                       slb_CommandPool*        commandPool,
                       slb_DescriptorSetLayout descriptorSetLayout,
                       slb_DescriptorPool      descriptorPool)
{
    DialogueBox* box = slb_Vector_Get(dialogueBoxes, dialogueIndex);
    RenderObject* obj = slb_Vector_Get(renderObjects, dialogueIndex + 1);

    // Store the data we need before cleanup
    char text[1024];
    strcpy(text, box->text);
    char event[1024];
    strcpy(event, box->event);
    vec2 pos;
    glm_vec2_copy(obj->position, pos);

    // Store connections
    slb_Vector* tempConnections = slb_Vector_Create(sizeof(int), 1);
    for (int i = 0; i < box->connections->size; i++)
    {
        int* connection = slb_Vector_Get(box->connections, i);
        slb_Vector_PushBack(tempConnections, connection);
    }

    // Store the old number of text objects BEFORE cleanup
    int oldNumTextObjects = box->numTextObjects;

    // Clean up old text objects
    for (int i = box->beginningTextIndex;
         i < box->numTextObjects + box->beginningTextIndex; i++)
    {
        TextObject* textObj = slb_Vector_Get(textObjects, i);
        DestroyTextObject(textObj, device);
    }

    // Remove old elements (from highest index to lowest to avoid shifting issues)
    for (int i = box->numTextObjects + box->beginningTextIndex - 1;
         i >= box->beginningTextIndex; i--)
    {
        slb_Vector_Remove(textObjects, i);
    }

    // Clean up old render object
    DestroyRenderObject(obj, device);
    slb_Vector_Remove(renderObjects, dialogueIndex + 1);

    // Free the old dialogue box connections
    slb_Vector_Free(box->connections);
    slb_Vector_Remove(dialogueBoxes, dialogueIndex);

    // Update text indices for dialogue boxes that come after this one
    // This must be done BEFORE creating the new dialogue box
    for (int i = dialogueIndex; i < dialogueBoxes->size; i++)
    {
        DialogueBox* laterBox = slb_Vector_Get(dialogueBoxes, i);
        laterBox->beginningTextIndex -= oldNumTextObjects;
    }

    // Create new dialogue box at the same index
    CreateDialogueBoxAtIndex(
        text, pos, 0.01f, renderObjects, textObjects, dialogueBoxes,
        dialogueIndex, physicalDevice, device, commandPool,
        descriptorSetLayout, descriptorPool);

    // Restore event and connections
    DialogueBox* newBox = slb_Vector_Get(dialogueBoxes, dialogueIndex);
    strcpy(newBox->event, event);

    // Restore connections
    for (int i = 0; i < tempConnections->size; i++)
    {
        int* connection = slb_Vector_Get(tempConnections, i);
        slb_Vector_PushBack(newBox->connections, connection);
    }

    slb_Vector_Free(tempConnections);

    currentDialogueBox = dialogueIndex + 1; // +1 for render object index
}

LineObject CreateLineObject(vec3 startPos, vec3 endPos, vec3 color,
                            float                   lineWidth,
                            slb_PhysicalDevice      physicalDevice,
                            slb_Device*             device,
                            slb_CommandPool*        commandPool,
                            slb_DescriptorSetLayout layout,
                            slb_DescriptorPool      pool)
{
    LineObject lineObj = {0};

    glm_vec3_copy(color, lineObj.color);
    lineObj.lineWidth = lineWidth;
    glm_mat4_identity(lineObj.transform);

    // Create line vertices
    Vertex lineVertices[2] = {
        {{startPos[0], startPos[1], startPos[2]}, {0.0f, 0.0f}},
        {{endPos[0], endPos[1], endPos[2]}, {1.0f, 0.0f}}};

    lineObj.vertexCount = 2;

    // Create vertex buffer
    VkDeviceSize bufferSize = sizeof(lineVertices);

    slb_Buffer stagingBuffer = slb_Buffer_Create(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        physicalDevice, device);

    void* data;
    vkMapMemory(device->device, stagingBuffer.memory, 0, bufferSize,
                0, &data);
    memcpy(data, lineVertices, bufferSize);
    vkUnmapMemory(device->device, stagingBuffer.memory);

    lineObj.vertexBuffer = slb_Buffer_Create(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, device);

    slb_CopyBuffer(stagingBuffer.buffer, lineObj.vertexBuffer.buffer,
                   bufferSize, device, commandPool);

    vkDestroyBuffer(device->device, stagingBuffer.buffer, NULL);
    vkFreeMemory(device->device, stagingBuffer.memory, NULL);

    // Create uniform buffers
    VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        lineObj.descriptorSet.buffers[i] = slb_Buffer_Create(
            uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            physicalDevice, device);

        vkMapMemory(device->device,
                    lineObj.descriptorSet.buffers[i].memory, 0,
                    uniformBufferSize, 0,
                    &lineObj.descriptorSet.buffersMap[i]);
    }

    // Create descriptor sets
    VkDescriptorSetLayout layouts[SLB_FRAMES_IN_FLIGHT];
    for (int i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        layouts[i] = layout;
    }

    VkDescriptorSetAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = SLB_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(
            device->device, &allocInfo,
            lineObj.descriptorSet.descriptorSets) != VK_SUCCESS)
    {
        slb_Error("Failed to allocate line descriptor sets",
                  slb_ErrorType_Error);
    }

    for (size_t i = 0; i < SLB_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {0};
        bufferInfo.buffer = lineObj.descriptorSet.buffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // For lines, we'll use a dummy texture (you could create a
        // white 1x1 texture)
        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.imageLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView =
            fontAtlas.imageView; // Reuse font atlas as dummy texture
        imageInfo.sampler = fontAtlas.sampler;

        VkWriteDescriptorSet descriptorWrites[2] = {0};

        descriptorWrites[0].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet =
            lineObj.descriptorSet.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet =
            lineObj.descriptorSet.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device->device, 2, descriptorWrites, 0,
                               NULL);
    }

    return lineObj;
}


void CreateDialogueBox(const char* text, vec2 pos, float textScale,
                       slb_Vector*             renderObjects,
                       slb_Vector*             textObjects,
                       slb_Vector*             dialogueBoxes,
                       slb_PhysicalDevice      physicalDevice,
                       slb_Device*             device,
                       slb_CommandPool*        commandPool,
                       slb_DescriptorSetLayout descriptorSetLayout,
                       slb_DescriptorPool      descriptorPool)
{
    CreateDialogueBoxAtIndex(
        text, pos, textScale, renderObjects, textObjects,
        dialogueBoxes, dialogueBoxes->size, physicalDevice, device,
        commandPool, descriptorSetLayout, descriptorPool);
}

void LoadDialogueBoxes(
    const char* filename, slb_Vector* renderObjects,
    slb_Vector* textObjects, slb_Vector* dialogueBoxes,
    slb_Vector* lineObjects, slb_PhysicalDevice physicalDevice,
    slb_Device* device, slb_CommandPool* commandPool,
    slb_DescriptorSetLayout descriptorSetLayout,
    slb_DescriptorPool      descriptorPool)
{
    // Clear existing dialogue boxes (keep cursor at index 0)
    // Clean up existing dialogue boxes
    for (int i = 1; i < renderObjects->size; i++)
    {
        RenderObject* obj = slb_Vector_Get(renderObjects, i);

        // Destroy render object resources
        vkDestroyBuffer(device->device, obj->vertexBuffer.buffer,
                        NULL);
        vkFreeMemory(device->device, obj->vertexBuffer.memory, NULL);
        vkDestroyBuffer(device->device, obj->indexBuffer.buffer,
                        NULL);
        vkFreeMemory(device->device, obj->indexBuffer.memory, NULL);

        vkDestroyImageView(device->device, obj->texture.imageView,
                           NULL);
        vkDestroySampler(device->device, obj->texture.sampler, NULL);
        vkDestroyImage(device->device, obj->texture.image, NULL);
        vkFreeMemory(device->device, obj->texture.memory, NULL);

        for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT; j++)
        {
            vkUnmapMemory(device->device,
                          obj->descriptorSet.buffers[j].memory);
            vkDestroyBuffer(device->device,
                            obj->descriptorSet.buffers[j].buffer,
                            NULL);
            vkFreeMemory(device->device,
                         obj->descriptorSet.buffers[j].memory, NULL);
        }
    }

    // Clean up text objects
    for (int i = 0; i < textObjects->size; i++)
    {
        TextObject* textObj = slb_Vector_Get(textObjects, i);
        DestroyTextObject(textObj, device);
    }

    // Clean up line objects
    for (int i = 0; i < lineObjects->size; i++)
    {
        LineObject* lineObj = slb_Vector_Get(lineObjects, i);

        vkDestroyBuffer(device->device, lineObj->vertexBuffer.buffer,
                        NULL);
        vkFreeMemory(device->device, lineObj->vertexBuffer.memory,
                     NULL);

        for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT; j++)
        {
            vkUnmapMemory(device->device,
                          lineObj->descriptorSet.buffers[j].memory);
            vkDestroyBuffer(device->device,
                            lineObj->descriptorSet.buffers[j].buffer,
                            NULL);
            vkFreeMemory(device->device,
                         lineObj->descriptorSet.buffers[j].memory,
                         NULL);
        }
    }

    // Clean up dialogue boxes
    for (int i = 0; i < dialogueBoxes->size; i++)
    {
        DialogueBox* box = slb_Vector_Get(dialogueBoxes, i);
        if (box->connections)
        {
            slb_Vector_Free(box->connections);
        }
    }

    // Clear vectors (keep cursor at index 0 for renderObjects)
    while (renderObjects->size > 1)
    {
        slb_Vector_Remove(renderObjects, renderObjects->size - 1);
    }

    textObjects->size = 0;
    dialogueBoxes->size = 0;
    lineObjects->size = 0;

    // Load from JSON file
    slb_Json json = slb_Json_LoadFromFile(filename);
    if (!json)
    {
        printf("Failed to load file: %s\n", filename);
        return;
    }

    int boxCount = slb_Json_GetArraySize(json);

    // First pass: Create all dialogue boxes
    for (int i = 0; i < boxCount; i++)
    {
        slb_Json boxJson = slb_Json_GetArrayElement(json, i);

        vec2 position;
        slb_Json_LoadFloat2(boxJson, "position", position);

        char text[1024];
        slb_Json_LoadString(boxJson, "text", text);

        CreateDialogueBox(text, position, 0.01f, renderObjects,
                          textObjects, dialogueBoxes, physicalDevice,
                          device, commandPool, descriptorSetLayout,
                          descriptorPool);

        DialogueBox* newBox =
            slb_Vector_Get(dialogueBoxes, dialogueBoxes->size - 1);

        slb_Json_LoadString(boxJson, "event", newBox->event);
    }

    // Create connections
    for (int i = 0; i < boxCount; i++)
    {
        slb_Json boxJson = slb_Json_GetArrayElement(json, i);

        int connectionsCount =
            slb_Json_GetIntArraySize(boxJson, "connections");

        if (connectionsCount > 0)
        {
            int connections[16];
            slb_Json_LoadIntArray(boxJson, "connections",
                                  connections);

            DialogueBox* sourceBox = slb_Vector_Get(dialogueBoxes, i);
            RenderObject* sourceObj = slb_Vector_Get(
                renderObjects, i + 1); // +1 to account for cursor

            for (int j = 0; j < connectionsCount; j++)
            {
                int targetIndex =
                    connections[j] -
                    1; // Convert to 0-based index (connections are
                       // stored as 1-based)

                if (targetIndex >= 0 &&
                    targetIndex < renderObjects->size - 1)
                {
                    RenderObject* targetObj = slb_Vector_Get(
                        renderObjects,
                        targetIndex + 1); // +1 for cursor

                    LineObject line = CreateLineObject(
                        (vec3) {sourceObj->position[0], 0.0f,
                                sourceObj->position[1]},
                        (vec3) {targetObj->position[0], 0.0f,
                                targetObj->position[1]},
                        (vec3) {1.0f, 1.0f, 1.0f}, 3.0f,
                        physicalDevice, device, commandPool,
                        descriptorSetLayout, descriptorPool);

                    line.firstBoxIndex = i;
                    line.secondBoxIndex = targetIndex;

                    slb_Vector_PushBack(lineObjects, &line);

                    // Add connection to dialogue box
                    int connectionValue =
                        targetIndex + 1; // Store as 1-based index
                    slb_Vector_PushBack(sourceBox->connections,
                                        &connectionValue);
                }
            }
        }
    }

    slb_Json_Destroy(json);
}

RenderObject* currentRenderObject;
DialogueBox*  currentDialogueBoxObject;

int  firstConnectionDialogueBox = -1;
int  secondConnectionDialogueBox = -1;
bool isConnecting = false;

bool preferencesWindow = false;
bool manualWindow = false;

int main(int argc, char** argv)
{
    slb_Window window =
        slb_Window_Create("Diagmaker", 1600, 900, false, true);

    slb_Camera camera = slb_Camera_Create((vec3) {0.0f, 5.0f, 0.0f},
                                          (vec3) {0.0f, 0.0f, 1.0f},
                                          0.0f, -90.0f, 80.0f);

    vec3 prevMousePosition = {0.0f, 0.0f, 0.0f};
    vec3 mouseDifference = {0.0f, 0.0f, 0.0f};
    vec3 mousePosition = {0.0f, 0.0f, 0.0f};
    vec3 cursorPosition = {0.0f, 0.0f, 0.0f};

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
        attributeDescriptions, 2, &descriptorSetLayout, 1,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    slb_Pipeline textPipeline = slb_Pipeline_Create(
        &device, &swapchain, renderPass, "shaders/text_vert.spv",
        "shaders/text_frag.spv", &bindingDescription,
        attributeDescriptions, 2, &descriptorSetLayout, 1,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    slb_Pipeline linePipeline = slb_Pipeline_Create(
        &device, &swapchain, renderPass, "shaders/line_vert.spv",
        "shaders/line_frag.spv", &bindingDescription,
        attributeDescriptions, 2, &descriptorSetLayout, 1,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    slb_CommandPool commandPool =
        slb_CommandPool_Create(physicalDevice, &device, surface);

    // Initialize FreeType and create font atlas
    if (InitializeFreeType("res/fonts/arial.ttf", 48, physicalDevice,
                           &device, &commandPool) != 0)
    {
        slb_Error("Failed to initialize FreeType",
                  slb_ErrorType_Error);
        return -1;
    }

    VkDescriptorPoolSize poolSizes[2] = {0};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount =
        SLB_FRAMES_IN_FLIGHT *
        (MAX_RENDER_OBJECTS + MAX_TEXT_OBJECTS);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount =
        SLB_FRAMES_IN_FLIGHT *
            (MAX_RENDER_OBJECTS + MAX_TEXT_OBJECTS) +
        SLB_FRAMES_IN_FLIGHT;

    slb_DescriptorPool descriptorPool = slb_DescriptorPool_Create(
        poolSizes, 2,
        SLB_FRAMES_IN_FLIGHT *
                (MAX_RENDER_OBJECTS + MAX_TEXT_OBJECTS) +
            SLB_FRAMES_IN_FLIGHT,
        &device);

    slb_ImGui_Init(window.window, instance, descriptorPool,
                   renderPass, physicalDevice, device.device,
                   commandPool.commandPool, device.graphicsQueue);

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

    slb_Vector* renderObjects =
        slb_Vector_Create(sizeof(RenderObject), 1);

    slb_Vector* textObjects =
        slb_Vector_Create(sizeof(TextObject), 1);

    slb_Vector* dialogueBoxes =
        slb_Vector_Create(sizeof(DialogueBox), 1);

    slb_Vector* lineObjects =
        slb_Vector_Create(sizeof(LineObject), 1);

    RenderObject curs = CreateRenderObject(
        "res/textures/cursor.png", (vec2) {0.0f, 0.0f},
        (vec2) {0.2f, 0.2f}, physicalDevice, &device, &commandPool,
        descriptorSetLayout, descriptorPool);

    slb_Vector_PushBack(renderObjects, &curs);

    CreateDialogueBox(
        "I've been away for a while and hope you didn't do anything "
        "while i was gone\nHey there\nHow ya doin\n",
        (vec2) {0.0f, 1.0f}, 0.01f, renderObjects, textObjects,
        dialogueBoxes, physicalDevice, &device, &commandPool,
        descriptorSetLayout, descriptorPool);

    bool isDragging = false;

    float lastFrame = 0.0f;
    float currentTime = 0.0f;
    float deltaTime = 0.0f;
    float fps = 0.0f;
    int   frameCount = 0;
    float lastTime = glfwGetTime();
    float timeAccumulator = 0.0f;
    char  fpsString[16] = {0};

    while (!slb_Window_ShouldClose(&window))
    {
        currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrame;
        float elapsed = currentTime - lastTime;
        lastFrame = currentTime;

        bool mouseOverGui = slb_ImGui_IsHovering();

        // DIALOUGE BOX SYSTEM
        // ---

        if (slb_Input_GetKeyDown(&window, SLB_KEY_DELETE))
        {
            // DELETE THE SELECTED DIALOGUE BOX
            if (currentDialogueBox != -1 &&
                currentDialogueBox != 0) // Don't delete cursor
            {
                int dialogueIndex =
                    currentDialogueBox -
                    1; // Convert to dialogue box index
                DialogueBox* boxToDelete =
                    slb_Vector_Get(dialogueBoxes, dialogueIndex);
                RenderObject* objToDelete =
                    slb_Vector_Get(renderObjects, currentDialogueBox);

                // Clean up text objects associated with this dialogue
                // box
                for (int i = boxToDelete->beginningTextIndex;
                     i < boxToDelete->numTextObjects +
                             boxToDelete->beginningTextIndex;
                     i++)
                {
                    TextObject* textObj =
                        slb_Vector_Get(textObjects, i);
                    DestroyTextObject(textObj, &device);
                }

                // Clean up render object
                DestroyRenderObject(objToDelete, &device);

                // Remove associated line objects that connect to or
                // from this dialogue box
                for (int i = lineObjects->size - 1; i >= 0; i--)
                {
                    LineObject* line = slb_Vector_Get(lineObjects, i);

                    if (line->firstBoxIndex == dialogueIndex ||
                        line->secondBoxIndex == dialogueIndex)
                    {
                        // Clean up line resources
                        vkDestroyBuffer(device.device,
                                        line->vertexBuffer.buffer,
                                        NULL);
                        vkFreeMemory(device.device,
                                     line->vertexBuffer.memory, NULL);

                        for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT;
                             j++)
                        {
                            vkUnmapMemory(
                                device.device,
                                line->descriptorSet.buffers[j]
                                    .memory);
                            vkDestroyBuffer(
                                device.device,
                                line->descriptorSet.buffers[j].buffer,
                                NULL);
                            vkFreeMemory(
                                device.device,
                                line->descriptorSet.buffers[j].memory,
                                NULL);
                        }

                        slb_Vector_Remove(lineObjects, i);
                    }
                }

                // Update line indices for remaining lines (shift down
                // indices that are higher than deleted box)
                for (int i = 0; i < lineObjects->size; i++)
                {
                    LineObject* line = slb_Vector_Get(lineObjects, i);

                    if (line->firstBoxIndex > dialogueIndex)
                        line->firstBoxIndex--;
                    if (line->secondBoxIndex > dialogueIndex)
                        line->secondBoxIndex--;
                }

                // Remove connections from other dialogue boxes that
                // point to this one
                for (int i = 0; i < dialogueBoxes->size; i++)
                {
                    if (i != dialogueIndex)
                    {
                        DialogueBox* box =
                            slb_Vector_Get(dialogueBoxes, i);

                        for (int j = box->connections->size - 1;
                             j >= 0; j--)
                        {
                            int* connection =
                                slb_Vector_Get(box->connections, j);

                            // Remove connection if it points to
                            // deleted box
                            if (*connection == currentDialogueBox)
                            {
                                slb_Vector_Remove(box->connections,
                                                  j);
                            }
                            // Update connection indices that are
                            // higher than deleted box
                            else if (*connection > currentDialogueBox)
                            {
                                (*connection)--;
                            }
                        }
                    }
                }

                // Remove text objects (from highest index to lowest
                // to avoid shifting issues)
                for (int i = boxToDelete->numTextObjects +
                             boxToDelete->beginningTextIndex - 1;
                     i >= boxToDelete->beginningTextIndex; i--)
                {
                    slb_Vector_Remove(textObjects, i);
                }

                // Update text indices for dialogue boxes that come
                // after this one
                for (int i = dialogueIndex + 1;
                     i < dialogueBoxes->size; i++)
                {
                    DialogueBox* laterBox =
                        slb_Vector_Get(dialogueBoxes, i);
                    laterBox->beginningTextIndex -=
                        boxToDelete->numTextObjects;
                }

                // Remove render object
                slb_Vector_Remove(renderObjects, currentDialogueBox);

                // Free dialogue box connections and remove dialogue
                // box
                slb_Vector_Free(boxToDelete->connections);
                slb_Vector_Remove(dialogueBoxes, dialogueIndex);

                // Reset current selection
                currentDialogueBox = -1;
                currentDialogueBoxObject = NULL;
                currentRenderObject = NULL;
                isDragging = false;

                // If we were in the middle of connecting, reset that
                // too
                if (isConnecting && (firstConnectionDialogueBox ==
                                     currentDialogueBox))
                {
                    isConnecting = false;
                    firstConnectionDialogueBox = -1;
                }
            }
        }

        for (int i = 1; i < renderObjects->size; i++)
        {
            RenderObject* obj = slb_Vector_Get(renderObjects, i);

            if (cursorPosition[0] >=
                    obj->position[0] - obj->scale[0] / 2 &&
                cursorPosition[0] <=
                    obj->position[0] + obj->scale[0] / 2 &&
                cursorPosition[2] >=
                    obj->position[1] - obj->scale[1] / 2 &&
                cursorPosition[2] <=
                    obj->position[1] + obj->scale[1] / 2 &&
                !mouseOverGui)
            {
                if (slb_Input_GetMouseButtonDown(
                        &window, SLB_MOUSE_BUTTON_LEFT))
                {
                    currentDialogueBox = i;
                    currentDialogueBoxObject =
                        slb_Vector_Get(dialogueBoxes, i - 1);
                    currentRenderObject =
                        slb_Vector_Get(dialogueBoxes, i);
                    isDragging = true;
                }

                if (slb_Input_GetMouseButtonUp(&window,
                                               SLB_MOUSE_BUTTON_LEFT))
                {
                    isDragging = false;
                }

                if (slb_Input_GetMouseButtonDown(
                        &window, SLB_MOUSE_BUTTON_RIGHT))
                {
                    if (!isConnecting)
                    {
                        firstConnectionDialogueBox = i;
                        isConnecting = true;
                    }
                    else
                    {
                        secondConnectionDialogueBox = i;

                        RenderObject* obj2 = slb_Vector_Get(
                            renderObjects,
                            firstConnectionDialogueBox);

                        LineObject line = CreateLineObject(
                            (vec3) {obj2->position[0], 0.0f,
                                    obj2->position[1]},
                            (vec3) {obj->position[0], 0.0f,
                                    obj->position[1]},
                            (vec3) {1.0f, 1.0f, 1.0f}, 3.0f,
                            physicalDevice, &device, &commandPool,
                            descriptorSetLayout, descriptorPool);

                        line.firstBoxIndex =
                            firstConnectionDialogueBox - 1;
                        line.secondBoxIndex =
                            secondConnectionDialogueBox - 1;

                        slb_Vector_PushBack(lineObjects, &line);

                        DialogueBox* diagBox = slb_Vector_Get(
                            dialogueBoxes,
                            firstConnectionDialogueBox - 1);

                        slb_Vector_PushBack(diagBox->connections, &i);

                        isConnecting = false;
                    }
                }
            }
        }

        // ---

        // LINE OBJECT UPDATE SYSTEM
        // ---

        for (int i = 0; i < lineObjects->size; i++)
        {
            LineObject* line = slb_Vector_Get(lineObjects, i);

            if (line->firstBoxIndex != -1 &&
                line->secondBoxIndex != -1)
            {
                RenderObject* renderObj1 = slb_Vector_Get(
                    renderObjects, line->firstBoxIndex + 1);
                RenderObject* renderObj2 = slb_Vector_Get(
                    renderObjects, line->secondBoxIndex + 1);

                // Create new vertices with updated positions
                Vertex lineVertices[2] = {
                    {{renderObj1->position[0], 0.0f,
                      renderObj1->position[1]},
                     {0.0f, 0.0f}},
                    {{renderObj2->position[0], 0.0f,
                      renderObj2->position[1]},
                     {1.0f, 0.0f}}};

                // Update the vertex buffer with new positions
                VkDeviceSize bufferSize = sizeof(lineVertices);

                // Create staging buffer
                slb_Buffer stagingBuffer = slb_Buffer_Create(
                    bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    physicalDevice, &device);

                void* data;
                vkMapMemory(device.device, stagingBuffer.memory, 0,
                            bufferSize, 0, &data);
                memcpy(data, lineVertices, bufferSize);
                vkUnmapMemory(device.device, stagingBuffer.memory);

                // Copy to line's vertex buffer
                slb_CopyBuffer(stagingBuffer.buffer,
                               line->vertexBuffer.buffer, bufferSize,
                               &device, &commandPool);

                // Clean up staging buffer
                vkDestroyBuffer(device.device, stagingBuffer.buffer,
                                NULL);
                vkFreeMemory(device.device, stagingBuffer.memory,
                             NULL);
            }
        }

        // ---

        // BOX CREATION
        // ---

        if (slb_Input_GetMouseButtonDown(&window,
                                         SLB_MOUSE_BUTTON_MIDDLE) &&
            !mouseOverGui)
        {
            CreateDialogueBox(
                "Hello world",
                (vec2) {cursorPosition[0], cursorPosition[2]}, 0.01f,
                renderObjects, textObjects, dialogueBoxes,
                physicalDevice, &device, &commandPool,
                descriptorSetLayout, descriptorPool);
        }

        // ---

        // BOX DRAGGING
        // ---

        if (isDragging)
        {
            RenderObject* obj =
                slb_Vector_Get(renderObjects, currentDialogueBox);
            DialogueBox* diagBox =
                slb_Vector_Get(dialogueBoxes, currentDialogueBox - 1);

            for (int i = diagBox->beginningTextIndex;
                 i < diagBox->numTextObjects +
                         diagBox->beginningTextIndex;
                 i++)
            {

                TextObject* text = slb_Vector_Get(textObjects, i);

                text->position[0] += mouseDifference[0];
                text->position[1] += mouseDifference[2];
            }

            obj->position[0] += mouseDifference[0];
            obj->position[1] += mouseDifference[2];
        }

        // ---

        // View matrix
        mat4 view;
        slb_Camera_GetViewMatrix(&camera, view);

        // Projection matrix
        mat4 proj;
        glm_perspective(glm_rad(45.0f),
                        swapchain.swapchainExtent.width /
                            (float)swapchain.swapchainExtent.height,
                        0.1f, 1000.0f, proj);

        proj[1][1] *= -1;
        proj[0][0] *= -1;

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
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

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

        // Render sprites
        for (int i = 0; i < renderObjects->size; i++)
        {
            RenderObject* object = slb_Vector_Get(renderObjects, i);

            VkBuffer vertexBuffers[] = {object->vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers,
                                   offsets);
            vkCmdBindIndexBuffer(commandBuffer,
                                 object->indexBuffer.buffer, 0,
                                 VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.layout, 0, 1,
                &object->descriptorSet.descriptorSets[currentFrame],
                0, NULL);

            vkCmdDrawIndexed(commandBuffer,
                             sizeof(indices) / sizeof(indices[0]), 1,
                             0, 0, 0);

            // UPDATE UNIFORM BUFFERS
            // ---

            UniformBufferObject ubo = {0};

            // Model matrix
            glm_mat4_identity(ubo.model);
            glm_translate(ubo.model,
                          (vec3) {object->position[0], 0.0f,
                                  object->position[1]});
            glm_scale(ubo.model, (vec3) {object->scale[0], 0.0f,
                                         object->scale[1]});

            glm_mat4_copy(proj, ubo.proj);
            glm_mat4_copy(view, ubo.view);

            memcpy(object->descriptorSet.buffersMap[currentFrame],
                   &ubo, sizeof(ubo));
        }

        vkCmdBindPipeline(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          textPipeline.pipeline);

        // Render text
        for (int i = 0; i < textObjects->size; i++)
        {
            TextObject* textObj = slb_Vector_Get(textObjects, i);

            VkBuffer vertexBuffers[] = {textObj->vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers,
                                   offsets);

            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.layout, 0, 1,
                &textObj->descriptorSet.descriptorSets[currentFrame],
                0, NULL);

            vkCmdDraw(commandBuffer, textObj->vertexCount, 1, 0, 0);

            // UPDATE TEXT UNIFORM BUFFERS
            UniformBufferObject textUbo = {0};

            // Model matrix for text
            glm_mat4_identity(textUbo.model);
            glm_translate(textUbo.model,
                          (vec3) {textObj->position[0], 0.01f,
                                  textObj->position[1]});

            glm_mat4_copy(proj, textUbo.proj);
            glm_mat4_copy(view, textUbo.view);

            memcpy(textObj->descriptorSet.buffersMap[currentFrame],
                   &textUbo, sizeof(textUbo));
        }

        vkCmdBindPipeline(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          linePipeline.pipeline);

        // Set line width
        vkCmdSetLineWidth(commandBuffer, 2.0f);

        // Render lines
        for (int i = 0; i < lineObjects->size; i++)
        {
            LineObject* lineObj = slb_Vector_Get(lineObjects, i);

            VkBuffer vertexBuffers[] = {lineObj->vertexBuffer.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers,
                                   offsets);

            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                linePipeline.layout, 0, 1,
                &lineObj->descriptorSet.descriptorSets[currentFrame],
                0, NULL);

            vkCmdDraw(commandBuffer, lineObj->vertexCount, 1, 0, 0);

            UniformBufferObject lineUbo = {0};
            glm_mat4_copy(lineObj->transform, lineUbo.model);
            glm_mat4_copy(proj, lineUbo.proj);
            glm_mat4_copy(view, lineUbo.view);

            memcpy(lineObj->descriptorSet.buffersMap[currentFrame],
                   &lineUbo, sizeof(lineUbo));
        }

        slb_ImGui_NewFrame();

        slb_ImGui_Begin("Inspector");

        if (currentDialogueBox != -1)
        {
            DialogueBox* box =
                slb_Vector_Get(dialogueBoxes, currentDialogueBox - 1);

            if (slb_ImGui_InputTextMultiline("Text", box->text, 1024,
                                             0))
            {
                UpdateDialogueBox(
                    currentDialogueBox - 1, renderObjects,
                    textObjects, dialogueBoxes, lineObjects,
                    physicalDevice, &device, &commandPool,
                    descriptorSetLayout, descriptorPool);
            }

            slb_ImGui_InputText("Event", box->event, 1024, 0);
        }

        slb_ImGui_End();

        if (slb_ImGui_BeginMainMenuBar())
        {
            if (slb_ImGui_BeginMenu("File"))
            {
                if (slb_ImGui_MenuItem("Save"))
                {
                    slb_Json json = slb_Json_Create();

                    for (int i = 0; i < dialogueBoxes->size; i++)
                    {
                        slb_Json j = slb_Json_Create();

                        DialogueBox* box =
                            slb_Vector_Get(dialogueBoxes, i);
                        RenderObject* obj =
                            slb_Vector_Get(renderObjects, i + 1);

                        slb_Json_SaveFloat2(j, "position",
                                            obj->position);
                        slb_Json_SaveString(j, "text", box->text);
                        slb_Json_SaveString(j, "event", box->event);

                        slb_Json_CreateIntArray(j, "connections");
                        slb_Json_SaveIntArray(
                            j, "connections",
                            (int*)box->connections->data,
                            box->connections->size);

                        slb_Json_PushBack(json, j);

                        slb_Json_Destroy(j);
                    }

                    slb_Json_SaveToFile(json, "untitled.diagsv");

                    slb_Json_Destroy(json);
                }
                if (slb_ImGui_MenuItem("Load"))
                {
                    LoadDialogueBoxes(
                        "untitled.diagsv", renderObjects, textObjects,
                        dialogueBoxes, lineObjects, physicalDevice,
                        &device, &commandPool, descriptorSetLayout,
                        descriptorPool);
                }
                if (slb_ImGui_MenuItem("Export"))
                {
                    slb_Json json = slb_Json_Create();

                    for (int i = 0; i < dialogueBoxes->size; i++)
                    {
                        slb_Json j = slb_Json_Create();

                        DialogueBox* box =
                            slb_Vector_Get(dialogueBoxes, i);
                        RenderObject* obj =
                            slb_Vector_Get(renderObjects, i + 1);

                        slb_Json_SaveString(j, "text", box->text);
                        slb_Json_SaveString(j, "event", box->event);

                        slb_Json_CreateIntArray(j, "connections");
                        slb_Json_SaveIntArray(
                            j, "connections",
                            (int*)box->connections->data,
                            box->connections->size);

                        slb_Json_PushBack(json, j);

                        slb_Json_Destroy(j);
                    }

                    slb_Json_SaveToFile(json, "untitled.diag");

                    slb_Json_Destroy(json);
                }

                slb_ImGui_EndMenu();
            }

            if (slb_ImGui_BeginMenu("Help"))
            {
                if (slb_ImGui_MenuItem("Manual"))
                {
                    manualWindow = true;
                }

                slb_ImGui_EndMenu();
            }

            slb_ImGui_EndMainMenuBar();
        }

        if (manualWindow)
        {
            slb_ImGui_BeginFlag("Manual", &manualWindow);

            slb_ImGui_TextLong(
                "DIAGMAKER MANUAL:\n\nDiagmaker is an application "
                "which allows you make dialogue trees. \nYou can "
                "make "
                "a dialogue node by pressing middle click, \nyou can "
                "move these nodes around by dragging them with left "
                "click. \nYou can connect these nodes up to one "
                "another by pressing a node with right click,\nand "
                "then pressing right click on the one you want to "
                "connect it to.\nIf you left click a node, you will "
                "select it and will be able to see it in the "
                "inspector. \nEach node has two properties, text and "
                "an event. \nYou can modify both within the "
                "inspector.\n"
                "The event is not shown in the program but only in "
                "the inspector.\nIf you want to delete a node, then "
                "select it and press delete.");

            slb_ImGui_End();
        }

        slb_ImGui_EndFrame(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            slb_Error("Failed to record command buffer",
                      slb_ErrorType_Error);
        }

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

        slb_Camera_CursorToWorld(&camera, mousePosition[0],
                                 mousePosition[1], 1600, 900, proj,
                                 view, cursorPosition);

        glm_vec3_sub(cursorPosition, prevMousePosition,
                     mouseDifference);

        glm_vec2_copy(
            (vec2) {cursorPosition[0], cursorPosition[2]},
            ((RenderObject*)slb_Vector_Get(renderObjects, 0))
                ->position);

        glm_vec3_copy(cursorPosition, prevMousePosition);

        ControlCamera(&camera, &window, deltaTime * 15.0f);

        slb_Window_Update(&window);
    }

    // Cleanup
    vkDeviceWaitIdle(device.device);

    // Destroy text objects
    for (int i = 0; i < textObjects->size; i++)
    {
        TextObject* textObj = slb_Vector_Get(textObjects, i);
        vkDestroyBuffer(device.device, textObj->vertexBuffer.buffer,
                        NULL);
        vkFreeMemory(device.device, textObj->vertexBuffer.memory,
                     NULL);

        for (size_t j = 0; j < SLB_FRAMES_IN_FLIGHT; j++)
        {
            vkUnmapMemory(device.device,
                          textObj->descriptorSet.buffers[j].memory);
            vkDestroyBuffer(device.device,
                            textObj->descriptorSet.buffers[j].buffer,
                            NULL);
            vkFreeMemory(device.device,
                         textObj->descriptorSet.buffers[j].memory,
                         NULL);
        }
    }

    // Cleanup font atlas
    vkDestroyImageView(device.device, fontAtlas.imageView, NULL);
    vkDestroySampler(device.device, fontAtlas.sampler, NULL);
    vkDestroyImage(device.device, fontAtlas.image, NULL);
    vkFreeMemory(device.device, fontAtlas.memory, NULL);

    slb_Vector_Free(renderObjects);
    slb_Vector_Free(textObjects);

    return 0;
}
