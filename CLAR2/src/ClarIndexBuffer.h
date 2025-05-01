#pragma once

#include "clar_device.h"

namespace CLAR {
	class IndexBuffer {
	public:
		IndexBuffer(Device& device, const std::vector<uint32_t>& indices);
		~IndexBuffer();

		IndexBuffer(IndexBuffer&& other) noexcept;

		operator VkBuffer() const { return m_Buffer; }
		VkDeviceSize Size() const { return m_Size; }
	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		VkDeviceSize m_Size = 0;

		Device& m_Device;
	};
}
