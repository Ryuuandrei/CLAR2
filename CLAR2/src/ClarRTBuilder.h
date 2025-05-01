#pragma once

#include "clar_device.h"
#include "ClarAllocator.h"
#include "ClarModel.h"
#include <glm/gtx/quaternion.hpp>
#include "ClarMaterial.h"

namespace CLAR {

	struct BlasInput {
		uint32_t modelId;
		VkAccelerationStructureGeometryKHR asGeometry;
		VkAccelerationStructureBuildRangeInfoKHR asBuildOffset;
	};

	struct ASBuildInfo {
		uint32_t blasId;
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo;
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo;
		AccelerationStructure as;

		ASBuildInfo() {
			buildInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
			rangeInfo = {};
			sizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
			as = {};
		}
	};

	struct Instance {
		Model* model;
		std::shared_ptr<Material> material;
		glm::vec3 displacement;
		glm::vec3 scale;
		glm::vec3 rotation; // in radians
		uint32_t instanceCustomIndex;

		glm::mat4 TransformMatrix() const {
			glm::mat4 transform = glm::mat4(1.0f);

			// Apply translation
			transform = glm::translate(transform, displacement);

			// Apply rotation using quaternions
			glm::quat rotationQuat = glm::quat(glm::radians(rotation)); // Convert Euler angles to quaternion
			transform *= glm::toMat4(rotationQuat); // Convert quaternion to matrix and apply

			// Apply scale
			transform = glm::scale(transform, scale);

			return transform;
		}

	};

	class RTBuilder {
	public:
		RTBuilder(Device& device, Allocator& allocator);
		~RTBuilder();

		std::vector<ASBuildInfo> BuildBlas(const std::vector<BlasInput>& allblas);
		AccelerationStructure BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances) const;
		AccelerationStructure UpdateTlas(AccelerationStructure& tlas, const std::vector<VkAccelerationStructureInstanceKHR>& instances) const;

	private:
		Device& m_Device;
		Allocator& m_Allocator;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		/*std::vector<ASBuildInfo> buildAs;
		AccelerationStructure m_Tlas;*/

		BlasInput ModelToVkgeometry(const Model* model);
        
	};
}
