#pragma once

#include "vma/vk_mem_alloc.h"
#include "clar_device.h"

namespace CLAR {

	struct Buffer {
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		VkDescriptorBufferInfo DescriptorInfo() const { return { buffer, 0,  VK_WHOLE_SIZE/* allocationInfo.size */ }; }
		void Write(const void* src, VkDeviceSize size, VkDeviceSize offset = 0) const;
	};

	struct Image {
		VkImage image;
		VmaAllocation allocation;
	};

	struct Texture {
		Image image;
		VkDescriptorImageInfo descriptor;
		uint32_t mipLevels = 1;
		int width = 0;
		int height = 0;
	};

	struct AccelerationStructure {
		Buffer buffer;
		VkAccelerationStructureKHR handle;
	};

	class Allocator {
	public:
		Allocator(Device& device);
		~Allocator();

		Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0) const;

		template<typename T>
		Buffer CreateBuffer(const std::vector<T>& elements, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0) const;
		template<typename T>
		Buffer CreateBuffer(const T& element, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0) const;

		Image CreateImage(VkExtent2D size, VkFormat format, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags = 0, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, uint32_t mipLevels = 1) const;
		Texture CreateTexture(const Image& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;

		AccelerationStructure CreateAccelerationStructure(VkAccelerationStructureCreateInfoKHR& createInfo) const;

		void DestroyBuffer(const Buffer& buffer) const;
		void DestroyImage(const Image& image) const;
		void DestroyTexture(const Texture& texture) const;
		void DestroyAccelerationStructure(const AccelerationStructure& as) const;
	private:
		VmaAllocator m_Allocator;
		Device& m_Device;

	private:
		void Init(const Device& device);
		/*std::vector<Buffer*> m_Buffers;
		std::vector<Image*> m_Images;
		std::vector<Texture2*> m_Textures;*/
	};
	
	template<typename T>
	inline Buffer Allocator::CreateBuffer(const std::vector<T>& elements, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) const
	{
		VkDeviceSize size = elements.size() * sizeof(T);

		Buffer buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, flags);
		Buffer stagingBuffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		stagingBuffer.Write(elements.data(), size);

		m_Device.CopyBuffer(stagingBuffer.buffer, buffer.buffer, size);
		DestroyBuffer(stagingBuffer);
		return buffer;
	}

	template<typename T>
	inline Buffer Allocator::CreateBuffer(const T& element, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) const
	{
		VkDeviceSize size = sizeof(T);

		Buffer buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, flags);
		Buffer stagingBuffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		stagingBuffer.Write(&element, size);

		m_Device.CopyBuffer(stagingBuffer.buffer, buffer.buffer, size);
		DestroyBuffer(stagingBuffer);
		return buffer;
	}
}
