#pragma once

#include "ClarRayTracingPipeline.h"
#include "ClarDescriptors.h"

namespace CLAR {
	struct PushConstantRay
	{
		alignas(16) glm::vec4 clearColor;
		alignas(4) float      deltaTime = 0.f;
		alignas(4) float	  time = 0.f;
		alignas(4) int		  lightsNumber;
	};

	class RayTracingSystem {
	public:
		RayTracingSystem(Device& device);
		~RayTracingSystem();

		void Init(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);

		void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
		void CreatePipeline();

		void Prepare(const VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet>& descriptorSet) const;

		void BindPL(VkCommandBuffer commandBuffer) const;
		void BindDescSet(VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet>& descriptorSet) const;
		VkResult GetRayTracingShaderGroupHandles(uint32_t groupCount, size_t dataSize, void* pData) const { return vkGetRayTracingShaderGroupHandlesKHR(m_Device, *m_RaytracingPipeline, 0, groupCount, dataSize, pData); };
		void PushConstants(VkCommandBuffer commandBuffer, const PushConstantRay& pcRay) const { vkCmdPushConstants(commandBuffer, m_RaytracingPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantRay), &pcRay); }

	private:
		Device& m_Device;

		std::unique_ptr<RayTracingPipeline> m_RaytracingPipeline;
		VkPipelineLayout m_RaytracingPipelineLayout = VK_NULL_HANDLE;
	};
}
