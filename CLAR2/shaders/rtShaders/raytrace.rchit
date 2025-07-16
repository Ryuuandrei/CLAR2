#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "../raycommon.glsl"
#define M_PI 3.1415926535897932384626433832795

vec3 any_perpendicular(vec3 normal) {
    // Choose a vector that is NOT parallel to the normal
    vec3 tangent = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    
    // Compute a perpendicular vector using cross product
    return normalize(cross(normal, tangent));
}

vec3 randomCosineDirection(vec3 normal, vec2 seed)
{
    float r1 = random(seed);
    float r2 = random(seed.yx);

    // Convert random numbers to hemisphere
    float theta = acos(sqrt(r1));
    float phi = 2.0 * M_PI * r2;

    // Convert spherical coordinates to Cartesian
    vec3 localRay = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

    // Transform from local space to world space
    vec3 T = normalize(any_perpendicular(normal));
    vec3 B = cross(normal, T);

    return localRay.x * T + localRay.y * B + localRay.z * normal;
}

vec3 rfrct(vec3 uv, vec3 n, float eta) {
	float cosTheta = min(dot(-uv, n), 1.0);
    vec3 rOutPerp = eta * (uv + cosTheta * n);
    vec3 rOutParl = -sqrt(abs(1.0 - dot(rOutPerp, rOutPerp))) * n;
    return rOutPerp + rOutParl;
}

double reflectance(double cosine, double refraction_index) {
    // Use Schlick's approximation for reflectance.
    double r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0*r0;
    return r0 + (1-r0) * pow(float(1.0 - cosine),5.0f);
}

struct Vertex
{
	vec3 pos;
	vec3 nrm;
    vec3 color;
    vec2 texCoord;
};

struct ObjDesc
{
	uint64_t vertexAddress;
	uint64_t indexAddress;
    vec3 albedo;
    uint materialType;
    float fuzz;
};

struct Light {
    mat4 matrix;
    uint index;
};

hitAttributeEXT vec3 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 1, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

layout(set = 1, binding = 2, scalar) buffer LightBuffer {
    Light lights[];
} lb;

layout(push_constant) uniform _PushConstantRay {
    vec4  clearColor;
    float deltaTime;
    float time;
    int   lightsNumber;
} pcRay;

float scattering(vec3 normal, vec3 dir)
{
	return max(dot(normal, normalize(dir)), 0.0) / M_PI;
}

void sampleLight(vec2 seed, out vec3 pos, out vec3 normal, out uint hitLightIndex)
{

    // Step 1: Get all attributes
	uint idx = uint(random(seed) * float(pcRay.lightsNumber));

    mat4 model = lb.lights[idx].matrix;
    hitLightIndex = lb.lights[idx].index;
    ObjDesc    objResource = objDesc.i[hitLightIndex];
	Indices    indices     = Indices(objResource.indexAddress);
    Vertices   vertices    = Vertices(objResource.vertexAddress);

    uint triIdx = uint(8 + random(seed + 1) * 2);

	// Indices of the triangle
    ivec3 i = indices.i[triIdx];
  
    // Vertex of the triangle
    Vertex v0 = vertices.v[i.x];
    Vertex v1 = vertices.v[i.y];
    Vertex v2 = vertices.v[i.z];

    vec3 wp0 = (model * vec4(v0.pos, 1.0)).xyz;
    vec3 wp1 = (model * vec4(v1.pos, 1.0)).xyz;
    vec3 wp2 = (model * vec4(v2.pos, 1.0)).xyz;

    // Step 2: Generate a random point inside the triangle
    float r1 = random(seed.yx);
    float r2 = random(seed);
    if (r1 + r2 > 1.0) { r1 = 1.0 - r1; r2 = 1.0 - r2; }  // Keep inside triangle

    pos = wp0 * (1.0 - r1 - r2) + wp1 * r1 + wp2 * r2;
    vec3 nrm = v0.nrm * (1.0 - r1 - r2) + v1.nrm * r1 + v2.nrm * r2;

    // Step 3: Compute the normal at the sampled point
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    normal = normalize(normalMatrix * nrm);
}


void main()
{
    ObjDesc    objResource = objDesc.i[gl_InstanceCustomIndexEXT];
	Indices    indices     = Indices(objResource.indexAddress);
    Vertices   vertices    = Vertices(objResource.vertexAddress);
    vec3 albedo = objResource.albedo;
    float fuzz = objResource.fuzz;

	// Indices of the triangle
    ivec3 ind = indices.i[gl_PrimitiveID];
    
    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    vec2 seed = vec2(pcRay.time);

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    // Computing the coordinates of the hit position
    const vec3 pos      = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

    // Computing the normal at hit position
    const vec3 nrm      = v0.nrm * barycentrics.x + v1.nrm * barycentrics.y + v2.nrm * barycentrics.z;
    const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

    prd.worldHitPos = worldPos;

    // Lambertian
    if (objResource.materialType == 0)
    {
//        // Compute cosine-weighted sampling PDF
        vec3 cosineSample = normalize(randomCosineDirection(worldNrm, (worldNrm + worldPos).xy));

        vec3 lightPos;
        vec3 lightNrm;
        uint idx;

        if (pcRay.lightsNumber > 0)
        {
            // Sample a light source
            sampleLight((worldNrm + worldPos).xy, lightPos, lightNrm, idx);
        }
        else
        {
            // If no lights are present, use a default position and normal
            prd.nextDirection = cosineSample;
            prd.hitValue *= albedo;
            return;
        }
        sampleLight((worldNrm + worldPos).yx, lightPos, lightNrm, idx);
        vec3 lightSampleDir = lightPos - worldPos;
        float light_dist_squared = dot(lightSampleDir, lightSampleDir);
        lightSampleDir = normalize(lightSampleDir);

        float p = 0.5;
        if (random((worldNrm + worldPos).xz) < p)
        {
            prd.nextDirection = cosineSample;
        }
		else
        {
			prd.nextDirection = lightSampleDir;
        }

        float pdf_cos = scattering(worldNrm, prd.nextDirection);

        // Compute light sampling PDF
        float light_cosine = abs(dot(lightNrm, prd.nextDirection));
    
        float lightArea = objDesc.i[idx].fuzz;
        float pdf_light = light_dist_squared / (lightArea * light_cosine);

        // Multiple Importance Sampling (MIS)
        prd.hitValue *= pdf_cos;
        prd.hitValue /= (p * pdf_cos + (1.0-p) * pdf_light);

    }

    // Metal
    if (objResource.materialType == 1)
	{
        vec3 randomDir = 2 * normalize(random3D((worldNrm + worldPos).xy)) - 1;
		prd.nextDirection = reflect(gl_WorldRayDirectionEXT, worldNrm) + fuzz * randomDir;
	}

    // Dielectric
    if (objResource.materialType == 2)
	{
//        bool frontFace = dot(gl_WorldRayDirectionEXT, worldNrm) < 0.0;
        bool frontFace = (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT);
        // Flip the normal if it's a back face hit
        vec3 correctedNormal = frontFace ? worldNrm : -worldNrm;

        // Use appropriate indices of refraction depending on whether we are entering or exiting the material
        float refractionRatio = frontFace ? (1.0 / fuzz) : fuzz;

        double cosTheta = min(dot(-gl_WorldRayDirectionEXT, correctedNormal), 1.0);
        double sinTheta = sqrt(1.0 - cosTheta * cosTheta);

        bool cannotRefract = refractionRatio * sinTheta > 1.0;
        // Calculate the refracted direction
        vec3 randomDir = 2 * normalize(random3D((worldNrm + worldPos).xy)) - 1;
        if (cannotRefract || reflectance(cosTheta, refractionRatio) > random(randomDir.xy))
		{
			prd.nextDirection = reflect(gl_WorldRayDirectionEXT, correctedNormal);
		}
		else
		{
			prd.nextDirection = refract(gl_WorldRayDirectionEXT, correctedNormal, refractionRatio);
		}
	}

    // Difuse
    if (objResource.materialType == 3)
    {
        prd.miss = true;
    }

    prd.hitValue *= albedo;
}