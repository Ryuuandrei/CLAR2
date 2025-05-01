#include "ClarComputePipeline.h"

namespace CLAR {

	ComputePipeline::ComputePipeline(Device& device)
		: m_Device(device)
	{
	}

	ComputePipeline::~ComputePipeline()
	{
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
	}

	void ComputePipeline::Init(const PipelineBuilder& builder)
	{
		VkComputePipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = builder._shaderStages[0],
			.layout = builder._pipelineLayout,
			/*.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1*/
		};

		if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}
	}

	void ComputePipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}
}
