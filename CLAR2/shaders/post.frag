#version 450
#extension GL_ARB_shader_image_load_store : require

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(binding = 0, rgba32f) uniform readonly image2D u_LastAccumulatedTxt;
layout(binding = 1, rgba32f) uniform writeonly image2D u_CurrentAccumulatedTxt;
layout(binding = 2, rgba32f) uniform readonly image2D u_CurrentnoisyTxt;
layout(binding = 3, rgba32f) uniform readonly image2D u_PositionMap;

// the general flow is u_lastAccumulatedTxt + u_currentnoisyTxt = u_currentAccumulatedTxt

layout(binding = 4) uniform UBO
{
   mat4 reprojectionMatrix;
};

layout(push_constant) uniform _PushConstantRay {
    vec4  clearColor;
    float deltaTime;
    float time;
    int   lightsNumber;
} pcRay;

vec3 RGBToYCoCg (const in vec3 rgbColor)
{
	return rgbColor;
	const mat3 colorTransformMatrix = mat3 (
		0.25f, 0.5f, -0.25f,
		0.5f, 0.0f, 0.5f,
		0.25f, -0.5f, -0.25f
	);

	return colorTransformMatrix * rgbColor;
}

vec3 YCoCgToRGB (const in vec3 YCoCgColor)
{
	return YCoCgColor;
	const mat3 colorTransformMatrix = mat3 (
		1.f, 1.f, 1.f,
		1.f, 0.f, -1.f,
		-1.f, 1.f, -1.f
	);

	return colorTransformMatrix * YCoCgColor;
}

vec3 CalcClipNeighbourhood(const in vec3 lastLight, const in ivec2 texCoord)
{
	vec3 color_min = vec3 (1.0);
	vec3 color_max = vec3 (0.0);

	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			vec3 color = imageLoad(u_CurrentnoisyTxt, texCoord + ivec2(i, j)).xyz;
			color = RGBToYCoCg (color);

			color_min = min (color_min, color);
			color_max = max (color_max, color);
		}
	}

	vec3 colorSample = RGBToYCoCg (lastLight);

	vec3 p_clip = 0.5 * (color_max + color_min);
	vec3 e_clip = 0.5 * (color_max - color_min);

	vec3 v_clip = colorSample - p_clip;
	vec3 v_unit = v_clip / e_clip;
	vec3 a_unit = abs (v_unit);
	float max_unit = max (a_unit.x, max (a_unit.y, a_unit.z));

	if (max_unit > 1.0) {
		return YCoCgToRGB (p_clip + v_clip / max_unit);
	}

	return lastLight;
}


void main()
{
    vec2  uv    = outUV;
    float gamma = 1. / 2.0;
    //  fragColor   = clamp(pow(texture(noisyTxt, uv).rgba, vec4(gamma)), 0.0, 1.0);
    vec4 currentColor = imageLoad(u_CurrentnoisyTxt, ivec2(gl_FragCoord));

    vec4 position = imageLoad(u_PositionMap, ivec2(gl_FragCoord));

//    if (position == vec4(0.0, 0.0, 0.0, 1.0))
//    {
//		imageStore(u_CurrentAccumulatedTxt, ivec2(gl_FragCoord), currentColor);
//		fragColor = currentColor;
//		return;
//	}

    vec4 prevClip = reprojectionMatrix * position;
    vec2 prevNDC = prevClip.xy / prevClip.w;
    vec2 prevUV  = prevNDC.xy * 0.5 + 0.5;

	bvec2 a = greaterThan(prevUV, vec2(1.0));
	bvec2 b = lessThan(prevUV, vec2(0.0));

	if (any(bvec2(any(a), any(b))))
	{
		imageStore(u_CurrentAccumulatedTxt, ivec2(gl_FragCoord), currentColor);
		fragColor = currentColor;
		return;
	}

    ivec2 prevPixel = ivec2(prevUV * vec2(imageSize(u_LastAccumulatedTxt)));
    vec4 lastColor = imageLoad(u_LastAccumulatedTxt, prevPixel);

	vec3 clampedTemporalFilterColor = CalcClipNeighbourhood(lastColor.xyz, ivec2(gl_FragCoord));

	if (isnan (clampedTemporalFilterColor.x)) {
		imageStore(u_CurrentAccumulatedTxt, ivec2(gl_FragCoord), currentColor);
        fragColor = currentColor;
        return;
	}

	float blendWeight = pow(0.95, pcRay.deltaTime * 60.0);
    
//    if (lastColor == vec4(0.0, 0.0, 0.0, 1.0))
//	{
//		imageStore(currentnoisyTxt, ivec2(gl_FragCoord), currentColor);
//        fragColor = currentColor;
//		return;
//	}
    vec3 finalColor = mix(currentColor.xyz, clampedTemporalFilterColor, blendWeight);
    imageStore(u_CurrentAccumulatedTxt, ivec2(gl_FragCoord.xy), vec4(finalColor, 1.0));

    fragColor = vec4(finalColor, 1.0);
}
