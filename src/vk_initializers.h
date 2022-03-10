#pragma once

#include <vk_types.h>

namespace vkInit
{
    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyType, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}
