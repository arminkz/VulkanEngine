#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"


class VulkanContext {
public:
    VulkanContext(SDL_Window* window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    SDL_Window* window = nullptr;

    VkInstance instance;
    VkSurfaceKHR surface = nullptr;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;

private:

    bool _validationLayersAvailable = true;

    void createVulkanInstance();
    void setupDebugMessenger();
    void createSurface(SDL_Window* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createDescriptorPool();
    void createCommandPool();

    // Debug messenger for validation layers
    VkDebugUtilsMessengerEXT debugMessenger;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    bool isDeviceSuitable(VkPhysicalDevice device);

    void printVulkanInfo();
};