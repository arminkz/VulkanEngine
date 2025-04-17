#pragma once
#include "stdafx.h"

struct VulkanContext {
    VkInstance instance;
    VkSurfaceKHR surface = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkCommandPool commandPool;
};