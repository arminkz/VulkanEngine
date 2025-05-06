#include "DeviceModel.h"
#include "geometry/DeviceMesh.h"
#include "VulkanHelper.h"
#include "DescriptorSet.h"

int DeviceModel::objectIDCounter = 0;

DeviceModel::DeviceModel(
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
)
    : _name(name)
    , _ctx(std::move(ctx))
    , _objectID(++objectIDCounter)
    , _mesh(std::move(mesh))
    , _baseColorTexture(std::move(baseColorTexture))
    , _unlitColorTexture(std::move(unlitColorTexture))
    , _normalMapTexture(std::move(normalMapTexture))
    , _specularTexture(std::move(specularTexture))
    , _overlayColorTexture(std::move(overlayColorTexture))
    , _textureSampler(std::move(textureSampler))
{
    _color = glm::vec3(0.f);
    _modelMatrix = modelMatrix;

    material.hasBaseColorTexture = _baseColorTexture ? 1 : 0;
    material.hasUnlitColorTexture = _unlitColorTexture ? 1 : 0;
    material.hasNormalMapTexture = _normalMapTexture ? 1 : 0;
    material.hasSpecularTexture = _specularTexture ? 1 : 0;
    material.hasOverlayColorTexture = _overlayColorTexture ? 1 : 0;
    material.ambientStrength = 0.1f;
    material.specularStrength = 3.0f;
    material.sunShadeMode = 0;

    _materialUBO = std::make_unique<UniformBuffer<Material>>(_ctx);
    _materialUBO->update(material);

    // Per-model descriptor set
    DeviceTexture* dummyTexture = DeviceTexture::getDummy(_ctx);
    VkSampler sampler = _textureSampler->getSampler();
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _materialUBO->getDescriptorInfo()), // Material UBO
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_baseColorTexture ? _baseColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),      // Base color texture
        Descriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_unlitColorTexture ? _unlitColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),    // Unlit color texture
        Descriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_normalMapTexture ? _normalMapTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),      // Normal map texture
        Descriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_specularTexture ? _specularTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),        // Specular texture
        Descriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_overlayColorTexture ? _overlayColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)) // Overlay color texture
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);
}

DeviceModel::~DeviceModel()
{
}

void DeviceModel::drawGUI()
{
    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Always);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    ImGui::Begin(_name.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Object ID: %d", _objectID);
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", _modelMatrix[3][0], _modelMatrix[3][1], _modelMatrix[3][2]);
    ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", glm::degrees(glm::eulerAngles(glm::quat_cast(_modelMatrix)))[0], glm::degrees(glm::eulerAngles(glm::quat_cast(_modelMatrix)))[1], glm::degrees(glm::eulerAngles(glm::quat_cast(_modelMatrix)))[2]);
    ImGui::Text("Scale: (%.2f, %.2f, %.2f)", _modelMatrix[0][0], _modelMatrix[1][1], _modelMatrix[2][2]);
    ImGui::End();
}