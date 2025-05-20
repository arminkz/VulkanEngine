#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Model.h"
#include "Scene.h"
#include "Pipeline.h"


class GlowSphere : public Model
{
public:
    GlowSphere(std::shared_ptr<VulkanContext> ctx, 
               std::string name, 
               std::shared_ptr<DeviceMesh> mesh, 
               std::shared_ptr<Pipeline> pipeline);

    ~GlowSphere();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

private:
    std::shared_ptr<Pipeline> _pipeline;
};