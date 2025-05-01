#include "ClarPipelineBuilder.h"

namespace CLAR {

	PipelineBuilder::PipelineBuilder(Device& device, VkRenderPass renderPass, VkExtent2D extent)
		: m_Device(device), _renderPass(renderPass), _extent(extent)
	{
		_inputAssembly = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		_rasterizer = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 0.f,
			.lineWidth = 1.f,
		};

		_colorBlendAttachment = {
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		_multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		_depthStencil = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};
		_vertexAttributeDescriptions = {};

		_pipelineLayout = VK_NULL_HANDLE;
	}

	PipelineBuilder::~PipelineBuilder()
	{
		for (auto& shaderStage : _shaderStages) {
			vkDestroyShaderModule(m_Device, shaderStage.module, nullptr);
		}
	}

	void PipelineBuilder::SetShader(const std::filesystem::path& shaderPath, VkShaderStageFlagBits stage)
	{
		_shaderStages.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = stage,
			.module = CreateShaderModule(readFile(shaderPath)),
			.pName = "main"
			});
	}

	void PipelineBuilder::SetVertexShaders(const std::filesystem::path& vertShaderPath)
	{
		PipelineBuilder::SetShader(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
	}

	void PipelineBuilder::SetFragmentShaders(const std::filesystem::path& fragShaderPath)
	{
		PipelineBuilder::SetShader(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	void PipelineBuilder::SetComputeShaders(const std::filesystem::path& compShaderPath)
	{
		PipelineBuilder::SetShader(compShaderPath, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	void PipelineBuilder::SetRayGenShader(const std::filesystem::path& rayGenShaderPath)
	{
		if (_shaderStages.size() != 0) {
			throw std::runtime_error("Ray generation shader must be the first shader in the list!");
		}
		PipelineBuilder::SetShader(rayGenShaderPath, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	}

	void PipelineBuilder::SetMissShader(const std::filesystem::path& missShaderPath)
	{
		if (_shaderStages.size() != 1) {
			throw std::runtime_error("Miss shader must be the second shader in the list!");
		}
		PipelineBuilder::SetShader(missShaderPath, VK_SHADER_STAGE_MISS_BIT_KHR);
	}

	void PipelineBuilder::SetMiss2Shader(const std::filesystem::path& missShaderPath)
	{
		if (_shaderStages.size() != 2) {
			throw std::runtime_error("Miss shader must be the third shader in the list!");
		}
		PipelineBuilder::SetShader(missShaderPath, VK_SHADER_STAGE_MISS_BIT_KHR);
	}

	void PipelineBuilder::SetClosestHitShader(const std::filesystem::path& closestHitShaderPath)
	{
		if (_shaderStages.size() != 2) {
			throw std::runtime_error("Closest hit shader must be the third shader in the list!");
		}
		PipelineBuilder::SetShader(closestHitShaderPath, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	}

	void PipelineBuilder::SetIntersectionShader(const std::filesystem::path& intersectionShaderPath)
	{
		if (_shaderStages.size() != 3) {
			throw std::runtime_error("Intersection shader must be the fourth shader in the list!");
		}
		PipelineBuilder::SetShader(intersectionShaderPath, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
	}

	void PipelineBuilder::SetVertexInputDescription(VkVertexInputBindingDescription vertexBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions)
	{
		_vertexBindingDescription = vertexBindingDescription;
		_vertexAttributeDescriptions = vertexAttributeDescriptions;
	}

	void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
	{
		_inputAssembly.topology = topology;
		_inputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	void PipelineBuilder::SetPolygonMode(VkPolygonMode mode)
	{
		_rasterizer.polygonMode = mode;
		_rasterizer.lineWidth = 1.f;
	}

	void PipelineBuilder::SetCullMode(VkCullModeFlags mode, VkFrontFace frontFace)
	{
		_rasterizer.cullMode = mode;
		_rasterizer.frontFace = frontFace;
	}

	void PipelineBuilder::DisableAlphaBlending()
	{
		_colorBlendAttachment.blendEnable = VK_FALSE;
	}

	void PipelineBuilder::EnableAlphaBlending()
	{
		_colorBlendAttachment.blendEnable = VK_TRUE;
		_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}

	void PipelineBuilder::EnableMultiSampling()
	{
		_multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = m_Device.MsaaSamples,
			.sampleShadingEnable = VK_TRUE,
			.minSampleShading = .2f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};
	}

	void PipelineBuilder::DisableMultiSampling()
	{
		_multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};
	}

	void PipelineBuilder::Clear()
	{
		_inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

		_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

		_colorBlendAttachment = {};

		_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

		_pipelineLayout = {};

		_depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		_shaderStages.clear();

		_vertexBindingDescription = {};

		_vertexAttributeDescriptions.clear();

		_extent = {};

		_renderPass = VK_NULL_HANDLE;
	}

	VkShaderModule PipelineBuilder::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

}