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
//      Contains the definition for the CHwPipeline::ShaderBuilder class.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CHwShaderCache;

//+-----------------------------------------------------------------------------
//
//  Class:
//      Builder
//
//  Synopsis:
//      Takes the generic an array of generic HwPipelineItem structs and sends
//      the corresponding HwShaderPipelineItem structures to the pipeline.
//
//------------------------------------------------------------------------------
class CHwShaderPipelineBuilder :
    public CHwPipelineBuilder
{
public:
    CHwShaderPipelineBuilder(
        __in_ecount(1) CHwShaderPipeline *pHP
        );

    ~CHwShaderPipelineBuilder();

    HRESULT Setup(
        bool f2D,
        MilCompositingMode::Enum eCompositingMode,
        __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
        __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext    *pEffectContext
        );

    HRESULT GetHwShader(
        __deref_out_ecount(1) CHwPipelineShader **ppHwShader
        );

    override HRESULT Set_Constant(
        __in_ecount(1) CHwConstantColorSource *pConstant
        );

    override HRESULT Set_Texture(
        __in_ecount(1) CHwTexturedColorSource *pTexture
        );

    override HRESULT Set_RadialGradient(
        __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient
        );

    override HRESULT Mul_ConstAlpha(
        CHwConstantAlphaColorSource *pAlphaColorSource
        );

    override HRESULT Mul_AlphaMask(
        __in_ecount(1) CHwTexturedColorSource *pAlphaMaskColorSource
        );

    override HRESULT Add_Lighting(
        __inout_ecount(1) CHwLightingColorSource *pLightingSource
        );

    HRESULT SetupHwLighting(
        float flNormalScale,
        CHwShader::LightingValues eLightingPass,
        UINT uNumDirectionalLights,
        UINT uNumPointLights,
        UINT uNumSpotLights,
        __inout_ecount(1) CHwLightingColorSource *pLightingSource
        );

    HRESULT AddShaderPipelineItem(
        __in_ecount_opt(1) CHwColorSource *pHwColorSource,
        int iSampler,
        MilVertexFormatAttribute mvfaSourceLocation,
        ShaderFunctions::Enum oShaderFunction
        );

    void GetShaderParameterHandles(
        __in_ecount(1) ShaderFunctions::Enum oFunction,
        UINT uNumVertexParameters,
        DBG_PARAM_COMMA(__in_ecount_opt(uNumVertexParameters) const ShaderFunctionConstantData::Enum *rgDbgVertexConstantDataTypes)
        __out_ecount_opt(uNumVertexParameters) MILSPHandle *rgVertexParameterHandles,
        UINT uNumPixelParameters,
        DBG_PARAM_COMMA(__in_ecount_opt(uNumPixelParameters) const ShaderFunctionConstantData::Enum *rgDbgPixelConstantDataTypes)
        __out_ecount_opt(uNumPixelParameters) MILSPHandle *rgPixelParameterHandles
        );

private:

    HRESULT SetupPerPrimitiveAntiAliasingBlend();

    void FinalizeOperations(
        MilCompositingMode::Enum eCompositingMode
        );

    HRESULT Mul_TextureInternal(
        __in_ecount(1) CHwTexturedColorSource *pTexture,
        ShaderFunctions::Enum eFunction
        );

    HRESULT Mul_RadialGradientInternal(
        __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient,
        ShaderFunctions::Enum const eFunction
        );

    override HRESULT Mul_BlendColorsInternal(
        __in_ecount(1) CHwColorComponentSource *pBlendColorSource
        );


private:
    CHwShaderPipeline *m_pHPNoRef;

    CHwShaderCache *m_pCache;

    UINT m_uPixelShaderCurrentConstantPosition;
    UINT m_uVertexShaderCurrentConstantPosition;

    bool m_f2D;
    bool m_fHwLightingAdded;
};


