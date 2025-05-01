#include "ClarGraphicsPipeline.h"

namespace CLAR {

    GraphicsPipeline::GraphicsPipeline(Device& device)
        : m_Device(device)
    {
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    }

    void GraphicsPipeline::Init(const PipelineBuilder& builder)
    {
        
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = builder._vertexAttributeDescriptions.size() != 0 ? 1U : 0U,
            .pVertexBindingDescriptions = &builder._vertexBindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(builder._vertexAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = builder._vertexAttributeDescriptions.data()
        };

        VkViewport viewport{
            .x = 0.f,
            .y = 0.f,
            .width = (float)builder._extent.width,
            .height = (float)builder._extent.height,
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor{
            .offset = {0, 0},
            .extent = builder._extent
        };

        VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        VkPipelineColorBlendStateCreateInfo colorBlending{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &builder._colorBlendAttachment,
            .blendConstants = { 0.f ,  0.f , 0.f , 0.f }
        };

        VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = (uint32_t)builder._shaderStages.size(),
            .pStages = builder._shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &builder._inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &builder._rasterizer,
            .pMultisampleState = &builder._multisampling,
            .pDepthStencilState = &builder._depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = builder._pipelineLayout,
            .renderPass = builder._renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1,
        };

        if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

    void GraphicsPipeline::Bind(VkCommandBuffer commandBuffer) const
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }
}
