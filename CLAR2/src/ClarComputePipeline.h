#pragma once

#include "ClarPipelineBuilder.h"

namespace CLAR {

	class ComputePipeline {
	public:
		ComputePipeline(Device& device);
		~ComputePipeline();

		void Init(const PipelineBuilder& builder);
		void Bind(VkCommandBuffer commandBuffer) const;

		operator VkPipeline() const { return m_Pipeline; }
	private:
		Device& m_Device;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}