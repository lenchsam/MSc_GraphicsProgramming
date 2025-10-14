//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 vOutputColor;
}

cbuffer ConstantBufferPBR : register(b2)
{
    float metallicness;		//4 bytes
    float rough;			//4 bytes
    int IBLType;			//4 bytes
    //float3 AlbedoColour;	//12 bytes
	
    float Padding; //4 bytes
}

Texture2D albedoMap : register(t0);
SamplerState samLinear : register(s0);

static const float PI = 3.14159265f;

#define MAX_LIGHTS 1
// Light types.
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct Light
{
	float4      Position;               // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4      Direction;              // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4      Color;                  // 16 bytes
										//----------------------------------- (16 byte boundary)
	float       SpotAngle;              // 4 bytes
	float       ConstantAttenuation;    // 4 bytes
	float       LinearAttenuation;      // 4 bytes
	float       QuadraticAttenuation;   // 4 bytes
										//----------------------------------- (16 byte boundary)
	int         LightType;              // 4 bytes
	bool        Enabled;                // 4 bytes
	int2        Padding;                // 8 bytes
										//----------------------------------- (16 byte boundary)
};  // Total:                           // 80 bytes (5 * 16)

cbuffer LightProperties : register(b1)
{
	float4 EyePosition;                 // 16 bytes
										//----------------------------------- (16 byte boundary)
	float4 GlobalAmbient;               // 16 bytes
										//----------------------------------- (16 byte boundary)
	Light Lights[MAX_LIGHTS];           // 80 * 8 = 640 bytes
}; 


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float4 Tangent : TANGENT;
    float2 Tex : TEXCOORD0;
    uint4 Joints : BLENDINDICES0;
    float4 Weights : BLENDWEIGHT0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	float4 worldPos : POSITION;
	float3 Norm : NORMAL;
	float2 Tex : TEXCOORD0;
};


float4 DoDiffuse(Light light, float3 L, float3 N)
{
	float NdotL = max(0, dot(N, L));
	return light.Color * NdotL;
}

float4 DoSpecular(Light lightObject, float3 pixelToEyeVectorNormalised, float3 pixelToLightVectorNormalised, float3 Normal)
{
    float lightIntensity = saturate(dot(Normal, pixelToLightVectorNormalised));
	float4 specular = float4(0, 0, 0, 0);
	if (lightIntensity > 0.0f)
	{
		// note the reflection equation requires the *light to pixel* vector - hence we reverse it
        float3 reflection = reflect(-pixelToLightVectorNormalised, Normal);
        specular = pow(saturate(dot(reflection, pixelToEyeVectorNormalised)), 4); // 32 = specular power Material.SpecularPower
    }

	return specular;
}

float DoAttenuation(Light light, float d)
{
	return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}

struct LightingResult
{
	float4 Diffuse;
	float4 Specular;
};

LightingResult DoPointLight(Light light, float3 pixelToLightVectorNormalised, float3 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N)
{
	LightingResult result;

    float attenuation = DoAttenuation(light, distanceFromPixelToLight);
    attenuation = 1;

    result.Diffuse = DoDiffuse(light, pixelToLightVectorNormalised, N) * attenuation;
    result.Specular = DoSpecular(light, pixelToEyeVectorNormalised, pixelToLightVectorNormalised, N) * attenuation; 

	return result;
}

LightingResult ComputeLighting(float4 pixelToLightVectorNormalised, float4 pixelToEyeVectorNormalised, float distanceFromPixelToLight, float3 N)
{
	LightingResult totalResult = { { 0, 0, 0, 0 },{ 0, 0, 0, 0 } };

	[unroll]
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		LightingResult result = { { 0, 0, 0, 0 },{ 0, 0, 0, 0 } };

		if (!Lights[i].Enabled) 
			continue;
		
        result = DoPointLight(Lights[i], pixelToLightVectorNormalised.xyz, pixelToEyeVectorNormalised.xyz, distanceFromPixelToLight, N);
		
		totalResult.Diffuse += result.Diffuse;
		totalResult.Specular += result.Specular;
	}

	totalResult.Diffuse = saturate(totalResult.Diffuse);
	totalResult.Specular = saturate(totalResult.Specular);

	return totalResult;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
	output.worldPos = output.Pos;
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );

	// multiply the normal by the world transform (to go from model space to world space)
	output.Norm = mul(float4(input.Norm, 0), World).xyz;

	output.Tex = input.Tex;
    
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

float NormalDistribution(float roughness, float3 N, float3 H)
{
    float roughnessSqr = pow(roughness, 2);
    float NdotHsqr = pow(dot(N, H), 2);
    return roughnessSqr / (PI * pow((NdotHsqr * (roughnessSqr - 1) + 1), 2));
}

float G_Sub(float NdotV, float k)
{
    return NdotV / (NdotV * (1 - k) + k);
}

float Geometry(float3 N, float3 V, float3 L, float roughness)
{
    float k = pow(roughness + 1, 2) / 8.0;
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return G_Sub(NdotV, k) * G_Sub(NdotL, k);

}

float4 PS_PBR(PS_INPUT IN) : SV_TARGET
{	
	//-------------------------------------------------------------- Part A PBR Lighting
    float3 albedo = float3(1.0, 0.78, 0.34);
    float metallic = metallicness;
    float roughness = rough;
	
    float3 N = IN.Norm;
	
    float3 V = normalize(EyePosition - IN.worldPos);
    float3 L = normalize(Lights[0].Position - IN.worldPos); //Light Vector
    float3 H = normalize(V+L);
	
    float cosTheta = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
	
	//calculate fresnel
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
	
	//Fresnel-Schlick Approximation
    float3 F = FresnelSchlick(F0, cosTheta);

	//specular
    float D = NormalDistribution(roughness, N, H);
	
	// test
    //return float4(D, D, D, 1.0);
	
    float G = Geometry(N, V, L, roughness);
	
	// test
    //return float4(G, G, G, 1.0);
	
    float3 numerator = float3(D * G * F);
    float denom = float(4 * NdotL * cosTheta) + 0.001f;
	
    float3 SpecularBRDF = numerator / denom; //final specular BRDF
	
	// test
    //return float4(SpecularBRDF, 1.0);
	
	//diffuse lighting
	
    float3 kD = float3(1.0, 1.0, 1.0) - F;
    kD *= (1.0 - metallic); //fade out diffuse for metals

    float3 diffuse = kD * (albedo / PI);
	
    float3 LightOutgoing = (diffuse + SpecularBRDF) * NdotL;

	//-------------------------------------------------------------- Part B Environment Lighting
	//IBL = Image Based Lighting
    float3 finalIBL = float3(0, 0, 0);

    int typeIBL = IBLType;

    if (typeIBL == 0)
    {
        float3 ambientColour = float3(0.1, 0.1, 0.1);
        finalIBL = ambientColour * albedo * kD;
    }
    else if (typeIBL == 1)
    {

    }
    else if (typeIBL == 2)
    {

    }
	
    float3 colour = finalIBL + LightOutgoing;

    return float4(colour, 1.0);

}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PS_Normal(PS_INPUT IN) : SV_TARGET
{
    float4 pixelToLightVectorNormalised = normalize(Lights[0].Position - IN.worldPos);
    float4 pixelToEyeVectorNormalised = normalize(EyePosition - IN.worldPos);
    float distanceFromPixelToLight = length(IN.worldPos - Lights[0].Position);
	
    LightingResult lit = ComputeLighting(pixelToLightVectorNormalised, pixelToEyeVectorNormalised, distanceFromPixelToLight, normalize(IN.Norm));

	// NOTE we aren't using any material properties just yet. 
    float4 ambient = GlobalAmbient;  
	float4 diffuse = lit.Diffuse;
	float4 specular = lit.Specular;

    float4 texColor;
	texColor = albedoMap.Sample(samLinear, IN.Tex);
	
    float4 diffuseColor = (ambient + diffuse + specular) * texColor;
	
	return diffuseColor;
}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
	return vOutputColor;
}
