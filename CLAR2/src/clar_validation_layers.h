#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace CLAR {

#ifdef NDEBUG
	const bool enableValidationLayer = false;
#else
	const bool enableValidationLayer = true;
#endif

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();
}
