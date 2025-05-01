#version 450

layout(binding = 0) uniform MVP{
	mat4 model;
	mat4 view;
	mat4 proj;
} mvp;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_PointSize = 14.0;
    gl_Position = mvp.proj * mvp.view * vec4(mvp.model[0].xyz, 1.0);
    fragColor = vec3(1.0, 0.0, 0.0);

//	gl_Position = vec4(positions[gl_VertexIndex], 1.0, 1.0);
//	fragColor = colors[gl_VertexIndex].rgb;
}