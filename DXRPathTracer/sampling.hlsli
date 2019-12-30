static const float Pi = 3.141592654f;
static const float Pi2 = 6.283185307f;
static const float Pi_2 = 1.570796327f;
static const float Pi_4 = 0.7853981635f;
static const float InvPi = 0.318309886f;
static const float InvPi2 = 0.159154943f;


uint getNewSeed(uint param1, uint param2, uint numPermutation)
{
	uint s0 = 0;
	uint v0 = param1;
	uint v1 = param2;
	
	for(uint perm = 0; perm < numPermutation; perm++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1<<4) + 0xa341316c) ^ (v1+s0) ^ ((v1>>5) + 0xc8013ea4);
		v1 += ((v0<<4) + 0xad90777d) ^ (v0+s0) ^ ((v0>>5) + 0x7e95761e);
	}

	return v0;
}

float rnd(inout uint seed)
{
	seed = (1664525u * seed + 1013904223u);
	return ((float) (seed & 0x00FFFFFF) / (float) 0x01000000);
}

float3 applyRotationMappingZToN(in float3 N, in float3 v)	// --> https://math.stackexchange.com/questions/180418/calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d
{
	float  s = (N.z >= 0.0f) ? 1.0f : -1.0f;
	v.z *= s;

	float3 h = float3(N.x, N.y, N.z + s);
	float  k = dot(v, h) / (1.0f + abs(N.z));

	return k * h - v;
}

float3 sample_hemisphere_cos(inout uint seed)
{
	float3 sampleDir;

	float param1 = rnd(seed);
	float param2 = rnd(seed);

	// Uniformly sample disk.
	float r   = sqrt( param1 );
	float phi = 2.0f * Pi * param2;
	sampleDir.x = r * cos( phi );
	sampleDir.y = r * sin( phi );

	// Project up to hemisphere.
	sampleDir.z = sqrt( max(0.0f, 1.0f - r*r) );

	return sampleDir;
}

float TrowbridgeReitz(in float cos2, in float alpha2)
{
	float x = alpha2 + (1-cos2)/cos2;
	return alpha2 / (Pi*cos2*cos2*x*x);
}

float3 sample_hemisphere_TrowbridgeReitzCos(in float alpha2, inout uint seed)
{
	float3 sampleDir;

	float u = rnd(seed);
	float v = rnd(seed);

	float tan2theta = alpha2 * (u / (1-u));
	float cos2theta = 1 / (1 + tan2theta);
	float sinTheta = sqrt(1 - cos2theta);
	float phi = 2 * Pi * v;

	sampleDir.x = sinTheta * cos(phi);
	sampleDir.y = sinTheta * sin(phi);
	sampleDir.z = sqrt(cos2theta);

	return sampleDir;
}

float Smith_TrowbridgeReitz(in float3 wi, in float3 wo, in float3 wm,  in float3 wn, in float alpha2)
{
	if(dot(wo, wm) < 0 || dot(wi, wm) < 0)
		return 0.0f;

	float cos2 = dot(wn, wo);
	cos2 *= cos2;
	float lambda1 = 0.5 * ( -1 + sqrt(1 + alpha2*(1-cos2)/cos2) );
	cos2 = dot(wn, wi);
	cos2 *= cos2;
	float lambda2 = 0.5 * ( -1 + sqrt(1 + alpha2*(1-cos2)/cos2) );
	return 1 / (1 + lambda1 + lambda2);
}

