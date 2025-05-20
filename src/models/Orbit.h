#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "Model.h"
#include "Scene.h"
#include "geometry/DeviceMesh.h"
#include "Pipeline.h"


class Orbit : public Model
{
public:
    Orbit(std::shared_ptr<VulkanContext> ctx, 
          std::string name, 
          std::shared_ptr<DeviceMesh> mesh, 
          std::shared_ptr<Pipeline> pipeline);

    ~Orbit();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

private:
    std::shared_ptr<Pipeline> _pipeline;
    std::shared_ptr<Pipeline> _selectionPipeline;
};