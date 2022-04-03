#pragma once

#include <vector>
#include <string>
#include <deque>
#include <functional>

#include "glm/glm.hpp"
#include "vk_mem_alloc.h"

#include "vk_types.h"
#include "vk_mesh.h"

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 renderMatrix;
};

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()> && function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        for (auto deletor : deletors)
            deletor();

        deletors.clear();
    }
};

class VulkanEngine {
    public:
        int selectedShader{0};
        bool isInitialized{false};
        int frameNumber {0};
        VkExtent2D windowExtent{ 1700 , 900 };
        struct SDL_Window* window{ nullptr };

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkSurfaceKHR surface;

        VkSwapchainKHR swapchain;
        VkFormat swapchainImageFormat;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;

        VkQueue graphicsQueue;
        uint32_t graphicsQueueFamily;

        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        VkRenderPass renderPass;
        std::vector<VkFramebuffer> framebuffers;

        VkSemaphore renderSemaphore;
        VkSemaphore presentSemaphore;
        VkFence renderFence;

        VkPipeline trianglePipeline;
        VkPipeline coloredTrianglePipeline;
        
        VkPipeline meshPipeline;
        Mesh triangleMesh;
        Mesh monkeyMesh;

        VkPipelineLayout graphicsPipelineLayout;
        VkPipelineLayout meshPipelineLayout;

        DeletionQueue mainDeletionQueue;

        VmaAllocator allocator;

        void init();

        void cleanup();

        void draw();

        void run();

    private:
        void initVulkan();
        void initSwapchain();
        void initCommands();
        void initDefaultRenderpass();
        void initFramebuffers();
        void initSyncStructures();
        bool loadShaderModule(std::string filepath, VkShaderModule *outShaderModule);
        void initPipelines();

        void loadMeshes();
        void uploadMesh(Mesh &mesh);
};

class PipelineBuilder
{
    public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineLayout pipelineLayout;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo multisampling;

        VkViewport viewport;
        VkRect2D scissor;

        VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
};
