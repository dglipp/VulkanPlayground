#include <vk_mesh.h>

VertexInputDescription Vertex::getVertexDescription()
{
    VertexInputDescription description;

    VkVertexInputBindingDescription mainBinding = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

    description.bindings.push_back(mainBinding);

    VkVertexInputAttributeDescription positionAttribute = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position)};

    VkVertexInputAttributeDescription normalAttribute = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal)};

    VkVertexInputAttributeDescription colorAttribute = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)};

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);

    return description;
}