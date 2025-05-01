#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <array>

#include "clar_device.h"

namespace CLAR {

    class PipelineBuilder {
    public:
        enum StageIndices
        {
            Raygen,
            Miss,
            ClosestHit,
            ShaderGroupCount
        };

    public:
        std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
        // Instead of pointers, use actual arrays or structs
        VkVertexInputBindingDescription _vertexBindingDescription;
        std::vector<VkVertexInputAttributeDescription> _vertexAttributeDescriptions;
        VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
        VkPipelineRasterizationStateCreateInfo _rasterizer;
        VkPipelineColorBlendAttachmentState _colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo _multisampling;
        VkPipelineLayout _pipelineLayout;
        VkPipelineDepthStencilStateCreateInfo _depthStencil;
        VkRenderPass _renderPass;
        VkExtent2D _extent;

        PipelineBuilder(Device& device, VkRenderPass renderPass = VK_NULL_HANDLE, VkExtent2D extent = {});
        ~PipelineBuilder();

        void SetShader(const std::filesystem::path& shaderPath, VkShaderStageFlagBits stage);
        void SetVertexShaders(const std::filesystem::path& vertShaderPath);
        void SetFragmentShaders(const std::filesystem::path& fragShaderPath);
        void SetComputeShaders(const std::filesystem::path& compShaderPath);
        void SetRayGenShader(const std::filesystem::path& rayGenShaderPath);
        void SetMissShader(const std::filesystem::path& missShaderPath);
        void SetMiss2Shader(const std::filesystem::path& missShaderPath);
        void SetClosestHitShader(const std::filesystem::path& closestHitShaderPath);
        void SetIntersectionShader(const std::filesystem::path& intersectionShaderPath);

        void SetVertexInputDescription(VkVertexInputBindingDescription vertexBindingDescription, const std::vector<VkVertexInputAttributeDescription>& vertexAttributeDescriptions);
        void SetInputTopology(VkPrimitiveTopology topology);
        void SetPolygonMode(VkPolygonMode mode);
        void SetCullMode(VkCullModeFlags mode, VkFrontFace frontFace);
        void DisableAlphaBlending();
        void EnableAlphaBlending();

        void EnableMultiSampling();
        void DisableMultiSampling();

        void Clear();
    private:
        Device& m_Device;
        VkShaderModule CreateShaderModule(const std::vector<char>& code);

        static std::vector<char> readFile(const std::filesystem::path& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();
            return buffer;
        }
	};
}