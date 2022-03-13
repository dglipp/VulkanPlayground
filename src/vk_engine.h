﻿#pragma once

#include <vector>
#include <string>

#include <vk_types.h>

class VulkanEngine {
    public:
        bool isInitialized{ false };
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

        VkPipeline graphicsPipeline;
        VkPipelineLayout graphicsPipelineLayout;

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