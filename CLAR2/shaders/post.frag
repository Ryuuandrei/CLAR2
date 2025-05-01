#version 450
layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D lastnoisyTxt;
layout(binding = 1) uniform sampler2D currentnoisyTxt;



void main()
{
  vec2  uv    = outUV;
  float gamma = 1. / 2.0;
//  fragColor   = clamp(pow(texture(noisyTxt, uv).rgba, vec4(gamma)), 0.0, 1.0);
  fragColor   = texture(currentnoisyTxt, uv);


//    vec2 uv = outUV;
//
//    float alpha = 0.9; // Blend factor for temporal accumulation
//
//    // Fetch colors from both frames
//    vec3 lastColor = texture(lastNoisyTxt, uv).rgb;
//    vec3 currentColor = texture(currentNoisyTxt, uv).rgb;
//
//    // Accumulate in linear space (before gamma correction)
////    vec3 accumulatedColor = mix(lastColor, currentColor, 1.0 - alpha);
//	vec3 accumulatedColor = currentColor + lastColor;
//
//    // Apply gamma correction after accumulation
//    float gamma = 1.0 / 2.0;
//    fragColor = vec4(pow(accumulatedColor, vec3(gamma)), 1.0);
}
