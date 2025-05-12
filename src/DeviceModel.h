#pragma once
#include "stdafx.h"
#include "geometry/DeviceMesh.h"
#include "DeviceTexture.h"
#include "TextureSampler.h"
#include "VulkanContext.h"
#include "UniformBuffer.h"
#include "DescriptorSet.h"

// Model on GPU
class DeviceModel
{
public:
    DeviceModel(
        std::string name,
        std::shared_ptr<VulkanContext> ctx, 
        std::shared_ptr<DeviceMesh> mesh,
        glm::mat4 modelMatrix,
        std::shared_ptr<DeviceTexture> baseColorTexture,
        std::shared_ptr<DeviceTexture> unlitColorTexture,
        std::shared_ptr<DeviceTexture> normalMapTexture,
        std::shared_ptr<DeviceTexture> specularTexture,
        std::shared_ptr<DeviceTexture> overlayColorTexture,
        std::shared_ptr<TextureSampler> textureSampler
    );
    
    ~DeviceModel();

    const int getID() const { return _objectID; }

    const DeviceMesh* getDeviceMesh() const { return _mesh.get(); }

    const DeviceTexture* getBaseColorTexture() const { return _baseColorTexture.get(); }
    const DeviceTexture* getUnlitColorTexture() const { return _unlitColorTexture.get(); }
    const DeviceTexture* getNormalMapTexture() const { return _normalMapTexture.get(); }
    const DeviceTexture* getSpecularTexture() const { return _specularTexture.get(); }
    const DeviceTexture* getOverlayColorTexture() const { return _overlayColorTexture.get(); }

    glm::mat4 getModelMatrix() const { return _modelMatrix; }
    void setModelMatrix(glm::mat4 modelMatrix) { _modelMatrix = modelMatrix; }
    glm::vec3 getPosition() const { return glm::vec3(_modelMatrix[3]); }

    struct Material {
        alignas(4)  int hasBaseColorTexture;
        alignas(4)  int hasUnlitColorTexture;
        alignas(4)  int hasNormalMapTexture;
        alignas(4)  int hasSpecularTexture;
        alignas(4)  int hasOverlayColorTexture;

        alignas(4)  float ambientStrength;
        alignas(4)  float specularStrength;
        alignas(4)  float overlayOffset;

        alignas(4)  int sunShadeMode;
    } material;

    const UniformBuffer<Material>* getMaterialUBO() const { return _materialUBO.get(); }
    void updateMaterial() { _materialUBO->update(material); }

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

    void drawGUI();

    glm::vec3 glowColor = glm::vec3(0.f, 0.f, 0.f);

private:
    static int objectIDCounter;
    int _objectID;
    std::string _name;

    std::shared_ptr<VulkanContext> _ctx;
    
    std::shared_ptr<DeviceMesh> _mesh;

    std::shared_ptr<DeviceTexture> _baseColorTexture;
    std::shared_ptr<DeviceTexture> _unlitColorTexture;
    std::shared_ptr<DeviceTexture> _normalMapTexture;
    std::shared_ptr<DeviceTexture> _specularTexture;
    std::shared_ptr<DeviceTexture> _overlayColorTexture;

    std::shared_ptr<TextureSampler> _textureSampler;

    glm::vec3 _color;
    glm::mat4 _modelMatrix;

    std::unique_ptr<UniformBuffer<Material>> _materialUBO;
    std::unique_ptr<DescriptorSet> _descriptorSet;
};
