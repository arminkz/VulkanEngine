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
    Scene(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<SwapChain> swapChain);
    ~Scene();

    // Update the scene (called every frame before drawing) (0 < currentImage < MAX_FRAMES_IN_FLIGHT)
    virtual void update(uint32_t currentImage);

    // Child classes should implement this method to create their own scene
    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;

    // Handle mouse click events
    virtual void handleMouseClick(float mouseX, float mouseY) = 0;
    virtual void handleMouseDrag(float dx, float dy) = 0;
    virtual void handleMouseWheel(float dy) = 0;

protected:
    std::shared_ptr<VulkanContext> _ctx;

    std::shared_ptr<SwapChain> _swapChain;

    // Current frame index
    uint32_t _currentFrame = 0;
};
