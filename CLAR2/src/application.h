#pragma once

#include <iostream>
#include <fstream>

#include <filesystem>

#include "clar_vertex.h"
#include "ClarDescriptors.h"
#include "ClarRenderer.h"
#include "ClarTextureManager.h"

#include "ClarUbo.h"
#include "ClarModel.h"

#include "ClarShaderStorageBuffer.h"
#include "ClarGridSystem.h"
#include "ClarComputeSystem.h"
#include "ClarParticleSystem.h"
#include "ClarRayTracingSystem.h"
#include "PostSystem.h"

#include "ClarCamera.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"
#include "ClarAllocator.h"

#include "ClarMaterial.h"
#include "ClarRTBuilder.h"

#include "imguizmo/ImGuizmo.h"


namespace CLAR {

    struct UniformBufferObject {
        glm::mat4 prev_view;
        glm::mat4 prev_proj;
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct GridUniformBufferObject {
		UniformBufferObject ubo;
		float scroll;
	};


    struct ObjDesc {
        VkDeviceAddress vertexAddress;
        VkDeviceAddress indexAddress;
        glm::vec3 albedo;
        MaterialType material;
        union
        {
            float fuzz;
            float refractionIndex;
        };
    };

    struct LightDesc {
        glm::mat4 model;
        uint32_t index;
	};

    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 720;

    const std::string MODEL_PATH = "../models/viking_room.obj";
    const std::string TEXTURE_PATH = "../textures/texture.jpg";

    constexpr float RAD(float degrees) {
        return degrees * 0.01745329252f;
    }

    class HelloTriangleApplication {
    public:
        void run();

    private:
        Window m_Window{ WIDTH, HEIGHT, "Vulkan!" };

        Device m_Device{ m_Window };

        Renderer m_Renderer{ m_Device , m_Window };

        Allocator m_Allocator{ m_Device };

        RTBuilder m_RtBuilder{ m_Device, m_Allocator };

        std::unordered_map<std::string, Model*> m_Models;

        std::vector<Buffer> m_UniformBuffers;
        std::vector<Buffer> m_ComputeUniformBuffers;

        UniformBufferObject ubo{};

        std::array<glm::mat4, 2> m_ProjMatrices = { glm::mat4(1.0f), glm::mat4(1.0f) };
        std::array<glm::mat4, 2> m_ViewMatrices = { glm::mat4(1.0f), glm::mat4(1.0f) };


        DescriptorSetLayout m_DescriptorSetLayout{ m_Device };
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_DescriptorSets;
        GridSystem gridSystem{ m_Device };

        TextureManager tm{ m_Device, m_Allocator };

        VkSampler m_TextureSampler;

        void initVulkan();
        void initImGui();
        void initRayTracing();

        void mainLoop();
        void cleanup();

        void createTextureSampler();

        void drawFrame();
        void Update(uint32_t currentImage);
        BlasInput ModelToVkgeometry(const Model* model);

        RenderSystem renderSystem{ m_Device };
        PostSystem postSystem{ m_Device };

        DescriptorSetLayout m_PostDescriptorSetLayout{ m_Device };
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_PostDescriptorSets;

        DescriptorSetLayout ImGuiDescriptorSetLayout{ m_Device };
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> ImGuiDescriptorSets;

        ComputeSystem computeSystem{ m_Device };
        DescriptorSetLayout m_ComputeDescriptorSetLayout {m_Device};
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_ComputeDescriptorSets;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
        RayTracingSystem rtSystem{ m_Device };

        /*VkAccelerationStructureKHR m_BottomLevelAS;
        VkAccelerationStructureKHR m_TopLevelAS;

        Buffer m_AccelerationStructureBuffer;
        Buffer m_TLAccelerationStructureBuffer;*/

        AccelerationStructure m_Tlas;
        std::vector<ASBuildInfo> buildAs;

        DescriptorSetLayout m_RtDescriptorSetLayout{ m_Device };
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_RtDescriptorSets;
        void CreateRtDescriptorSets();

        VkRenderPass    m_OffscreenRenderPass{ VK_NULL_HANDLE };
        VkFramebuffer   m_OffscreenFramebuffer{ VK_NULL_HANDLE };
        std::vector<Texture>        m_OffscreenColor;
        Texture        m_OffscreenDepth;
        VkFormat        m_OffscreenColorFormat{ VK_FORMAT_R32G32B32A32_SFLOAT };
        VkFormat        m_OffscreenDepthFormat{ VK_FORMAT_X8_D24_UNORM_PACK32 };
        void CreateOffscreenRender();

        void CreateRtShaderBindingTable();
        Buffer m_rtSBTBuffer;
        VkStridedDeviceAddressRegionKHR m_rgenRegion{};
        VkStridedDeviceAddressRegionKHR m_missRegion{};
        VkStridedDeviceAddressRegionKHR m_hitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};

        void raytrace(const VkCommandBuffer& cmdBuf, const glm::vec4& clearColor);

        PushConstantRay m_pcRay {};
        Buffer m_BobjDesc;
        Buffer m_LightModelsBuffer;


        std::vector<ObjDesc> m_ObjectDescriptions;
        std::vector<std::pair<std::string, Instance>> m_Instances;


        Camera camera;
        float dt = 0.0f;
        float scroll = 0.0f;


        ImGuizmo::OPERATION m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
        int selectedInstanceIndex = -1; // Default to no selection


        bool m_InstanceUpdated = false;
    };
}
