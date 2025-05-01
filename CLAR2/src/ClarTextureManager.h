#pragma once

#include "clar_device.h"
#include "ClarAllocator.h"
#include <unordered_map>
#include <filesystem>

namespace CLAR {

	/*struct Texture {
		VkImage m_TextureImage = VK_NULL_HANDLE;
		VkDeviceMemory m_TextureImageMemory = VK_NULL_HANDLE;
		VkImageView m_TextureImageView = VK_NULL_HANDLE;
		uint32_t mipLevels = 1;
	};*/

	class TextureManager {
	public:
		TextureManager(Device& device, Allocator& Allocator);
		~TextureManager();

		void LoadFromFile(const std::filesystem::path& path, const char* name, bool generateMipMaps = false);
		Texture* operator[](const char* name);
	private:
		Device& m_Device;
		Allocator& m_Allocator;

		std::unordered_map<std::string, Texture> storage;
		Texture CreateTextureImage(const std::filesystem::path& path, bool generateMipMaps);
		void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	};
}

