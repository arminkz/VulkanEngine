#pragma once

#include "stdafx.h"
#include "interface/Model.h"
#include "interface/SelectableModel.h"
#include "Texture2D.h"
#include "Pipeline.h"
#include "DescriptorSet.h"
#include "Scene.h"

class Scene;
class SolarSystemScene;

class Planet : public SelectableModel
{
public:
    Planet(std::shared_ptr<VulkanContext> ctx, 
           std::string name, 
           std::shared_ptr<DeviceMesh> mesh,
           std::shared_ptr<Texture2D> baseColorTexture,
           std::weak_ptr<Model> parent,
           float planetSize,
           float orbitRadius);
           
    ~Planet();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

    void calculateModelMatrix();

protected:
    std::weak_ptr<Model> _parent;             //Weak pointer to parent planet (if any)

    float _size = 1.0f;                        //Used for scaling the model
    float _orbitRadius = 0.0f;                 
    float _orbitAngle = 0.0f; 

    std::shared_ptr<Texture2D> _baseColorTexture;
    std::unique_ptr<DescriptorSet> _descriptorSet;

};