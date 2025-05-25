#pragma once

#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Scene.h"
#include "Pipeline.h"
#include "interface/Model.h"
#include "interface/SelectableModel.h"

class Sun : public SelectableModel
{
public:
    Sun(std::shared_ptr<VulkanContext> ctx, 
        std::string name, 
        std::shared_ptr<DeviceMesh> mesh,
        float planetSize);

    ~Sun();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

    void calculateModelMatrix();

    // Used in Bloom effect
    const glm::vec3 glowColor = glm::vec3(1.f, 0.3f, 0.0f);

protected:

    float _size = 1.0f;
};