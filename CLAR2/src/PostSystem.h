#pragma once
#include "ClarRenderSystem.h"

namespace CLAR {
	class PostSystem : public RenderSystem {
	public:
		PostSystem(Device& device) : RenderSystem(device) {}
		~PostSystem() = default;

		void CreatePipeline(
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

		void CreatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout) override
		{
			VkPushConstantRange pushConstant{ VK_SHADER_STAGE_FRAGMENT_BIT,
								 0, sizeof(PushConstantRay) };

			VkPipelineLayoutCreateInfo pipelineLayoutInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = descriptorSetLayout ? 1u : 0u,
				.pSetLayouts = descriptorSetLayout,
				.pushConstantRangeCount = 1,
				.pPushConstantRanges = &pushConstant
			};

			if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}
		}


		template<typename T>
		void PushConstants(VkCommandBuffer commandBuffer, const T& pcRay) const { vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(T), &pcRay); }

		void PushConstants(VkCommandBuffer commandBuffer, uint32_t size, const void* pcRay) const { vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, pcRay); }
	};
}
