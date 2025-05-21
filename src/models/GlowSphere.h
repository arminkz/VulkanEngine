#pragma once
#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Model.h"
#include "Scene.h"
#include "Pipeline.h"


class GlowSphere : public Model
{
public:
    GlowSphere(std::shared_ptr<VulkanContext> ctx, 
               std::string name, 
               std::shared_ptr<DeviceMesh> mesh);

    ~GlowSphere();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

private:

    struct GlowSphereInfo {
        alignas(16) glm::vec3 color;
        alignas(4) float coeffScatter = 3.0f;
        alignas(4) float powScatter = 3.0f;
        alignas(4) int isLightSource = 0;
    };

    std::unique_ptr<UniformBuffer<GlowSphereInfo>> _glowSphereUBO;

    std::unique_ptr<DescriptorSet> _descriptorSet;
};