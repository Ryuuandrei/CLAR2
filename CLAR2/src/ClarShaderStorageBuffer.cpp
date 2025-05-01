#include "ClarShaderStorageBuffer.h"

namespace CLAR {
	ShaderStorageBuffer::ShaderStorageBuffer(Device& device, VkDeviceSize size, const void* src)
		: m_Device(device), m_Size(size)
	{
		m_Device.createBuffer(m_Size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Buffer, m_BufferMemory);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_Device.createBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, m_Size, 0, &data);
		memcpy(data, src, (size_t)m_Size);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		m_Device.CopyBuffer(stagingBuffer, m_Buffer, m_Size);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	ShaderStorageBuffer::~ShaderStorageBuffer()
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		vkFreeMemory(m_Device, m_BufferMemory, nullptr);
	}

	ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
		: m_Device(other.m_Device), m_Buffer(other.m_Buffer), m_BufferMemory(other.m_BufferMemory), m_Size(other.m_Size)
	{
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_BufferMemory = VK_NULL_HANDLE;
		other.m_Size = 0;
	}
}
