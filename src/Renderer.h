#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "Camera.h"
#include "Pipeline.h"
#include "DeviceModel.h"
#include "AtmosphereModel.h"
#include "DescriptorSet.h"
#include "SwapChain.h"
#include "FrameBuffer.h"

class Renderer {

public:
    Renderer(std::shared_ptr<VulkanContext> ctx);
    ~Renderer();

    bool initialize();
    void drawFrame();
    void handleMouseClick(float mouseX, float mouseY);
    void informFramebufferResized() { _framebufferResized = true; };

    SwapChain* getSwapChain() { return _swapChain.get(); };
    VkSampleCountFlagBits getMSAASamples() { return _msaaSamples; };

    Camera* getCamera() { return _camera.get(); };
    GUI* getGUI() { return _gui.get(); };
    

private:
    std::shared_ptr<VulkanContext> _ctx;

    // Swapchain
    std::shared_ptr<SwapChain> _swapChain;

    // Calculated constants
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    // GUI (ImGui)
    std::unique_ptr<GUI> _gui = nullptr;


    std::unique_ptr<Camera> _camera = nullptr;
    uint32_t _currentTargetObjectID;

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
    std::vector<std::shared_ptr<DeviceModel>> _allModels;                     // Planets + Sun + Rings
    std::vector<std::shared_ptr<DeviceModel>> _planetModels;                  // Planets only
    std::vector<std::shared_ptr<DeviceModel>> _orbitModels;                   // Orbits
    std::vector<std::shared_ptr<AtmosphereModel>> _atmosphereModels;
    std::unordered_map<int, std::shared_ptr<DeviceModel>> _selectableObjects; // Selectable objects

    // Models that need special treatment
    std::shared_ptr<DeviceModel> _skyBoxModel = nullptr;
    std::shared_ptr<DeviceModel> _sunModel = nullptr;
    std::shared_ptr<AtmosphereModel> _sunGlowModel = nullptr;

    // Called when the window is resized
    void invalidate();


    // Main render resources
    std::unique_ptr<Pipeline> _pipeline;
    std::unique_ptr<Pipeline> _sunPipeline;
    std::unique_ptr<Pipeline> _orbitPipeline;
    std::unique_ptr<Pipeline> _atmospherePipeline;

    VkRenderPass _renderPass;
    void createMainRenderPass();

    std::vector<std::unique_ptr<FrameBuffer>> _mainFrameBuffers;
    void createMainFrameBuffers();

    void recreateMainRenderResources();


    // Object selection rendering (has no swapchain, offscreen rendering)
    std::unique_ptr<Pipeline> _objectSelectionPipeline;

    VkRenderPass _objectSelectionRenderPass;
    void createObjectSelectionRenderPass();

    std::unique_ptr<FrameBuffer> _objectSelectionFrameBuffer;
    void createObjectSelectionFrameBuffer();

    void recreateObjectSelectionResources(); 
    
    
    // Glow pass
    struct GlowPassPushConstants {
        alignas(16) glm::mat4 model;
        alignas(16) glm::vec3 glowColor;
    } _glowPushConstants;
    std::unique_ptr<Pipeline> _glowPipeline;
    //std::vector<std::shared_ptr<DeviceModel>> _glowModels;


    // Blur pass
    struct BlurSettings {
        float blurScale = 3.0f;
        float blurStrength = 1.f;
    } blurSettings;
    std::unique_ptr<UniformBuffer<BlurSettings>> _blurSettingsUBO;
    std::unique_ptr<Pipeline> _blurVertPipeline;
    std::unique_ptr<Pipeline> _blurHorizPipeline;
    std::unique_ptr<DescriptorSet> _blurVertDescriptorSet;
    std::unique_ptr<DescriptorSet> _blurHorizDescriptorSet;

    std::unique_ptr<Pipeline> _compositePipeline;
    std::unique_ptr<DescriptorSet> _compositeDescriptorSet;

    VkRenderPass _offscreenRenderPass;
    VkRenderPass _offscreenRenderPassMSAA;
    void createOffscreenRenderPasses();
    std::vector<std::unique_ptr<FrameBuffer>> _offscreenFrameBuffers;
    void createOffscreenFrameBuffers();



    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;

    void updateUniformBuffer(uint32_t currentImage);
    
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void recordBloomCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool createSyncObjects();

    uint32_t querySelectionImage(float mouseX, float mouseY);


};