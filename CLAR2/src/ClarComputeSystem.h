#pragma once

#include "ClarComputePipeline.h"
#include "ClarDescriptors.h"

namespace CLAR {
	class ComputeSystem {
	public:
		ComputeSystem(Device& device);
		~ComputeSystem();

		void Init(const VkDescriptorSetLayout* descriptorSetLayout, const std::filesystem::path& compShaderPath);

		void CreatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout);
		void CreatePipeline(const std::filesystem::path& compShaderPath = "../shaders/comp.spv");

		void Prepare(const VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const;

		void BindPL(VkCommandBuffer commandBuffer) const;
		void BindDescSet(VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const;

	private:
		Device& m_Device;

		std::unique_ptr<ComputePipeline> m_ComputePipeline;
		VkPipelineLayout m_ComputePipelineLayout = VK_NULL_HANDLE;
	};
}