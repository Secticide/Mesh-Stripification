
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

// Ensure matrices are row-major
#pragma pack_matrix(row_major)

// Model a given surface point from the fragment shader to pass to lighting calculation functions
struct SurfacePoint {

	float3				pos;
	float2				texCoord;
	float3				normal;
	float4				matDiffuse;
	float4				matSpecular;
};

//
// constant buffer declarations
//

cbuffer camera : register(b0) {

	float4x4			viewProjMatrix;
	float3				eyePos;
};

//
// Textures and samplers
//

Texture2D				myTexture : register(t0);
SamplerState			linearSampler : register(s0);

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------

// input fragment - this is the per-fragment packet interpolated by the rasteriser stage.  SV_POSITION is consumed by the pipeline and not input here
struct fragmentInputPacket {

	float3				posW		: POSITION; // vertex in world coords
	float3				normalW		: NORMAL; // normal in world coords
	float4				matDiffuse	: DIFFUSE;
	float4				matSpecular	: SPECULAR;
	float2				texCoord	: TEXCOORD;
	float4				posH		: SV_POSITION;
};

struct fragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};

//--------------------------------------------------------------------------------------
// Fragment Shader
//--------------------------------------------------------------------------------------
fragmentOutputPacket pixelShader(fragmentInputPacket inputFragment) {

	fragmentOutputPacket outputFragment;

	float3 N = normalize(inputFragment.normalW);
	SurfacePoint v = {inputFragment.posW, inputFragment.texCoord, N, inputFragment.matDiffuse, inputFragment.matSpecular};
	
	float3 color = myTexture.Sample(linearSampler, v.texCoord).rgb;
	
	outputFragment.fragmentColour = float4(color, 1.0);	

	return outputFragment;
}
