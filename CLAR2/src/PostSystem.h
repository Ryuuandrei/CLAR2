#pragma once
#include "ClarRenderSystem.h"

namespace CLAR {
	class PostSystem : public RenderSystem {
	public:
		PostSystem(Device& device) : RenderSystem(device) {}
		~PostSystem() = default;

		virtual void CreatePipeline(
			VkRenderPass renderPass,
			VkExtent2D extent,
			const std::filesystem::path& vertShaderPath = "../shaders/post.vert.spv",
			const std::filesystem::path& fragShaderPath = "../shaders/post.frag.spv") override
		{
			m_Pipeline = std::make_unique<GraphicsPipeline>(m_Device);

			PipelineBuilder builder(m_Device, renderPass, extent);
			builder.SetVertexShaders(vertShaderPath);
			builder.SetFragmentShaders(fragShaderPath);
			builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			builder._pipelineLayout = m_PipelineLayout;

			m_Pipeline->Init(builder);
		}
	};
}
