#include "ClarUbo.h"

namespace CLAR {

	UniformBuffer::UniformBuffer(Device& device, VkDeviceSize size)
		: m_Device(device), m_Size(size)
	{
		m_Device.createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_Buffer, m_BufferMemory);

		vkMapMemory(m_Device, m_BufferMemory, 0, size, 0, &data);
	}

	UniformBuffer::~UniformBuffer()
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		vkFreeMemory(m_Device, m_BufferMemory, nullptr);
	}

	UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
		: m_Device(other.m_Device), m_Size(other.m_Size)
	{
		m_Buffer = other.m_Buffer;
		m_BufferMemory = other.m_BufferMemory;
		data = other.data;

		other.m_Buffer = VK_NULL_HANDLE;
		other.m_BufferMemory = VK_NULL_HANDLE;
		other.data = nullptr;
	}
	void UniformBuffer::Write(const void* src, size_t size)
	{
		memcpy(data, src, size);
	}
}