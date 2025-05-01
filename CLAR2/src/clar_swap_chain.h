#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

#include "clar_device.h"

namespace CLAR {
	class SwapChain {
	public:
		SwapChain(Device& device);
		SwapChain(Device& device, std::shared_ptr<SwapChain> oldSwapChain);
		~SwapChain();

		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;

		VkResult AcquireNextImage(uint32_t* imageIndex) const;
		VkResult SubmitCommandBuffer(const VkCommandBuffer* commandBuffer, uint32_t* imageIndex);
		void SubmitComputeCommandBuffer(const VkCommandBuffer* commandBuffer);
		void SyncCompute();
		void NextFrame();

		VkFormat GetFormat() const { return m_SwapChainImageFormat; }
		VkExtent2D GetExtent() const { return m_SwapChainExtent; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer& GetFrameBuffer(int index) { return m_SwapChainFrameBuffers[index]; }

		void EnableMultiSampling();

	private:
		void Init();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateColorResources();
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();

		VkFormat findDepthFormat() const;

		VkSwapchainKHR m_SwapChain;
		std::shared_ptr<SwapChain> m_OldSwapChain;

		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;

		VkImage m_ColorImage;
		VkDeviceMemory m_ColorImageMemory;
		VkImageView m_ColorImageView;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;

		std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

		VkRenderPass m_RenderPass;
		Device& m_Device;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		std::vector<VkFence> computeInFlightFences;
		std::vector<VkSemaphore> computeFinishedSemaphores;

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
		size_t currentFrame = 0;
		bool multiSamplingEnabled = false;

		friend class Renderer;
	};
}
