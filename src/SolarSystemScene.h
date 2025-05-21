#pragma once 
#include "stdafx.h"
#include "VulkanContext.h"
#include "Scene.h"
#include "Renderer.h"
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
    void update() override;

private:

    VkRenderPass _renderPass;
    VkRenderPass _offscreenRenderPass;
    VkRenderPass _offscreenRenderPassMSAA;
    VkRenderPass _objectSelectionRenderPass;
    void createRenderPasses();

    std::shared_ptr<Pipeline> _planetPipeline;
    std::shared_ptr<Pipeline> _orbitPipeline;
    std::shared_ptr<Pipeline> _glowSpherePipeline;
    std::shared_ptr<Pipeline> _skyBoxPipeline;
    std::shared_ptr<Pipeline> _sunPipeline;
    std::shared_ptr<Pipeline> _earthPipeline;
    std::unique_ptr<Pipeline> _objectSelectionPipeline;
    void createPipelines();
    void connectPipelines();

    std::vector<std::shared_ptr<Planet>> _planets;
    std::vector<std::unique_ptr<Orbit>> _orbits;
    std::vector<std::unique_ptr<GlowSphere>> _glowSpheres;
    std::unique_ptr<SkyBox> _skyBox;
    std::shared_ptr<Sun> _sun;
    std::shared_ptr<Earth> _earth;
    void createModels();


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
