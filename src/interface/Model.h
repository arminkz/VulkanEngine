#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Scene.h"
#include "Pipeline.h"

class Scene;

class Model
{
public:
    const std::string& getName() const { return _name; }
    glm::mat4 getModelMatrix() const { return _modelMatrix; }
    glm::vec3 getPosition() const { return glm::vec3(_modelMatrix[3]); }
    const DeviceMesh* getDeviceMesh() const { return _mesh.get(); }

    virtual void draw(VkCommandBuffer commandBuffer, const Scene& scene) = 0;
    void setPipeline(std::shared_ptr<Pipeline> pipeline) { _pipeline = std::move(pipeline); }

protected:
    Model(std::shared_ptr<VulkanContext> ctx, std::string name, std::shared_ptr<DeviceMesh> mesh);
    virtual ~Model() {};

    std::shared_ptr<VulkanContext> _ctx;
    std::string _name;
    std::shared_ptr<DeviceMesh> _mesh;
    std::weak_ptr<Pipeline> _pipeline;

    glm::mat4 _modelMatrix;
};