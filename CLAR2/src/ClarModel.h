#pragma once

#include <filesystem>
#include <vector>

#include "clar_vertex.h"
#include "ClarVertexBuffer.h"
#include "ClarIndexBuffer.h"
#include "ClarAllocator.h"

namespace CLAR {
	/*enum Material {
		LAMBERTIAN,
		METAL,
		DIELECTRIC,
	};*/

	/*struct BlasInput {
		VkAccelerationStructureGeometryKHR asGeometry;
		VkAccelerationStructureBuildRangeInfoKHR asBuildOffset;
	};

	struct ASBuildInfo {
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
		AccelerationStructure as;

		ASBuildInfo() {
			buildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
			rangeInfo = {};
			sizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR } ;
			as = {};
		}
	};*/

	struct Model {
		uint32_t id;
		std::vector<Vertex> mesh;
		std::vector<uint32_t> indices;
		Buffer m_VertexBuffer;
		Buffer m_IndexBuffer;
		Model();

		void Draw(VkCommandBuffer commandBuffer) const;
		void LoadModel(const std::filesystem::path& file);
		void LoadModel(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	};
}
