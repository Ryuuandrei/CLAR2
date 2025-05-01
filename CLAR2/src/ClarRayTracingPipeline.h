#pragma once

#include "ClarPipelineBuilder.h"

namespace CLAR {
	class RayTracingPipeline {
	public:
		RayTracingPipeline(Device& device);
		~RayTracingPipeline();

		void Init(const PipelineBuilder& builder);
		void Bind(VkCommandBuffer commandBuffer) const;

		operator VkPipeline() const { return m_Pipeline; }
	private:
		Device& m_Device;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

	};
}
