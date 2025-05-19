#pragma once
#include "stdafx.h"
#include "Model.h"
#include "Texture2D.h"
#include "../interface/ISelectable.h"
#include "Pipeline.h"
#include "DescriptorSet.h"
#include "Scene.h"

class Planet : public Model, public ISelectable
{
public:
    Planet(std::shared_ptr<VulkanContext> ctx, 
           std::string name, 
           std::shared_ptr<DeviceMesh> mesh, 
           std::shared_ptr<Texture2D> baseColorTexture,
           std::shared_ptr<Pipeline> pipeline,
           std::shared_ptr<Pipeline> selectionPipeline);
           
    ~Planet();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

private:
    // Planet-specific data
    std::shared_ptr<Texture2D> _baseColorTexture;
    std::shared_ptr<Pipeline> _pipeline;
    std::shared_ptr<Pipeline> _selectionPipeline;
    std::unique_ptr<DescriptorSet> _descriptorSet;
};