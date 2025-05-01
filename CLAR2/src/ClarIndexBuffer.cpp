#include "ClarIndexBuffer.h"

namespace CLAR {

	IndexBuffer::IndexBuffer(Device& device, const std::vector<uint32_t>& indices)
		: m_Device(device), m_Size(indices.size() * sizeof(uint32_t))
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_Device.createBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, m_Size, 0, &data);
		memcpy(data, indices.data(), (size_t)m_Size);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		m_Device.createBuffer(m_Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_Buffer, m_BufferMemory);

		m_Device.CopyBuffer(stagingBuffer, m_Buffer, m_Size);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	IndexBuffer::~IndexBuffer()
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		vkFreeMemory(m_Device, m_BufferMemory, nullptr);
	}

	IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
		: m_Device(other.m_Device), m_Size(other.m_Size), m_Buffer(other.m_Buffer), m_BufferMemory(other.m_BufferMemory)
	{
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_BufferMemory = VK_NULL_HANDLE;
		other.m_Size = 0;
	}

}