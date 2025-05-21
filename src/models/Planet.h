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
           std::shared_ptr<Texture2D> baseColorTexture);
           
    ~Planet();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

private:
    // Planet-specific data
    std::shared_ptr<Texture2D> _baseColorTexture;

    std::unique_ptr<DescriptorSet> _descriptorSet;
};