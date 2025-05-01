#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 circleCenter;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float value;
} ubo;

// Uniforms to control the line thickness and spacing
float lineThickness = 0.002; // Thickness of the lines
float lineSpacing = 0.1;     // Spacing between the lines

void main() {
    // Adjust line thickness based on zoom level
    lineThickness -= 0.001 * fract(ubo.value);

    // Calculate the zoom factor based on ubo.value
    float zoomFactor = 4.0 + 4.0 * fract(ubo.value);
    float zoomFactor2 = 2.0 + 2.0 * fract(ubo.value);

    // Scale the translated UV coordinates for zooming
    fract(circleCenter.xz) - 0.5;

    vec2 zoomedUV = (fragUV - circleCenter.xz) / lineSpacing / zoomFactor;
    vec2 zoomedUV2 = (fragUV - circleCenter.xz) / lineSpacing / zoomFactor2;

    // Calculate the grid UVs and their fractional parts
    vec2 gridUVFract = fract(zoomedUV + circleCenter.xz);
    vec2 gridUVFract2 = fract(zoomedUV2 + circleCenter.xz * 2);

    // Determine if the current fragment is within the line thickness for the grid pattern
    bool isVerticalLine = gridUVFract.x < (lineThickness / lineSpacing);
    bool isHorizontalLine = gridUVFract.y < (lineThickness / lineSpacing);

    bool isVerticalLine2 = gridUVFract2.x < (lineThickness / lineSpacing);
    bool isHorizontalLine2 = gridUVFract2.y < (lineThickness / lineSpacing);

    // Calculate the distance from the fragment to the circle center
    float distanceToCenter = distance(fragUV, circleCenter.xz);

    // Apply a fade effect based on the distance from the center
    float fadeFactor = smoothstep(0.1, 0.7, distanceToCenter / 2.0);

    // Determine the colors of the lines
    vec3 lineColor = (isVerticalLine || isHorizontalLine) ? vec3(1.0) : vec3(0.0);
    vec3 lineColor2 = (isVerticalLine2 || isHorizontalLine2) ? vec3(1.0) : vec3(0.0);

    // Output the final color with fade effect
    fragColor = vec4(lineColor, 1.0 - fadeFactor) + smoothstep(0.0, 1.0, fract(ubo.value)) * vec4(lineColor2, 1.0 - fadeFactor);
}
