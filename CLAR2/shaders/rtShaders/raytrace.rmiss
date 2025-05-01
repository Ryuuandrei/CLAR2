#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(push_constant) uniform _PushConstantRay
{
  vec4  clearColor;
  float deltaTime;
  float lightIntensity;
  int   lightType;
} pcRay;

void main()
{
	prd.hitValue *= pcRay.clearColor.xyz;
	prd.miss = true;
}