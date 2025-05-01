#pragma once

#include "clar_device.h"

namespace CLAR {
	class UniformBuffer {
	public:
		UniformBuffer(Device& device, VkDeviceSize size);
		~UniformBuffer();

		UniformBuffer(UniformBuffer&& other) noexcept;
		void Write(const void* src, size_t size);

		operator VkBuffer() const { return m_Buffer; }
		void* GetMappedMemory() const { return data; }
		VkDescriptorBufferInfo DescriptorInfo() const { return { m_Buffer, 0, m_Size }; }
	private:
		Device& m_Device;

		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		void* data = nullptr;
		VkDeviceSize m_Size = 0;
	};
}
