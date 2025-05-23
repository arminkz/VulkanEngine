#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Scene.h"
#include "Pipeline.h"


class Model
{
public:
    Model(std::shared_ptr<VulkanContext> ctx, std::string name, std::shared_ptr<DeviceMesh> mesh);
    ~Model();

    const std::string& getName() const { return _name; }
    glm::mat4 getModelMatrix() const { return _modelMatrix; }

    virtual void draw(VkCommandBuffer commandBuffer, const Scene& scene) = 0;
    void setPipeline(std::weak_ptr<Pipeline> pipeline) { _pipeline = std::move(pipeline); }

protected:
    std::shared_ptr<VulkanContext> _ctx;
    std::string _name;
    std::shared_ptr<DeviceMesh> _mesh;
    std::weak_ptr<Pipeline> _pipeline;

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