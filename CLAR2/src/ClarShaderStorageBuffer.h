#pragma once

#include "clar_device.h"

namespace CLAR {
	class ShaderStorageBuffer {
	public:
		ShaderStorageBuffer(Device& device, VkDeviceSize size, const void* src);
		~ShaderStorageBuffer();

		ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;
		/*void Map();
		void Unmap();

		void Write(const void* data, VkDeviceSize size);
		void Read(void* data, VkDeviceSize size);*/

		operator VkBuffer() const { return m_Buffer; }
		VkDeviceSize Size() const { return m_Size; }
		VkDescriptorBufferInfo DescriptorInfo() const { return { m_Buffer, 0, m_Size }; }
		VkBuffer Get() const { return m_Buffer; }
	private:
		Device& m_Device;
		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		VkDeviceSize m_Size;
	};
}