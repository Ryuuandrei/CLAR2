#pragma once

#include "clar_device.h"
#include "clar_vertex.h"

namespace CLAR {

	class VertexBuffer {
	public:
		VertexBuffer(Device& device, const std::vector<Vertex>& vertices);
		~VertexBuffer();

		VertexBuffer(VertexBuffer&& other) noexcept;

		operator VkBuffer() const { return m_Buffer; }
		VkDeviceSize Size() const { return m_Size; }
	private:
		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;
		VkDeviceSize m_Size = 0;

		Device& m_Device;
	};
}