#include "ClarComputeSystem.h"
#include "Particle.h"

namespace CLAR {

	ComputeSystem::ComputeSystem(Device& device)
		: m_Device(device)
	{
	}

	ComputeSystem::~ComputeSystem()
	{
		vkDestroyPipelineLayout(m_Device, m_ComputePipelineLayout, nullptr);
	}

	void ComputeSystem::Init(const VkDescriptorSetLayout* descriptorSetLayout, const std::filesystem::path& compShaderPath)
	{
		CreatePipelineLayout(descriptorSetLayout);
		CreatePipeline(compShaderPath);
	}

	void ComputeSystem::CreatePipeline(const std::filesystem::path& compShaderPath)
	{
		m_ComputePipeline = std::make_unique<ComputePipeline>(m_Device);

		PipelineBuilder builder(m_Device);
		builder.SetComputeShaders(compShaderPath);
		builder._pipelineLayout = m_ComputePipelineLayout;

		m_ComputePipeline->Init(builder);
	}

	void ComputeSystem::Prepare(const VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const
	{
		BindPL(commandBuffer);

		BindDescSet(commandBuffer, descriptorSet);
	}

	void ComputeSystem::BindPL(VkCommandBuffer commandBuffer) const
	{
		m_ComputePipeline->Bind(commandBuffer);
	}

	void ComputeSystem::BindDescSet(VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, descriptorSet, 0, nullptr);
	}

	void ComputeSystem::CreatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)(descriptorSetLayout ? 1 : 0),
			.pSetLayouts = descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr
		};

		if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_ComputePipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}
}

