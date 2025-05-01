#pragma once

#include "ClarPipelineBuilder.h"

namespace CLAR {

	class GraphicsPipeline {
	public:
		GraphicsPipeline(Device& device);
		~GraphicsPipeline();

		void Init(const PipelineBuilder& builder);
		void Bind(VkCommandBuffer commandBuffer) const;

		operator VkPipeline() const { return m_Pipeline; }
	private:
		Device& m_Device;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

	};

}
