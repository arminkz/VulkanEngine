#include "AtmosphereModel.h"


AtmosphereModel::AtmosphereModel(std::shared_ptr<VulkanContext> ctx, 
    std::shared_ptr<DeviceMesh> mesh, 
    glm::mat4 modelMatrix, 
    glm::vec3 color, 
    float coeffScatter, 
    float powScatter, 
    bool isLightSource)

    : _ctx(std::move(ctx)), _mesh(std::move(mesh)), _modelMatrix(modelMatrix), _color(color)
{
    // Create UBO for atmosphere information
    _atmosphereUBO = std::make_unique<UniformBuffer<AtmosphereInfo>>(_ctx);
    AtmosphereInfo info{};
    info.color = _color;
    info.coeffScatter = coeffScatter;
    info.powScatter = powScatter;
    info.isLightSource = isLightSource ? 1 : 0;
    _atmosphereUBO->update(info);

    // Per-model descriptor set
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _atmosphereUBO->getDescriptorInfo())
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);
}

AtmosphereModel::~AtmosphereModel()
{
}
