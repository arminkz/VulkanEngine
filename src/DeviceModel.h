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
    DeviceModel(
        std::shared_ptr<VulkanContext> ctx, 
        std::shared_ptr<DeviceMesh> mesh,
        std::shared_ptr<DeviceTexture> baseColorTexture = nullptr,
        std::shared_ptr<DeviceTexture> unlitColorTexture = nullptr,
        std::shared_ptr<DeviceTexture> normalMapTexture = nullptr,
        std::shared_ptr<DeviceTexture> specularTexture = nullptr,
        std::shared_ptr<DeviceTexture> overlayColorTexture = nullptr
    );
    
    ~DeviceModel();

    const DeviceMesh* getDeviceMesh() const { return _mesh.get(); }

    const DeviceTexture* getBaseColorTexture() const { return _baseColorTexture.get(); }
    const DeviceTexture* getUnlitColorTexture() const { return _unlitColorTexture.get(); }
    const DeviceTexture* getNormalMapTexture() const { return _normalMapTexture.get(); }
    const DeviceTexture* getSpecularTexture() const { return _specularTexture.get(); }
    const DeviceTexture* getOverlayColorTexture() const { return _overlayColorTexture.get(); }

private:
    std::shared_ptr<VulkanContext> _ctx;
    
    std::shared_ptr<DeviceMesh> _mesh;

    std::shared_ptr<DeviceTexture> _baseColorTexture;
    std::shared_ptr<DeviceTexture> _unlitColorTexture;
    std::shared_ptr<DeviceTexture> _normalMapTexture;
    std::shared_ptr<DeviceTexture> _specularTexture;
    std::shared_ptr<DeviceTexture> _overlayColorTexture;

    glm::vec3 _color;
    glm::mat4 _modelMatrix;
};