#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    // Using smoothstep to create a sharper transition
    float alpha = smoothstep(0.4, 0.5, dist);

    outColor = vec4(fragColor, 1.0 - alpha);
}
