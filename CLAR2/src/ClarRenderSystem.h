#pragma once

#include "ClarGraphicsPipeline.h"
#include "ClarDescriptors.h"

namespace CLAR {
	class RenderSystem {
	public:
		RenderSystem(Device& device);
		virtual ~RenderSystem();

		void Init(const VkDescriptorSetLayout* descriptorSetLayout, VkRenderPass renderPass, VkExtent2D extent, const std::filesystem::path& vertShaderPath = "shaders/vert.spv", const std::filesystem::path& fragShaderPath = "shaders/frag.spv");

		virtual void CreatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout);
		virtual void CreatePipeline(VkRenderPass renderPass, VkExtent2D extent, const std::filesystem::path& vertShaderPath = "shaders/vert.spv", const std::filesystem::path& fragShaderPath = "shaders/frag.spv");

		void Prepare(const VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const;

		void BindPL(VkCommandBuffer commandBuffer) const;
		void BindDescSet(VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const;

	protected:
		Device& m_Device;

		std::unique_ptr<GraphicsPipeline> m_Pipeline;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	};
}
