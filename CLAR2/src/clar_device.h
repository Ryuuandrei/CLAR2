#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>

#include "clar_queue_family_indices.h"
#include "clar_swapchain_support_details.h"

#include "clar_debug_messenger.h"
#include "clar_window.h"

namespace CLAR {

	extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	extern PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddressKHR;
	extern PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	extern PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	extern PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	extern PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	extern PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	extern PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	extern PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	extern PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
	extern PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;

	const int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
		VK_NV_RAY_TRACING_EXTENSION_NAME
	};

	class Device {
	public:
		Device(Window& window);
		~Device();

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&&) = delete;
		Device& operator=(Device&&) = delete;

		VkSampleCountFlagBits MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const;
		VkFormatProperties GetPhysicalDeviceFormatProperties(VkFormat format) const;
		VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const;

		QueueFamilyIndices findQueueFamilies() const;
		SwapChainSupportDetails querySwapChainSupport() const;

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		VkCommandBuffer beginSingleTimeCommands() const;
		void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
		void SingleTimeCommand(const std::function<void(VkCommandBuffer&)>& func) const;
		void CreateImage(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
		VkImageView CreateImageView(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

		VkExtent2D GetExtent() const;
		VkSurfaceKHR GetSurface() const;
		VkCommandPool GetCommandPool() const;

		VkQueue GetGraphicsQueue() const;
		VkQueue GetPresentQueue() const;

		VkPhysicalDevice GPU() const { return m_PhysicalDevice; };
		operator VkDevice() const { return m_Device; };
		VkInstance GetInstance() const { return m_Instance; };
		VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer) const;

	private:
		VkInstance m_Instance;
		void CreateInstance();

		VkDebugUtilsMessengerEXT m_DebugMessenger;
		void SetupDebugMessenger();

		VkSurfaceKHR m_Surface;
		void CreateSurface();

		VkPhysicalDevice m_PhysicalDevice;
		void PickPhysicalDevice();

		VkDevice m_Device;
		void CreateLogicalDevice();

		VkCommandPool m_CommandPool;
		void CreateCommandPool();

		Window& m_Window;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		int rateDeviceSuitability(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		VkSampleCountFlagBits getMaxUsableSampleCount() const;

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

		template<typename T>
		auto LoadFunction(VkDevice device, const char* name) {
			T func = (T)vkGetDeviceProcAddr(device, name);
			if (func != nullptr)
				return func;
			else
				throw std::runtime_error("Failed to load " + std::string(name));
		}
	};
}
