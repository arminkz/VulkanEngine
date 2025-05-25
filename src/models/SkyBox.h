#pragma once

#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "interface/Model.h"
#include "Scene.h"
#include "Pipeline.h"
#include "TextureCubemap.h"

class SkyBox : public Model
{
public:
    SkyBox(std::shared_ptr<VulkanContext> ctx, 
           std::string name, 
           std::shared_ptr<DeviceMesh> mesh, 
           std::shared_ptr<TextureCubemap> cubemapTexture);

    ~SkyBox();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

private:
    std::shared_ptr<TextureCubemap> _cubemapTexture;

    std::unique_ptr<DescriptorSet> _descriptorSet;
};