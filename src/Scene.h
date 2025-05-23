#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "UniformBuffer.h"
#include "DescriptorSet.h"
#include "Renderer.h"


class Scene
{
public:
    Scene(std::shared_ptr<VulkanContext> ctx, Renderer& renderer);
    ~Scene();

    // Child classes should implement this method to create their own scene
    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;

    // Update the scene (called every frame before drawing) (0 < currentImage < MAX_FRAMES_IN_FLIGHT)
    virtual void update(uint32_t currentImage);

    // Invalidate (Renderer informs the scene that the swapchain has been recreated)
    //void setSwapChain(std::shared_ptr<SwapChain> swapchain) { _swapChain = std::move(swapchain); }

    // Get Scene Descriptor Set
    const DescriptorSet* getSceneDescriptorSet() const { return _sceneDescriptorSets[_currentFrame].get(); }

protected:
    std::shared_ptr<VulkanContext> _ctx;
    Renderer& _renderer;

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

private:
    // Current frame index
    uint32_t _currentFrame = 0;
};
