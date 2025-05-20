#pragma once 
#include "stdafx.h"
#include "VulkanContext.h"
#include "Scene.h"
#include "models/Planet.h"
#include "models/Sun.h"
#include "models/Earth.h"
#include "models/Orbit.h"
#include "models/GlowSphere.h"


class SolarSystemScene : public Scene
{
public:
    SolarSystemScene(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<SwapChain> swapchain);
    ~SolarSystemScene();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void update() override;

private:

    std::vector<std::unique_ptr<Planet>> _planets;
    std::vector<std::unique_ptr<Orbit>> _orbits;
    std::vector<std::unique_ptr<GlowSphere>> _glowSpheres;

    std::unique_ptr<Sun> _sun;
    std::unique_ptr<Earth> _earth;

};
