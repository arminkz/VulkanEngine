#include "Renderer.h"
#include "stdafx.h"
#include "VulkanHelper.h"
#include "Pipeline.h"
#include "GUI.h"
#include "loader/ObjLoader.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "geometry/DeviceMesh.h"
#include "geometry/MeshFactory.h"

Renderer::Renderer(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
}

Renderer::~Renderer()
{
    // Wait for any unfinished GPU tasks
    vkDeviceWaitIdle(_ctx->device);

    // //Destroy dummy texture
    // DeviceTexture::cleanupDummy();

    // _ctx = nullptr;
}

void Renderer::initialize()
{
    // Create swap chain
    _swapChain = std::make_shared<SwapChain>(_ctx);

    // Initialize Scene
    _scene = std::make_unique<SolarSystemScene>(_ctx, _swapChain);


    //Sun atmosphere
    // glm::mat4 sunAtmosphereModelMat = glm::mat4(1.f);
    // sunAtmosphereModelMat = glm::scale(sunAtmosphereModelMat, glm::vec3(sizeSun * 1.3f));
    // std::shared_ptr<AtmosphereModel> sunAtmosphere = std::make_shared<AtmosphereModel>(_ctx, sphereDMesh, sunAtmosphereModelMat, glm::vec3(1.f, 0.3f, 0.0f), 0.4f, 2.0f, true);
    // _atmosphereModels.push_back(sunAtmosphere);
    //_sunGlowModel = sunAtmosphere;
    //_allModels.push_back(sunAtmosphere);
    
    
    // Create ImGUI
    // _gui = std::make_unique<GUI>(_ctx);
    // _gui->init(_swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height);
    // _gui->initResources(_renderPass, _msaaSamples);

    createCommandBuffers();
    createSyncObjects();
}


void Renderer::invalidate()
{
    vkDeviceWaitIdle(_ctx->device);

    //recreateMainRenderResources();
    //recreateObjectSelectionResources();
    //_gui->init(_swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height);
}

// void Renderer::recreateMainRenderResources()
// {
//     // Clean up framebuffers
//     for (size_t i = 0; i < _mainFrameBuffers.size(); i++) {
//         _mainFrameBuffers[i] = nullptr;
//     }

//     // Clean up Swapchain
//     _swapChain->cleanupSwapChain();

//     // Create new swapchain
//     _swapChain->createSwapChain();

//     // Create new framebuffers
//     createMainFrameBuffers();
// }


// void Renderer::recreateObjectSelectionResources()
// {
//     _objectSelectionFrameBuffer = nullptr;
//     createObjectSelectionFrameBuffer();
// }


// void Renderer::updateUniformBuffer(uint32_t currentImage) {
//     static auto startTime = std::chrono::high_resolution_clock::now();
//     auto currentTime = std::chrono::high_resolution_clock::now();

//     float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

//     // Update camera position based on time
//     _camera->advanceAnimation(time - _sceneInfo.time);

//     // Rotate earth
//     float earthRotationSpeed = 0.002f; // Adjust this value to control the rotation speed
//     //float earthRotationAngle = time * earthRotationSpeed;
//     _planetModels[4]->setModelMatrix(glm::rotate(_planetModels[4]->getModelMatrix(), earthRotationSpeed, glm::vec3(0.f, 0.f, 1.f)));

//     // Update SceneInfo UBO
//     _sceneInfo.view = _camera->getViewMatrix();
//     _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)_swapChain->getSwapChainExtent().width / (float)_swapChain->getSwapChainExtent().height, 0.1f, 1000.f);
//     _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan
//     _sceneInfo.time = time;
//     _sceneInfo.cameraPosition = _camera->getPosition();
//     _sceneInfoUBOs[currentImage]->update(_sceneInfo);
// }



void Renderer::createCommandBuffers() {
    // Allocate command buffer
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _ctx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

    if (vkAllocateCommandBuffers(_ctx->device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffers!");
    } else {
        spdlog::info("Command buffers allocated successfully");
    }
}


void Renderer::createSyncObjects() {
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
        }
    }
}


void Renderer::drawFrame() {
    vkWaitForFences(_ctx->device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_ctx->device, _swapChain->getSwapChain(), UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        invalidate();
        return;
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to acquire swap chain image!");
        return;
    }

    // Reset the fence before drawing
    vkResetFences(_ctx->device, 1, &_inFlightFences[_currentFrame]);
    vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

    // Update ImGui frame
    // _gui->newFrame();

    // ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Always);
    // ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Always);
    // ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    // ImGui::Begin("General", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // // FPS
    // ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    // ImGui::End();

    // GUI elements can be added here
    //_selectableObjects[_currentTargetObjectID]->drawGUI();

    // _gui->endFrame();
    // _gui->updateBuffers(_currentFrame);

    // Update uniform buffer
    // updateUniformBuffer(_currentFrame);

    // Update Scene
    _scene->update(_currentFrame);

    // Record command buffer
    _scene->recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

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

    VkSwapchainKHR swapChains[] = {_swapChain->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(_ctx->presentQueue, &presentInfo);
    
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        invalidate();
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


void Renderer::handleMouseClick(float mouseX, float mouseY) {
    // Handle mouse click events in the scene
    _scene->handleMouseClick(mouseX, mouseY);
}


void Renderer::handleMouseDrag(float dx, float dy) {
    // Handle mouse drag events in the scene
    _scene->handleMouseDrag(dx, dy);
}


void Renderer::handleMouseWheel(float dy) {
    // Handle mouse wheel events in the scene
    _scene->handleMouseWheel(dy);
}