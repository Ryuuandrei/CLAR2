
struct hitPayload
{
	vec3 hitValue;
	vec3 nextDirection;
    vec3 worldHitPos;
	bool miss;
};

int MAX_DEPTH = 7;

float random(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 random3D(vec2 st) {
    return vec3(random(st), random(st + vec2(1.0, 1.0)), random(st + vec2(2.0, 2.0)));
}

vec3 randomUnitVec3(vec2 seed) {
    int i = 0;
    while (true) {
        vec3 p = 2 * random3D(seed + i) - 1;
        float norm = dot(p, p);
        if (1.0e-160 < norm && norm <= 1.0) return p / sqrt(norm);
    }
}