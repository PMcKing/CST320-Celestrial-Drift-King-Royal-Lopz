//--------------------------------------------------------------------------------------
// File: lecture 8.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
cbuffer ConstantBuffer : register(b0)
	{
	matrix World;
	matrix View;
	matrix Projection;
	float4 animation;
	};



//--------------------------------------------------------------------------------------
struct VS_INPUT
	{
	float4 Pos : POSITION;
	float2 Tex : TEXCOORD0;
	};

struct PS_INPUT
	{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT VS(VS_INPUT input)
	{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = mul(input.Pos, World);
	output.Pos = mul(pos, View);
	output.Pos = mul(output.Pos, Projection);

	output.Tex = input.Tex;
	
	return output;
	}

//**************************************************************************************
float4 PS(PS_INPUT input) : SV_Target
	{
	float2 texcoord = input.Tex;
	texcoord.x = input.Tex.x / animation.x;
	texcoord.y = input.Tex.y / animation.y;
	float onex = 1. / animation.x;
	float oney = 1. / animation.y;
	texcoord.x += onex * animation.z;
	texcoord.y += oney * animation.w;
	float4 tex = txDiffuse.SampleLevel(samLinear, texcoord,0);
	
	//tex.a = 1;
	return tex;
	}