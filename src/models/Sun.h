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
        float planetSize);

    ~Sun();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

    void calculateModelMatrix();

protected:

    float _size = 1.0f;
};