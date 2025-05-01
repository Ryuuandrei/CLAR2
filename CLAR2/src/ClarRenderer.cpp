#include "ClarRenderer.h"
#include <array>

namespace CLAR {

	Renderer::Renderer(Device& device, Window& window)
		: m_Device{device}, m_Window{window}
	{
		RecreateSwapChain();
		CreateCommandBuffers();
		CreateComputeCommandBuffers();
	}

	Renderer::~Renderer()
	{
		DestroyCommandBuffers();
	}

	VkCommandBuffer Renderer::Begin()
	{
		auto cmdBuf = BeginFrame();
		cmdBuf ? BeginRenderPass() : void();

		return cmdBuf;
	}

	void Renderer::End()
	{
		EndRenderPass();
		EndFrame();
	}

	VkCommandBuffer Renderer::BeginFrame()
	{
		auto result = m_SwapChain->AcquireNextImage(&currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return VK_NULL_HANDLE;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetCommandBuffer(m_CommandBuffers[currentFrame], 0);

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		
		if (vkBeginCommandBuffer(m_CommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		return m_CommandBuffers[currentFrame];

	}

	void Renderer::BeginRenderPass()
	{
		// start render pass
		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_SwapChain->m_RenderPass,
			.framebuffer = m_SwapChain->GetFrameBuffer(currentImageIndex),
			.renderArea = {
				.offset = { 0, 0 },
				.extent = m_SwapChain->GetExtent()
			}
		};

		std::array<VkClearValue, 2> clearValues;
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.f,
			.y = 0.f,
			.width = (float)m_SwapChain->GetExtent().width,
			.height = (float)m_SwapChain->GetExtent().height,
			.minDepth = 0.f,
			.maxDepth = 1.f
		};
		vkCmdSetViewport(m_CommandBuffers[currentFrame], 0, 1, &viewport);

		VkRect2D scissor{
			.offset = {0, 0},
			.extent = m_SwapChain->GetExtent()
		};
		vkCmdSetScissor(m_CommandBuffers[currentFrame], 0, 1, &scissor);
	}

	void Renderer::BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo)
	{
		vkCmdBeginRenderPass(m_CommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{
			.x = 0.f,
			.y = 0.f,
			.width = (float)m_SwapChain->GetExtent().width,
			.height = (float)m_SwapChain->GetExtent().height,
			.minDepth = 0.f,
			.maxDepth = 1.f
		};
		vkCmdSetViewport(m_CommandBuffers[currentFrame], 0, 1, &viewport);

		VkRect2D scissor{
			.offset = {0, 0},
			.extent = m_SwapChain->GetExtent()
		};
		vkCmdSetScissor(m_CommandBuffers[currentFrame], 0, 1, &scissor);
	}

	void Renderer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffers[currentFrame]);
	}

	void Renderer::EndFrame()
	{
		if (vkEndCommandBuffer(m_CommandBuffers[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		auto result = m_SwapChain->SubmitCommandBuffer(&m_CommandBuffers[currentFrame], &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window.WasWindowResized()) {
			m_Window.ResetWindowResizedFlag();
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain images!");
		}

		m_SwapChain->NextFrame();
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	VkCommandBuffer Renderer::BeginCompute()
	{
		m_SwapChain->SyncCompute();

		vkResetCommandBuffer(computeCommandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(computeCommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		return computeCommandBuffers[currentFrame];
	}

	void Renderer::EndCompute()
	{
		if (vkEndCommandBuffer(computeCommandBuffers[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}

		m_SwapChain->SubmitComputeCommandBuffer(&computeCommandBuffers[currentFrame]);
	}

	void Renderer::BlitToSwapChain(VkImage image)
	{
		VkImageBlit blitRegion{};
		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.mipLevel = 0;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = {
			static_cast<int32_t>(m_SwapChain->m_SwapChainExtent.width),
			static_cast<int32_t>(m_SwapChain->m_SwapChainExtent.height),
			1
		};

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.mipLevel = 0;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = {
			static_cast<int32_t>(m_SwapChain->m_SwapChainExtent.width),
			static_cast<int32_t>(m_SwapChain->m_SwapChainExtent.height),
			1
		};

		// Transition the source (resolved color image) to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		//m_Device.transitionImageLayout(image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

		// Transition the destination (swapchain image) to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		m_Device.transitionImageLayout(m_SwapChain->m_SwapChainImages[currentImageIndex], m_SwapChain->m_SwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

		// Blit the resolved color image to the swapchain image
		m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_GENERAL,
				m_SwapChain->m_SwapChainImages[currentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blitRegion,
				VK_FILTER_LINEAR // or VK_FILTER_NEAREST depending on your preference
			);
		});

		// Transition the swapchain image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		m_Device.transitionImageLayout(m_SwapChain->m_SwapChainImages[currentImageIndex], m_SwapChain->m_SwapChainImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

	}

	void Renderer::RecreateSwapChain()
	{
		auto extent = m_Device.GetExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = m_Device.GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device);
		if (m_SwapChain == nullptr) {
			m_SwapChain = std::make_unique<SwapChain>(m_Device);
		}
		else
		{
			std::shared_ptr<SwapChain> oldSwapChain = std::move(m_SwapChain);
			m_SwapChain = std::make_unique<SwapChain>(m_Device, oldSwapChain);
		}
	}

	void Renderer::CreateCommandBuffers()
	{
		m_CommandBuffers.resize(CLAR::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_Device.GetCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size())
		};

		if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void Renderer::CreateComputeCommandBuffers()
	{
		computeCommandBuffers.resize(CLAR::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_Device.GetCommandPool(),
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size())
		};

		if (vkAllocateCommandBuffers(m_Device, &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}

	void Renderer::DestroyCommandBuffers()
	{
		vkFreeCommandBuffers(
			m_Device,
			m_Device.GetCommandPool(),
			static_cast<uint32_t>(m_CommandBuffers.size()),
			m_CommandBuffers.data());
		m_CommandBuffers.clear();
	}

}