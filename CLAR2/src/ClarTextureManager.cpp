#include "ClarTextureManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace CLAR {

	TextureManager::TextureManager(Device& device, Allocator& allcator)
		: m_Device(device), m_Allocator(allcator)
	{
	}

	TextureManager::~TextureManager()
	{
		for (auto& [_, tex] : storage)
		{
            m_Allocator.DestroyTexture(tex);
		}
	}

    void TextureManager::LoadFromFile(const std::filesystem::path& path, const char* name, bool generateMipMaps)
    {
        if (auto got = storage.find(name); got != storage.end())
            m_Allocator.DestroyTexture(got->second);
        storage[name] = CreateTextureImage(path, generateMipMaps);
    }

    Texture* TextureManager::operator[](const char* name)
    {
        if (auto got = storage.find(name); got != storage.end())
            return &storage[name];
        return nullptr;
    }

	Texture TextureManager::CreateTextureImage(const std::filesystem::path& path, bool generateMipMaps)
	{
        int texWidth, texHeight, texChannels;
        uint32_t mipLevels = 1;
        stbi_uc* pixels = stbi_load(path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        if (generateMipMaps)
            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        Buffer stagingBuffer = m_Allocator.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        stagingBuffer.Write(pixels, static_cast<size_t>(imageSize));

        stbi_image_free(pixels);

        Image image = m_Allocator.CreateImage({ (uint32_t)texWidth, (uint32_t)texHeight }, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, VK_SAMPLE_COUNT_1_BIT, mipLevels);

        m_Device.transitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        m_Device.copyBufferToImage(stagingBuffer.buffer, image.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        Texture texture = m_Allocator.CreateTexture(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_Allocator.DestroyBuffer(stagingBuffer);

        if (generateMipMaps)
            GenerateMipmaps(texture.image.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, texture.mipLevels);

        texture.height = texHeight;
        texture.width = texWidth;

        return texture;
	}

    void TextureManager::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

        // Check if image format supports linear blitting
        VkFormatProperties formatProperties = m_Device.GetPhysicalDeviceFormatProperties(imageFormat);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        m_Device.SingleTimeCommand([&](auto& commandBuffer)
            {
                VkImageMemoryBarrier barrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = image,
                    .subresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                int32_t mipWidth = texWidth;
                int32_t mipHeight = texHeight;

                for (uint32_t i = 1; i < mipLevels; i++) {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    vkCmdBlitImage(commandBuffer,
                        image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit,
                        VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                barrier.subresourceRange.baseMipLevel = mipLevels - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            });

    }
}