
#include "vk_engine.h"

#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "glm/gtx/transform.hpp"

#include <vk_types.h>
#include <vk_initializers.h>

#define VK_CHECK(x)                                        \
    do                                                     \
    {                                                      \
        VkResult err = x;                                  \
        if (err)                                           \
        {                                                  \
            std::cout << "Vk error: " << err << std::endl; \
            exit(EXIT_FAILURE);                            \
        }                                                  \
                                                           \
    } while (0);

void VulkanEngine::init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(
        "Vulkan Playground",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowExtent.width,
        windowExtent.height,
        window_flags);

    initVulkan();
    initSwapchain();
    initCommands();
    initDefaultRenderpass();
    initFramebuffers();
    initSyncStructures();
    initPipelines();

    loadMeshes();
    isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (isInitialized)
    {
        vkWaitForFences(device, 1, &renderFence, true, 1000000000);

        mainDeletionQueue.flush();

        vmaDestroyAllocator(allocator);

        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);

        SDL_DestroyWindow(window);
    }
}

void VulkanEngine::draw()
{
    VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 10E9));
    VK_CHECK(vkResetFences(device, 1, &renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 10E9, presentSemaphore, nullptr, &swapchainImageIndex));

    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

    VkCommandBuffer cmd = commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr};

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    VkClearValue clearValue{
        .color = {0.0f, 0.0f, abs(sin(frameNumber / 120.f))}};

    VkRenderPassBeginInfo rpInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = renderPass,
        .framebuffer = framebuffers[swapchainImageIndex],
        .renderArea{.offset{.x = 0, .y = 0}, .extent = windowExtent},
        .clearValueCount = 1,
        .pClearValues = &clearValue};

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (selectedShader == 0)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
    else if (selectedShader == 1)
    {   
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, coloredTrianglePipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
    else if (selectedShader == 2)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &monkeyMesh.vertexBuffer.buffer, &offset);

        glm::vec3 camPos = {0.0f, 0.0f, -2.0f};
        glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1700.f / 900.0f, 0.1f, 200.0f);
        projection[1][1] *= -1;
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(frameNumber * 0.4f), glm::vec3(0, 1, 0));
        glm::mat4 meshMatrix = projection * view * model;

        MeshPushConstants constants = {
            .renderMatrix = meshMatrix};

        vkCmdPushConstants(cmd, meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
        vkCmdDraw(cmd, monkeyMesh.vertices.size(), 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderSemaphore};

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence));

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &swapchainImageIndex};

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    ++frameNumber;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    while (!bQuit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
                bQuit = true;
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_SPACE)
                    ++selectedShader;
                    if(selectedShader > 2)
                        selectedShader = 0;
            }
        }
        draw();
    }
}

void VulkanEngine::initVulkan()
{
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Vulkan Playground")
                        .request_validation_layers(true)
                        .require_api_version(1, 1, 0)
                        .use_default_debug_messenger()
                        .build();

    vkb::Instance vkb_instance = inst_ret.value();
    instance = vkb_instance.instance;
    debugMessenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(window, instance, &surface);

    vkb::PhysicalDeviceSelector selector{vkb_instance};
    vkb::PhysicalDevice pd = selector
                                 .set_minimum_version(1, 1)

                                 .set_surface(surface)
                                 .select()
                                 .value();

    vkb::DeviceBuilder deviceBuilder{pd};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    device = vkbDevice.device;
    physicalDevice = pd.physical_device;

    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = physicalDevice,
        .device = device,
        .instance = instance};

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanEngine::initSwapchain()
{
    vkb::SwapchainBuilder swapchainBuilder{physicalDevice, device, surface};

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      .use_default_format_selection()
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(windowExtent.width, windowExtent.height)
                                      .build()
                                      .value();

    swapchain = vkbSwapchain.swapchain;
    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();
    swapchainImageFormat = vkbSwapchain.image_format;

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroySwapchainKHR(device, swapchain, nullptr); });
}

void VulkanEngine::initCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo = vkInit::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::commandBufferAllocateInfo(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer));

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroyCommandPool(device, commandPool, nullptr); });
}

void VulkanEngine::initDefaultRenderpass()
{
    VkAttachmentDescription colorAttachment = {
        .format = swapchainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentReference colorAttachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference};

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass};

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass))

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroyRenderPass(device, renderPass, nullptr); });
}

void VulkanEngine::initFramebuffers()
{
    VkFramebufferCreateInfo fbInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .renderPass = renderPass,
        .attachmentCount = 1,
        .width = windowExtent.width,
        .height = windowExtent.height,
        .layers = 1};

    const uint32_t swapchainImageCount = swapchainImages.size();
    framebuffers.resize(swapchainImageCount);

    for (int i = 0; i < swapchainImageCount; ++i)
    {
        fbInfo.pAttachments = &swapchainImageViews[i];
        VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]));

        mainDeletionQueue.pushFunction([=]()
                                       { vkDestroyFramebuffer(device, framebuffers[i], nullptr);
                                         vkDestroyImageView(device, swapchainImageViews[i], nullptr); });
    }
}

void VulkanEngine::initSyncStructures()
{
    VkFenceCreateInfo fenceCreateInfo = vkInit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroyFence(device, renderFence, nullptr); });

    VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::semaphoreCreateInfo();

    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroySemaphore(device, presentSemaphore, nullptr);
                                     vkDestroySemaphore(device, renderSemaphore, nullptr); });
}

bool VulkanEngine::loadShaderModule(std::string filepath, VkShaderModule *outShaderModule)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Can't open file: " << filepath << std::endl;
        return false;
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = buffer.size() * sizeof(uint32_t),
        .pCode = buffer.data()};

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

void VulkanEngine::initPipelines()
{
    VkShaderModule triangleFragShader;
    if (!loadShaderModule("../shaders/triangle.frag.spv", &triangleFragShader))
    {
        std::cout << "Error building triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule triangleVertShader;
    if (!loadShaderModule("../shaders/triangle.vert.spv", &triangleVertShader))
    {
        std::cout << "Error building triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Triangle vertex shader successfully loaded" << std::endl;
    }

    VkShaderModule coloredTriangleFragShader;
    if (!loadShaderModule("../shaders/coloredTriangle.frag.spv", &coloredTriangleFragShader))
    {
        std::cout << "Error building colored triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Colored Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule coloredTriangleVertShader;
    if (!loadShaderModule("../shaders/coloredTriangle.vert.spv", &coloredTriangleVertShader))
    {
        std::cout << "Error building colored triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Colored Triangle vertex shader successfully loaded" << std::endl;
    }

    VkShaderModule meshTriangleFragShader;
    if (!loadShaderModule("../shaders/triangleMesh.frag.spv", &meshTriangleFragShader))
    {
        std::cout << "Error building mesh triangle fragment shader module" << std::endl;
    }
    else
    {
        std::cout << "Mesh Triangle fragment shader successfully loaded" << std::endl;
    }

    VkShaderModule meshTriangleVertShader;
    if (!loadShaderModule("../shaders/triangleMesh.vert.spv", &meshTriangleVertShader))
    {
        std::cout << "Error building mesh triangle vertex shader module" << std::endl;
    }
    else
    {
        std::cout << "Mesh Triangle vertex shader successfully loaded" << std::endl;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkInit::pipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout));

    VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkInit::pipelineLayoutCreateInfo();
    VkPushConstantRange pushConstant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(MeshPushConstants)};

    meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    meshPipelineLayoutInfo.pushConstantRangeCount = 1;
    VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

    PipelineBuilder pipelineBuilder;
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
    pipelineBuilder.vertexInputInfo = vkInit::vertexInputStateCreateInfo();
    pipelineBuilder.inputAssembly = vkInit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)windowExtent.width,
        .height = (float)windowExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f};

    pipelineBuilder.scissor = {
        .offset = {0, 0},
        .extent = windowExtent};

    pipelineBuilder.rasterizer = vkInit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    pipelineBuilder.multisampling = vkInit::multisampleStateCreateInfo();
    pipelineBuilder.colorBlendAttachment = vkInit::colorBlendAttachmentState();
    pipelineBuilder.pipelineLayout = graphicsPipelineLayout;
    trianglePipeline = pipelineBuilder.buildPipeline(device, renderPass);

    pipelineBuilder.shaderStages.clear();
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, coloredTriangleVertShader));
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, coloredTriangleFragShader));
    coloredTrianglePipeline = pipelineBuilder.buildPipeline(device, renderPass);

    VertexInputDescription vertexDescription = Vertex::getVertexDescription();
    pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

    pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
    pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

    pipelineBuilder.shaderStages.clear();
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshTriangleVertShader));
    pipelineBuilder.shaderStages.push_back(vkInit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, meshTriangleFragShader));

    pipelineBuilder.pipelineLayout = meshPipelineLayout;
    meshPipeline = pipelineBuilder.buildPipeline(device, renderPass);

    vkDestroyShaderModule(device, coloredTriangleVertShader, nullptr);
    vkDestroyShaderModule(device, coloredTriangleFragShader, nullptr);
    vkDestroyShaderModule(device, triangleVertShader, nullptr);
    vkDestroyShaderModule(device, triangleFragShader, nullptr);
    vkDestroyShaderModule(device, meshTriangleVertShader, nullptr);
    vkDestroyShaderModule(device, meshTriangleFragShader, nullptr);

    mainDeletionQueue.pushFunction([=]()
                                   { vkDestroyPipeline(device, meshPipeline, nullptr);
                                     vkDestroyPipeline(device, coloredTrianglePipeline, nullptr);
                                     vkDestroyPipeline(device, trianglePipeline, nullptr);
                                     vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
                                     vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr); });
}

void VulkanEngine::loadMeshes()
{
    triangleMesh.vertices.resize(3);

    triangleMesh.vertices[0].position = {0.0f, -1.0f, 0.0f};
    triangleMesh.vertices[1].position = {1.0f, 1.0f, 0.0f};
    triangleMesh.vertices[2].position = {-1.0f, 1.0f, 0.0f};

    triangleMesh.vertices[0].color = {0.0f, 1.0f, 0.0f};
    triangleMesh.vertices[1].color = {0.0f, 1.0f, 0.0f};
    triangleMesh.vertices[2].color = {0.0f, 1.0f, 0.0f};

    monkeyMesh.loadObj("../assets/monkey_smooth.obj");

    uploadMesh(triangleMesh);
    uploadMesh(monkeyMesh);
}

void VulkanEngine::uploadMesh(Mesh &mesh)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = mesh.vertices.size() * sizeof(Vertex),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};

    VmaAllocationCreateInfo vmaAllocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU};

    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr))

    mainDeletionQueue.pushFunction([=]()
                                   { vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation); });

    void *data;
    vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
    std::memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
}

VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass)
{
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = (uint32_t)shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .layout = pipelineLayout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE};

    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        std::cout << "Failed to create graphics pipeline" << std::endl;
        return VK_NULL_HANDLE;
    }
    return newPipeline;
}