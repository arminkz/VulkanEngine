#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "UniformBuffer.h"
#include "DescriptorSet.h"


class AtmosphereModel
{
public:
    AtmosphereModel(
        std::shared_ptr<VulkanContext> ctx,
        std::shared_ptr<DeviceMesh> mesh,
        glm::mat4 modelMatrix,
        glm::vec3 color,
        float coeffScatter = 3.0f,
        float powScatter = 3.0f,
        bool isLightSource = false
    );

    ~AtmosphereModel();

    const DeviceMesh* getDeviceMesh() const { return _mesh.get(); }

    glm::mat4 getModelMatrix() const { return _modelMatrix; }
    void setModelMatrix(glm::mat4 modelMatrix) { _modelMatrix = modelMatrix; }

    glm::vec3 getCenter() const { return _center; }
    void setCenter(glm::vec3 center) { _center = center; }

    // UBO passed to the fragment shader
    struct AtmosphereInfo {
        alignas(16) glm::vec3 color;
        alignas(4) float coeffScatter = 3.0f;
        alignas(4) float powScatter = 3.0f;
        alignas(4) int isLightSource = 0;
    };

    const UniformBuffer<AtmosphereInfo>* getAtmosphereInfoUBO() const { return _atmosphereUBO.get(); }
    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

private:
    std::shared_ptr<VulkanContext> _ctx;
    
    std::shared_ptr<DeviceMesh> _mesh;
    glm::mat4 _modelMatrix;
    glm::vec3 _color;

    glm::vec3 _center;

    std::unique_ptr<UniformBuffer<AtmosphereInfo>> _atmosphereUBO;
    std::unique_ptr<DescriptorSet> _descriptorSet;
};