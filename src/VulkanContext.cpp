#include "VulkanContext.h"
#include "OSHelper.h"

VulkanContext::VulkanContext(SDL_Window* window)
    : window(window)
{
    createVulkanInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createDescriptorPool();
    createCommandPool();
    loadPipelineCache("pipeline_cache.bin");
}

VulkanContext::~VulkanContext() {
    // Save the pipeline cache to a file
    savePipelineCache("pipeline_cache.bin");
    spdlog::info("Destroying Vulkan context...");
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);

    if (debugMessenger) {
        auto destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugUtilsMessengerEXT != nullptr) {
            destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

void VulkanContext::createVulkanInstance() {
    // Initialize Vulkan instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    // Check for validation layer support
    _validationLayersAvailable = isInstanceLayerAvailable("VK_LAYER_KHRONOS_validation");
    if (!_validationLayersAvailable) {
        spdlog::warn("Validation layers not available, disabling validation layers.");
    }else {
        const uint32_t validationLayerCount = 1;
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
        instanceCreateInfo.ppEnabledLayerNames = validationLayers;
        instanceCreateInfo.enabledLayerCount = validationLayerCount;
    }

    // Required extensions 
    // SDL extensions
    uint32_t sdlExtensionCount = 0;
    const char * const *sdlExtensionsRaw = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    std::vector<const char*> requiredExtensions(sdlExtensionsRaw, sdlExtensionsRaw + sdlExtensionCount);

    // Debug Utils extension
    if(!isInstanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        spdlog::warn("Debug Utils extension not available!");
    }else {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Other extensions can be added here as needed
    if(!isInstanceExtensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
        spdlog::warn("VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME not available!");
    }else {
        requiredExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Create Vulkan instance
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    spdlog::info("Vulkan instance created successfully");
}

void VulkanContext::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

    auto createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (createDebugUtilsMessengerEXT != nullptr) {
        if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            spdlog::error("Failed to set up debug messenger!");
        } else {
            spdlog::info("Debug messenger set up successfully");
        }
    } else {
        spdlog::error("vkGetInstanceProcAddr failed to find vkCreateDebugUtilsMessengerEXT function!");
    }
}

void VulkanContext::createSurface(SDL_Window* window) {
    // A surface is a platform-specific representation of the window where Vulkan will render its output.
    // its a window tied to a swapchain.
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error("Failed to create Vulkan surface!");
    }
    spdlog::info("Vulkan surface created successfully");
}

void VulkanContext::pickPhysicalDevice() {
    // Pick a suitable physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            spdlog::info("Found Suitable GPU: {}", deviceProperties.deviceName);
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void VulkanContext::createLogicalDevice() {
    // Queue family indices
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Store the queue family with graphics support
    std::optional<uint32_t> graphicsFamily;
    // Store the queue family with present support
    std::optional<uint32_t> presentFamily;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
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

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    spdlog::info("Logical device created successfully");

    // Get the graphics queue handle
    vkGetDeviceQueue(device, graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily.value(), 0, &presentQueue);
}

void VulkanContext::createDescriptorPool() {

    // Descriptor usage counts per type
    uint32_t totalUBOs = 100;
    uint32_t totalSamplers = 70;
    uint32_t maxSets = 50;

    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, totalUBOs },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, totalSamplers }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    spdlog::info("Descriptor pool created successfully");
    }

void VulkanContext::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = VulkanHelper::findQueueFamilies(physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    spdlog::info("Command pool created successfully");
}


bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) {
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
    SwapChainSupportDetails swapChainSupport = VulkanHelper::querySwapChainSupport(device, surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    
    // AND all the checks together
    return isDiscreteGPU && hasRequiredExtensions && swapChainAdequate;
}

bool VulkanContext::isInstanceLayerAvailable(const char* layerName) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layer : availableLayers) {
        if (strcmp(layerName, layer.layerName) == 0) {
            return true;
        }
    }
    return false;
}

bool VulkanContext::isInstanceExtensionAvailable(const char* extensionName) {
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    for (const auto& extension : availableExtensions) {
        if (strcmp(extensionName, extension.extensionName) == 0) {
            return true;
        }
    }
    return false;
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    spdlog::error("{}", pCallbackData->pMessage);

    return VK_FALSE;
}

void VulkanContext::loadPipelineCache(const std::string& filename) {
    std::ifstream file(OSHelper::getExecutableDir() + filename, std::ios::binary | std::ios::ate);
    std::vector<char> cacheData;
    if (file.is_open()) {
        size_t size = static_cast<size_t>(file.tellg());
        cacheData.resize(size);
        file.seekg(0);
        file.read(cacheData.data(), size);
        file.close();

        VkPipelineCacheCreateInfo cacheCreateInfo{};
        cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        cacheCreateInfo.initialDataSize = size;
        cacheCreateInfo.pInitialData = cacheData.data();

        if (vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &pipelineCache) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline cache from file.");
        }
        spdlog::info("Pipeline cache loaded from file: {}", filename);
    } else {
        spdlog::warn("No pipeline cache file found, creating a new one.");
        VkPipelineCacheCreateInfo cacheCreateInfo{};
        cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &pipelineCache);
    }
}

void VulkanContext::savePipelineCache(const std::string& filename) {
    size_t dataSize = 0;
    vkGetPipelineCacheData(device, pipelineCache, &dataSize, nullptr);
    std::vector<char> cacheData(dataSize);
    vkGetPipelineCacheData(device, pipelineCache, &dataSize, cacheData.data());

    std::ofstream file(OSHelper::getExecutableDir() + filename, std::ios::binary);
    if (file.is_open()) {
        file.write(cacheData.data(), dataSize);
        file.close();
        spdlog::info("Pipeline cache saved to file: {}", filename);
    } else {
        spdlog::error("Failed to save pipeline cache to file: {}", filename);
    }
}

void VulkanContext::printVulkanInfo() {
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