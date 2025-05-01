#include "ClarRayTracingPipeline.h"

namespace CLAR {

	RayTracingPipeline::RayTracingPipeline(Device& device)
		: m_Device(device)
	{
	}

	RayTracingPipeline::~RayTracingPipeline()
	{
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
	}

	void RayTracingPipeline::Init(const PipelineBuilder& builder)
	{
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

		VkRayTracingShaderGroupCreateInfoKHR groupInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR; // TODO: Implement
		groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR; // TODO: Implement
		groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR; // TODO: Implement

		// Raygen
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupInfo.generalShader = PipelineBuilder::StageIndices::Raygen;
		groups.push_back(groupInfo);

		// Miss
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupInfo.generalShader = PipelineBuilder::StageIndices::Miss;
		groups.push_back(groupInfo);

		//// Miss 2
		//groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		//groupInfo.generalShader = PipelineBuilder::StageIndices::Miss2;
		//groups.push_back(groupInfo);

		// Closest Hit
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		groupInfo.closestHitShader = PipelineBuilder::StageIndices::ClosestHit;
		groups.push_back(groupInfo);

		// Intersection
		groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
		groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		groupInfo.intersectionShader = 3; // TODO: Implement
		groups.push_back(groupInfo);

		VkPipelineLibraryCreateInfoKHR libraryInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
			.libraryCount = 0, // TODO: Implement
			.pLibraries = nullptr // TODO: Implement
		};

		VkRayTracingPipelineInterfaceCreateInfoKHR libraryInterface = {
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR,
			.maxPipelineRayPayloadSize = 0, // TODO: Implement
			.maxPipelineRayHitAttributeSize = 0 // TODO: Implement
		};

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
		};

		VkRayTracingPipelineCreateInfoKHR pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
			.stageCount = static_cast<uint32_t>(builder._shaderStages.size()),
			.pStages = builder._shaderStages.data(),
			.groupCount = PipelineBuilder::StageIndices::ShaderGroupCount,
			.pGroups = groups.data(),
			.maxPipelineRayRecursionDepth = 1, // TODO: can work like this, figure out how to implement
			.pLibraryInfo = nullptr, // TODO: can work like this, figure out how to implement
			.pLibraryInterface = nullptr, // TODO: can work like this, figure out how to implement
			.pDynamicState = nullptr, // TODO: can work like this, figure out how to implement
			.layout = builder._pipelineLayout,
		};

		if (vkCreateRayTracingPipelinesKHR(m_Device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Ray Tracing Pipeline");
		}
	}

	void RayTracingPipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline);
	}

}
