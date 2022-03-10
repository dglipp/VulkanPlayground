#pragma once

#include <vector>

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
};
