#pragma once
#include "ClarRenderSystem.h"

namespace CLAR {
	class GridSystem :public RenderSystem 
	{
	public:
		GridSystem(Device& device) : RenderSystem(device) {}
		~GridSystem() = default;

		virtual void CreatePipeline(
			VkRenderPass renderPass,
			VkExtent2D extent,
			const std::filesystem::path& vertShaderPath = "../shaders/vert.spv",
			const std::filesystem::path& fragShaderPath = "../shaders/frag.spv") override
		{
			m_Pipeline = std::make_unique<GraphicsPipeline>(m_Device);

			PipelineBuilder builder(m_Device, renderPass, extent);
			builder.SetVertexShaders(vertShaderPath);
			builder.SetFragmentShaders(fragShaderPath);
			//builder.SetVertexInputDescription(Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
			builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			builder._pipelineLayout = m_PipelineLayout;

			m_Pipeline->Init(builder);
		}
	};
}
