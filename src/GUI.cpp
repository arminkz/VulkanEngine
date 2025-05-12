#include "GUI.h"
#include "VulkanHelper.h"
#include "font/lato.h"
#include "font/IconsFontAwesome5.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

GUI::GUI(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Disable ini file saving/loading
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Style
    loadFonts();
    ImGui::StyleColorsDark();
    io.FontGlobalScale = _scaleFactor; // Scale all fonts by this factor
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(_scaleFactor); // Scale all sizes by this factor

    // Color scheme
    // vulkanStyle = ImGui::GetStyle();
    // vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    // vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    // vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    // vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    // vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // ImGuiStyle& style = ImGui::GetStyle();
    // style = vulkanStyle;

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL3_InitForVulkan(_ctx->window);

    // Create Buffers
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _vertexBuffers[i] = std::make_unique<Buffer>(_ctx);
        _indexBuffers[i] = std::make_unique<Buffer>(_ctx);
    }
}

GUI::~GUI()
{
    //ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void GUI::init(float width, float height)
{
    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(width, height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void GUI::initResources(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples)
{
    ImGuiIO& io = ImGui::GetIO();

    // Get font atlas data from ImGui
    unsigned char* fontData;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    
    // Create device texture for font atlas
    _fontTexture = std::make_unique<DeviceTexture>(_ctx, fontData, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, 1);

    // Create texture sampler for font atlas
    _textureSampler = std::make_unique<TextureSampler>(_ctx, 1);

    // Descriptor set layout
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _fontTexture->getDescriptorInfo(_textureSampler->getSampler())) // Font texture
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);

    // PushConstants
    VkPushConstantRange pcRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock) };

    // ImGui Needs its own pipeline
    PipelineParams imguiPipelineParams;
    imguiPipelineParams.name = "ImGui";
    imguiPipelineParams.descriptorSetLayouts = { _descriptorSet->getDescriptorSetLayout() };
    imguiPipelineParams.pushConstantRanges = { pcRange };
    imguiPipelineParams.renderPass = renderPass;

    imguiPipelineParams.cullMode = VK_CULL_MODE_NONE;
    imguiPipelineParams.blendEnable = true;
    imguiPipelineParams.depthTest = false;
    imguiPipelineParams.depthWrite = false;
    imguiPipelineParams.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    imguiPipelineParams.msaaSamples = msaaSamples;

    // Vertex bindings and attributes
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(ImDrawVert);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(ImDrawVert, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(ImDrawVert, uv);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attributeDescriptions[2].offset = offsetof(ImDrawVert, col);

    imguiPipelineParams.vertexBindingDescription = bindingDescription;
    imguiPipelineParams.vertexAttributeDescriptions = attributeDescriptions;

    // Create pipeline
    _pipeline = std::make_unique<Pipeline>(_ctx, "spv/imgui_vert.spv", "spv/imgui_frag.spv", imguiPipelineParams);
}

void GUI::newFrame()
{
    ImGui::NewFrame();
}

void GUI::endFrame(){
    ImGui::Render();
}

void GUI::updateBuffers(int currentFrame)
{
    ImDrawData* drawData = ImGui::GetDrawData();

    VkDeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return;
    }

    // Vertex buffer
    if ((_vertexBuffers[currentFrame]->getBuffer() == VK_NULL_HANDLE) || (_vertexCount != drawData->TotalVtxCount)) {

        // Unmap and destroy the old buffer
        _vertexBuffers[currentFrame]->destroy();
        
        // Create a new vertex buffer
        _vertexBuffers[currentFrame]->initialize(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // Index buffer
    if ((_indexBuffers[currentFrame]->getBuffer() == VK_NULL_HANDLE) || (_indexCount != drawData->TotalIdxCount)) {

        // Unmap and destroy the old buffer
        _indexBuffers[currentFrame]->destroy();

        // Create a new index buffer
        _indexBuffers[currentFrame]->initialize(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    
    // Upload data
    ImDrawVert* vtxDst = (ImDrawVert*)_vertexBuffers[currentFrame]->getMappedMemory();
    ImDrawIdx* idxDst = (ImDrawIdx*)_indexBuffers[currentFrame]->getMappedMemory();

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += cmd_list->VtxBuffer.Size;
        idxDst += cmd_list->IdxBuffer.Size;
    }
}

void GUI::drawFrame(VkCommandBuffer commandBuffer, uint32_t currentImage) {

    ImGuiIO& io = ImGui::GetIO();

    _pipeline->bind(commandBuffer);

    std::array<VkDescriptorSet, 1> descriptorSets = { _descriptorSet->getDescriptorSet() };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ImGui::GetIO().DisplaySize.x;
    viewport.height = ImGui::GetIO().DisplaySize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // UI scale and translate via push constants
    pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
    pushConstBlock.translate = glm::vec2(-1.0f);
    vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

    // Render commands
    ImDrawData* imDrawData = ImGui::GetDrawData();
    int32_t vertexOffset = 0;
    int32_t indexOffset = 0;

    if (imDrawData->CmdListsCount > 0) {

        VkDeviceSize offsets[1] = { 0 };
        std::array<VkBuffer, 1> vertexBuffers = { _vertexBuffers[currentImage]->getBuffer() };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffers[currentImage]->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                VkRect2D scissorRect;
                scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
                scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
                scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
                vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }
}

bool GUI::handleEvents(SDL_Event* event) {
    return ImGui_ImplSDL3_ProcessEvent(event);
}

bool GUI::isCapturingEvent()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void GUI::loadFonts()
{
    ImFontConfig fontConfig;
    ImGuiIO& io = ImGui::GetIO();

    std::copy_n("Lato", 5, fontConfig.Name);
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(lato_compressed_data_base85, 15.0f, &fontConfig);

    std::copy_n("FontAwesome", 12, fontConfig.Name);
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	fontConfig.MergeMode = true;
	fontConfig.PixelSnapH = true;
	fontConfig.GlyphMinAdvanceX = 20.0f;
	fontConfig.GlyphMaxAdvanceX = 20.0f;
    io.Fonts->AddFontFromFileTTF("fonts/fa-regular-400.ttf", 13.0f, &fontConfig, icons_ranges);
	io.Fonts->AddFontFromFileTTF("fonts/fa-solid-900.ttf", 13.0f, &fontConfig, icons_ranges);
}