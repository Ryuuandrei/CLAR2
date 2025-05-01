#include "ClarModel.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
namespace CLAR {
    static inline uint32_t nextId = 0;

    Model::Model()
    {
        this->id = nextId++;
	}

    void Model::Draw(VkCommandBuffer commandBuffer) const
    {
        VkBuffer vertexBuffers[] = { m_VertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    void Model::LoadModel(const std::filesystem::path& file)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.string().c_str(), "../models")) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        int i = 0;
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],    
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {
                    attrib.colors[3 * index.vertex_index + 0],
                    attrib.colors[3 * index.vertex_index + 1],
                    attrib.colors[3 * index.vertex_index + 2]
                };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(this->mesh.size());
                    this->mesh.push_back(vertex);
                }

                this->indices.push_back(uniqueVertices[vertex]);
                i++;
            }
        }
    }

    void Model::LoadModel(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        this->mesh = vertices;
		this->indices = indices;
    }

    inline static uint32_t id = 0;
}
