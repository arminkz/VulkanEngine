#pragma once
#include "stdafx.h"
#include "VulkanContext.h"


class GUI
{
public:
    GUI(std::shared_ptr<VulkanContext> ctx);
    ~GUI();

    // void initialize(VkRenderPass renderPass, VkDescriptorPool descriptorPool, VkCommandPool commandPool, VkQueue queue, int width, int height);

private:
    std::shared_ptr<VulkanContext> _ctx;

    ImGui_ImplVulkanH_Window _imguiWindowData;
};