#include "clar_swap_chain.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <ranges>

namespace CLAR {

	SwapChain::SwapChain(CLAR::Device& device)
        : m_Device(device)
	{
        Init();
	}

    SwapChain::SwapChain(Device& device, std::shared_ptr<SwapChain> oldSwapChain)
        : m_Device(device), m_OldSwapChain(oldSwapChain)
    {
        Init();
        m_OldSwapChain = nullptr;
    }

    void SwapChain::EnableMultiSampling()
    {
        multiSamplingEnabled = true;
        Init();
    }

    void SwapChain::Init()
    {
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateColorResources();
        CreateDepthResources();
        CreateFramebuffers();
        CreateSyncObjects();
    }

    SwapChain::~SwapChain()
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);

            vkDestroySemaphore(m_Device, computeFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_Device, computeInFlightFences[i], nullptr);
        }

        vkDestroyImageView(m_Device, m_ColorImageView, nullptr);
        vkDestroyImage(m_Device, m_ColorImage, nullptr);
        vkFreeMemory(m_Device, m_ColorImageMemory, nullptr);

        vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
        vkDestroyImage(m_Device, m_DepthImage, nullptr);
        vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

        for (auto frameBuffer : m_SwapChainFrameBuffers) {
            vkDestroyFramebuffer(m_Device, frameBuffer, nullptr);
        }

        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

        for (auto imageView : m_SwapChainImageViews) {
            vkDestroyImageView(m_Device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
    {
        /*for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }*/
        if (auto it = std::ranges::find_if(availableFormats, [](const VkSurfaceFormatKHR& surfaceFormat) { return surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; }); it != availableFormats.end())
            return *it;

        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
    {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        VkExtent2D actualExtent = m_Device.GetExtent();
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void SwapChain::CreateSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = m_Device.querySwapChainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        m_SwapChainImageFormat = surfaceFormat.format;
        m_SwapChainExtent = extent;

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_Device.GetSurface(),
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        };

        QueueFamilyIndices indices = m_Device.findQueueFamilies();
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_OldSwapChain ? m_OldSwapChain->m_SwapChain : VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
        m_SwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());
    }

    void SwapChain::CreateImageViews()
    {
        m_SwapChainImageViews.resize(m_SwapChainImages.size());
        for (size_t i = 0; i < m_SwapChainImages.size(); ++i) {
            m_SwapChainImageViews[i] = m_Device.CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void SwapChain::CreateColorResources()
    {
        VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .extent{
                .width = m_SwapChainExtent.width,
                .height = m_SwapChainExtent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = multiSamplingEnabled ? m_Device.MsaaSamples : VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = /*VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        m_Device.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ColorImage, m_ColorImageMemory);

        m_ColorImageView = m_Device.CreateImageView(m_ColorImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        m_Device.transitionImageLayout(m_ColorImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
    }

    void SwapChain::CreateDepthResources()
    {
        VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = findDepthFormat(),
            .extent{
                .width = m_SwapChainExtent.width,
                .height = m_SwapChainExtent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = multiSamplingEnabled ? m_Device.MsaaSamples : VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        m_Device.CreateImage(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

        m_DepthImageView = m_Device.CreateImageView(m_DepthImage, findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        m_Device.transitionImageLayout(m_DepthImage, findDepthFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    }

    void SwapChain::CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{
            .format = m_SwapChainImageFormat,
            .samples = multiSamplingEnabled ? m_Device.MsaaSamples : VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = multiSamplingEnabled ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 
        };

        VkAttachmentReference colorAttachmentRef{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        VkAttachmentDescription depthAttachment{
            .format = findDepthFormat(),
            .samples = multiSamplingEnabled ? m_Device.MsaaSamples : VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentReference depthAttachmentRef{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentDescription colorAttachmentResolve{
            .format = m_SwapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference colorAttachmentResolveRef{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpass{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pResolveAttachments = multiSamplingEnabled ? &colorAttachmentResolveRef : nullptr,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };

        VkSubpassDependency dependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };

        std::vector<VkAttachmentDescription> attachments;
        if (multiSamplingEnabled) {
			attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
		}
		else {
			attachments = { colorAttachment, depthAttachment };
		}

        VkRenderPassCreateInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency
        };

        if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void SwapChain::CreateFramebuffers()
    {
        m_SwapChainFrameBuffers.resize(m_SwapChainImageViews.size());

        for (size_t i = 0; i < m_SwapChainImageViews.size(); ++i) {
            std::vector<VkImageView> attachments;
            multiSamplingEnabled ? attachments = {
                m_ColorImageView,
                m_DepthImageView,
                m_SwapChainImageViews[i]
            } : attachments = { 
                m_SwapChainImageViews[i],
                m_DepthImageView
            };

            VkFramebufferCreateInfo frameBufferInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = m_RenderPass,
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = m_SwapChainExtent.width,
                .height = m_SwapChainExtent.height,
                .layers = 1
            };

            if (vkCreateFramebuffer(m_Device, &frameBufferInfo, nullptr, &m_SwapChainFrameBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to crate framebuffer!");
            }
        }
    }

    void SwapChain::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create semaphore!");
            }

            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute synchronization objects for a frame!");
            }
        }
    }

    VkResult SwapChain::AcquireNextImage(uint32_t* imageIndex) const
    {
        vkWaitForFences(m_Device, 1, &m_InFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        return vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
    }

    VkResult SwapChain::SubmitCommandBuffer(const VkCommandBuffer* commandBuffer, uint32_t* imageIndex)
    {
        vkResetFences(m_Device, 1, &m_InFlightFences[currentFrame]);

        VkSemaphore waitSemaphore[] = { computeFinishedSemaphores[currentFrame], m_ImageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSemaphore signalSemaphore[] = { m_RenderFinishedSemaphores[currentFrame] };

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 2,
            .pWaitSemaphores = waitSemaphore,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphore
        };

        if (vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw buffer!");
        }

        VkSwapchainKHR swapChains[] = { m_SwapChain };

        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphore,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = imageIndex,
            .pResults = nullptr
        };

        VkResult result = vkQueuePresentKHR(m_Device.GetPresentQueue(), &presentInfo);
        
        return result;
    }

    void SwapChain::SubmitComputeCommandBuffer(const VkCommandBuffer* commandBuffer)
    {
		vkResetFences(m_Device, 1, &computeInFlightFences[currentFrame]);

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &computeFinishedSemaphores[currentFrame]
		};

		if (vkQueueSubmit(m_Device.GetGraphicsQueue(), 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute buffer!");
		}

		//currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void SwapChain::SyncCompute()
    {
        vkWaitForFences(m_Device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    }

    void SwapChain::NextFrame()
    {
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkFormat SwapChain::findDepthFormat() const
    {
        return m_Device.FindSupportedFormat(
            { VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

}