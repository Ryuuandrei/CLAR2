#include "ClarAllocator.h"

namespace CLAR {

	Allocator::Allocator(Device& device)
		: m_Device(device)
	{
		Init(device);
	}

	Allocator::~Allocator()
	{
		vmaDestroyAllocator(m_Allocator);
	}

	Buffer Allocator::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags) const
	{
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = flags;

		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo);
		return { buffer, allocation, allocationInfo };
	}

	void Allocator::DestroyBuffer(const Buffer& buffer) const
	{
		vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
	}

	void Allocator::DestroyImage(const Image& image) const
	{
		vmaDestroyImage(m_Allocator, image.image, image.allocation);
	}

	void Allocator::DestroyTexture(const Texture& texture) const
	{
		vkDestroyImageView(m_Device, texture.descriptor.imageView, nullptr);
		vkDestroySampler(m_Device, texture.descriptor.sampler, nullptr);
		DestroyImage(texture.image);
	}

	void Allocator::DestroyAccelerationStructure(const AccelerationStructure& as) const
	{
		DestroyBuffer(as.buffer);
		vkDestroyAccelerationStructureKHR(m_Device, as.handle, nullptr);
	}

	Image Allocator::CreateImage(VkExtent2D size, VkFormat format, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, VkSampleCountFlagBits samples, uint32_t mipLevels) const
	{
		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent{
				.width = (uint32_t)size.width,
				.height = (uint32_t)size.height,
				.depth = 1
			},
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = samples,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = flags;

		VkImage image;
		VmaAllocation allocation;
		VmaAllocationInfo allocationInfo;

		vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo);
		return { image, allocation };
	}

	Texture Allocator::CreateTexture(const Image& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
	{
		Texture texture{};
		texture.image = image;
		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange{
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		if (vkCreateImageView(m_Device, &viewInfo, nullptr, &texture.descriptor.imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}

		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.mipLodBias = 0.0f,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0,
			.maxLod = 1.0f, // TO DO: change this
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VkPhysicalDeviceProperties properties = m_Device.GetPhysicalDeviceProperties();

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f; // TO DO: change this

		// samplerInfo.anisotropyEnable = VK_FALSE;
		// samplerInfo.maxAnisotropy = 1.0f;

		if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &texture.descriptor.sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		texture.mipLevels = mipLevels;

		return texture;
	}

	AccelerationStructure Allocator::CreateAccelerationStructure(VkAccelerationStructureCreateInfoKHR& createInfo) const
	{
		Buffer buffer = CreateBuffer(createInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		VkAccelerationStructureKHR accelerationstructure{};

		createInfo.buffer = buffer.buffer;
		vkCreateAccelerationStructureKHR(m_Device, &createInfo, nullptr, &accelerationstructure);
		return { buffer, accelerationstructure };
	}

	void Allocator::Init(const Device& device)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = device.GPU();
		allocatorInfo.device = device;
		allocatorInfo.instance = device.GetInstance();
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	void Buffer::Write(const void* src, VkDeviceSize size, VkDeviceSize offset) const
	{
		// if the buffer was not created with VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT first map the memory
		/*if (allocationInfo.pMappedData == nullptr)
		{
			vmaMapMemory(m_Allocator, allocation, &allocationInfo.pMappedData);
		}*/
		memcpy((char*)allocationInfo.pMappedData + offset, src, size);
	}

}
