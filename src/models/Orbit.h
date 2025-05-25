#pragma once

#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Scene.h"
#include "Pipeline.h"
#include "interface/Model.h"


class Orbit : public Model
{
public:
    Orbit(std::shared_ptr<VulkanContext> ctx, 
          std::string name, 
          std::shared_ptr<DeviceMesh> mesh,
          std::weak_ptr<Model> parent,
          float orbitSize);

    ~Orbit();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

    void calculateModelMatrix();

protected:

    std::weak_ptr<Model> _parent; // Weak pointer to parent planet (if any)
    
    float _orbitSize = 1.0f;           // Used for scaling the model
};