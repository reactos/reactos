// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
// File: HorizontalBlur.fx
//
// Desc: Effect file for image post-processing sample.  This effect contains
//       a single technique with a pixel shader that blurs an image
//       using a weighted sampling kernel.
//

//-----------------------------------------------------------------------------

// Size is the height of the source texture for vertical shaders, and the width
// for horizontal shaders.
float size : register(C0);
float sampleStart : register(c1);
float numSamples : register(c2);
float4x4 weights : register(c3);

sampler2D g_samSrcColor : register(s0);

static const float maxSamples = 15; // max samples in ps_2_0 is 16, but we can do 15 at
                                    // most to stay under the instruction limit

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos  : POSITION;
    float4 Color: COLOR0;
    float2 TexCoord : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 position : POSITION, float4 color : COLOR0, float2 texcoord : TEXCOORD0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
#pragma warning(disable:3206)
    output.Pos.x = position;
#pragma warning(default)
    output.Color = color;
    output.TexCoord = texcoord;
    return output;
}

//-----------------------------------------------------------------------------
// Pixel Shader: HorizontalBlur
// Desc: Blurs the image horizontally
//-----------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : COLOR0
{
    float2 Tex = input.TexCoord;
    float4 Color = 0;
    float2 sampleTex;
    sampleTex.y = Tex.y;
    for (int i = 0; i < maxSamples; i++)
    {
	sampleTex.x = Tex.x + ((sampleStart + i) / size);
        Color += (tex2D( g_samSrcColor, sampleTex ) * weights[i%4][i/4]); // matrices are column-major by default
    }
    
    return Color;
}


