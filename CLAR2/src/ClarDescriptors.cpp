#include "ClarDescriptors.h"
#include <stdexcept>

namespace CLAR {

    DescriptorSetLayout::DescriptorSetLayout(Device& device) : m_Device(device)
    {
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }

    void DescriptorSetLayout::PushBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, uint32_t count)
    {
        bindings.push_back({
           .binding = binding,
           .descriptorType = descriptorType,
           .descriptorCount = count,
           .stageFlags = shaderStage,
           .pImmutableSamplers = nullptr,
        });
    }

    void DescriptorSetLayout::PushBinding(const VkDescriptorSetLayoutBinding& bindingSpec)
    {
        bindings.push_back(bindingSpec);
    }

    void DescriptorSetLayout::CreateDescriptorPool(VkDescriptorPoolCreateFlags _flags)
    {
        std::vector<VkDescriptorPoolSize> poolSizes{};

        uint32_t maxSets = 0;
        for (const auto& binding : bindings)
        {
            poolSizes.push_back({
                binding.descriptorType, MAX_FRAMES_IN_FLIGHT * binding.descriptorCount
                });

            maxSets += MAX_FRAMES_IN_FLIGHT * binding.descriptorCount;
        }

        if (poolSizes.empty()) {
            throw std::runtime_error("Descriptor pool creation failed: No descriptor bindings found.");
        }

        /*poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);*/

        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = _flags,
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };

        if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> DescriptorSetLayout::AllocateDescriptorSets() const
    {
        assert(m_DescriptorSetLayout != VK_NULL_HANDLE, "Layout is VK_NULL_HANDLE!");
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_DescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
            .pSetLayouts = layouts.data(),
        };

        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        if (vkAllocateDescriptorSets(m_Device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        return descriptorSets;
    }

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> DescriptorSetLayout::CreateSets()
    {
        /*VkDescriptorSetLayoutBinding uboLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        };

        VkDescriptorSetLayoutBinding samplerLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };*/

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        CreateDescriptorPool();
        return AllocateDescriptorSets();
    }

    void DescriptorSetLayout::PushVertexUniformBuffer(uint32_t binding)
    {
        PushBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    }

    void DescriptorSetLayout::PushFragmentUniformBuffer(uint32_t binding)
	{
		PushBinding(binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

    void DescriptorSetLayout::PushComputeStorageBuffer(uint32_t binding)
	{
		PushBinding(binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	}

    // --------------------------------------------------------------------------------------

    DescritorWriter& DescritorWriter::WriteUniformBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo)
    {

        m_DescriptorWrites[binding] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = nullptr,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = bufferInfo,
            };

        return *this;
    }

    DescritorWriter& DescritorWriter::WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo)
    {

        m_DescriptorWrites[binding] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = nullptr,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = imageInfo
            };

        return *this;
    }

    DescritorWriter& DescritorWriter::WriteStorageBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo)
    {
        m_DescriptorWrites[binding] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = nullptr,
				.dstBinding = binding,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = bufferInfo
			};

        return *this;
    }

    DescritorWriter& DescritorWriter::WriteAccelerationStructure(uint32_t binding, const VkWriteDescriptorSetAccelerationStructureKHR* accelerationStructureInfo)
    {
        m_DescriptorWrites[binding] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = accelerationStructureInfo,
            .dstSet = nullptr,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            };

        return *this;
    }

    DescritorWriter& DescritorWriter::WriteStorageImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo)
    {
        m_DescriptorWrites[binding] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = nullptr,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = imageInfo
        };

        return *this;
    }

    void DescritorWriter::Update(VkDescriptorSet dstSet, Device& device)
    {
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(m_DescriptorWrites.size());

        for (auto& [_, write] : m_DescriptorWrites)
        {
            write.dstSet = dstSet;
            writes.push_back(write);
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}