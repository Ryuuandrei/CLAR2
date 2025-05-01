#pragma once

#include "clar_device.h"
#include <filesystem>
#include <fstream>

namespace CLAR {
	class Pipeline {
	public:
		Pipeline(Device& device) : m_Device(device) {};
		~Pipeline() = default;

		virtual void Bind(VkCommandBuffer commandBuffer) const = 0;
		operator VkPipeline() const { return m_Pipeline; }

	protected:
		Device& m_Device;
		VkPipeline m_Pipeline;

		VkShaderModule CreateShaderModule(const std::vector<char>& code) {
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

		static std::vector<char> readFile(std::filesystem::path filepath) {
			std::ifstream file(filepath, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("filed to open file!");
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
