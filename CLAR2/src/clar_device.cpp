#include "clar_device.h"

#include <stdexcept>
#include <vector>
#include <set>
#include <map>
#include <format>

#include "clar_validation_layers.h"

#define RED(x) "\x1B[31m" x "\033[0m"
#define GREEN(x) "\x1B[32m" x "\033[0m"

namespace CLAR {

    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
    PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;
    PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV;

	Device::Device(Window& window) : m_Window{window}
	{
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();

        CLAR::vkCmdBuildAccelerationStructuresKHR = LoadFunction<PFN_vkCmdBuildAccelerationStructuresKHR>(m_Device, "vkCmdBuildAccelerationStructuresKHR");
        CLAR::vkCreateAccelerationStructureKHR = LoadFunction<PFN_vkCreateAccelerationStructureKHR>(m_Device, "vkCreateAccelerationStructureKHR");
        CLAR::vkCreateRayTracingPipelinesKHR = LoadFunction<PFN_vkCreateRayTracingPipelinesKHR>(m_Device, "vkCreateRayTracingPipelinesKHR");
        CLAR::vkGetAccelerationStructureBuildSizesKHR = LoadFunction<PFN_vkGetAccelerationStructureBuildSizesKHR>(m_Device, "vkGetAccelerationStructureBuildSizesKHR");
        CLAR::vkGetBufferDeviceAddressKHR = LoadFunction<PFN_vkGetBufferDeviceAddressKHR>(m_Device, "vkGetBufferDeviceAddressKHR");
        CLAR::vkGetAccelerationStructureDeviceAddressKHR = LoadFunction<PFN_vkGetAccelerationStructureDeviceAddressKHR>(m_Device, "vkGetAccelerationStructureDeviceAddressKHR");
        CLAR::vkDestroyAccelerationStructureKHR = LoadFunction<PFN_vkDestroyAccelerationStructureKHR>(m_Device, "vkDestroyAccelerationStructureKHR");
        CLAR::vkGetRayTracingShaderGroupHandlesKHR = LoadFunction<PFN_vkGetRayTracingShaderGroupHandlesKHR>(m_Device, "vkGetRayTracingShaderGroupHandlesKHR");
        CLAR::vkCmdTraceRaysKHR = LoadFunction<PFN_vkCmdTraceRaysKHR>(m_Device, "vkCmdTraceRaysKHR");
        CLAR::vkCmdWriteAccelerationStructuresPropertiesKHR = LoadFunction<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(m_Device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
        CLAR::vkCmdCopyAccelerationStructureKHR = LoadFunction<PFN_vkCmdCopyAccelerationStructureKHR>(m_Device, "vkCmdCopyAccelerationStructureKHR");
	    CLAR::vkCmdTraceRaysNV = LoadFunction<PFN_vkCmdTraceRaysNV>(m_Device, "vkCmdTraceRaysNV");
    }

    QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (size_t i = 0; i < queueFamilyCount; ++i)
        {
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices) break;
        }

        return indices;
    }

    VkQueue Device::GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

    VkQueue Device::GetPresentQueue() const
    {
        return m_PresentQueue;
    }

    VkDeviceAddress Device::GetBufferDeviceAddress(VkBuffer buffer) const
    {
        VkBufferDeviceAddressInfoKHR bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = buffer
		};
		return vkGetBufferDeviceAddressKHR(m_Device, &bufferInfo);
    }

    void Device::CreateInstance()
    {
        if (enableValidationLayer && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo
        };

        auto extensions = getRequiredExtensions();

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        createInfo.enabledLayerCount = 0;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayer)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            DebugMessenger::populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void Device::SetupDebugMessenger()
    {
        if (!enableValidationLayer) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        DebugMessenger::populateDebugMessengerCreateInfo(createInfo);

        if (DebugMessenger::CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void Device::PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with vulkan support!");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;

        for (const auto& device : devices) {
            /*VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            std::cout << RED("[SwapChain]: Device name : ") << deviceProperties.deviceName << '\n';*/

            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0) {
            m_PhysicalDevice = candidates.rbegin()->second;
            MsaaSamples = getMaxUsableSampleCount();
        }
        else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        std::cout << GREEN("[SwapChain]: Device name : ") << GetPhysicalDeviceProperties().deviceName << '\n';
        std::cout << std::format("Push constants max size: {}\n", GetPhysicalDeviceProperties().limits.maxPushConstantsSize);
    }

    void Device::CreateLogicalDevice()
    {
        /*uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionCount, NULL);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, NULL, &extensionCount, availableExtensions.data());

        for (uint32_t i = 0; i < extensionCount; ++i) {
            printf("Extension %d: %s\n", i, availableExtensions[i].extensionName);
        }*/
        QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {

            VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{
            .sampleRateShading = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
            .shaderFloat64 = VK_TRUE,
            .shaderInt64 = VK_TRUE,
        };

        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .pEnabledFeatures = &deviceFeatures
        };

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayer) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        // Enable buffer device address
        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
        deviceFeatures2.pNext = &bufferDeviceAddressFeatures;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);

        if (!bufferDeviceAddressFeatures.bufferDeviceAddress) {
            throw std::runtime_error("Buffer device address not supported");
        }

        // Enable acceleration structure
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        deviceFeatures2.pNext = &accelerationStructureFeatures;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);

        if (!accelerationStructureFeatures.accelerationStructure) {
			throw std::runtime_error("Acceleration structure not supported");
		}

        // Enable ray tracing
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
        deviceFeatures2.pNext = &rayTracingPipelineFeatures;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);

        if (!rayTracingPipelineFeatures.rayTracingPipeline) {
			throw std::runtime_error("Ray tracing not supported");
		}

        // enable shader storage image multisample
        /* shaderImageMultisampleFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_MULTISAMPLE_FEATURES_EXT };
        deviceFeatures2.pNext = &shaderImageMultisampleFeatures;
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);

        if (!shaderImageMultisampleFeatures.shaderImageGatherExtended) {
			throw std::runtime_error("shader image gather extended not supported");
		}*/


        createInfo.pNext = &bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.pNext = &accelerationStructureFeatures;
        accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;

        // Create the logical device
        if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
    }

    void Device::CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies();

        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
        };

        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void Device::CreateSurface()
    {
        m_Window.createSurfaceWindow(m_Instance, &m_Surface);
    }

    int Device::rateDeviceSuitability(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        if (indices && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy) {
            int score = 0;

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;
            return score;
        }
        else
        {
            return 0;
        }
    }

    bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t count = 0;

        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.data());

        std::set<std::string> requiredExtension(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtension.erase(extension.extensionName);
        }

        return requiredExtension.empty();
    }

    SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device) const {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
        }


        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSampleCountFlagBits Device::getMaxUsableSampleCount() const{
        VkPhysicalDeviceProperties physicalDeviceProperties = GetPhysicalDeviceProperties();

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkPhysicalDeviceProperties Device::GetPhysicalDeviceProperties() const
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
        return deviceProperties;
    }

    VkFormatProperties Device::GetPhysicalDeviceFormatProperties(VkFormat format) const
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
        return props;
    }

    VkPhysicalDeviceMemoryProperties Device::GetPhysicalDeviceMemoryProperties() const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
        return memProperties;
    }

    QueueFamilyIndices Device::findQueueFamilies() const
    {
        return findQueueFamilies(m_PhysicalDevice);
    }

    SwapChainSupportDetails Device::querySwapChainSupport() const
    {
        return querySwapChainSupport(m_PhysicalDevice);
    }

    uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties = GetPhysicalDeviceMemoryProperties();

        for (size_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (VkFormat format : candidates) {
            VkFormatProperties props = GetPhysicalDeviceFormatProperties(format);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    void Device::CreateImage(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const
    {
        
        if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties),
        };

        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(m_Device, image, imageMemory, 0);
    }

    VkImageView Device::CreateImageView(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
    {
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
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

        VkImageView imageView;

        if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void Device::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const
    {
        SingleTimeCommand([&](auto& commandBuffer)
            {
                VkImageMemoryBarrier barrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0, // TODO
                    .dstAccessMask = 0, // TODO
                    .oldLayout = oldLayout,
                    .newLayout = newLayout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = image,
                    .subresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = mipLevels,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                };

                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;

                if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_FORMAT_X8_D24_UNORM_PACK32) {
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                    // if the format has a stencil component
                    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                    }
                }
                else {
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                }
                    
                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL){
                    barrier.srcAccessMask = 0;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    destinationStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for the transfer operation to finish writing
                    barrier.dstAccessMask = 0; // No read or write operations will be performed after the transition

                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // Source is transfer stage
                    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // No further operations on the image
                }
                else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Wait for color attachment writes to finish
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT; // We are reading from the image in the transfer stage

                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                }
                else {
                    throw std::invalid_argument("unsupported layout transition!");
                }

                vkCmdPipelineBarrier(
                    commandBuffer,
                    sourceStage, destinationStage,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            });

    }

    void Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
    {
        VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failedto create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)
        };

        /*VkMemoryAllocateFlagsInfoKHR flags_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR };
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;*/

        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
    }

    void Device::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
    {
        SingleTimeCommand([&](auto& commandBuffer)
            {
                VkBufferCopy copyRegion{
                    .srcOffset = 0,
                    .dstOffset = 0,
                    .size = size
                };

                vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            });
    }

    void Device::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
    {
        SingleTimeCommand([&](auto& commandBuffer)
            {
                VkBufferImageCopy region{
                    .bufferOffset = 0,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,

                    .imageSubresource{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },

                    .imageOffset = { 0, 0, 0 },
                    .imageExtent = {
                        width,
                        height,
                        1
                    }
                };

                vkCmdCopyBufferToImage(
                    commandBuffer,
                    buffer,
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &region
                );
            });
    }

    VkCommandBuffer Device::beginSingleTimeCommands() const
    {
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer) const
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer
        };

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
    }

    void Device::SingleTimeCommand(const std::function<void(VkCommandBuffer&)>& func) const
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        func(commandBuffer);
        endSingleTimeCommands(commandBuffer);
    }

    VkExtent2D Device::GetExtent() const
    {
        int width, height;
        m_Window.getFramebufferSize(&width, &height);

        return {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    }

    VkSurfaceKHR Device::GetSurface() const
    {
        return m_Surface;
    }

    VkCommandPool Device::GetCommandPool() const
    {
        return m_CommandPool;
    }

    Device::~Device()
    {
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

        vkDestroyDevice(m_Device, nullptr);

        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

        if (enableValidationLayer)
        {
            DebugMessenger::DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        }

        vkDestroyInstance(m_Instance, nullptr);
    }

}
