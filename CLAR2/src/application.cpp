#include <stdexcept>
#include <cstdlib>
#include <set>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>

//#define NDEBUG //UNCOMMENT THIS TO DISABLE VALIDATION LAYERS
//#define FRAMECOUNTER
#include "application.h"
#include "Particle.h"
#include <random>
#include "utils/Timer.h"
#include "imgui/imgui.h"

#define PARTICLE_COUNT 8192

namespace CLAR {

    static uint32_t align_up(uint32_t size, uint32_t alignment)
	{
		return (size + alignment - 1) & ~(alignment - 1);
	}

    static VkTransformMatrixKHR glmToVkTransform(const glm::mat4& matrix)
	{
		VkTransformMatrixKHR transform;
		for (uint32_t i = 0; i < 3; i++)
		{
			for (uint32_t j = 0; j < 4; j++)
			{
				transform.matrix[i][j] = matrix[j][i];
			}
		}
		return transform;
	}

    void HelloTriangleApplication::run() {
        //std::cout << "Hello Triangle!" << std::endl;
        initVulkan();
        //std::cout << "Vulkan initialized!" << std::endl;
        initImGui();
        //std::cout << "ImGui initialized!" << std::endl;
        initRayTracing();
        //std::cout << "Ray Tracing initialized!" << std::endl;
        mainLoop();
        cleanup();
    }

    void HelloTriangleApplication::initVulkan() {
        camera = Camera(glm::vec3(0.0f, 1.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f));

        m_Window.SetResizeCallback([&]() {
            CreateOffscreenRender();

            VkDescriptorImageInfo imageInfo{ {}, m_OffscreenColor[0].descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                DescritorWriter()
                    .WriteStorageImage(1, &imageInfo)
                    .Update(m_RtDescriptorSets[i], m_Device);
            }

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
            {
                DescritorWriter()
                    .WriteImage(0, &m_OffscreenColor[0].descriptor)
                    .Update(m_PostDescriptorSets[i], m_Device);
            }
		});

        m_OffscreenColor.resize(2);
        CreateOffscreenRender();

        m_DescriptorSetLayout.PushBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_VERTEX_BIT);
        m_DescriptorSetLayout.PushBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT);
        m_DescriptorSetLayout.PushBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        m_DescriptorSets = m_DescriptorSetLayout.CreateSets();

        renderSystem.Init(m_DescriptorSetLayout,
			m_OffscreenRenderPass,
			m_Renderer.GetSwapChainExtent()
		);
        
        m_PostDescriptorSetLayout.PushBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_PostDescriptorSetLayout.PushBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

        m_PostDescriptorSets = m_PostDescriptorSetLayout.CreateSets();
        postSystem.Init(m_PostDescriptorSetLayout,
            m_Renderer.GetSwapChainRenderPass(),
            m_Renderer.GetSwapChainExtent(),
            "shaders/post.vert.spv",
            "shaders/post.frag.spv"
        );

        tm.LoadFromFile("textures/texture.jpg", "tex", true);
        tm.LoadFromFile("textures/viking_room.png", "vik", true);

        /*m_Models["house"] = new Model();
        m_Models["house"]->LoadModel("models/Medieval_building.obj");*/

        m_Models["sphere"] = new Model();
        m_Models["sphere"]->LoadModel("models/sphere.obj");

        m_Models["cube"] = new Model();
        m_Models["cube"]->LoadModel("models/cube.obj");

        m_Models["square"] = new Model();
        m_Models["square"]->LoadModel({ { {-1.5f, 0.f, -1.5f} },
                                            { {1.5f, 0.f, -1.5f} },
                                            { {1.5f, 0.f, 1.5f} },
                                            { {-1.5f, 0.f, 1.5f} } },
            {0, 1, 3, 1, 2, 3}
        );

        m_Models["casa"] = new Model();
        m_Models["casa"]->LoadModel("models/Medieval_building.obj");

        //m_Models["house"]->m_VertexBuffer = m_Allocator.CreateBuffer(m_Models["house"]->mesh, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        //m_Models["house"]->m_IndexBuffer = m_Allocator.CreateBuffer(m_Models["house"]->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        m_Models["sphere"]->m_VertexBuffer = m_Allocator.CreateBuffer(m_Models["sphere"]->mesh, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_Models["sphere"]->m_IndexBuffer = m_Allocator.CreateBuffer(m_Models["sphere"]->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        m_Models["square"]->m_VertexBuffer = m_Allocator.CreateBuffer(m_Models["square"]->mesh, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_Models["square"]->m_IndexBuffer = m_Allocator.CreateBuffer(m_Models["square"]->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        m_Models["cube"]->m_VertexBuffer = m_Allocator.CreateBuffer(m_Models["cube"]->mesh, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_Models["cube"]->m_IndexBuffer = m_Allocator.CreateBuffer(m_Models["cube"]->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        m_Models["casa"]->m_VertexBuffer = m_Allocator.CreateBuffer(m_Models["casa"]->mesh, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        m_Models["casa"]->m_IndexBuffer = m_Allocator.CreateBuffer(m_Models["casa"]->indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        // in real life use something like VulkanMemoryAllocator library because there is a limit on how many allocations
        // we can do on the gpu

        auto ground = std::make_shared<Lambertian>(glm::vec3(0.8f, 0.8f, 0.0f));
        auto center = std::make_shared<Lambertian>(glm::vec3(0.1f, 0.2f, 0.5f));
        auto left = std::make_shared<Dielectric>(1.5f);
        auto left2 = std::make_shared<Dielectric>(1.f / 1.5f);
        auto right = std::make_shared<Metal>(glm::vec3(0.8f, 0.85f, 0.88f), 0.0f);

        auto left_red = std::make_shared<Lambertian>(glm::vec3(1.0f, 0.2f, 0.2f));
        auto back_green = std::make_shared<Lambertian>(glm::vec3(0.2f, 1.0f, 0.2f));
        auto right_blue = std::make_shared<Lambertian>(glm::vec3(0.2f, 0.2f, 1.0f));
        auto upper_orange = std::make_shared<Lambertian>(glm::vec3(1.0f, 0.5f, 0.0f));
        auto lower_teal = std::make_shared<Lambertian>(glm::vec3(0.2f, 0.8f, 0.8f));

        auto red = std::make_shared<Lambertian>(glm::vec3(.65f, .05f, .05f));
        auto green = std::make_shared<Lambertian>(glm::vec3(.12f, .45f, .15f));
        auto white = std::make_shared<Lambertian>(glm::vec3(.73f, .73f, .73f));
        auto light = std::make_shared<DiffuseLight>(glm::vec3(1.0f), 8.1f);

        m_Instances.emplace_back("Right Wall", Instance{ m_Models["square"], red, glm::vec3(1.5f, 1.5f, 0.0f), glm::vec3(1.f), glm::vec3(0.f, 0.f, 90.f), 0 });
        m_Instances.emplace_back("Light", Instance{ m_Models["square"], light, glm::vec3(0.0f, 2.999f, 0.0f), glm::vec3(.3f), glm::vec3(0.f), 1 });

        m_Instances.emplace_back("Left Wall", Instance{ m_Models["square"], green, glm::vec3(-1.5f, 1.5f, 0.0f), glm::vec3(1.f), glm::vec3(0.f, 0.f, -90.f), 2 });
        m_Instances.emplace_back("Ground", Instance{ m_Models["square"], white, glm::vec3(0.0f), glm::vec3(1.f), glm::vec3(0.f), 3 });
        m_Instances.emplace_back("Back Wall", Instance{ m_Models["square"], white, glm::vec3(0.0f, 1.5f, -1.5f), glm::vec3(1.f), glm::vec3(90.f, 0.f, 0.f), 4 });
        m_Instances.emplace_back("Floor", Instance{ m_Models["square"], white, glm::vec3(0.0f, 3.f, 0.0f), glm::vec3(1.f), glm::vec3(180.f, 0.f, 0.f), 5 });
        m_Instances.emplace_back("Sphere", Instance{ m_Models["sphere"], left, glm::vec3(0.5f, 0.5f, 0.6f), glm::vec3(0.5f), glm::vec3(0.f, -15.f, 0.f), 6 });
        m_Instances.emplace_back("Cube", Instance{ m_Models["cube"], white, glm::vec3(-0.6f, 1.f, -0.5f), glm::vec3(1.f, 2.f, 1.f), glm::vec3(0.f, 18.f, 0.f), 7 });
        m_Instances.emplace_back("Light2", Instance{ m_Models["square"], red, glm::vec3(0.0f, 1.999f, 0.0f), glm::vec3(.3f), glm::vec3(0.f), 8 });


        //m_Instances.emplace_back("Casa", Instance{ m_Models["casa"], white, glm::vec3(0.0f), glm::vec3(1.f), glm::vec3(0.f), 8 });
        for (const auto& [_, instance] : m_Instances)
        {
            ObjDesc desc{};

            desc.vertexAddress = m_Device.GetBufferDeviceAddress(instance.model->m_VertexBuffer.buffer);
            desc.indexAddress = m_Device.GetBufferDeviceAddress(instance.model->m_IndexBuffer.buffer);
            desc.material = instance.material->GetType();
            desc.albedo = instance.material->GetAlbedo();
            desc.fuzz = instance.material->GetFuzzOrRefractionIndex();

            m_ObjectDescriptions.push_back(desc);
		}

        m_BobjDesc = m_Allocator.CreateBuffer(m_ObjectDescriptions, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        std::vector<LightDesc> lightDescription(10);
        m_pcRay.lightsNumber = 0;
        for (size_t i = 0; i < m_Instances.size(); ++i)
		{
			if (m_Instances[i].second.material->GetType() == MaterialType::DIFFUSE_LIGHT)
            {
                lightDescription[m_pcRay.lightsNumber++] = { m_Instances[i].second.TransformMatrix(), (uint32_t)i };
            }
		}
        m_LightModelsBuffer = m_Allocator.CreateBuffer(lightDescription, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

        
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_UniformBuffers.emplace_back(
                m_Allocator.CreateBuffer(sizeof(UniformBufferObject),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
            );

            
		}

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            auto bufferInfo = m_UniformBuffers[i].DescriptorInfo();
            auto odbufferInfo = m_BobjDesc.DescriptorInfo();
            auto lightModelsInfo = m_LightModelsBuffer.DescriptorInfo();

            DescritorWriter()
                .WriteUniformBuffer(0, &bufferInfo)
                .WriteStorageBuffer(1, &odbufferInfo)
                .WriteStorageBuffer(2, &lightModelsInfo)
                .Update(m_DescriptorSets[i], m_Device);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            DescritorWriter()
                .WriteImage(0, &m_OffscreenColor[(i - 1) % MAX_FRAMES_IN_FLIGHT].descriptor)
                .WriteImage(1, &m_OffscreenColor[i].descriptor)
                .Update(m_PostDescriptorSets[i], m_Device);
        }

    }

    void HelloTriangleApplication::initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); //(void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiBackendFlags_PlatformHasViewports;
        io.ConfigFlags |= ImGuiBackendFlags_RendererHasViewports;


        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        ImGuiDescriptorSetLayout.PushBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
        ImGuiDescriptorSetLayout.CreateDescriptorPool(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        /*ImGuiDescriptorSets = ImGuiDescriptorSetLayout.
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            DescritorWriter()
                .WriteImage(0, &tm["tex"]->descriptor)
                .Update(ImGuiDescriptorSets[i], m_Device);
        }*/

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_Device.GetInstance();
        init_info.PhysicalDevice = m_Device.GPU();             
        init_info.Device = m_Device;
        init_info.QueueFamily = m_Device.findQueueFamilies().graphicsFamily.value();
        init_info.Queue = m_Device.GetGraphicsQueue();
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = ImGuiDescriptorSetLayout.GetDescriptorPool();
        init_info.RenderPass = m_Renderer.GetSwapChainRenderPass();
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = m_Renderer.GetSwapChainImageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = [](VkResult err) {
                if (err != VK_SUCCESS)
                {
                    throw std::runtime_error("ImGui_ImplVulkan_Init failed");
                }
            };
        ImGui_ImplVulkan_Init(&init_info);

        //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable

        
        /*m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer) {
			ImGui_ImplVulkan_CreateFontsTexture();
		});*/
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));  // 50% transparency


        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowRounding = 4.0f;
        style.ScrollbarSize = 12.0f;
        style.FramePadding = ImVec2(6, 4);
        style.ItemSpacing = ImVec2(8, 6);

        style.WindowRounding = 6.0f;       // Nice soft corners
        style.WindowBorderSize = 0.0f;      // Remove the border
    }

    void HelloTriangleApplication::initRayTracing()
    {
        VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        prop2.pNext = &m_rtProperties;
        vkGetPhysicalDeviceProperties2(m_Device.GPU(), &prop2);
        
        // There can be more than one blass and each blas can have multiple geometries
        // So we can loop over all the models and then create more geometries for each model
        // This would translate into 2 for loops

        std::vector<BlasInput> allBlas;
        allBlas.reserve(m_Models.size());

        for (const auto& [name, model] : m_Models)
		{
			allBlas.emplace_back(ModelToVkgeometry(model));
		}

        buildAs = m_RtBuilder.BuildBlas(allBlas); 

        // Create the top-level acceleration structure

        std::vector<VkAccelerationStructureInstanceKHR> instances;
        instances.reserve(m_Instances.size());

        for (const auto& [_, ins] : m_Instances)
        {
            VkAccelerationStructureInstanceKHR instance{};
            instance.transform = glmToVkTransform(ins.TransformMatrix());
            instance.instanceCustomIndex = ins.instanceCustomIndex;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            //VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;

            // write a function to get the device address of the element in buildas whose blasId is ins.blasId
            auto blas = std::find_if(buildAs.begin(), buildAs.end(), [&](const ASBuildInfo& b) { return b.blasId == ins.model->id; });
            VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
            addressInfo.accelerationStructure = blas->as.handle;
            instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(m_Device, &addressInfo);

            instances.push_back(instance);
        }
        m_Tlas = m_RtBuilder.BuildTlas(instances);

        CreateRtDescriptorSets();

        rtSystem.Init({ m_RtDescriptorSetLayout, m_DescriptorSetLayout });

        CreateRtShaderBindingTable();
    }

    void HelloTriangleApplication::mainLoop() {
        auto startTime = std::chrono::high_resolution_clock::now();

        while (!m_Window.shouldClose())
        {
            glfwPollEvents();
            Update(m_Renderer.GetCurrentFrame());

            drawFrame();

            if (m_Window.KeyState(GLFW_KEY_W) == GLFW_PRESS) camera.MoveForward(dt);

            if (m_Window.KeyState(GLFW_KEY_S) == GLFW_PRESS) camera.MoveForward(-dt);

            if (m_Window.KeyState(GLFW_KEY_A) == GLFW_PRESS) camera.MoveRight(-dt);

            if (m_Window.KeyState(GLFW_KEY_D) == GLFW_PRESS) camera.MoveRight(dt);

            if (m_Window.KeyState(GLFW_KEY_SPACE) == GLFW_PRESS) camera.MoveUp(dt);

            if (m_Window.KeyState(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.MoveUp(-dt);

            if (m_Window.KeyState(GLFW_KEY_Q) == GLFW_PRESS)
			{
                m_CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
			}

            if (m_Window.KeyState(GLFW_KEY_E) == GLFW_PRESS)
            {
                m_CurrentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
			}

            if (m_Window.KeyState(GLFW_KEY_R) == GLFW_PRESS)
			{
				m_CurrentGizmoOperation = ImGuizmo::OPERATION::SCALE;
            }

            double xpos, ypos;
            m_Window.GetCursorDelta(&xpos, &ypos);
            camera.LookUp(-ypos * dt);
            camera.LookRight(-xpos * dt);

            m_Window.GetCursorPos(&xpos, &ypos);

            //std::cout << xpos << " " << ypos << "\n";

            /*if (m_Window.KeyState(GLFW_KEY_UP) == GLFW_PRESS) camera.RotateAlongPivotUp(-dt);
            if (m_Window.KeyState(GLFW_KEY_DOWN) == GLFW_PRESS) camera.RotateAlongPivotUp(dt);

            if (m_Window.KeyState(GLFW_KEY_LEFT) == GLFW_PRESS) camera.RotateAlongPivotRight(-dt);
            if (m_Window.KeyState(GLFW_KEY_RIGHT) == GLFW_PRESS) camera.RotateAlongPivotRight(dt);*/


            auto currentTime = std::chrono::high_resolution_clock::now();
            dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            startTime = currentTime;
        }

        vkDeviceWaitIdle(m_Device);
    }

    void HelloTriangleApplication::cleanup() {

        m_Allocator.DestroyBuffer(m_BobjDesc);
        m_Allocator.DestroyBuffer(m_LightModelsBuffer);

        vkDestroyRenderPass(m_Device, m_OffscreenRenderPass, nullptr);
        vkDestroyFramebuffer(m_Device, m_OffscreenFramebuffer, nullptr);
        m_Allocator.DestroyTexture(m_OffscreenColor[0]);
        m_Allocator.DestroyTexture(m_OffscreenColor[1]);
        m_Allocator.DestroyTexture(m_OffscreenDepth);

        vkDestroySampler(m_Device, m_TextureSampler, nullptr);

        m_Allocator.DestroyBuffer(m_rtSBTBuffer);

        for (auto& [name, model] : m_Models)
        {
            m_Allocator.DestroyBuffer(model->m_VertexBuffer);
            m_Allocator.DestroyBuffer(model->m_IndexBuffer);
            delete model;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_Allocator.DestroyBuffer(m_UniformBuffers[i]);
		}

        m_Allocator.DestroyAccelerationStructure(m_Tlas);
        for (auto& blas : buildAs)
		{
            m_Allocator.DestroyAccelerationStructure(blas.as);
		}

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void HelloTriangleApplication::createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0,
            .maxLod = static_cast<float>(tm["tex"]->mipLevels),
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkPhysicalDeviceProperties properties = m_Device.GetPhysicalDeviceProperties();

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(tm["tex"]->mipLevels);

        // samplerInfo.anisotropyEnable = VK_FALSE;
        // samplerInfo.maxAnisotropy = 1.0f;

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void HelloTriangleApplication::drawFrame() {

        {
            auto ComputeCommandBuffer = m_Renderer.BeginCompute();

            /*computeSystem.Prepare(ComputeCommandBuffer, &m_ComputeDescriptorSets[m_Renderer.GetCurrentFrame()]);

            vkCmdDispatch(ComputeCommandBuffer, PARTICLE_COUNT / 256, 1, 1);*/

            m_Renderer.EndCompute();
        }

        // i am gui
        static bool useRaytracer = true;
        static glm::vec4 clearColor = glm::vec4(0.5f, 0.7f, 1.0f, 0.00f);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {clearColor[0], clearColor[1], clearColor[2], clearColor[3]} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGuizmo::BeginFrame();

            {
                if (selectedInstanceIndex >= 0 && selectedInstanceIndex < m_Instances.size())
                {
                    // Optional: Set up a full-screen invisible window
                    ImGui::SetNextWindowPos(ImVec2(0, 0));
                    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                    ImGui::Begin("DockSpaceHost",
                        nullptr,
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_NoNavFocus |
                        ImGuiWindowFlags_NoBackground);

                    //ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                    //ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

                    // Guizmo
                    ImGuizmo::SetOrthographic(false);
                    ImGuizmo::SetDrawlist();
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImVec2 windowSize = ImGui::GetWindowSize();

                    ImGuizmo::SetRect(windowPos.x, windowPos.y, windowSize.x, windowSize.y);

                    auto view = camera.LookAt();
                    auto projection = glm::perspective(glm::radians(60.0f), (float)m_Renderer.GetSwapChainExtent().width / m_Renderer.GetSwapChainExtent().height, 0.1f, 10.0f);

                    auto& instance = m_Instances[selectedInstanceIndex].second;

                    auto model = instance.TransformMatrix();

                    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), m_CurrentGizmoOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(model));

                    if (ImGuizmo::IsUsing())
                    {
                        glm::vec3 translation, rotation, scale;
                        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
                        instance.displacement = translation;
                        instance.rotation = rotation;
                        instance.scale = scale;
                        m_InstanceUpdated = true;
                    }

                    ImGui::End();
                }

                //ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x / 3.5f, ImGui::GetIO().DisplaySize.y });
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f)); // RGBA, alpha = 0.5
                if (ImGui::Begin("Debug", nullptr,
                    ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoScrollbar
                ))
                {

                    //ImGui::Image(reinterpret_cast<ImTextureID>(m_PostDescriptorSets[m_Renderer.GetCurrentFrame()]),
                    //    { (float)m_Renderer.GetSwapChainExtent().width / 2, (float)m_Renderer.GetSwapChainExtent().height / 2 } );


                    ImGui::ColorEdit3("Clear color", reinterpret_cast<float*>(&clearColor));
                    ImGui::Checkbox("Ray Tracer mode", &useRaytracer);  // Switch between raster and ray tracing
                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                    ImGui::SeparatorText("Scene Hierarchy");

                    for (size_t i = 0; i < m_Instances.size(); i++) {
                        // Use ImGui::Selectable or ImGui::Button for list items
                        if (ImGui::Selectable(m_Instances[i].first.c_str(), selectedInstanceIndex == i)) {
                            // Set the selected instance index when clicked
                            selectedInstanceIndex = i;
                        }
                    }

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        // User clicked inside this window
                        selectedInstanceIndex = -1; // Deselect all instances
                    }
                




                
                    if (selectedInstanceIndex >= 0 && selectedInstanceIndex < m_Instances.size())
                    {

                        // Space everything else to the top by pushing Y position close to the bottom
                        float propsHeight = 200.f; // Height you want for your static bottom section
                        float spaceLeft = ImGui::GetContentRegionAvail().y - propsHeight;
                        if (spaceLeft > 0)
                            ImGui::Dummy(ImVec2(0.0f, spaceLeft)); // Push everything above

                        ImGui::SeparatorText("Instance Properties");

                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f)); // RGBA, alpha = 0.5
                        if (ImGui::BeginChild("InstancePropertiesChild", ImVec2(0, 0), true))
                        {

                            auto& instance = m_Instances[selectedInstanceIndex].second;
                            auto& objDescription = m_ObjectDescriptions[selectedInstanceIndex];

                            m_InstanceUpdated |= ImGui::DragFloat3("Position", reinterpret_cast<float*>(&instance.displacement), 0.01f);
                            m_InstanceUpdated |= ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&instance.rotation), 0.01f);
                            m_InstanceUpdated |= ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&instance.scale), 0.01f);

                            //ImGui::Checkbox("Is Visible", &instance.visible);
                            const char* labels[] = { "Lambertian", "Metal", "Dielectric", "Diffuse Light" };
                            const MaterialType values[] = { MaterialType::LAMBERTIAN, MaterialType::METAL, MaterialType::DIELECTRIC, MaterialType::DIFFUSE_LIGHT };  // Actual values

                            MaterialType selectedValue = objDescription.material;  // Get the current selected value

                            int selectedIndex = -1;  // Initialize selected index
                            for (int i = 0; i < IM_ARRAYSIZE(values); i++) {
                                if (values[i] == selectedValue) {
                                    selectedIndex = i;
                                    break;
                                }
                            }

                            if (ImGui::Combo("Material", &selectedIndex, labels, IM_ARRAYSIZE(labels)))
                            {
                                selectedValue = values[selectedIndex];  // Update the actual value
                                objDescription.material = selectedValue;
                                instance.material = MaterialFactory::CreateMaterial(selectedValue, objDescription.albedo, objDescription.fuzz);
                                m_InstanceUpdated = true;
                            }

                            ImGui::ColorEdit3("Albedo", reinterpret_cast<float*>(&objDescription.albedo), 0.01f);

                            if (selectedValue == MaterialType::DIELECTRIC)
                            {
                                ImGui::DragFloat("Refraction Index", &objDescription.fuzz, 0.01f, 0.f);
                            }
                            else if (selectedValue == MaterialType::METAL)
                            {
                                ImGui::DragFloat("Fuzz", &objDescription.fuzz, 0.01f);
                            }
                            else if (selectedValue == MaterialType::DIFFUSE_LIGHT)
                            {
                                ImGui::DragFloat("Light Area", &objDescription.fuzz, 0.01f);
                            }

                        }
                        ImGui::EndChild(); // End scrollable area
                        ImGui::PopStyleColor();
                    }

                }

                ImGui::PopStyleColor();
                ImGui::End();

                // Build FPS text
                std::string fpsText = "FPS: " + std::to_string((int)ImGui::GetIO().Framerate);

                // Set text position
                ImVec2 pos = ImVec2(ImGui::GetIO().DisplaySize.x - 60, 0.0f);

                // Draw directly on background (no window needed)
                ImGui::GetBackgroundDrawList()->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize(),
                    pos,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 1.0f)),
                    fpsText.c_str()
                );
            }
        }

        if (auto commandBuffer = m_Renderer.BeginFrame())
        {
            /*{
                gridSystem.BindPL(commandBuffer);
                gridSystem.BindDescSet(commandBuffer, &m_DescriptorSets[m_Renderer.GetCurrentFrame()]);

                vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            }


            {
                particleSystem.BindPL(commandBuffer);
                particleSystem.BindDescSet(commandBuffer, &m_DescriptorSets2[m_Renderer.GetCurrentFrame()]);

                VkDeviceSize offsets[] = { 0 };
                auto buf = shaderStorageBuffers[m_Renderer.GetCurrentFrame()].Get();
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buf, offsets);
                vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
            }*/
            if (useRaytracer)
            {
                raytrace(commandBuffer, clearColor);
            }
            else
            {
                VkRenderPassBeginInfo offscreenRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
                offscreenRenderPassBeginInfo.clearValueCount = 2;
                offscreenRenderPassBeginInfo.pClearValues = clearValues.data();
                offscreenRenderPassBeginInfo.renderPass = m_OffscreenRenderPass;
                offscreenRenderPassBeginInfo.framebuffer = m_OffscreenFramebuffer;
                offscreenRenderPassBeginInfo.renderArea = { {0, 0}, m_Renderer.GetSwapChainExtent() };

                m_Renderer.BeginRenderPass(offscreenRenderPassBeginInfo);

                renderSystem.Prepare(commandBuffer, &m_DescriptorSets[m_Renderer.GetCurrentFrame()]);
                m_Models["cube"]->Draw(commandBuffer);
                m_Models["square"]->Draw(commandBuffer);

                m_Renderer.EndRenderPass();
            }

            // QUAD bulshit and iamgui
            {
                m_Renderer.BeginRenderPass();

                postSystem.Prepare(commandBuffer, &m_PostDescriptorSets[m_Renderer.GetCurrentFrame()]);
                vkCmdDraw(commandBuffer, 3, 1, 0, 0);

                ImGui::Render();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
                m_Renderer.EndRenderPass();
            }

            m_Renderer.EndFrame();
        }

    }

    void HelloTriangleApplication::Update(uint32_t currentImage) {

        m_ProjMatrices[currentImage] = glm::perspective(glm::radians(60.0f), (float)m_Renderer.GetSwapChainExtent().width / m_Renderer.GetSwapChainExtent().height, 0.1f, 10.0f);
        m_ProjMatrices[currentImage][1][1] *= -1;
        m_ViewMatrices[currentImage] = camera.LookAt();

        ubo = {
                .prev_view = m_ViewMatrices[(currentImage - 1) % 2],
                .prev_proj = m_ProjMatrices[(currentImage - 1) % 2],
                .model = glm::mat4(1),
                .view = glm::inverse(m_ViewMatrices[currentImage]),
                .proj = glm::inverse(m_ProjMatrices[currentImage])
        };

        m_UniformBuffers[currentImage].Write(&ubo, sizeof(UniformBufferObject));
        m_BobjDesc.Write(m_ObjectDescriptions.data(), m_ObjectDescriptions.size() * sizeof(ObjDesc));

        if (m_InstanceUpdated)
        {
            std::vector<LightDesc> lightDescription(10);
            m_pcRay.lightsNumber = 0;
            for (size_t i = 0; i < m_Instances.size(); ++i) 
            {
                if (m_Instances[i].second.material->GetType() == MaterialType::DIFFUSE_LIGHT)
                {
                    lightDescription[m_pcRay.lightsNumber++] = { m_Instances[i].second.TransformMatrix(), (uint32_t)i };
                }
            }

            m_LightModelsBuffer.Write(lightDescription.data(), lightDescription.size() * sizeof(LightDesc));
            

            m_InstanceUpdated = false;
            std::vector<VkAccelerationStructureInstanceKHR> instances;
            instances.reserve(m_Instances.size());

            for (const auto& [_, ins] : m_Instances)
            {
                VkAccelerationStructureInstanceKHR instance{};
                instance.transform = glmToVkTransform(ins.TransformMatrix());
                instance.instanceCustomIndex = ins.instanceCustomIndex;
                instance.mask = 0xFF;
                instance.instanceShaderBindingTableRecordOffset = 0;
                instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                //VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;

                // write a function to get the device address of the element in buildas whose blasId is ins.blasId
                auto blas = std::find_if(buildAs.begin(), buildAs.end(), [&](const ASBuildInfo& b) { return b.blasId == ins.model->id; });
                VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
                addressInfo.accelerationStructure = blas->as.handle;
                instance.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(m_Device, &addressInfo);

                instances.push_back(instance);
            }
            m_Tlas = m_RtBuilder.UpdateTlas(m_Tlas, instances);
        }
    }

    BlasInput HelloTriangleApplication::ModelToVkgeometry(const Model* model)
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

    void HelloTriangleApplication::CreateRtDescriptorSets()
    {
        
        m_RtDescriptorSetLayout.PushBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        m_RtDescriptorSetLayout.PushBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        m_RtDescriptorSetLayout.PushBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        m_RtDescriptorSets = m_RtDescriptorSetLayout.CreateSets();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
            VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
            descASInfo.accelerationStructureCount = 1;
            descASInfo.pAccelerationStructures = &m_Tlas.handle;
            /*VkDescriptorImageInfo lastImageInfo{ {}, m_OffscreenColor[(i - 1) % MAX_FRAMES_IN_FLIGHT].descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};
            VkDescriptorImageInfo currentImageInfo{ {}, m_OffscreenColor[i].descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };*/

			DescritorWriter()
				.WriteAccelerationStructure(0, &descASInfo)
				.WriteStorageImage(1, &m_OffscreenColor[(i - 1) % MAX_FRAMES_IN_FLIGHT].descriptor)
                .WriteStorageImage(2, &m_OffscreenColor[i].descriptor)
				.Update(m_RtDescriptorSets[i], m_Device);
		}
    }

    void HelloTriangleApplication::CreateRtShaderBindingTable()
    {
        uint32_t missCount{ 1 }; // 2 if 2 miss shaders
        uint32_t hitCount{ 1 };
        auto     handleCount = 1 + missCount + hitCount;
        uint32_t handleSize = m_rtProperties.shaderGroupHandleSize;

        // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
        uint32_t handleSizeAligned = align_up(handleSize, m_rtProperties.shaderGroupHandleAlignment);

        m_rgenRegion.stride = align_up(handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
        m_rgenRegion.size = m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
        m_missRegion.stride = handleSizeAligned;
        m_missRegion.size = align_up(missCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
        m_hitRegion.stride = handleSizeAligned;
        m_hitRegion.size = align_up(hitCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);

        // Get the shader group handles
        uint32_t             dataSize = handleCount * handleSize;
        std::vector<uint8_t> handles(dataSize);
        auto result = rtSystem.GetRayTracingShaderGroupHandles(handleCount, dataSize, handles.data());

        assert(result == VK_SUCCESS);

        // Allocate a buffer for storing the SBT.
        VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
        m_rtSBTBuffer = m_Allocator.CreateBuffer(sbtSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
        //m_debug.setObjectName(m_rtSBTBuffer.buffer, std::string("SBT"));  // Give it a debug name for NSight.

        VkDeviceAddress sbtAddress = m_Device.GetBufferDeviceAddress(m_rtSBTBuffer.buffer);
        m_rgenRegion.deviceAddress = sbtAddress;
        m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
        m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_missRegion.size;

        // Helper to retrieve the handle data
        auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

        uint32_t handleIdx{ 0 };
        uint8_t* pData{ nullptr };

        // Copy the handles into the SBT
        pData = (uint8_t*)m_rtSBTBuffer.allocationInfo.pMappedData;
        memcpy(pData, getHandle(handleIdx++), handleSize);  // Raygen

        // Miss
        pData = (uint8_t*)m_rtSBTBuffer.allocationInfo.pMappedData + m_rgenRegion.size;
        for (uint32_t c = 0; c < missCount; c++)
        {
            memcpy(pData, getHandle(handleIdx++), handleSize);
            pData += m_missRegion.stride;
        }

        // Hit
        pData = (uint8_t*)m_rtSBTBuffer.allocationInfo.pMappedData + m_rgenRegion.size + m_missRegion.size;
        for (uint32_t c = 0; c < hitCount; c++)
        {
            memcpy(pData, getHandle(handleIdx++), handleSize);
            pData += m_hitRegion.stride;
        }
    }

    void HelloTriangleApplication::raytrace(const VkCommandBuffer& cmdBuf, const glm::vec4& clearColor)
    {
        // Initializing push constant values
        m_pcRay.clearColor = clearColor;
        m_pcRay.time += dt;
        m_pcRay.deltaTime = dt;

        std::vector<VkDescriptorSet> descSets{ m_RtDescriptorSets[m_Renderer.GetCurrentFrame()], m_DescriptorSets[m_Renderer.GetCurrentFrame()] };
        
        rtSystem.Prepare(cmdBuf, descSets);

        rtSystem.PushConstants(cmdBuf, m_pcRay);

        vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, m_Renderer.GetSwapChainExtent().width, m_Renderer.GetSwapChainExtent().height, 1);
    }

    void HelloTriangleApplication::CreateOffscreenRender()
    {
        m_Allocator.DestroyTexture(m_OffscreenColor[0]);
        m_Allocator.DestroyTexture(m_OffscreenColor[1]);
        m_Allocator.DestroyTexture(m_OffscreenDepth);

        // color image
        {
            Image image = m_Allocator.CreateImage(m_Renderer.GetSwapChainExtent(), m_OffscreenColorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_STORAGE_BIT);

            m_OffscreenColor[0] = m_Allocator.CreateTexture(image, m_OffscreenColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            m_Device.transitionImageLayout(image.image, m_OffscreenColorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
            m_OffscreenColor[0].descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        {
            Image image = m_Allocator.CreateImage(m_Renderer.GetSwapChainExtent(), m_OffscreenColorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_STORAGE_BIT);

            m_OffscreenColor[1] = m_Allocator.CreateTexture(image, m_OffscreenColorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            m_Device.transitionImageLayout(image.image, m_OffscreenColorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
            m_OffscreenColor[1].descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }

        // fill the offscreencolor with black pixels
        {
            /*m_Device.SingleTimeCommand([&](VkCommandBuffer commandBuffer) {
                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = 0;
                subresourceRange.levelCount = 1;
                subresourceRange.baseArrayLayer = 0;
                subresourceRange.layerCount = 1;

				VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				vkCmdClearColorImage(commandBuffer, m_OffscreenColor[0].image.image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
                vkCmdClearColorImage(commandBuffer, m_OffscreenColor[1].image.image, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
			});*/
        }


        // depth image
        {
			Image image = m_Allocator.CreateImage(m_Renderer.GetSwapChainExtent(), m_OffscreenDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

			m_OffscreenDepth = m_Allocator.CreateTexture(image, m_OffscreenDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            m_Device.transitionImageLayout(image.image, m_OffscreenDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
            m_OffscreenDepth.descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

        VkAttachmentDescription colorAttachment{
            .format = m_OffscreenColorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        VkAttachmentReference colorAttachmentRef{
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        VkAttachmentDescription depthAttachment{
            .format = m_OffscreenDepthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentReference depthAttachmentRef{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentDescription resolveAttachment{
            .format = m_OffscreenColorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
            };

        VkAttachmentReference resolveAttachmentRef{
			.attachment = 2,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

        VkSubpassDescription subpass{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
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

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency
        };

        if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_OffscreenRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        std::array<VkImageView, 2> frameBufferAttachments = { m_OffscreenColor[0].descriptor.imageView, m_OffscreenDepth.descriptor.imageView};

        VkFramebufferCreateInfo frameBufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_OffscreenRenderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = frameBufferAttachments.data(),
            .width = m_Renderer.GetSwapChainExtent().width,
            .height = m_Renderer.GetSwapChainExtent().height,
            .layers = 1
        };

        if (vkCreateFramebuffer(m_Device, &frameBufferInfo, nullptr, &m_OffscreenFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to crate framebuffer!");
        }
    }

}