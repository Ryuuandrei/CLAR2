#include "ClarRTBuilder.h"

namespace CLAR {
	RTBuilder::RTBuilder(Device& device, Allocator& allocator)
		: m_Device(device), m_Allocator(allocator)
	{
	}

	RTBuilder::~RTBuilder()
	{
	}

    std::vector<ASBuildInfo> RTBuilder::BuildBlas(const std::vector<BlasInput>& allBlas)
    {
        uint32_t     nbBlas = static_cast<uint32_t>(allBlas.size()); // Number of BLASes
        VkDeviceSize asTotalSize{ 0 };     // Memory size of all allocated BLAS
        uint32_t     nbCompactions{ 0 };   // Nb of BLAS requesting compaction
        VkDeviceSize maxScratchSize{ 0 };  // Largest scratch size

        std::vector<ASBuildInfo> buildAs(nbBlas);

        for (size_t i = 0; i < nbBlas; ++i)
        {
            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                .srcAccelerationStructure = VK_NULL_HANDLE,
                .dstAccelerationStructure = VK_NULL_HANDLE,
                .geometryCount = 1, // there is one geometry on the blas 
                .pGeometries = &allBlas[i].asGeometry,
                .scratchData = 0
            };

            buildAs[i].buildInfo = buildInfo;
            buildAs[i].rangeInfo = allBlas[i].asBuildOffset;
            buildAs[i].blasId = allBlas[i].modelId;

            // rangeInfo = blasInput.asBuildOffset

            VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            vkGetAccelerationStructureBuildSizesKHR(m_Device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildAs[i].buildInfo,
                &buildAs[i].rangeInfo.primitiveCount,
                &buildSizeInfo);

            buildAs[i].sizeInfo = buildSizeInfo;

            asTotalSize += buildSizeInfo.accelerationStructureSize;
            maxScratchSize = std::max(maxScratchSize, buildSizeInfo.buildScratchSize);
            nbCompactions += (buildAs[i].buildInfo.flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR) != 0;
        }
        Buffer scratchBuffer = m_Allocator.CreateBuffer(maxScratchSize + 128, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        VkDeviceAddress scratchAddress = m_Device.GetBufferDeviceAddress(scratchBuffer.buffer);

        // Allocate a query pool for storing the needed size for every BLAS compaction.
        VkQueryPool queryPool{ VK_NULL_HANDLE };
        if (nbCompactions > 0)  // Is compaction requested?
        {
            assert(nbCompactions == nbBlas);  // Don't allow mix of on/off compaction
            VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
            qpci.queryCount = nbBlas;
            qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
            vkCreateQueryPool(m_Device, &qpci, nullptr, &queryPool);
        }

        // Batching creation/compaction of BLAS to allow staying in restricted amount of memory
        std::vector<uint32_t> indices;  // Indices of the BLAS to create
        VkDeviceSize          batchSize{ 0 };
        VkDeviceSize          batchLimit{ 256'000'000 };  // 256 MB

        for (uint32_t idx = 0; idx < nbBlas; idx++)
        {
            indices.push_back(idx);
            batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
            // Over the limit or last BLAS element
            if (batchSize >= batchLimit || idx == nbBlas - 1)
            {
                if (queryPool)  // For querying the compaction size
                    vkResetQueryPool(m_Device, queryPool, 0, static_cast<uint32_t>(indices.size()));
                uint32_t queryCnt{ 0 };
                for (const auto& idx : indices)
                {
                    m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer) {
                        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
                        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                        createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;  // Will be used to allocate memory.

                        buildAs[idx].as = m_Allocator.CreateAccelerationStructure(createInfo);

                        buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.handle;
                        buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress + (128 - scratchAddress % 128);

                        VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildAs[idx].rangeInfo;

                        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildAs[idx].buildInfo, &pBuildOffsetInfo);

                        VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
                        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
                        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

                        if (queryPool)
                        {
                            // Add a query to find the 'real' amount of memory needed, use for compaction
                            vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, 1, &buildAs[idx].buildInfo.dstAccelerationStructure,
                                VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
                        }
                        });
                }
                if (queryPool)
                {
                    m_Device.SingleTimeCommand([&](VkCommandBuffer& commandBuffer) {
                        uint32_t                    queryCtn{ 0 };
                        std::vector<AccelerationStructure> cleanupAS;  // previous AS to destroy

                        // Get the compacted size result back
                        std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
                        vkGetQueryPoolResults(m_Device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
                            compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

                        for (auto idx : indices)
                        {
                            cleanupAS.emplace_back(buildAs[idx].as);
                            buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++];  // new reduced size

                            // Creating a compact version of the AS
                            VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
                            asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
                            asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                            buildAs[idx].as = m_Allocator.CreateAccelerationStructure(asCreateInfo);

                            // Copy the original BLAS to a compact version
                            VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
                            copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
                            copyInfo.dst = buildAs[idx].as.handle;
                            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                            vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyInfo);
                        }

                        for (const auto& as : cleanupAS)
                            m_Allocator.DestroyAccelerationStructure(as);
                        });

                    //// Destroy the non-compacted version
                    //destroyNonCompacted(indices, buildAs);
                }
                // Reset

                batchSize = 0;
                indices.clear();
            }
        }

        m_Allocator.DestroyBuffer(scratchBuffer);

        return buildAs;
    }

    AccelerationStructure RTBuilder::BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances) const
    {
        uint32_t countInstance = static_cast<uint32_t>(instances.size());
        Buffer instanceBuffer = m_Allocator.CreateBuffer(instances, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
        Buffer scratchBuffer;

        AccelerationStructure tlas;

        m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer)
            {
                VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    0, 1, &barrier, 0, nullptr, 0, nullptr);

                // Wraps a device pointer to the above uploaded instances.
                VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
                instancesVk.data.deviceAddress = m_Device.GetBufferDeviceAddress(instanceBuffer.buffer);

                // Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
                VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
                topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
                topASGeometry.geometry.instances = instancesVk;

                // Find sizes
                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
                buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                    VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
                buildInfo.geometryCount = 1;
                buildInfo.pGeometries = &topASGeometry;
                buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // can also be update
                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
                buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

                VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
                vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
                    &countInstance, &sizeInfo);

                VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
                createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
                createInfo.size = sizeInfo.accelerationStructureSize;

                tlas = m_Allocator.CreateAccelerationStructure(createInfo);

                // Build the TLAS
                scratchBuffer = m_Allocator.CreateBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

                // Update build information
                buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
                buildInfo.dstAccelerationStructure = tlas.handle;
                buildInfo.scratchData.deviceAddress = m_Device.GetBufferDeviceAddress(scratchBuffer.buffer);

                // Build Offsets info: n instances
                VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{ countInstance, 0, 0, 0 };
                const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

                // Build the TLAS
                vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &pBuildOffsetInfo);

            });

        m_Allocator.DestroyBuffer(scratchBuffer);
        m_Allocator.DestroyBuffer(instanceBuffer);

        return tlas;
    }

    AccelerationStructure RTBuilder::UpdateTlas(AccelerationStructure& tlas, const std::vector<VkAccelerationStructureInstanceKHR>& instances) const
    {
        uint32_t countInstance = static_cast<uint32_t>(instances.size());
        Buffer instanceBuffer = m_Allocator.CreateBuffer(instances, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
        Buffer scratchBuffer;

        m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer)
            {
                VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    0, 1, &barrier, 0, nullptr, 0, nullptr);

                // Wraps a device pointer to the updated instances.
                VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
                instancesVk.data.deviceAddress = m_Device.GetBufferDeviceAddress(instanceBuffer.buffer);

                // Put the above into a VkAccelerationStructureGeometryKHR.
                VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
                topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
                topASGeometry.geometry.instances = instancesVk;

                // Update build info
                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
                buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                    VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
                buildInfo.geometryCount = 1;
                buildInfo.pGeometries = &topASGeometry;
                buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
                buildInfo.srcAccelerationStructure = tlas.handle;
                buildInfo.dstAccelerationStructure = tlas.handle;

                VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
                vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
                    &countInstance, &sizeInfo);

                // Allocate scratch buffer for update
                scratchBuffer = m_Allocator.CreateBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
                buildInfo.scratchData.deviceAddress = m_Device.GetBufferDeviceAddress(scratchBuffer.buffer);

                // Build Offsets info: n instances
                VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{ countInstance, 0, 0, 0 };
                const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

                // Perform the TLAS update
                vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &pBuildOffsetInfo);
            });

        m_Allocator.DestroyBuffer(scratchBuffer);
        m_Allocator.DestroyBuffer(instanceBuffer);

        return tlas;
    }

	BlasInput RTBuilder::ModelToVkgeometry(const Model* model)
    {
        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = m_Device.GetBufferDeviceAddress(model->m_VertexBuffer.buffer);
        triangles.vertexStride = sizeof(Vertex);
        triangles.maxVertex = static_cast<uint32_t>(model->mesh.size()) - 1;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = m_Device.GetBufferDeviceAddress(model->m_IndexBuffer.buffer);
        triangles.transformData = {};

        VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        asGeom.geometry.triangles = triangles;

        VkAccelerationStructureBuildRangeInfoKHR offset;
        offset.firstVertex = 0;
        offset.primitiveCount = static_cast<uint32_t>(model->indices.size()) / 3;
        offset.primitiveOffset = 0;
        offset.transformOffset = 0;

        return { model->id, asGeom, offset };
    }


}