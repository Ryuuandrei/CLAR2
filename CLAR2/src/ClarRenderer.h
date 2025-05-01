#pragma once

#include "clar_device.h"
#include "clar_swap_chain.h"

namespace CLAR {
	class Renderer {
	public:
		Renderer(Device& device, Window& window);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkCommandBuffer Begin();
		void End();

		VkCommandBuffer BeginFrame();
		void EndFrame();

		void BeginRenderPass();
		void BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo);
		void EndRenderPass();

		VkCommandBuffer BeginCompute();
		void EndCompute();

		VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
		VkExtent2D GetSwapChainExtent() const { return m_SwapChain->GetExtent(); }
		VkFormat GetSwapChainImageFormat() const { return m_SwapChain->m_SwapChainImageFormat; }
		VkImageView GetSwapChainColorImageView() const { return m_SwapChain->m_ColorImageView; }
		VkImage GetSwapChainColorImage() const { return m_SwapChain->m_ColorImage; }
		size_t GetSwapChainImageCount() const { return m_SwapChain->m_SwapChainImages.size(); }
		size_t GetCurrentFrame() const { return currentFrame; }
		bool IsComputeEnabled() const { return computeEnabled; }
		/*void EnableCompute();
		void DisableCompute();*/

		void BlitToSwapChain(VkImage image);

	private:
		Device& m_Device;
		Window& m_Window;
		std::unique_ptr<SwapChain> m_SwapChain;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkCommandBuffer> computeCommandBuffers;

		void RecreateSwapChain();
		void CreateCommandBuffers();
		void CreateComputeCommandBuffers();
		void DestroyCommandBuffers();
		uint32_t currentImageIndex;
		size_t currentFrame = 0;
		bool computeEnabled = false;
	};

}
