#include "ClarRenderSystem.h"
#include "Particle.h"
#include "clar_vertex.h"

namespace CLAR {

    RenderSystem::RenderSystem(Device& device)
        : m_Device(device)
    {
    }

    RenderSystem::~RenderSystem()
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    }


    void RenderSystem::Init(const VkDescriptorSetLayout* descriptorSetLayout, VkRenderPass renderPass, VkExtent2D extent, const std::filesystem::path& vertShaderPath, const std::filesystem::path& fragShaderPath)
	{
		CreatePipelineLayout(descriptorSetLayout);
		CreatePipeline(renderPass, extent, vertShaderPath, fragShaderPath);
	}

    void RenderSystem::CreatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout)
	{
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = descriptorSetLayout ? 1u : 0u,
            .pSetLayouts = descriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
	}

    void RenderSystem::CreatePipeline(VkRenderPass renderPass, VkExtent2D extent, const std::filesystem::path& vertShaderPath, const std::filesystem::path& fragShaderPath)
    {
        m_Pipeline = std::make_unique<GraphicsPipeline>(m_Device);

        PipelineBuilder builder(m_Device, renderPass, extent);
        builder.SetVertexShaders(vertShaderPath);
        builder.SetFragmentShaders(fragShaderPath);
        builder.SetVertexInputDescription(Vertex::getBindingDescription(), Vertex::getAttributeDescriptions());
        builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        builder._pipelineLayout = m_PipelineLayout;

        m_Pipeline->Init(builder);
    }

    void RenderSystem::BindPL(VkCommandBuffer commandBuffer) const
    {
        m_Pipeline->Bind(commandBuffer);
    }

    void RenderSystem::BindDescSet(VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, descriptorSet, 0, nullptr);
    }

    void RenderSystem::Prepare(const VkCommandBuffer commandBuffer, const VkDescriptorSet* descriptorSet) const
    {
        BindPL(commandBuffer);
        BindDescSet(commandBuffer, descriptorSet);
    }

}
