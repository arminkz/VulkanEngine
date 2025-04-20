#include "Renderer.h"
#include "stdafx.h"
#include "VulkanHelper.h"

#include "loader/ObjLoader.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "geometry/DeviceMesh.h"
#include "geometry/MeshFactory.h"

Renderer::Renderer()
{
    // Constructor implementation (if needed)
}

Renderer::~Renderer()
{

    // Wait for any unfinished GPU tasks
    vkDeviceWaitIdle(_ctx->device);

    _model = nullptr;
    _model2 = nullptr;
    _textureSampler = nullptr;

    cleanupSwapChain();

    // Clean up Vulkan resources
    for(size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_ctx->device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_ctx->device, _renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(_ctx->device, _inFlightFences[i], nullptr);
    }

    // for (auto framebuffer : _swapChainFramebuffers) {
    //     vkDestroyFramebuffer(_ctx->device, framebuffer, nullptr);
    // }

    vkDestroyPipeline(_ctx->device, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_ctx->device, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_ctx->device, _renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(_ctx->device, _uniformBuffers[i], nullptr);
        vkFreeMemory(_ctx->device, _uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(_ctx->device, _descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_ctx->device, _descriptorSetLayout, nullptr);
    
    // for (auto imageView : _swapChainImageViews) {
    //     vkDestroyImageView(_ctx->device, imageView, nullptr);
    // }

    // Destroy all members of VulkanContext
    vkDestroyCommandPool(_ctx->device, _ctx->commandPool, nullptr);
    vkDestroyDevice(_ctx->device, nullptr);
    vkDestroySurfaceKHR(_ctx->instance, _ctx->surface, nullptr);

    // Destroy debug messenger if it was created
    if (debugMessenger) {
        auto destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessengerEXT != nullptr) {
            destroyDebugUtilsMessengerEXT(_ctx->instance, debugMessenger, nullptr);
        }
    }

    vkDestroyInstance(_ctx->instance, nullptr);
    
}

bool Renderer::initialize(SDL_Window* _window)
{
    // Store the window reference
    this->_window = _window;

    // Create VulkanContext
    createVulkanContext();

    // Create Camera
    _camera = std::make_unique<TurnTableCamera>();

    // Model creation
    HostMesh hmesh = MeshFactory::createSphereMesh(1.0f, 64, 64);
    std::shared_ptr<DeviceMesh> dmesh = std::make_shared<DeviceMesh>(_ctx, hmesh);
    std::shared_ptr<DeviceTexture> colorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_daymap.jpg");
    std::shared_ptr<DeviceTexture> unlitTexture = nullptr; //std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_nightmap.jpg");
    std::shared_ptr<DeviceTexture> normalTexture = std::make_shared<DeviceTexture>(_ctx, "textures/EarthNormal.png");
    std::shared_ptr<DeviceTexture> specularTexture = std::make_shared<DeviceTexture>(_ctx, "textures/EarthSpec.png");
    std::shared_ptr<DeviceTexture> overlayTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_clouds.jpg");
    _model = std::make_unique<DeviceModel>(_ctx, dmesh, colorTexture, unlitTexture, normalTexture, specularTexture, overlayTexture);

    HostMesh hmesh2 = MeshFactory::createSphereMesh(0.5f, 64, 64);
    std::shared_ptr<DeviceMesh> dmesh2 = std::make_shared<DeviceMesh>(_ctx, hmesh2);
    std::shared_ptr<DeviceTexture> texture2 = std::make_shared<DeviceTexture>(_ctx, "textures/2k_moon.jpg");
    _model2 = std::make_unique<DeviceModel>(_ctx, dmesh2, texture2);
    
    //Mesh mesh = ObjLoader::load("models/viking_room.obj");
    //_model = std::make_unique<Model>(_ctx, mesh, "textures/viking_room.png");

    // Texture Sampler
    _textureSampler = std::make_unique<TextureSampler>(_ctx, colorTexture->getMipLevels());

    _msaaSamples = getMaxMsaaSampleCount();
    spdlog::info("Max MSAA samples: {}", static_cast<int>(_msaaSamples));

    // Create swap chain
    if (!createSwapChain()) return false;

    // Create render pass
    if (!createRenderPass()) return false;

    // Create descriptor set layout
    if (!createDescriptorSetLayout()) return false;

    // Create graphics pipeline
    if (!createGraphicsPipeline()) return false;

    // Create color resources
    createColorResources();

    // Create depth resources
    createDepthResources();

    // Create framebuffers
    if (!createFramebuffers()) return false;

    // Create uniform buffers
    if (!createUniformBuffers()) return false;

    // Create descriptor pool
    if (!createDescriptorPool()) return false;

    // Create descriptor sets
    if (!createDescriptorSets()) return false;

    // Create command buffers
    if (!createCommandBuffers()) return false;

    // Create sync objects
    if (!createSyncObjects()) return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    spdlog::error("{}", pCallbackData->pMessage);

    return VK_FALSE;
}

// Debug
void Renderer::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    auto createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_ctx->instance, "vkCreateDebugUtilsMessengerEXT");

    if (createDebugUtilsMessengerEXT != nullptr) {
        if (createDebugUtilsMessengerEXT(_ctx->instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            spdlog::error("Failed to set up debug messenger!");
        } else {
            spdlog::info("Debug messenger set up successfully");
        }
    } else {
        spdlog::error("vkGetInstanceProcAddr failed to find vkCreateDebugUtilsMessengerEXT function!");
    }
}


// Functions to initialize VulkanContext (do we need to move this to VulkanContext??)
void Renderer::createVulkanContext() 
{
    _ctx = std::make_shared<VulkanContext>();
    createVulkanInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}

bool Renderer::createVulkanInstance() {
        // Initialize Vulkan instance
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "LearnVulkan";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;
    
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
    
        // Check for validation layer support
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                _validationLayersAvailable = false;
            }
        }
    
        if (_validationLayersAvailable) {
            const uint32_t validationLayerCount = 1;
            const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
            instanceCreateInfo.ppEnabledLayerNames = validationLayers;
            instanceCreateInfo.enabledLayerCount = validationLayerCount;
        }
    
        // Required extensions 
        // Get the required extensions for the SDL + Debug Utils
        uint32_t sdlExtensionCount = 0;
        const char * const *sdlExtensionsRaw = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        std::vector<const char*> requiredExtensions(sdlExtensionCount + 1);
        requiredExtensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME; // Add the surface extension to the list
        for (uint32_t i = 0; i < sdlExtensionCount; ++i) {
            requiredExtensions[i+1] = sdlExtensionsRaw[i];
        }
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
    
        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &_ctx->instance);
        if (result != VK_SUCCESS) {
            spdlog::error("Failed to create Vulkan instance");
            return false;
        }
    
        spdlog::info("Vulkan instance created successfully");
        return true;
}

bool Renderer::createSurface() {
    // A surface is a platform-specific representation of the window where Vulkan will render its output.
    // its a window tied to a swapchain.
    if (!SDL_Vulkan_CreateSurface(_window, _ctx->instance, nullptr, &_ctx->surface)) {
        spdlog::error("Failed to create Vulkan surface: {}", SDL_GetError());
        return false;
    }
    spdlog::info("Vulkan surface created successfully");
    return true;
}

bool Renderer::pickPhysicalDevice() {
    // Pick a suitable physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_ctx->instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        spdlog::error("Failed to find GPUs with Vulkan support!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_ctx->instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (isDeviceSuitable(device)) {
            _ctx->physicalDevice = device;
            spdlog::info("Found Suitable GPU: {}", deviceProperties.deviceName);
            break;
        }
    }
    if (_ctx->physicalDevice == VK_NULL_HANDLE) {
        spdlog::error("Failed to find a suitable GPU!");
        return false;
    }
}

bool Renderer::createLogicalDevice() {
    // Queue family indices
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_ctx->physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_ctx->physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Store the queue family with graphics support
    std::optional<uint32_t> graphicsFamily;
    // Store the queue family with present support
    std::optional<uint32_t> presentFamily;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_ctx->physicalDevice, i, _ctx->surface, &presentSupport);
        if (presentSupport) {
            presentFamily = i;
        }
    }

    // Create logical device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // Specify the queue create info for graphics and present queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {graphicsFamily.value(), presentFamily.value()};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    // Specify the device extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Specify the device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // Enable anisotropic filtering
    deviceFeatures.sampleRateShading = VK_TRUE; // Enable sample rate shading
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (vkCreateDevice(_ctx->physicalDevice, &deviceCreateInfo, nullptr, &_ctx->device) != VK_SUCCESS) {
        spdlog::error("Failed to create logical device!");
        return false;
    }
    else {
        spdlog::info("Logical device created successfully");
    }

    // Get the graphics queue handle
    vkGetDeviceQueue(_ctx->device, graphicsFamily.value(), 0, &_ctx->graphicsQueue);
    vkGetDeviceQueue(_ctx->device, presentFamily.value(), 0, &_ctx->presentQueue);

    return true;
}

bool Renderer::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_ctx->physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(_ctx->device, &poolInfo, nullptr, &_ctx->commandPool) != VK_SUCCESS) {
        spdlog::error("Failed to create command pool!");
        return false;
    } else {
        spdlog::info("Command pool created successfully");
        return true;
    }
}



bool Renderer::isDeviceSuitable(VkPhysicalDevice device) {
    // Check if the device is suitable for our needs (graphics, compute, etc.)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Check if the device is discrete GPU
    bool isDiscreteGPU = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    // Check if the device supports certain extensions
    std::set<std::string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    bool hasRequiredExtensions = requiredExtensions.empty();

    // Check if the device swapchain is adequate
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    
    // AND all the checks together
    return isDiscreteGPU && hasRequiredExtensions && swapChainAdequate;
}

QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _ctx->surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails Renderer::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _ctx->surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _ctx->surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _ctx->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _ctx->surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _ctx->surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Check for mailbox mode first (triple buffering)
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    // Fallback to FIFO mode (double buffering)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_GetWindowSizeInPixels(_window, &width, &height);
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

bool Renderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_ctx->physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = _ctx->surface;

    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_ctx->physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_ctx->device, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS) {
        spdlog::error("Failed to create swap chain!");
        return false;
    } else {
        spdlog::info("Swap chain created successfully");
    }

    vkGetSwapchainImagesKHR(_ctx->device, _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_ctx->device, _swapChain, &imageCount, _swapChainImages.data());

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    _swapChainImageViews.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {

        // Create image views for each swap chain image
        _swapChainImageViews[i] = VulkanHelper::createImageView(_ctx, _swapChainImages[i], _swapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    return true;
}

bool Renderer::recreateSwapChain() {
    vkDeviceWaitIdle(_ctx->device);

    // Destroy old swap chain
    cleanupSwapChain();

    // Create new swap chain
    createSwapChain();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    return true;
}

void Renderer::cleanupSwapChain() {
    // Clean up color resources
    vkDestroyImageView(_ctx->device, _colorImageView, nullptr);
    vkDestroyImage(_ctx->device, _colorImage, nullptr);
    vkFreeMemory(_ctx->device, _colorImageMemory, nullptr);

    // Clean up depth resources
    vkDestroyImageView(_ctx->device, _depthImageView, nullptr);
    vkDestroyImage(_ctx->device, _depthImage, nullptr);
    vkFreeMemory(_ctx->device, _depthImageMemory, nullptr);

    // Clean up swap chain resources
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(_ctx->device, _swapChainFramebuffers[i], nullptr);
    }
    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        vkDestroyImageView(_ctx->device, _swapChainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(_ctx->device, _swapChain, nullptr);
}

bool Renderer::createRenderPass() {
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = _msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = _swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = _msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Attach depth attachment
    subpass.pResolveAttachments = &colorAttachmentResolveRef; // Attach resolve attachment

    // Subpass has to wait for the swapchain to be ready before it can start rendering
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_ctx->device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        spdlog::error("Failed to create render pass!");
        return false;
    }else {
        spdlog::info("Render pass created successfully");
        return true;
    }
}

bool Renderer::createDescriptorSetLayout() {

    std::array<VkDescriptorSetLayoutBinding, 7> bindings;

    // Create descriptor set layout for UBO (Uniform Buffer Object)
    // This layout will be used to bind the uniform buffer to the vertex shader (MVP)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0; // This should match the number in shader.
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
    bindings[0] = uboLayoutBinding;

    // This layout will be used to bind the uniform buffer to the fragment shader (Material)
    VkDescriptorSetLayoutBinding materialUBOLayoutBinding{};
    materialUBOLayoutBinding.binding = 1; // This should match the number in shader.
    materialUBOLayoutBinding.descriptorCount = 1;
    materialUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialUBOLayoutBinding.pImmutableSamplers = nullptr; // Optional
    bindings[1] = materialUBOLayoutBinding;

    // Create descriptor set layout for textures
    // This layout will be used to bind the texture sampler to the fragment shader (texture)
    // We have 5 textures.
    for (int i=2; i<7; i++) {
        VkDescriptorSetLayoutBinding textureLayoutBinding{};
        textureLayoutBinding.binding = i;
        textureLayoutBinding.descriptorCount = 1;
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureLayoutBinding.pImmutableSamplers = nullptr; // Optional
        bindings[i] = textureLayoutBinding;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(_ctx->device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor set layout!");
        return false;
    } else {
        spdlog::info("Descriptor set layout created successfully");
    }

    return true;
}

bool Renderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 7> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    for(int i=2; i<7; i++){
        poolSizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    }


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if(vkCreateDescriptorPool(_ctx->device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
        spdlog::error("Failed to create descriptor pool!");
        return false;
    } else {
        spdlog::info("Descriptor pool created successfully");
    }
}

bool Renderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(_ctx->device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate descriptor sets!");
        return false;
    } else {
        spdlog::info("Descriptor sets allocated successfully");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(MVP);

        auto materialBufferInfo = _model->getMaterialUBO()->getDescriptorInfo();
        auto baseColorTextureInfo = _model->getBaseColorTexture()->getDescriptorInfo(_textureSampler->getSampler());
        auto unlitColorTextureInfo = _model->getUnlitColorTexture()->getDescriptorInfo(_textureSampler->getSampler());
        auto normalMapTextureInfo = _model->getNormalMapTexture()->getDescriptorInfo(_textureSampler->getSampler());
        auto specularTextureInfo = _model->getSpecularTexture()->getDescriptorInfo(_textureSampler->getSampler());
        auto overlayColorTextureInfo = _model->getOverlayColorTexture()->getDescriptorInfo(_textureSampler->getSampler());

        // Bind the buffer to the descriptor set
        std::array<VkWriteDescriptorSet, 7> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &materialBufferInfo;

        // Bind the image to the descriptor set
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = _descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pImageInfo = &baseColorTextureInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = _descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pImageInfo = &unlitColorTextureInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = _descriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pImageInfo = &normalMapTextureInfo;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = _descriptorSets[i];
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pImageInfo = &specularTextureInfo;

        descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[6].dstSet = _descriptorSets[i];
        descriptorWrites[6].dstBinding = 6;
        descriptorWrites[6].dstArrayElement = 0;
        descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[6].descriptorCount = 1;
        descriptorWrites[6].pImageInfo = &overlayColorTextureInfo;

        vkUpdateDescriptorSets(_ctx->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

bool Renderer::createGraphicsPipeline() {
    // Load the shader code from binary files
    auto vertShaderCode = readBinaryFile("shaders/vert.spv");
    auto fragShaderCode = readBinaryFile("shaders/frag.spv");

    // Create shader modules
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // Create vertex shader stage info
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // Create fragment shader stage info
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 1- Vertex input state
    // This is where we specify the vertex input format (e.g., position, color, texture coordinates, etc.)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // 2- Input assembly state
    // This is where we specify the topology of the vertex data (e.g., triangle list, line list, etc.)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 3- Viewport state
    // This is where we specify the viewport and scissor rectangle for rendering
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Geometry shader / Tessellation control shader
    // Not used in this example, but you can add them here if needed

    // 4- Rasterization state
    // This is where we specify how to rasterize the primitives (e.g., line width, culling mode, etc.)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 5- Multisampling state (Anti-aliasing)
    // This is where we specify the multisampling settings (e.g., sample count, sample mask, etc.)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = 0.2f; // Optional
    multisampling.rasterizationSamples = _msaaSamples;

    // 6- Depth and stencil state
    // This is where we specify the depth and stencil settings (e.g., depth test, depth write, etc.)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // Lower depth = closer to camera
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    // 7- Color blending state
    // This is where we specify the color blending settings (e.g., blend enable, blend factors, etc.)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic states
    // This is where we specify the dynamic states (e.g., viewport, scissor, etc.)
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Pipeline layout
    // This is where we specify the pipeline layout (e.g., descriptor sets, push constants, etc.)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(_ctx->device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Graphics pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
        spdlog::error("Failed to create graphics pipeline!");
        return false;
    }else {
        spdlog::info("Graphics pipeline created successfully");
    }

    // Destroy shader modules after creating the pipeline
    vkDestroyShaderModule(_ctx->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_ctx->device, vertShaderModule, nullptr);

    return true;
}

bool Renderer::createFramebuffers() {
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            _colorImageView,
            _depthImageView,
            _swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_ctx->device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create framebuffer!");
            return false;
        }
    }

    return true;
}

bool Renderer::createUniformBuffers() {
    // Create uniform buffer
    VkDeviceSize bufferSize = sizeof(MVP); // Size of the uniform buffer

    _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VulkanHelper::createBuffer(_ctx,
            bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            _uniformBuffers[i], _uniformBuffersMemory[i]);

        // Map the buffer memory into CPU addressable space
        vkMapMemory(_ctx->device, _uniformBuffersMemory[i], 0, bufferSize, 0, &_uniformBuffersMapped[i]);
    }

    return true;
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    MVP mvp{};
    mvp.model = glm::rotate(glm::mat4(1.0f), 0.1f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.model = glm::translate(mvp.model, glm::vec3(-2.f,-2.f,0.f));
    mvp.model = glm::rotate(mvp.model, 0.3f * time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //mvp.model = glm::mat4(1.0f);
    mvp.view = _camera->getViewMatrix(); 
    mvp.projection = glm::perspective(glm::radians(45.0f), (float)_swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 100.0f);
    mvp.projection[1][1] *= -1; // Invert Y axis for Vulkan
    
    memcpy(_uniformBuffersMapped[currentImage], &mvp, sizeof(mvp)); // Copy the MVP data to the mapped memory (Upload to GPU)
}


void Renderer::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat(); // Find a suitable depth format

    VulkanHelper::createImage(_ctx, _swapChainExtent.width, _swapChainExtent.height, 
        depthFormat,
        1, // Number of mip levels
        _msaaSamples, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _depthImage, _depthImageMemory);

    //transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    _depthImageView = VulkanHelper::createImageView(_ctx, _depthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

}

void Renderer::createColorResources() {

    VkFormat colorFormat = _swapChainImageFormat; // Use the swap chain image format for color resources

    VulkanHelper::createImage(_ctx, 
        _swapChainExtent.width,
        _swapChainExtent.height, 
        colorFormat,
        1,
        _msaaSamples, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, // Image tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _colorImage, _colorImageMemory);

    _colorImageView = VulkanHelper::createImageView(_ctx, _colorImage, colorFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_ctx->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    spdlog::error("Failed to find supported format!");
    return VK_FORMAT_UNDEFINED;
}

VkFormat Renderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkSampleCountFlagBits Renderer::getMaxMsaaSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(_ctx->physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT; // Fallback to 1 sample
}

bool Renderer::hasStencilComponent(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return true;
        default:
            return false;
    }
}


bool Renderer::createCommandBuffers() {
    // Allocate command buffer
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _ctx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

    if (vkAllocateCommandBuffers(_ctx->device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffers!");
        return false;
    } else {
        spdlog::info("Command buffers allocated successfully");
        return true;
    }
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        spdlog::error("Failed to begin recording command buffer!");
        return;
    }

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _swapChainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // Clear color
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    
    // Draw call here (e.g., vkCmdDraw)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChainExtent.width;
    viewport.height = (float) _swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);         
    
    VkBuffer vertexBuffers[] = {_model->getDeviceMesh()->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
    vkCmdBindIndexBuffer(commandBuffer, _model->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[_currentFrame], 0, nullptr);

    //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_model->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        spdlog::error("Failed to record command buffer!");
        return;
    }
}

bool Renderer::createSyncObjects() {
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Initially signaled

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_ctx->device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_ctx->device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_ctx->device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create semaphores / fences for frame {}!", i);
            return false;
        }
    }
    return true;
}

void Renderer::drawFrame() {
    vkWaitForFences(_ctx->device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_ctx->device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to acquire swap chain image!");
        return;
    }

    // Reset the fence before drawing
    vkResetFences(_ctx->device, 1, &_inFlightFences[_currentFrame]);

    // Update uniform buffer
    updateUniformBuffer(_currentFrame);

    vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);
    recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_ctx->graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        spdlog::error("Failed to submit draw command buffer!");
        return;
    }

    // Present the image to the swap chain (after the command buffer is done)
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(_ctx->presentQueue, &presentInfo);
    
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_ctx->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        spdlog::error("Failed to create shader module!");
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

void Renderer::printVulkanInfo() {
    spdlog::info("--------------------------------");
    spdlog::info("Vulkan API version: {}.{}",  VK_API_VERSION_MAJOR(VK_API_VERSION_1_3), VK_API_VERSION_MINOR(VK_API_VERSION_1_3));
    spdlog::info("--------------------------------");

    // Print available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    spdlog::info("Available layers:");
    for (const auto& layer : availableLayers) {
        spdlog::info("  {}", layer.layerName);
    }

    spdlog::info("------------------------------------");
    
    // Print available extensions
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    spdlog::info("Available extensions:");
    for (const auto& extension : availableExtensions) {
        spdlog::info("  {}", extension.extensionName);
    }

    spdlog::info("------------------------------------");
    spdlog::info("Validation layers available: {}", _validationLayersAvailable ? "true" : "false");
}

void Renderer::printSwapChainSupportDetails(const SwapChainSupportDetails& details){
    spdlog::info("Swap chain support details:");
    spdlog::info("  Capabilities: minImageCount = {}, maxImageCount = {}, currentExtent = ({}, {}), minImageExtent = ({}, {}), maxImageExtent = ({}, {}), maxImageArrayLayers = {}",
        details.capabilities.minImageCount, details.capabilities.maxImageCount,
        details.capabilities.currentExtent.width, details.capabilities.currentExtent.height,
        details.capabilities.minImageExtent.width, details.capabilities.minImageExtent.height,
        details.capabilities.maxImageExtent.width, details.capabilities.maxImageExtent.height,
        details.capabilities.maxImageArrayLayers);
}