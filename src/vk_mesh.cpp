#include <vk_mesh.h>

#include <iostream>

#include "tiny_obj_loader.h"

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

bool Mesh::loadObj(std::string filename)
{
    tinyobj::attrib_t attrib;

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), nullptr);

    if(!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }
    if(!err.empty())
    {
        std::cerr << err << std::endl;
        return false;
    }

    for (size_t s = 0; s < shapes.size(); ++s)
    {
        size_t indexOffset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
        {
            int fv = 3;

            for (size_t v = 0; v < fv; ++v)
            {
                tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                tinyobj::real_t nx = attrib.normals[3 * idx.vertex_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.vertex_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.vertex_index + 2];

                Vertex newVertex = {
                    .position = {vx, vy, vz},
                    .normal = {nx, ny, nz},
                    .color = newVertex.normal};

                vertices.push_back(newVertex);
            }
            indexOffset += fv;
        }
    }

    return true;
}