#pragma once
#include "stdafx.h"
#include "geometry/DeviceMesh.h"
#include "DeviceTexture.h"
#include "VulkanContext.h"

// Model on GPU
class DeviceModel
{
public:
    DeviceModel(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<DeviceMesh> mesh, glm::vec3 color);
    DeviceModel(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<DeviceMesh> mesh, std::shared_ptr<DeviceTexture> texture);

    ~DeviceModel();

private:
    std::shared_ptr<VulkanContext> _ctx;
    
    std::shared_ptr<DeviceMesh> _mesh;
    std::shared_ptr<DeviceTexture> _texture;
    glm::vec3 _color;
    glm::mat4 _modelMatrix;
};