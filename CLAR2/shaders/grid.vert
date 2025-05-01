#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float value;
} ubo;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 circleCenter;

void main() {
    const vec3 positions[3] = vec3[](
        vec3(-4.0, 0, -2.0),
        vec3(-1.0, 0, 4.0),
        vec3( 4.0, 0, -1.0)
    );
    const vec2 uvs[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(positions[gl_VertexIndex], 1.0);
    fragUV = (ubo.model * vec4(positions[gl_VertexIndex], 1.0)).xz;
    circleCenter = (ubo.model * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
}
