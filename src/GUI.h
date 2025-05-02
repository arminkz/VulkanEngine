#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "DeviceTexture.h"
#include "TextureSampler.h"
#include "DescriptorSet.h"
#include "Pipeline.h"

class GUI
{
public:
    GUI(std::shared_ptr<VulkanContext> ctx);
    ~GUI();

    void init(float width, float height);
    void initResources(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);

    void newFrame();

    void updateBuffers();

    void drawFrame(VkCommandBuffer commandBuffer);

    bool handleEvents(SDL_Event* event);
    bool isCapturingEvent();

private:
    std::shared_ptr<VulkanContext> _ctx;

    std::unique_ptr<DeviceTexture> _fontTexture = nullptr;
    std::unique_ptr<TextureSampler> _textureSampler = nullptr;
    std::unique_ptr<DescriptorSet> _descriptorSet = nullptr;
    std::unique_ptr<Pipeline> _pipeline = nullptr;

    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
    void* _vertexBufferMapped = nullptr;
    uint32_t _vertexCount = 0;

    VkBuffer _indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _indexBufferMemory = VK_NULL_HANDLE;
    void* _indexBufferMapped = nullptr;
    uint32_t _indexCount = 0;

    float _scaleFactor = 1.f; // Scale factor for UI elements

    ImGuiStyle vulkanStyle; // Style for ImGui

    // UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;
};