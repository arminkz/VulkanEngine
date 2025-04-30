#pragma once
#include "stdafx.h"
#include "VulkanContext.h"


struct PipelineParams
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    VkPushConstantRange pushConstantRange;
    VkRenderPass renderPass;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    bool depthTest = true;
    bool depthWrite = true;
    bool transparency = true;
    bool backSide = false;
};

class Pipeline
{
public:
    Pipeline(std::shared_ptr<VulkanContext> ctx, const std::string& vertShaderPath, const std::string& fragShaderPath, const PipelineParams& params);
    ~Pipeline();

    VkPipeline getPipeline() const { return _pipeline; }
    VkPipelineLayout getPipelineLayout() const { return _pipelineLayout; }

    void bind(VkCommandBuffer commandBuffer);

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    void createPipelineLayout(const PipelineParams& params);
    void createGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath, const PipelineParams& params);
    VkShaderModule createShaderModule(const std::vector<char>& code);
};