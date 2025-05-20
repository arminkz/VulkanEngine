#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "UniformBuffer.h"
#include "DescriptorSet.h"
#include "SwapChain.h"


class Scene
{
public:
    Scene(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<SwapChain> swapchain);
    ~Scene();

    // Child classes should implement this method to create their own scene
    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;
    virtual void update();

    // Invalidate (Renderer informs the scene that the swapchain has been recreated)
    void setSwapChain(std::shared_ptr<SwapChain> swapchain) { _swapChain = std::move(swapchain); }

    // Get Scene Descriptor Set
    const DescriptorSet* getSceneDescriptorSet() const { return _sceneDescriptorSets[_currentFrame].get(); }

private:
    std::shared_ptr<VulkanContext> _ctx;
    std::shared_ptr<SwapChain> _swapChain;

    // Camera
    std::unique_ptr<Camera> _camera = nullptr;

    // Scene information (Global information that we need to pass to the shader)
    struct SceneInfo {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;

        alignas(4) float time;
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) glm::vec3 lightColor;
    } _sceneInfo;
    std::array<std::unique_ptr<UniformBuffer<SceneInfo>>, MAX_FRAMES_IN_FLIGHT> _sceneInfoUBOs;
    std::array<std::unique_ptr<DescriptorSet>, MAX_FRAMES_IN_FLIGHT> _sceneDescriptorSets;

    // Current frame index
    uint32_t _currentFrame = 0;
};
