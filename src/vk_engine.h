#pragma once

#include <vector>
#include <string>
#include <deque>
#include <functional>

#include <vk_types.h>

struct DeletionQueue {
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

        VkPipelineLayout graphicsPipelineLayout;

        DeletionQueue mainDeletionQueue;

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
