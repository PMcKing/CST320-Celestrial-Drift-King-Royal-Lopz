//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register( t0 );
Texture2D txDepth : register(t1);
SamplerState samLinear : register( s0 );

cbuffer ConstantBuffer : register( b0 )
{
matrix World;
matrix View;
matrix Projection;
matrix LightView;
float4 info;
float4 CameraPos;
};



//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
	float4 Norm : NORMAL0;
	float4 OPos : POSITION;
	float4 WorldPos : POSITION1;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT VS(VS_INPUT input)
	{
	PS_INPUT output = (PS_INPUT)0;
	output.OPos = input.Pos;
	output.WorldPos = mul(input.Pos, World);
	output.Pos = mul(output.WorldPos, View);
	output.Pos = mul(output.Pos, Projection);


	output.OPos = mul(output.WorldPos, View);
	output.OPos = mul(output.OPos, Projection);


	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm = normalize( mul(input.Norm, World));
	float z = output.Pos.z;
	float w = output.Pos.w;
	//output.OPos = float4(z,w,z/w,1);
	//output.OPos = output.Pos;
	//output.OPos.w = 0.5;
	output.Pos = output.OPos;

	return output;
	}
PS_INPUT VS_screen(VS_INPUT input)
	{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = input.Pos;	
	output.Pos = pos;
	output.Tex = input.Tex;
	//lighing:
	//also turn the light normals in case of a rotation:
	output.Norm.xyz =input.Norm;
	output.Norm.w = 1;




	return output;
	}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSdepth(PS_INPUT input) : SV_Target
	{
	float4 texx;
	float4 pos = input.OPos;
	float depth = pos.z / pos.w;
	texx = float4(pos.z, pos.w, depth, 1);

	//texx = float4(pos.x, pos.y, pos.z, 1);
	//texx = float4(1, 1, 0, 1);
	return texx;
	}


float4 PS( PS_INPUT input) : SV_Target
{
//calculating shadows:

float4 pos = input.Pos;
float4 Opos = input.OPos;
float pixeldepth = 0;



float4 wpos = input.WorldPos;
//input.OPos.w = 1;

//wpos = mul(wpos, World);
wpos = mul(wpos, LightView);
wpos = mul(wpos, Projection);

float shadowlight = 1.0;//1 .. no shadow


pixeldepth = pos.z / pos.w;
pixeldepth = input.OPos.x/ input.OPos.y;
pixeldepth = wpos.z / wpos.w;
//pixeldepth = pos.z / pos.w;
//return float4(pixeldepth, pixeldepth, pixeldepth, 1);


//this chunck of code determines the view pixelss
float2 texdpos = wpos.xy / wpos.w;
texdpos.x = texdpos.x*0.5 + 0.5;
texdpos.y = texdpos.y* (-0.5) + 0.5;

float4 depth = txDepth.SampleLevel(samLinear, texdpos,3);
float d = depth.x / depth.y;
if (pixeldepth > (d + 0.000001))
	shadowlight = 0.5f;

float4 texture_color = txDiffuse.Sample(samLinear, input.Tex);
float4 color = texture_color;

float3 LightPosition = float3(30, 100, 0);
float3 lightDir = normalize(input.WorldPos - LightPosition);

// Note: Non-uniform scaling not supported
float diffuseLighting = saturate(dot(input.Norm, -lightDir)); // per pixel diffuse lighting
float LightDistanceSquared = 15000;
															// Introduce fall-off of light intensity
diffuseLighting *= (LightDistanceSquared / dot(LightPosition - input.WorldPos, LightPosition - input.WorldPos));

// Using Blinn half angle modification for perofrmance over correctness
float3 h = normalize(normalize(-CameraPos.xyz - input.WorldPos) - lightDir);
float SpecularPower = 15;
float specLighting = pow(saturate(dot(h, input.Norm)), SpecularPower);
float3 AmbientLightColor = float3(1, 1, 1)*0.01;
float3 SpecularColor = float3(1, 1, 1);
	color = (saturate(
	//AmbientLightColor +
	(texture_color *  diffuseLighting * 0.6) + // Use light diffuse vector as intensity multiplier
	(SpecularColor * specLighting * 0.5) // Use light specular vector as intensity multiplier
	), 1);
	//color.rgb = diffuseLighting;
	color.rgb = texture_color * diffuseLighting + specLighting;
	color.rgb *= shadowlight;
return color;
}
//********************
float2 PixelOffsets[9] =
	{
		{ -0.004, -0.004 },
		{ -0.003, -0.003 },
		{ -0.002, -0.002 },
		{ -0.001, -0.001 },
		{ 0.000, 0.000 },
		{ 0.001, 0.001 },
		{ 0.002, 0.002 },
		{ 0.003, 0.003 },
		{ 0.004, 0.004 },
	};

static const float BlurWeights[9] =
	{
	0.026995,
	0.064759,
	0.120985,
	0.176033,
	0.199471,
	0.176033,
	0.120985,
	0.064759,
	0.026995,
	};

float4 PS_screen(PS_INPUT input) : SV_Target
	{
	float4 texx = txDiffuse.SampleLevel(samLinear, input.Tex , 0);
	return float4(texx.rgb, 1);

	/*float4 bloom = float4(0,0,0,0);
	float span = 7;
	//float span = 6 + 5 * sin(5*g_time.x);
	int tt = abs(span);
	for (float i = -tt; i <= tt; i=i+0.3)
		{
		float ofs = i;
		bloom += txDiffuse.Sample(samLinear, input.Tex + float2(0,ofs / 100)) / (2.0 * tt + 1.0);
		}
	bloom *= bloom;
	bloom = bloom*0.7;
	//return bloom;
	float4 result= txDiffuse.Sample(samLinear, input.Tex) + bloom;
	result.a = 1;
	return result;
	*/
	


	float4 glow = txDiffuse.SampleLevel(samLinear, input.Tex,5);
	
	float4 glowsum = float4(0, 0, 0, 0);
	float t = 0.002*2;
	for (int xx = -10; xx < 10;xx++)
		for (int yy = -10; yy < 10; yy++)
			{
			float g = txDiffuse.SampleLevel(samLinear, input.Tex + float2(t*xx,t*yy), 1).r;
			
			g = saturate(g - 0.5)*0.4;
			//float distance = sqrt(xx*xx + yy*yy);
			float distance = xx*xx + yy*yy;
			g = g *(196 - distance) / 196.;
			g = pow(g, 4)*25;
			glowsum += g ;

			}
	glowsum.a = 1;
	glowsum = saturate(glowsum);
	//return glowsum;
	float4 tex = txDiffuse.SampleLevel(samLinear, input.Tex, 0);

	tex += glowsum/2;
	tex.a = 1;
	return tex;


	float4 texture_color;
	float4 glow_color = float4(0, 0, 0, 0);
	t = 0.02;
	glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, 0), 4);
	glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(t, 0), 4);
	glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, t), 4);
	glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(-t, 0), 4);
	glow_color += txDiffuse.SampleLevel(samLinear, input.Tex + float2(0, -t), 4);
	glow_color /= 4.;
	
	
	glow_color = saturate(glow_color*2 - 0.7);
	texture_color.a = 1;
	texture_color.rgb += glow_color.rgb;
	return texture_color;



	for (int i = 0; i < 5; i++)
		{
		float3 col = txDiffuse.Sample(samLinear, input.Tex + float2(0.01,0)*i);
		texture_color.rgb += col * (5-i) * 0.2;
		}
	return texture_color;
	}
