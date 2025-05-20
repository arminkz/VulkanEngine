#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Model.h"
#include "Scene.h"
#include "interface/ISelectable.h"
#include "Pipeline.h"


class Sun : public Model, public ISelectable
{
public:
    Sun(std::shared_ptr<VulkanContext> ctx, 
        std::string name, 
        std::shared_ptr<DeviceMesh> mesh, 
        std::shared_ptr<Pipeline> pipeline,
        std::shared_ptr<Pipeline> selectionPipeline);

    ~Sun();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

private:
    std::shared_ptr<Pipeline> _pipeline;
    std::shared_ptr<Pipeline> _selectionPipeline;
};