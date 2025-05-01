#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

#include "clar_device.h"
#include "ClarBuffer.h"

namespace CLAR {

	class DescriptorSetLayout {
	public:
		DescriptorSetLayout(Device& device);
		~DescriptorSetLayout();

		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		void PushBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, uint32_t count = 1);
		void PushBinding(const VkDescriptorSetLayoutBinding& bindingSpec);
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> CreateSets();

		void PushVertexUniformBuffer(uint32_t binding);
		void PushFragmentUniformBuffer(uint32_t binding);
		void PushComputeStorageBuffer(uint32_t binding);

		operator VkDescriptorSetLayout() { return m_DescriptorSetLayout; }
		operator VkDescriptorSetLayout* () { return &m_DescriptorSetLayout; }

		std::vector<VkDescriptorSetLayoutBinding>& GetBindings() { return bindings; }
		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

	private:
		Device& m_Device;
		VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayoutBinding> bindings{};

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> AllocateDescriptorSets() const;

	public:
		void CreateDescriptorPool(VkDescriptorPoolCreateFlags _flags = 0);
	};

	// ----------------------------------------------------------------------------------

	class DescritorWriter {
	public:
		DescritorWriter() = default;
		~DescritorWriter() = default;

		DescritorWriter& WriteUniformBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);
		DescritorWriter& WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);
		DescritorWriter& WriteStorageBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);
		DescritorWriter& WriteAccelerationStructure(uint32_t binding, const VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo);
		DescritorWriter& WriteStorageImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);
		void Update(VkDescriptorSet dstSet, Device& device);

	private:
		std::unordered_map<int, VkWriteDescriptorSet> m_DescriptorWrites;
	};
}

