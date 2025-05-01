#pragma once
#include "ClarRayTracingSystem.h"
#include "clar_vertex.h"

namespace CLAR {

	RayTracingSystem::RayTracingSystem(Device& device)
		: m_Device(device)
	{
	}

	RayTracingSystem::~RayTracingSystem()
	{
		vkDestroyPipelineLayout(m_Device, m_RaytracingPipelineLayout, nullptr);
	}

	void RayTracingSystem::Init(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout)
	{
		CreatePipelineLayout(descriptorSetLayout);
		CreatePipeline();
	}

	void RayTracingSystem::CreatePipeline()
	{
		m_RaytracingPipeline = std::make_unique<RayTracingPipeline>(m_Device);

		PipelineBuilder builder(m_Device);
		builder.SetRayGenShader("shaders/rtShaders/raytrace.rgen.spv");
		builder.SetMissShader("shaders/rtShaders/raytrace.rmiss.spv");
		//builder.SetMiss2Shader("shaders/raytraceShadow.rmiss.spv");
		builder.SetClosestHitShader("shaders/rtShaders/raytrace.rchit.spv");
		builder.SetIntersectionShader("shaders/rtShaders/raytrace.rint.spv");

		builder._pipelineLayout = m_RaytracingPipelineLayout;

		m_RaytracingPipeline->Init(builder);
	}

	void RayTracingSystem::Prepare(const VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet>& descriptorSet) const
	{
		m_RaytracingPipeline->Bind(commandBuffer);
		BindDescSet(commandBuffer, descriptorSet);
	}

	void RayTracingSystem::BindPL(VkCommandBuffer commandBuffer) const
	{
		m_RaytracingPipeline->Bind(commandBuffer);
	}

	void RayTracingSystem::BindDescSet(VkCommandBuffer commandBuffer, const std::vector<VkDescriptorSet>& descriptorSet) const
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RaytracingPipelineLayout, 0, static_cast<uint32_t>(descriptorSet.size()), descriptorSet.data(), 0, nullptr);
	}

	void RayTracingSystem::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayout)
	{
		VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
								 0, sizeof(PushConstantRay) };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(descriptorSetLayout.size()),
			.pSetLayouts = descriptorSetLayout.data(),
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstant
		};

		if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_RaytracingPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}
}
