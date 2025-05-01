#pragma once

#include <optional>

namespace CLAR {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        operator bool()
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

}
