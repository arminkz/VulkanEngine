#pragma once 
#include "stdafx.h"
#include "VulkanContext.h"
#include "Scene.h"
#include "Renderer.h"
#include "Pipeline.h"
#include "FrameBuffer.h"
#include "RenderPass.h"
#include "models/Planet.h"
#include "models/Sun.h"
#include "models/Earth.h"
#include "models/Orbit.h"
#include "models/GlowSphere.h"
#include "models/SkyBox.h"


class SolarSystemScene : public Scene
{
public:
    SolarSystemScene(std::shared_ptr<VulkanContext> ctx, Renderer& renderer);
    ~SolarSystemScene();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void update(uint32_t currentImage) override;

private:
    // Rnder passes
    std::unique_ptr<RenderPass> _renderPass;
    std::unique_ptr<RenderPass> _offscreenRenderPass;
    std::unique_ptr<RenderPass> _offscreenRenderPassMSAA;
    std::unique_ptr<RenderPass> _objectSelectionRenderPass;
    void createRenderPasses();

    // Framebuffers
    std::vector<std::unique_ptr<FrameBuffer>> _mainFrameBuffers;
    std::vector<std::unique_ptr<FrameBuffer>> _offscreenFrameBuffers;
    std::unique_ptr<FrameBuffer> _objectSelectionFrameBuffer;
    void createFrameBuffers();

    // Pipelines
    std::shared_ptr<Pipeline> _planetPipeline;
    std::shared_ptr<Pipeline> _orbitPipeline;
    std::shared_ptr<Pipeline> _glowSpherePipeline;
    std::shared_ptr<Pipeline> _skyBoxPipeline;
    std::shared_ptr<Pipeline> _sunPipeline;
    std::shared_ptr<Pipeline> _earthPipeline;

    std::unique_ptr<Pipeline> _glowPipeline;
    std::unique_ptr<Pipeline> _blurVertPipeline;
    std::unique_ptr<Pipeline> _blurHorizPipeline;
    std::unique_ptr<Pipeline> _compositePipeline;
    std::unique_ptr<Pipeline> _objectSelectionPipeline;
    void createPipelines();
    void connectPipelines();

    // Drawables (Models)
    std::vector<std::shared_ptr<Planet>> _planets;
    std::vector<std::unique_ptr<Orbit>> _orbits;
    std::vector<std::unique_ptr<GlowSphere>> _glowSpheres;
    std::unique_ptr<SkyBox> _skyBox;
    std::shared_ptr<Sun> _sun;
    std::shared_ptr<Earth> _earth;
    void createModels();


    // Texture Sampler for intermediate passes
    std::unique_ptr<TextureSampler> _ppTextureSampler;

    // Glow pass
    struct GlowPassPushConstants {
        alignas(16) glm::mat4 model;
        alignas(16) glm::vec3 glowColor;
    };
    
    // Blur pass
    struct BlurSettings {
        float blurScale = 3.0f;
        float blurStrength = 1.f;
    } blurSettings;
    std::unique_ptr<UniformBuffer<BlurSettings>> _blurSettingsUBO;
    std::unique_ptr<DescriptorSet> _blurVertDescriptorSet;
    std::unique_ptr<DescriptorSet> _blurHorizDescriptorSet;

    // Composite pass
    std::unique_ptr<DescriptorSet> _compositeDescriptorSet;


    // Planet Sizes
    const float sizeSun = 3.f;
    const float sizeMercury = 0.21f;
    const float sizeVenus = 0.48f;
    const float sizeEarth = 0.54f;
    const float sizeMars = 0.36f;
    const float sizeJupiter = 0.8f;
    const float sizeSaturn = 0.7f;
    const float sizeSaturnRing = 1.2f * sizeSaturn;
    const float sizeUranus = 0.4f;
    const float sizeNeptune = 0.4f;
    const float sizePluto = 0.2f;

    // Orbit Radii
    const float orbitRadMercury = 10.f;
    const float orbitRadVenus = 13.f;
    const float orbitRadEarth = 16.f;
    const float orbitRadMars = 22.f;
    const float orbitRadJupiter = 47.f;
    const float orbitRadSaturn = 77.f;
    const float orbitRadUranus = 119.f;
    const float orbitRadNeptune = 139.f;
    const float orbitRadPluto = 155.f;
};
