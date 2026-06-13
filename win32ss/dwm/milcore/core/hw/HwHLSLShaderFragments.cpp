// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Shader Functions
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//
// Null Vertex Shader Function
// 
// We can use this whenever there is no work in the vertex shader
// 
const VertexShaderFunction g_NullVertexShaderFunction
(
    NULL,
    0,
    0,
    NULL,
    0,
    NULL,
    false
);

//
// Null Pixel Shader Function
// 
// We can use this whenever there is no work in the pixel shader
// 
const PixelShaderFunction g_NullPixelShaderFunction
(
    NULL,
    0,
    0,
    NULL,
    0,
    NULL
);


#define DEFINE_SHADER_FUNCTION(name)  \
namespace name \
{
#define BODY \
    static const char sc_szBody[] 

// Vertex Shader specific macros
#define DEFINE_VERTEX_SHADER_FUNCTION(name) DEFINE_SHADER_FUNCTION(name)
#define VS_BODY BODY
#define VS_INPUTS \
    static const VertexFunctionParameter::Enum sc_rg_VS_Inputs[] 
#define VS_DATA \
    static const FunctionConstDataParameter sc_rg_VS_Data[] 
#define VS_LOOPABLE \
    static const bool sc_fLoopable
#define VS_END \
    static const VertexShaderFunction VS                  \
    (                                               \
        sc_szBody, sizeof(sc_szBody),       \
        ARRAYSIZE(sc_rg_VS_Inputs), sc_rg_VS_Inputs,  \
        __if_exists(sc_rg_VS_Data) { \
        ARRAYSIZE(sc_rg_VS_Data)  , sc_rg_VS_Data,     \
        }\
        __if_not_exists(sc_rg_VS_Data) { \
        0, NULL,     \
        }\
        __if_exists(sc_fLoopable) { \
        sc_fLoopable \
        }\
        __if_not_exists(sc_fLoopable) { \
        false \
        }\
    );                                              \
}


// Pixel Shader specific macros
#define DEFINE_PIXEL_SHADER_FUNCTION(name) DEFINE_SHADER_FUNCTION(name)
#define PS_BODY BODY
#define PS_INPUTS \
    static const PixelFunctionParameter::Enum sc_rg_PS_Inputs[] 
#define PS_DATA \
    static const FunctionConstDataParameter sc_rg_PS_Data[] 
#define PS_END \
    static const PixelShaderFunction PS                  \
    (                                               \
        sc_szBody, sizeof(sc_szBody),       \
        ARRAYSIZE(sc_rg_PS_Inputs), sc_rg_PS_Inputs,  \
        __if_exists(sc_rg_PS_Data) { \
        ARRAYSIZE(sc_rg_PS_Data)  , sc_rg_PS_Data     \
        }\
        __if_not_exists(sc_rg_PS_Data) { \
        0, NULL     \
        }\
    );                                              \
}

//+----------------------------------------------------------------------------
//
// Transform World2D by Matrix4x4 Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Transform_World2D_By_Matrix4x4)

    VS_BODY =
        "(\n"
        "    float4 WorldPos2D,\n"
        "    Transform2D_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    Output.Diffuse = float4(1.0, 1.0, 1.0, 1.0);\n"
        "    Output.Position = mul(WorldPos2D, Data.mat4x4WorldToProjection);\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Position,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };
    
    VS_DATA =
        {
            {
                "mat4x4WorldToProjection",
                ShaderFunctionConstantData::Matrix4x4
            }
        };
    
VS_END;

//+----------------------------------------------------------------------------
//
// Multiply By Input Diffuse Premultiplied
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(MultiplyByInputDiffuse)

    VS_BODY =
        "(\n"
        "    float4 Diffuse,\n"
        "    inout float4 BlendColor\n"
        "    )\n"
        "{\n"
        "    BlendColor = Diffuse;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Diffuse,
            VertexFunctionParameter::Interpolator_TexCoord4
        };

        //VS_DATA = {};

VS_END;

DEFINE_PIXEL_SHADER_FUNCTION(MultiplyByInputDiffuse_Premultiplied)

    PS_BODY =
        "(\n"
        "    float4 BlendColor,\n"
        "    inout float4 curPixelColor\n"
        "    )\n"
        "{\n"
        "    curPixelColor *= BlendColor;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord4,
            PixelFunctionParameter::ShaderOutputStruct
        };
    
    //PS_DATA = {};

PS_END;

//+----------------------------------------------------------------------------
//
// AntiAlias 2D Non Premultiplied Input Pixel Function
// 
//+----------------------------------------------------------------------------

DEFINE_PIXEL_SHADER_FUNCTION(MultiplyByInputDiffuse_NonPremultiplied)

    PS_BODY =
        "(\n"
        "    float4 Diffuse,\n"
        "    inout float4 curPixelColor\n"
        "    )\n"
        "{\n"
        "    curPixelColor.a *= Diffuse.a;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord4,
            PixelFunctionParameter::ShaderOutputStruct
        };
    
    //PS_DATA = {};
    
PS_END;

//+----------------------------------------------------------------------------
//
// Multiply Texture Transform from Pos2D Fragment
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Transform_World2D_By_Matrix3x2_Into_TexCoord2)

    VS_BODY =
        "(\n"
        "    float4 World2DPos,\n"
        "    MultiplyTexture_TransformPos2D_VS_ConstData data,\n"
        "    inout float2 uv\n"
        "    )\n"
        "{\n"
        "    uv.x = World2DPos.x*data.mat3x2TextureTransform[0][0] + World2DPos.y*data.mat3x2TextureTransform[1][0] + data.mat3x2TextureTransform[2][0];\n"
        "    uv.y = World2DPos.x*data.mat3x2TextureTransform[0][1] + World2DPos.y*data.mat3x2TextureTransform[1][1] + data.mat3x2TextureTransform[2][1];\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Position,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::Interpolator_TexCoord2
        };
    
    VS_DATA = 
        {
            {
                "mat3x2TextureTransform",
                ShaderFunctionConstantData::Matrix3x2
            }
        };

VS_END;

//+----------------------------------------------------------------------------
//
// Multiply Texture Transform from UV1 Fragment
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Transform_InputUV_By_Matrix3x2_Into_TexCoord2)

    VS_BODY =
        "(\n"
        "    float2 inputUV,\n"
        "    MultiplyTexture_Transform_InputUV_VS_ConstData data,\n"
        "    inout float2 outputUV\n"
        "    )\n"
        "{\n"
        "    outputUV.x = inputUV.x*data.mat3x2TextureTransform[0][0] + inputUV.y*data.mat3x2TextureTransform[1][0] + data.mat3x2TextureTransform[2][0];\n"
        "    outputUV.y = inputUV.x*data.mat3x2TextureTransform[0][1] + inputUV.y*data.mat3x2TextureTransform[1][1] + data.mat3x2TextureTransform[2][1];\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::VertexUV2,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::Interpolator_TexCoord2
        };
    
    VS_DATA =
        {
            {
                "mat3x2TextureTransform",
                ShaderFunctionConstantData::Matrix3x2
            }
        };

VS_END;

//+----------------------------------------------------------------------------
//
// Multiply Texture Transform from UV1 Fragment
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(MultiplyAlphaMask_Transformed_From_InputUV)

    VS_BODY =
        "(\n"
        "    float2 inputUV,\n"
        "    MultiplyAlphaMask_Transform_InputUV_VS_ConstData data,\n"
        "    inout float2 outputUV\n"
        "    )\n"
        "{\n"
        "    outputUV.x = inputUV.x*data.mat3x2TextureTransform[0][0] + inputUV.y*data.mat3x2TextureTransform[1][0] + data.mat3x2TextureTransform[2][0];\n"
        "    outputUV.y = inputUV.x*data.mat3x2TextureTransform[0][1] + inputUV.y*data.mat3x2TextureTransform[1][1] + data.mat3x2TextureTransform[2][1];\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::VertexUV2,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::Interpolator_TexCoord2
        };
    
    VS_DATA =
        {
            {
                "mat3x2TextureTransform",
                ShaderFunctionConstantData::Matrix3x2
            }
        };

VS_END;

//+----------------------------------------------------------------------------
//
// Multiply Texture Pixel Shader Function
// 
//+----------------------------------------------------------------------------
DEFINE_PIXEL_SHADER_FUNCTION(MultiplyTexture)

    PS_BODY =
        "(\n"
        "   float2 uv,\n"
        "   sampler TextureSampler,\n"
        "   inout float4 curPixelColor\n"
        "   )\n"
        "{\n"
        "    float4 sampledColor = tex2D(TextureSampler, uv);\n"
        "\n"
        "    curPixelColor *= sampledColor;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord2,  
            PixelFunctionParameter::Sampler,                 
            PixelFunctionParameter::ShaderOutputStruct,      
        };

    //
    // No Data
    //
PS_END;  

//+----------------------------------------------------------------------------
//
// Pass Texture coordinate through the vertex shader
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Pass_InputVertex_UV2_ToTexCoord2)

    VS_BODY =
        "(\n"
        "    float2 inputUV,\n"
        "    inout float2 outputUV\n"
        "    )\n"
        "{\n"
        "    outputUV = inputUV;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::VertexUV2,
            VertexFunctionParameter::Interpolator_TexCoord2
        };

    //VS_DATA {}

VS_END;

//+----------------------------------------------------------------------------
//
// Multiply Constant color pixel shader
// 
//+----------------------------------------------------------------------------

DEFINE_PIXEL_SHADER_FUNCTION(MultiplyConstant)

    PS_BODY =
        "(\n"
        "    MultiplyConstant_PS_ConstData data,\n"
        "    inout float4 curPixelColor\n"
        "    )\n"
        "{\n"
        "    curPixelColor *= data.diffuse;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::FragmentConstData,
            PixelFunctionParameter::ShaderOutputStruct
        };
    
    PS_DATA =
        {
            {
                "diffuse",
                ShaderFunctionConstantData::Float4
            }
        };

PS_END;

//+----------------------------------------------------------------------------
//
// Multiply By Alpha Pixel Shader
// 
//+----------------------------------------------------------------------------

DEFINE_PIXEL_SHADER_FUNCTION(Multiply_By_Alpha_Premultiplied)

    PS_BODY =
        "(\n"
        "    MultiplyAlpha_PS_ConstData data,\n"
        "    inout float4 curPixelColor\n"
        "    )\n"
        "{\n"
        "    curPixelColor *= data.alpha.a;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::FragmentConstData,
            PixelFunctionParameter::ShaderOutputStruct      
        };
    
    PS_DATA =
        {
            {
                "alpha",
                ShaderFunctionConstantData::Float4
            }
        };

PS_END;

//+----------------------------------------------------------------------------
//
// Multiply By Alpha NonPremultiplied Pixel Shader
// 
//+----------------------------------------------------------------------------                               

DEFINE_PIXEL_SHADER_FUNCTION(Multiply_By_Alpha_NonPremultiplied)

    PS_BODY =
        "(\n"
        "    MultiplyAlpha_NonPremultiplied_PS_ConstData data,\n"
        "    inout float4 curPixelColor\n"
        "    )\n"
        "{\n"
        "    curPixelColor.a *= data.alpha.a;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::FragmentConstData,
            PixelFunctionParameter::ShaderOutputStruct      
        };
    
    PS_DATA =
        {
            {
                "alpha",
                ShaderFunctionConstantData::Float4
            }
        };

PS_END;

//+----------------------------------------------------------------------------
//
// Multiply By Alpha Mask Pixel Shader
// 
//+----------------------------------------------------------------------------

DEFINE_PIXEL_SHADER_FUNCTION(Multiply_By_Alpha_Mask_Premultiplied)

    PS_BODY =
        "(\n"
        "    in    float2  uv,\n"
        "    in    sampler TextureSampler,\n"
        "    inout float4  curPixelColor\n"
        "    )\n"
        "{\n"
        "    float4 sampledColor = tex2D(TextureSampler, uv);\n"
        "\n"
        "    curPixelColor *= sampledColor.a;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord2,  
            PixelFunctionParameter::Sampler, 
            PixelFunctionParameter::ShaderOutputStruct      
        };

PS_END;

//+----------------------------------------------------------------------------
//
// Multiply By Alpha Mask NonPremultiplied Pixel Shader
// 
//+----------------------------------------------------------------------------                               

DEFINE_PIXEL_SHADER_FUNCTION(Multiply_By_Alpha_Mask_NonPremultiplied)

    PS_BODY =
        "(\n"
        "    in    float2  uv,\n"
        "    in    sampler TextureSampler,\n"
        "    inout float4  curPixelColor\n"
        "    )\n"
        "{\n"
        "    float4 sampledColor = tex2D(TextureSampler, uv);\n"
        "\n"
        "    curPixelColor.a *= sampledColor.a;\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord2,  
            PixelFunctionParameter::Sampler, 
            PixelFunctionParameter::ShaderOutputStruct      
        };

PS_END;

//+----------------------------------------------------------------------------
//
// MultiplyRadialGradientCentered Pixel Function
// 
//+----------------------------------------------------------------------------
DEFINE_PIXEL_SHADER_FUNCTION(MultiplyRadialGradientCentered)

    PS_BODY =
        "(\n"
        "    float2 samplePos,\n"
        "    sampler TextureSampler,\n"
        "    MultiplyRadialGradientCentered_PS_ConstData GradInfoParams,\n"
        "    inout float4 color\n"
        "    )\n"
        "{\n"
        "    float4 sampleGradientColor;\n"
        "\n"
        "    // Get distance (in unit circle) from sample point to the gradient origin:\n"
        "    float uc_dx = samplePos.x;\n"
        "    float uc_dy = samplePos.y;\n"
        "\n"
        "    float distToOriginSqr = uc_dx*uc_dx + uc_dy*uc_dy;\n"
        "\n"
        "    // Simple radial gradient\n"
        "    float sampleGradientTexCoord = sqrt(distToOriginSqr);\n"
        "\n"
        "    // Ensure that the gradient space does not wrap around,\n"
        "    // interpolating with the last stop at the center point.\n"
        "    if (sampleGradientTexCoord < GradInfoParams.flHalfTexelSizeNormalized)\n"
        "    {\n"
        "        sampleGradientTexCoord = GradInfoParams.flHalfTexelSizeNormalized;\n"
        "    }\n"
        "\n"
        "    sampleGradientColor = tex1D(TextureSampler, sampleGradientTexCoord);\n"
        "\n"
        "    color *= sampleGradientColor;\n"
        "}\n";

    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord2,  
            PixelFunctionParameter::Sampler,                 
            PixelFunctionParameter::FragmentConstData,  
            PixelFunctionParameter::ShaderOutputStruct,      
        };

    PS_DATA =
        {
            {
                "flHalfTexelSizeNormalized",
                ShaderFunctionConstantData::Float
            },            
        };

PS_END;

//+----------------------------------------------------------------------------
//
// MultiplyRadialGradientNonCentered Pixel Function
// 
//+----------------------------------------------------------------------------
DEFINE_PIXEL_SHADER_FUNCTION(MultiplyRadialGradientNonCentered)

    PS_BODY =
        "(\n"
        "    float2 samplePos,\n"
        "    sampler TextureSampler,\n"
        "    MultiplyRadialGradientNonCentered_PS_ConstData GradInfoParams,\n"
        "    inout float4 color\n"
        "    )\n"
        "{\n"
        "    //\n"
        "    // There are overflow issues in refrast and hw implementation of clamping.\n"
        "    // Therefore we need to clamp ourselves in areas of the shader that have a\n"
        "    // high risk of overflowing.\n"
        "    //\n"
        "    // We will go with 32768 as the maximum number of wraps that's support in \n"
        "    // supported since that's what refrast has.\n"
        "    //\n"
        "    #define MAX_RELIABLE_WRAP_VALUE 32768\n"
        "\n"
        "    float u;\n"
        "    \n"
        "    float2 sampleToFirstTexelRegionCenter = samplePos - GradInfoParams.ptFirstTexelRegionCenter;\n"
        "    float firstTexelRegionRadiusSquared = GradInfoParams.flHalfTexelSizeNormalized * GradInfoParams.flHalfTexelSizeNormalized;\n"
        "    \n"
        "    if (dot(sampleToFirstTexelRegionCenter, sampleToFirstTexelRegionCenter) <\n"
        "        firstTexelRegionRadiusSquared)\n"
        "    {\n"
        "        u = GradInfoParams.flHalfTexelSizeNormalized;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        // Get distance (in unit circle) from sample point to the gradient origin:\n"
        "        float2 sampleToOrigin = samplePos - GradInfoParams.ptGradOrigin;\n"
        "    \n"
        "        float A = dot(sampleToOrigin, sampleToOrigin);\n"
        "        \n"
        "        float B = 2.0f * dot(GradInfoParams.ptGradOrigin, sampleToOrigin);\n"
        "\n"
        "        float2 ptGradOriginPerp = {GradInfoParams.ptGradOrigin.y, -GradInfoParams.ptGradOrigin.x};\n"
        "        float sampleToOriginCrossOriginNorm = dot(sampleToOrigin, ptGradOriginPerp);\n"
        "\n"
        "        // see brushspan.cpp for an explanation of why the determinant is calculated this way.\n"
        "        float determinant = \n"
        "            4.0f * (  GradInfoParams.gradientSpanNormalized * GradInfoParams.gradientSpanNormalized * A\n"
        "                    - sampleToOriginCrossOriginNorm * sampleToOriginCrossOriginNorm);\n"
        "        \n"
        "        if (0.0f > determinant)\n"
        "        {\n"
        "            // This complex region appears when the gradient origin is outside the\n"
        "            // ellipse defining the end of the gradient. When rendering this region\n"
        "            // we choose the last texel color.\n"
        "            u = 1.0f - GradInfoParams.flHalfTexelSizeNormalized;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            u = (2 * A * GradInfoParams.gradientSpanNormalized) / (sqrt(determinant) - B);\n"
        "            \n"
        "            if (u < GradInfoParams.flHalfTexelSizeNormalized)\n"
        "            {\n"
        "                if (u < 0.0)\n"
        "                {\n"
        "                    // This negative region appears when the gradient origin is outside the\n"
        "                    // ellipse defining the end of the gradient. When rendering this region\n"
        "                    // we choose the last texel color.\n"
        "                    u = 1.0f - GradInfoParams.flHalfTexelSizeNormalized;\n"
        "                }\n"
        "                else                                                         \n"
        "                {\n"
        "                    // Ensure that the gradient space does not wrap around,\n"
        "                    // interpolating with the last stop at the center point.\n"
        "                    // This value for u picks the first texel in the texture.\n"
        "                    \n"
        "                    // Given an infinite precicision machine, we'd never get to this case since\n"
        "                    // we should have skipped the quadratic equation up top. Nevertheless,\n"
        "                    // we do not have an infinite precision machine, so we may still get here.\n"
        "                    u = GradInfoParams.flHalfTexelSizeNormalized;\n"
        "                }\n"
        "            }   \n"
        "            else\n"
        "            {\n"
        "                //\n"
        "                // Refrast & probably hw implement wrapping/clamping logic by first casting\n"
        "                // the float to an integer and then doing integer math.  They are not robust\n"
        "                // against integer overflow, so we need to do the check manually.\n"
        "                //\n"
        "\n"
        "                if (u > MAX_RELIABLE_WRAP_VALUE)\n"
        "                {\n"
        "                    u = 1.0f;\n"
        "                }\n"
        "            }\n"
        "        }\n"
        "    }    \n"
        "\n"
        "    color *= tex1D(TextureSampler, u);\n"
        "}\n";
    
    PS_INPUTS =
        {
            PixelFunctionParameter::Interpolator_TexCoord2,  
            PixelFunctionParameter::Sampler,                 
            PixelFunctionParameter::FragmentConstData,  
            PixelFunctionParameter::ShaderOutputStruct,      
        };

    PS_DATA =
        {
            {
                "ptGradOrigin",
                ShaderFunctionConstantData::Float2
            },
            {
                "ptFirstTexelRegionCenter",
                ShaderFunctionConstantData::Float2
            },
            {
                "gradientSpanNormalized",
                ShaderFunctionConstantData::Float
            },
            {
                "flHalfTexelSizeNormalized",
                ShaderFunctionConstantData::Float
            },            
        };

PS_END;

//+----------------------------------------------------------------------------
//
// GetTransform3D Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Get3DTransforms)

    VS_BODY =
        "(\n"
        "    in  Get3DTransforms_VS_ConstData Data,\n"
        "    out float4x4 mat4x4WorldViewTransform,\n"
        "    out float4x4 mat4x4WorldViewProjTransform,\n"
        "    out float4x4 mat4x4WorldViewAdjTransTransform\n"
        "    )\n"
        "{\n"
        "    mat4x4WorldViewTransform         = Data.mat4x4WorldViewTransform;\n"
        "    mat4x4WorldViewProjTransform     = Data.mat4x4WorldViewProjTransform;\n"
        "    mat4x4WorldViewAdjTransTransform = Data.mat4x4WorldViewAdjTransTransform;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::WorldViewTransform,
            VertexFunctionParameter::WorldViewProjTransform,
            VertexFunctionParameter::WorldViewAdjTransTransform,
        };

    VS_DATA =
        {
            {
                "mat4x4WorldViewTransform",
                ShaderFunctionConstantData::Matrix4x4
            },
            {
                "mat4x4WorldViewProjTransform",
                ShaderFunctionConstantData::Matrix4x4
            },
            {
                "mat4x4WorldViewAdjTransTransform",
                ShaderFunctionConstantData::Matrix4x4
            }
        };
    
VS_END;

//+----------------------------------------------------------------------------
//
// Transform World3D Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Transform_World3D)

    VS_BODY =
        "(\n"
        "    in    float4x4 mat4x4WorldViewTransform,\n"
        "    in    float4x4 mat4x4WorldViewProjTransform,\n"
        "    in    float4x4 mat4x4WorldViewAdjTransTransform,\n"
        "    inout float4   Position,\n"
        "    inout float3   Normal,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    Normal          = normalize(mul(Normal, (float3x3)mat4x4WorldViewAdjTransTransform));\n"
        "    // NOTE: Dividing the output position by w here will completely break\n"
        "    //       textures. The card needs to interpolate different 1/w values.\n" 
        "    Output.Position = mul(Position, mat4x4WorldViewProjTransform);\n"
        "    Position        = mul(Position, mat4x4WorldViewTransform);\n"
        "    Position        /= Position.w;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::WorldViewTransform,
            VertexFunctionParameter::WorldViewProjTransform,
            VertexFunctionParameter::WorldViewAdjTransTransform,
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::ShaderOutputStruct,
        };
    
VS_END;

//+----------------------------------------------------------------------------
//
// Ambient Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Ambient_Lighting)

    VS_BODY =
        "(\n"
        "    CalcAmbientLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    Output.Diffuse = Data.Color;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
        }; 
    
VS_END;

//+----------------------------------------------------------------------------
//
// Flip Normal Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Flip_Normal)

    VS_BODY =
        "(\n"
        "    inout float3 TransformedNormal\n"
        "    )\n"
        "{\n"
        "    TransformedNormal *= -1.0;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Normal,
        };
    
VS_END;

//+----------------------------------------------------------------------------
//
// Diffuse Directional Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Diffuse_Directional_Lighting)

    VS_BODY =
        "(\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcDiffuseDirectionalLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    Output.Diffuse.rgb += Data.Color.rgb * max(dot(TransformedNormal, Data.Direction), 0);\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Direction",
                ShaderFunctionConstantData::Float3
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// Diffuse Point Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Diffuse_Point_Lighting)

    VS_BODY =
        "(\n"
        "    in    float4   TransformedPosition,\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcDiffusePointLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    float3 VecToLight = Data.Position - TransformedPosition;\n"
        "    float DistToLight = length(VecToLight);\n"
        "    // normalize L\n"
        "    VecToLight /= DistToLight;\n"
        "\n"
        "    // the max is to ensure that the attenuation only diminishes the light\n" 
        "    float atten = 1.0 / max(Data.AttenAndRange.x\n"
        "                            + Data.AttenAndRange.y * DistToLight\n"
        "                            + Data.AttenAndRange.z * DistToLight * DistToLight,\n" 
        "                            1.0);\n"
        "\n"
        "    // AttenAndRange.w is the light's range\n"
        "    Output.Diffuse.rgb += Data.Color.rgb\n" 
        "                          * max(dot(TransformedNormal, VecToLight), 0)\n"
        "                          * atten\n"
        "                          * step(DistToLight, Data.AttenAndRange.w);\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Position",
                ShaderFunctionConstantData::Float4
            },
            {
                "AttenAndRange",
                ShaderFunctionConstantData::Float4
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// Diffuse Spot Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Diffuse_Spot_Lighting)

    VS_BODY =
        "(\n"
        "    in    float4   TransformedPosition,\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcDiffuseSpotLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    float3 VecToLight = Data.Position - TransformedPosition;\n"
        "    float DistToLight = length(VecToLight);\n"
        "    // normalize L\n"
        "    VecToLight /= DistToLight;\n"
        "\n"
        "    // the max is to ensure that the attenuation only diminishes the light\n" 
        "    float atten = 1.0 / max(Data.AttenAndRange.x\n"
        "                            + Data.AttenAndRange.y * DistToLight\n"
        "                            + Data.AttenAndRange.z * DistToLight * DistToLight,\n" 
        "                            1.0);\n"
        "\n"
        "    float rho = max(dot(Data.Direction, VecToLight), 0);\n"
        "    // CosHalfPhiAndCosDiff.x = cos(Phi/2)\n"
        "    // CosHalfPhiAndCosDiff.y = cos(Theta/2) - cos(Phi/2)\n"
        "    float spot = saturate((rho - Data.CosHalfPhiAndCosDiff.x) / Data.CosHalfPhiAndCosDiff.y);\n"
        "\n"
        "    // AttenAndRange.w is the light's range\n"
        "    Output.Diffuse.rgb += Data.Color.rgb\n" 
        "                          * max(dot(TransformedNormal, VecToLight), 0)\n"
        "                          * atten\n"
        "                          * spot\n"
        "                          * step(DistToLight, Data.AttenAndRange.w);\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Position",
                ShaderFunctionConstantData::Float4
            },
            {
                "AttenAndRange",
                ShaderFunctionConstantData::Float4
            },
            {
                "Direction",
                ShaderFunctionConstantData::Float3
            },
            {
                "CosHalfPhiAndCosDiff",
                ShaderFunctionConstantData::Float4
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// GetSpecularPower Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(GetSpecularPower)

    VS_BODY =
        "(\n"
        "    in    GetSpecularPower_VS_ConstData Data,\n"
        "    out   float SpecularPower,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    SpecularPower = Data.SpecularPower.x;\n"
        "    // Initialize the output alpha because the specular\n"
        "    // lighting functions ignore it.\n"
        "    Output.Diffuse.a = 0.0;\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::SpecularPower,
            VertexFunctionParameter::ShaderOutputStruct
        };

    VS_DATA =
        {
            {
                "SpecularPower",
                ShaderFunctionConstantData::Float4
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// Specular Directional Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Specular_Directional_Lighting)

    VS_BODY =
        "(\n"
        "    in    float    SpecularPower,\n"
        "    in    float4   TransformedPosition,\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcSpecularDirectionalLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    // Note: This does not actually generate a branch. The compiler translates this into\n"
        "    //       an instruction (slt) that returns 0 or 1 and multiplies that times the output color\n"
        "    if (dot(Data.Direction, TransformedNormal) > 0)\n"
        "    {\n"
        "        // in WorldView space, the camera is at <0> so just invert the position\n"
        "        float3 HalfVector = normalize(normalize(-TransformedPosition.xyz) + Data.Direction);\n"
        "        Output.Diffuse.rgb += Data.Color.rgb * pow(max(dot(HalfVector, TransformedNormal), 0), SpecularPower);\n"
        "    }\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::SpecularPower,
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Direction",
                ShaderFunctionConstantData::Float3
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// Specular Point Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Specular_Point_Lighting)

    VS_BODY =
        "(\n"
        "    in    float    SpecularPower,\n"
        "    in    float4   TransformedPosition,\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcSpecularPointLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    float3 VecToLight = Data.Position - TransformedPosition;\n"
        "\n"
        "    // Note: This does not actually generate a branch. The compiler translates this into\n"
        "    //       an instruction (slt) that returns 0 or 1 and multiplies that times the output color\n"
        "    if (dot(VecToLight, TransformedNormal) > 0)\n"
        "    {\n"
        "        float DistToLight = length(VecToLight);\n"
        "        // normalize L\n"
        "        VecToLight /= DistToLight;\n"
        "\n"
        "        // in WorldView space, the camera is at <0> so just invert the position\n"
        "        float3 HalfVector = normalize(normalize(-TransformedPosition.xyz) + VecToLight);\n"
        "\n"
        "        // the max is to ensure that the attenuation only diminishes the light\n" 
        "        float atten = 1.0 / max(Data.AttenAndRange.x\n"
        "                                + Data.AttenAndRange.y * DistToLight\n"
        "                                + Data.AttenAndRange.z * DistToLight * DistToLight,\n" 
        "                                1.0);\n"
        "\n"
        "        // AttenAndRange.w is the light's range\n"
        "        Output.Diffuse.rgb += Data.Color.rgb\n" 
        "                              * pow(max(dot(TransformedNormal, HalfVector), 0), SpecularPower)\n"
        "                              * atten\n"
        "                              * step(DistToLight, Data.AttenAndRange.w);\n"
        "    }\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::SpecularPower,
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Position",
                ShaderFunctionConstantData::Float4
            },
            {
                "AttenAndRange",
                ShaderFunctionConstantData::Float4
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
// Specular Spot Lighting Vertex Function
// 
//+----------------------------------------------------------------------------

DEFINE_VERTEX_SHADER_FUNCTION(Specular_Spot_Lighting)

    VS_BODY =
        "(\n"
        "    in    float    SpecularPower,\n"
        "    in    float4   TransformedPosition,\n"
        "    in    float3   TransformedNormal,\n"
        "    in    CalcSpecularSpotLighting_VS_ConstData Data,\n"
        "    inout VertexShaderOutput Output\n"
        "    )\n"
        "{\n"
        "    float3 VecToLight = Data.Position - TransformedPosition;\n"
        "\n"
        "    // Note: This does not actually generate a branch. The compiler translates this into\n"
        "    //       an instruction (slt) that returns 0 or 1 and multiplies that times the output color\n"
        "    if (dot(VecToLight, TransformedNormal) > 0)\n"
        "    {\n"
        "        float DistToLight = length(VecToLight);\n"
        "        // normalize L\n"
        "        VecToLight /= DistToLight;\n"
        "\n"
        "        // in WorldView space, the camera is at <0> so just invert the position\n"
        "        float3 HalfVector = normalize(normalize(-TransformedPosition.xyz) + VecToLight);\n"
        "\n"
        "        // the max is to ensure that the attenuation only diminishes the light\n" 
        "        float atten = 1.0 / max(Data.AttenAndRange.x\n"
        "                                + Data.AttenAndRange.y * DistToLight\n"
        "                                + Data.AttenAndRange.z * DistToLight * DistToLight,\n" 
        "                                1.0);\n"
        "\n"
        "        float rho = max(dot(Data.Direction, VecToLight), 0);\n"
        "        // CosHalfPhiAndCosDiff.x = cos(Phi/2)\n"
        "        // CosHalfPhiAndCosDiff.y = cos(Theta/2) - cos(Phi/2)\n"
        "        float spot = saturate((rho - Data.CosHalfPhiAndCosDiff.x) / Data.CosHalfPhiAndCosDiff.y);\n"
        "\n"
        "        // AttenAndRange.w is the light's range\n"
        "        Output.Diffuse.rgb += Data.Color.rgb\n" 
        "                              * pow(max(dot(TransformedNormal, HalfVector), 0), SpecularPower)\n\n"
        "                              * atten\n"
        "                              * spot\n"
        "                              * step(DistToLight, Data.AttenAndRange.w);\n"
        "    }\n"
        "}\n";
    
    VS_INPUTS =
        {
            VertexFunctionParameter::SpecularPower,
            VertexFunctionParameter::Position,
            VertexFunctionParameter::Normal,
            VertexFunctionParameter::FunctionConstData,
            VertexFunctionParameter::ShaderOutputStruct,
        };

    VS_DATA =
        {
            {
                "Color",
                ShaderFunctionConstantData::Float4
            },
            {
                "Position",
                ShaderFunctionConstantData::Float4
            },
            {
                "AttenAndRange",
                ShaderFunctionConstantData::Float4
            },
            {
                "Direction",
                ShaderFunctionConstantData::Float3
            },
            {
                "CosHalfPhiAndCosDiff",
                ShaderFunctionConstantData::Float4
            }
        }; 

    VS_LOOPABLE = true;
    
VS_END;

//+----------------------------------------------------------------------------
//
//
//  Final Combined Functions
//
// 
//+----------------------------------------------------------------------------

//
// NULL Shader Function
//
// NOTICE-2006/05/05-milesc Because the NullFunction is the one associated with
// the lighting color source, we must put BlendsColorSource here to get the
// pipeline to call IsOpaque on the lighting color source.
//
ShaderFunction g_NullFunction
(
    "NullFunction",                         // Fragment Name        
    TransparencyEffect::BlendsColorSource,  // Transparency Effect  
    g_NullVertexShaderFunction,             // Vertex Function      
    g_NullPixelShaderFunction               // Pixel Function       
);

//
// Shader Function description.
//
ShaderFunction g_Transform2D_Function
(
    "Transform2D",                      // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Transform_World2D_By_Matrix4x4::VS, // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);


//
// Shader Function description.
//
ShaderFunction g_oMultiplyByInputDiffuse_Function
(
    "MultiplyByInputDiffuse",
    TransparencyEffect::BlendsColorSource,
    MultiplyByInputDiffuse::VS,
    MultiplyByInputDiffuse_Premultiplied::PS
);


//
// Shader Function description.
//
ShaderFunction g_oMultiplyByInputDiffuse_NonPremultipledInput_Function
(
    "MultiplyByInputDiffuse_NonPremultipliedInput",
    TransparencyEffect::BlendsColorSource,
    MultiplyByInputDiffuse::VS,
    MultiplyByInputDiffuse_NonPremultiplied::PS
);

//
// Shader Function description.
//

ShaderFunction g_oMultiplyTexture_Transformed_From_InputUV_Function
(
    "MultiplyTexture_Transform_InputUV",
    TransparencyEffect::BlendsColorSource,
    Transform_InputUV_By_Matrix3x2_Into_TexCoord2::VS,
    MultiplyTexture::PS
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyTexture_From_Input_Vertex_TexCoord2_Function
(
    "MultiplyTexture_NoTransformFromUV",                  // Fragment Name
    TransparencyEffect::BlendsColorSource,
    Pass_InputVertex_UV2_ToTexCoord2::VS,   // Vertex Function
    MultiplyTexture::PS      // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyConstantFragment
(
    "MultiplyConstant",             // Fragment Name
    TransparencyEffect::BlendsColorSource,
    g_NullVertexShaderFunction,     // Vertex Function
    MultiplyConstant::PS          // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyAlphaFunction
(
    "MultiplyAlpha",                // Function Name
    TransparencyEffect::HasTransparency,
    g_NullVertexShaderFunction,     // Vertex Function
    Multiply_By_Alpha_Premultiplied::PS    // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiply_By_Alpha_Non_Premultiplied_Function
(
    "MultiplyAlpha_NonPremultiplied",       // Function Name
    TransparencyEffect::HasTransparency,
    g_NullVertexShaderFunction,                 // Vertex Function
    Multiply_By_Alpha_NonPremultiplied::PS // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyAlphaMask_From_Input_Vertex_TexCoord2_Function
(
    "MultiplyAlphaMask_NoTransformFromUV",      // Function Name
    TransparencyEffect::HasTransparency,
    Pass_InputVertex_UV2_ToTexCoord2::VS,
    Multiply_By_Alpha_Mask_Premultiplied::PS    // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyAlphaMask_Transformed_From_InputUV_Function
(
    "MultiplyAlphaMask_Transform_InputUV",      // Function Name
    TransparencyEffect::HasTransparency,
    MultiplyAlphaMask_Transformed_From_InputUV::VS,  // Vertex Function
    Multiply_By_Alpha_Mask_Premultiplied::PS    // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyRadialGradientCentered_Function
(
    "MultiplyRadialGradientCentered",                   // Function Name
    TransparencyEffect::BlendsColorSource,
    Pass_InputVertex_UV2_ToTexCoord2::VS,               // Vertex Function
    MultiplyRadialGradientCentered::PS                  // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_oMultiplyRadialGradientNonCentered_Function
(
    "MultiplyRadialGradientNonCentered",                // Function Name
    TransparencyEffect::BlendsColorSource,
    Pass_InputVertex_UV2_ToTexCoord2::VS,               // Vertex Function
    MultiplyRadialGradientNonCentered::PS               // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Get3DTransforms_Function
(
    "Get3DTransforms",                  // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Get3DTransforms::VS,                // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Transform3D_Function
(
    "Transform3D",                      // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Transform_World3D::VS,              // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Ambient_Lighting_Function
(
    "CalcAmbientLighting",              // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Ambient_Lighting::VS,               // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Flip_Normal_Function
(
    "FlipNormal",                       // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Flip_Normal::VS,                    // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Diffuse_Directional_Lighting_Function
(
    "CalcDiffuseDirectionalLighting",   // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Diffuse_Directional_Lighting::VS,   // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Diffuse_Point_Lighting_Function
(
    "CalcDiffusePointLighting",         // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Diffuse_Point_Lighting::VS,         // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Diffuse_Spot_Lighting_Function
(
    "CalcDiffuseSpotLighting",          // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Diffuse_Spot_Lighting::VS,          // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_GetSpecularPower_Function
(
    "GetSpecularPower",                 // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    GetSpecularPower::VS,               // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Specular_Directional_Lighting_Function
(
    "CalcSpecularDirectionalLighting",  // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Specular_Directional_Lighting::VS,  // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Specular_Point_Lighting_Function
(
    "CalcSpecularPointLighting",        // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Specular_Point_Lighting::VS,        // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Shader Function description.
//
ShaderFunction g_Specular_Spot_Lighting_Function
(
    "CalcSpecularSpotLighting",         // Fragment Name
    TransparencyEffect::NoTransparency, // Transparency Effect
    Specular_Spot_Lighting::VS,         // Vertex Function
    g_NullPixelShaderFunction           // Pixel Function
);

//
// Array of usable Shader Functions
//
// *** MUST BE IN THE SAME ORDER AS THE ENUM DEFINITON IN THE HEADER ***
//
const ShaderFunction *g_pHwHLSLShaderFunctions[] = 
{
    &g_NullFunction,

    &g_Transform2D_Function,
    &g_oMultiplyByInputDiffuse_Function,
    &g_oMultiplyByInputDiffuse_NonPremultipledInput_Function,
    &g_oMultiplyTexture_Transformed_From_InputUV_Function,
    &g_oMultiplyTexture_From_Input_Vertex_TexCoord2_Function,
    &g_oMultiplyConstantFragment,
    &g_oMultiplyAlphaFunction,
    &g_oMultiply_By_Alpha_Non_Premultiplied_Function,
    &g_oMultiplyAlphaMask_Transformed_From_InputUV_Function,
    &g_oMultiplyAlphaMask_From_Input_Vertex_TexCoord2_Function,
    &g_oMultiplyRadialGradientCentered_Function,
    &g_oMultiplyRadialGradientNonCentered_Function,

    &g_Get3DTransforms_Function,
    &g_Transform3D_Function,
    &g_Ambient_Lighting_Function,
    &g_Flip_Normal_Function,
    &g_Diffuse_Directional_Lighting_Function,
    &g_Diffuse_Point_Lighting_Function,
    &g_Diffuse_Spot_Lighting_Function,
    &g_GetSpecularPower_Function,
    &g_Specular_Directional_Lighting_Function,
    &g_Specular_Point_Lighting_Function,
    &g_Specular_Spot_Lighting_Function
};

C_ASSERT(ARRAYSIZE(g_pHwHLSLShaderFunctions)==ShaderFunctions::Total);


