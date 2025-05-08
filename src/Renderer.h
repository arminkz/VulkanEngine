#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "Camera.h"
#include "Pipeline.h"
#include "DeviceModel.h"
#include "AtmosphereModel.h"
#include "TextureSampler.h"
#include "DescriptorSet.h"
#include "GUI.h"
#include "SwapChain.h"

class Renderer {

private:
    std::shared_ptr<VulkanContext> _ctx;

    // Swapchain
    std::unique_ptr<SwapChain> _swapChain;

    // GUI (ImGui)
    std::unique_ptr<GUI> _gui = nullptr;

    std::unique_ptr<Camera> _camera = nullptr;
    uint32_t _currentTargetObjectID;
    std::unordered_map<int, std::shared_ptr<DeviceModel>> _selectableObjects;

    std::shared_ptr<TextureSampler> _textureSampler;

    // Global information that we need to pass to the shader
    struct SceneInfo {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;

        alignas(4) float time;
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) glm::vec3 lightColor;
    } _sceneInfo;
    std::array<std::unique_ptr<UniformBuffer<SceneInfo>>, MAX_FRAMES_IN_FLIGHT> _sceneInfoUBOs;
    std::array<std::unique_ptr<DescriptorSet>, MAX_FRAMES_IN_FLIGHT> _sceneDescriptorSets;

    struct PushConstants {
        alignas(16) glm::mat4 model;
        alignas(4) uint32_t objectID;
    } _pushConstants;

    // Models
    std::shared_ptr<DeviceModel> _sunModel = nullptr;
    std::vector<std::shared_ptr<DeviceModel>> _planetModels;
    std::vector<std::shared_ptr<DeviceModel>> _orbitModels;
    std::vector<std::shared_ptr<AtmosphereModel>> _atmosphereModels;

    // Main render resources
    std::unique_ptr<Pipeline> _pipeline;
    std::unique_ptr<Pipeline> _sunPipeline;
    std::unique_ptr<Pipeline> _orbitPipeline;
    std::unique_ptr<Pipeline> _atmospherePipeline;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;
    void createColorResources(); //A1
    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;
    void createDepthResources(); //A2
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    void createFramebuffers();   //A3
    VkRenderPass _renderPass;
    void createRenderPass();
    //TODO: merge A1, A2, A3 into one function
    void recreateRenderResources();
    void cleanupRenderResources(); // since this is only used in recreating, we can just merge it in the recreate function


    // Object selection rendering (has no swapchain, offscreen rendering)
    std::unique_ptr<Pipeline> _objectSelectionPipeline;

    // Object selection rendering (offscreen)
    VkImage _objectSelectionColorImage;
    VkDeviceMemory _objectSelectionColorImageMemory;
    VkImageView _objectSelectionColorImageView;
    void createObjectSelectionColorResources(); //B1
    VkImage _objectSelectionDepthImage;
    VkDeviceMemory _objectSelectionDepthImageMemory;
    VkImageView _objectSelectionDepthImageView;
    void createObjectSelectionDepthResources(); //B2
    VkFramebuffer _objectSelectionFramebuffer;
    void createObjectSelectionFramebuffer(); //B3
    VkRenderPass _objectSelectionRenderPass;
    void createObjectSelectionRenderPass();
    //TODO: merge B1, B2, B3 into one function
    void recreateObjectSelectionResources(); 
    void cleanupObjectSelectionResources(); // since this is only used in recreating, we can just merge it in the recreate function


    void invalidate(); // Called when the window is resized


    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;

    void updateUniformBuffer(uint32_t currentImage);
    
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool createSyncObjects();

    uint32_t querySelectionImage(float mouseX, float mouseY);

public:
    Renderer(std::shared_ptr<VulkanContext> ctx);
    ~Renderer();

    bool initialize();
    void drawFrame();
    void handleMouseClick(float mouseX, float mouseY);
    void informFramebufferResized() { _framebufferResized = true; };

    Camera* getCamera() { return _camera.get(); };
    GUI* getGUI() { return _gui.get(); };
};