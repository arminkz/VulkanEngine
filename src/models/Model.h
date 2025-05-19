#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Scene.h"


class Model
{
public:
    Model(std::shared_ptr<VulkanContext> ctx, std::string name, std::shared_ptr<DeviceMesh> mesh);
    ~Model();

    const std::string& getName() const { return _name; }

    virtual void draw(VkCommandBuffer commandBuffer, const Scene& scene) = 0;

protected:
    std::shared_ptr<VulkanContext> _ctx;
    std::string _name;
    std::shared_ptr<DeviceMesh> _mesh;
    glm::mat4 _modelMatrix;
};


Model::Model(std::shared_ptr<VulkanContext> ctx, std::string name, std::shared_ptr<DeviceMesh> mesh)
    : _ctx(std::move(ctx)), _name(std::move(name)), _mesh(std::move(mesh))
{
    // Initialize model matrix to identity
    _modelMatrix = glm::mat4(1.0f);
}

Model::~Model()
{
    // Cleanup resources if needed
}