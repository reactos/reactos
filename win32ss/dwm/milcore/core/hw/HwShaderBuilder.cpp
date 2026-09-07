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
//      Contains the implementation of the CHwPipelineBuilder class.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::CHwShaderPipelineBuilder
//
//  Synopsis:
//      Intializes members
//

CHwShaderPipelineBuilder::CHwShaderPipelineBuilder(
    __in_ecount(1) CHwShaderPipeline *pHP
    )
    : CHwPipelineBuilder(
        pHP,
        HwPipeline::Shader
        )
{
    m_pHPNoRef = pHP;
    m_pCache = NULL;
    m_fHwLightingAdded = false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::~CHwShaderPipelineBuilder
//
//  Synopsis:
//      Releases the cache.
//

CHwShaderPipelineBuilder::~CHwShaderPipelineBuilder()
{
    ReleaseInterfaceNoNULL(m_pCache);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Setup
//
//  Synopsis:
//      Converts the HwPipelineItem entries into HwShaderPipelineItem entries.
//

HRESULT 
CHwShaderPipelineBuilder::Setup(
    bool f2D,
    MilCompositingMode::Enum eCompositingMode,
    __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext
    )
{
    HRESULT hr = S_OK;

    m_uVertexShaderCurrentConstantPosition = 0;
    m_uPixelShaderCurrentConstantPosition = PIXEL_SHADER_TABLE_OFFSET;

    CHwPipelineBuilder::InitializePipelineMembers(
        eCompositingMode,
        pIGeometryGenerator
        );

    IFC(m_pHPNoRef->m_pDevice->GetHwShaderCache(
        &m_pCache
        ));

    m_f2D = f2D;

    m_pCache->Reset();

    if (m_f2D)
    {

        MILSPHandle rghVertexShaderHandles[1];
#if DBG
        ShaderFunctionConstantData::Enum oDbgTypes[1];

        oDbgTypes[0] = ShaderFunctionConstantData::Matrix4x4;
#endif

        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            ShaderFunctions::Prepare2DTransform
            ));

        GetShaderParameterHandles(
            ShaderFunctions::Prepare2DTransform,
            1,
            DBG_PARAM_COMMA(oDbgTypes)
            rghVertexShaderHandles,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL
            );
    }
    else
    {
        const UINT c_uNumParams = 3;

        MILSPHandle rghVertexShaderHandles[c_uNumParams];
#if DBG
        ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];

        oDbgTypes[0] = ShaderFunctionConstantData::Matrix4x4;
        oDbgTypes[1] = ShaderFunctionConstantData::Matrix4x4;
        oDbgTypes[2] = ShaderFunctionConstantData::Matrix4x4;
#endif

        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            ShaderFunctions::Get3DTransforms
            ));

        GetShaderParameterHandles(
            ShaderFunctions::Get3DTransforms,
            c_uNumParams,
            DBG_PARAM_COMMA(oDbgTypes)
            rghVertexShaderHandles,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL
            );

        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            ShaderFunctions::Prepare3DTransforms
            ));
    }


    IFC(CHwPipelineBuilder::SendPipelineOperations(
        pIPCS,
        pIEffects,
        pEffectContext,
        pIGeometryGenerator
        ));

    FinalizeOperations(eCompositingMode);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::FinalizeOperations
//
//  Synopsis:
//      Examine the pipeline after all the basic operations have been added and
//      make any adjustments to yield a valid pipeline.
//
//  Notes:
//      Currently it only walks operations to determine transparency and then
//      setups up the composition mode.
//

void
CHwShaderPipelineBuilder::FinalizeOperations(
    MilCompositingMode::Enum eCompositingMode
    )
{
    //
    // Source Over without transparency is equivalent to source copy, but
    // source copy is faster, so we check for it and promote the call to
    // sourcecopy.
    //
    if (eCompositingMode == MilCompositingMode::SourceOver)
    {
        //
        // Walk pipeline items and check for transparency operations and
        // transparent sources.
        //

        bool fPipelineHasTransparency = false;

        UINT uItems = m_pHPNoRef->m_rgItem.GetCount();

        if(m_fAntiAliasUsed)
        {
            fPipelineHasTransparency = true;
        }
        else
        {
            //
            // Since we're not using anti-aliasing (which uses transparency),
            // walk through the pipeline to see if the fragments/colorsources
            // have transparency.
            //

            while (uItems--)
            {
                const HwPipelineItem &oItem = m_pHPNoRef->m_rgItem[uItems];

                //
                // Check to see if this function is going to add transparency to the pipeline.
                //

                if (oItem.pFragment->TransparencyEffect != TransparencyEffect::NoTransparency)
                {
                    if (   oItem.pFragment->TransparencyEffect == TransparencyEffect::HasTransparency
                        || (   oItem.pHwColorSource
                            && !oItem.pHwColorSource->IsOpaque()
                           )
                       )
                    {
                        fPipelineHasTransparency = true;
                        break;
                    }
                    else
                    {
                        //
                        // If the TransparencyEffect isn't "NoTransparency",
                        // isn't "HasTransparency", and failed the above if
                        // statement, then it must be "BlendsColorSource" with
                        // a color source that doesn't have alpha.
                        //
                        Assert(   oItem.pFragment->TransparencyEffect == TransparencyEffect::BlendsColorSource
                               && (   oItem.pHwColorSource == NULL
                                   || oItem.pHwColorSource->IsOpaque()
                                      )
                                  );
                    }
                }
            }
        }

        if (!fPipelineHasTransparency)
        {
            eCompositingMode = MilCompositingMode::SourceCopy;
        }
    }

    //
    // Compute the final vertex attributes we must fill-in to send data to
    // DrawPrimitive.
    //
    // We always leave Z test enabled so we must always specify Z in vertices.
    //

    if (GetAvailableForGeneration() & MILVFAttrZ)
    {
        GenerateVertexAttribute(MILVFAttrZ);
    }

    //
    // Set composition mode
    //

    m_pHPNoRef->SetupCompositionMode(
        eCompositingMode
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::GetHwShader
//
//  Synopsis:
//      Retrieves the Shader.
//

HRESULT
CHwShaderPipelineBuilder::GetHwShader(
    __deref_out_ecount(1) CHwPipelineShader **ppHwShader
    )
{
    HRESULT hr = S_OK;
    
    IFC(m_pCache->GetHwShader(
        m_pHPNoRef->m_rgItem.GetDataBuffer(),
        m_pHPNoRef->m_rgItem.GetCount(),
        ppHwShader
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Set_Texture
//
//  Synopsis:
//      Adds the appropriate HwShaderPipelineItem.
//

HRESULT 
CHwShaderPipelineBuilder::Set_Texture(
    __in_ecount(1) CHwTexturedColorSource *pTexture
    )
{
    HRESULT hr = S_OK;

    //
    // Even though this is a Set Texture the pipeline multiplies it by an
    // initial value (1,1,1,1) to simplify the number of different fragments.
    //
    // NOTICE-2006/02/20-JasonHa  This multiply is premultiplication agnostic
    // since it is always combined with (1,1,1,1).  The premultiplication sense
    // of the color source is what is important and that is handle outside if
    // this method.
    //

    ShaderFunctions::Enum eFunction;

    if (!(GetAvailableForReference() & MILVFAttrUV1))
    {
        eFunction = ShaderFunctions::MultiplyTexture_NoTransformFromTexCoord;
    }
    else
    {
        eFunction = ShaderFunctions::MultiplyTexture_TransformFromVertexUV;
    }

    IFC(Mul_TextureInternal(
        pTexture,
        eFunction
        ));

    // This item is the first color operation despite using a multiply
    // operation as noted above.
    SetLastItemAsEarliestAvailableForAlphaMultiply();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Set_RadialGradient
//
//  Synopsis:
//      Adds a radial gradient to the pipeline
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::Set_RadialGradient(
    __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient
    )
{
    HRESULT hr = S_OK;

    ShaderFunctions::Enum const eFunction = (pRadialGradient->HasSeperateOriginFromCenter()) ?
        ShaderFunctions::MultiplyRadialGradientNonCentered : ShaderFunctions::MultiplyRadialGradientCentered;

    //
    // We can call multiply because we know this function is going to be called
    // in the beginning of the pipeline where the value the gradient is going
    // to multiply with is <1.0f,1.0f,1.0f,1.0f>.
    //
    IFC(Mul_RadialGradientInternal(
        pRadialGradient,
        eFunction
        ));

    SetLastItemAsEarliestAvailableForAlphaMultiply();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Set_Constant
//
//  Synopsis:
//      Adds the appropriate HwShaderPipelineItem.
//

HRESULT 
CHwShaderPipelineBuilder::Set_Constant(
    __in_ecount(1) CHwConstantColorSource *pConstant
    )
{
    HRESULT hr = S_OK;

    //
    // Even though this is a Set Constant the pipeline multiplies it by an
    // initial value (1,1,1,1) to simplify the number of different fragments.
    //
    // NOTICE-2006/02/20-JasonHa  This multiply is premultiplication agnostic
    // since it is always combined with (1,1,1,1).  The premultiplication sense
    // of the color source is what is important and that is handle outside if
    // this method.
    //

    if (   GetAvailableForGeneration() & MILVFAttrDiffuse
        && m_f2D
           )
    {
        ShaderFunctions::Enum oShaderFunction = ShaderFunctions::MultiplyConstant;

        if (m_eAlphaMultiplyOp == HBO_Multiply)
        {
            oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse;
        }
        else
        {
            Assert(m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly);
            oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse_NonPremultipledInput;
        }
    
        IFC(AddShaderPipelineItem(
            pConstant,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrDiffuse,
            oShaderFunction
            ));

        SetLastItemAsAAPiggyback();

        //
        // Use this function to assert that there are no parameters we have to
        // set for the shader item.
        //
        GetShaderParameterHandles(
            oShaderFunction,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL
            );
    }
    else
    {
        //
        // If we can't sneak diffuse into the vertex, we need to add a multiply
        // constant fragment and data to the shader.
        //
        ShaderFunctions::Enum oFunction = ShaderFunctions::MultiplyConstant;
    
        MILSPHandle rgPixelParameterHandles[1];
    
#if DBG
        ShaderFunctionConstantData::Enum rgDbgPixelParameterTypes[1];
    
        rgDbgPixelParameterTypes[0] = ShaderFunctionConstantData::Float4;
#endif
    
        IFC(AddShaderPipelineItem(
            pConstant,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            oFunction
            ));
    
        //
        // Get handles and verify types for constant data parameters.
        //
        GetShaderParameterHandles(
            oFunction,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL,
            1,
            DBG_PARAM_COMMA(rgDbgPixelParameterTypes)
            rgPixelParameterHandles
            );
    
        pConstant->SetColorShaderHandle(
            rgPixelParameterHandles[0]
            );
    }

    // This item is the first color operation despite using a multiply
    // operation as noted above.
    SetLastItemAsEarliestAvailableForAlphaMultiply();

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Mul_ConstAlpha
//
//  Synopsis:
//      Adds the appropriate HwShaderPipelineItem.
//

HRESULT
CHwShaderPipelineBuilder::Mul_ConstAlpha(
    CHwConstantAlphaColorSource *pAlphaColorSource
    )
{
    HRESULT hr = S_OK;

    float flAlpha = pAlphaColorSource->GetAlpha();
    CHwConstantAlphaScalableColorSource *pScalableAlphaSource = NULL;

    if (TryToMultiplyConstantAlphaToExistingStage(pAlphaColorSource))
    {        
        //
        // We've succeeded in multiplying the alpha color source to an existing
        // stage, so early out.
        //

        goto Cleanup;
    }

#if DBG
    ShaderFunctionConstantData::Enum rgDbgPixelParameterTypes[1];

    rgDbgPixelParameterTypes[0] = ShaderFunctionConstantData::Float4;
#endif

    if (   GetAvailableForGeneration() & MILVFAttrDiffuse
        && m_f2D
           )
    {
        ShaderFunctions::Enum oShaderFunction = ShaderFunctions::MultiplyConstant;

        if (m_eAlphaMultiplyOp == HBO_Multiply)
        {
            oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse;
        }
        else
        {
            Assert(m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly);
            oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse_NonPremultipledInput;
        }

        IFC(CHwConstantAlphaScalableColorSource::Create(
            m_pHPNoRef->m_pDevice,
            flAlpha,
            NULL,   // No original color source
            &m_pHPNoRef->m_dbScratch,
            &pScalableAlphaSource
            ));

        IFC(AddShaderPipelineItem(
            pScalableAlphaSource,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrDiffuse,
            oShaderFunction
            ));

        SetLastItemAsAAPiggyback();

        //
        // Use this function to basically assert that there are no parameters
        // we have to set for the shader item.
        //
        GetShaderParameterHandles(
            oShaderFunction,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL
            );
    }
    else
    {
        //
        // If we reach here then the pipeline builder wasn't able to find a
        // previous stage to multiply the alpha to, and diffuse wasn't
        // available to put in for anti-aliasing to apply it.
        //
        // We have to create a ShaderFragment that will pass the data down
        // and multiply it in the pixel shader.
        //

        MILSPHandle rgPixelParameterHandles[1];
        ShaderFunctions::Enum oFunction;

        if (m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly)
        {
            oFunction  = ShaderFunctions::MultiplyAlpha_NonPremultiplied;
        }
        else
        {
            oFunction  = ShaderFunctions::MultiplyAlpha;
        }

        IFC(AddShaderPipelineItem(
            pAlphaColorSource,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            oFunction
            ));
    
        GetShaderParameterHandles(
            oFunction,
            0,
            DBG_PARAM_COMMA(NULL)
            NULL,
            1,
            DBG_PARAM_COMMA(rgDbgPixelParameterTypes)
            rgPixelParameterHandles
            );
    
        pAlphaColorSource->SetShaderAlphaHandle(
            rgPixelParameterHandles[0]
            );
    }

Cleanup:
    ReleaseInterfaceNoNULL(pScalableAlphaSource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Mul_AlphaMask
//
//  Synopsis:
//      Alpha multiply using a texture
//

HRESULT
CHwShaderPipelineBuilder::Mul_AlphaMask(
    __in_ecount(1) CHwTexturedColorSource *pAlphaMask
    )
{
    HRESULT hr = S_OK;

    Assert(    m_eAlphaMultiplyOp == HBO_Multiply
            || m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly );

    ShaderFunctions::Enum eFunction;

    // Easy enough to support HBO_MultiplyAlphaOnly except that we don't need
    // to handle it right now because it's only used for vertex colors now.
    Assert(m_eAlphaMultiplyOp == HBO_Multiply);
    
    if (VerticesArePreGenerated())
    {
        eFunction = ShaderFunctions::MultiplyAlphaMask_TransformFromVertexUV;
    }
    else
    {
        eFunction = ShaderFunctions::MultiplyAlphaMask_NoTransformFromTexCoord;
    }

    IFC(Mul_TextureInternal(
        pAlphaMask,
        eFunction
        ));

    if (pAlphaMask->IsAlphaScalable())
    {
        // Remember this location holds an alpha scalable color source
        SetLastItemAsAlphaScalable();
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Mul_Texture
//
//  Synopsis:
//      Add the Texture with given multiply operation.  Texture coordinates are
//      tranformed unless MultiplyAlphaMask_NoTransformFromTexCoord or
//      MultiplyTexture_NoTransformFromTexCoord is passed in.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::Mul_TextureInternal(
    __in_ecount(1) CHwTexturedColorSource *pTexture,
    ShaderFunctions::Enum eFunction
    )
{
    HRESULT hr = S_OK;

    MILSPHandle rgVertexParameterHandles[2];
    MILSPHandle *phTransform = NULL;

#if DBG
    ShaderFunctionConstantData::Enum rgDbgVertexParameterTypes[2];
#endif

    int iSampler = ReserveCurrentTextureSampler();
    MilVertexFormatAttribute mvfaTextureCoordinate;

    if (!(GetAvailableForReference() & MILVFAttrUV1))
    {
        mvfaTextureCoordinate = MIL_TEXTURESTAGE_TO_MILVFATTR(iSampler);
    }
    else
    {
        // Always use the first texture coordinate for HW transform.
        mvfaTextureCoordinate = MILVFAttrUV1;
    }

    IFC(AddShaderPipelineItem(
        pTexture,
        iSampler,
        mvfaTextureCoordinate,
        eFunction
        ));

    UINT uNumParams = 0;

    if (   eFunction != ShaderFunctions::MultiplyTexture_NoTransformFromTexCoord
        && eFunction != ShaderFunctions::MultiplyAlphaMask_NoTransformFromTexCoord)
    {
        phTransform = &rgVertexParameterHandles[uNumParams];
#if DBG
        rgDbgVertexParameterTypes[uNumParams] = ShaderFunctionConstantData::Matrix3x2;
#endif

        uNumParams++;
    }

    //
    // Get handles and verify types for constant data parameters.
    //
    GetShaderParameterHandles(
        eFunction,
        uNumParams,
        DBG_PARAM_COMMA(rgDbgVertexParameterTypes)
        rgVertexParameterHandles,
        0,
        DBG_PARAM_COMMA(NULL)
        NULL
        );

    if (phTransform)
    {
        pTexture->SetTextureTransformHandle(
            *phTransform
            );
    }
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Mul_RadialGradientInternal
//
//  Synopsis:
//      Multiplies a centered or non centered radial gradient in the
//      shaderbuilder.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::Mul_RadialGradientInternal(
    __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient,
    ShaderFunctions::Enum const eFunction
    )
{
    HRESULT hr = S_OK;

    int iSampler = ReserveCurrentTextureSampler();
    MilVertexFormatAttribute mvfaTextureCoordinate;

    MILSPHandle rgPixelParameterHandles[5];
    UINT uNumPixelParameters = 0;

#if DBG
    ShaderFunctionConstantData::Enum rgDbgPixelParameterTypes[5];
#endif

    if(eFunction == ShaderFunctions::MultiplyRadialGradientCentered)
    {
        uNumPixelParameters = 1;

#if DBG
        rgDbgPixelParameterTypes[0] = ShaderFunctionConstantData::Float;
#endif
    }
    else
    {
        Assert(eFunction == ShaderFunctions::MultiplyRadialGradientNonCentered);

        uNumPixelParameters = 4;

#if DBG
        rgDbgPixelParameterTypes[0] = ShaderFunctionConstantData::Float2;
        rgDbgPixelParameterTypes[1] = ShaderFunctionConstantData::Float2;
        rgDbgPixelParameterTypes[2] = ShaderFunctionConstantData::Float;
        rgDbgPixelParameterTypes[3] = ShaderFunctionConstantData::Float;
#endif
    }

    if (!(GetAvailableForReference() & MILVFAttrUV1))
    {
        mvfaTextureCoordinate = MIL_TEXTURESTAGE_TO_MILVFATTR(iSampler);
    }
    else
    {
        // Always use the first texture coordinate for HW transform.
        mvfaTextureCoordinate = MILVFAttrUV1;
    }

    IFC(AddShaderPipelineItem(
        pRadialGradient,
        iSampler,
        mvfaTextureCoordinate,
        eFunction
        ));

    //
    // Get handles and verify types for constant data parameters.
    //
    GetShaderParameterHandles(
        eFunction,
        0,
        DBG_PARAM_COMMA(NULL)
        NULL,
        uNumPixelParameters,
        DBG_PARAM_COMMA(rgDbgPixelParameterTypes)
        rgPixelParameterHandles
        );

    if(eFunction == ShaderFunctions::MultiplyRadialGradientCentered)
    {
        pRadialGradient->SetCenteredRadialGradientParamData(
            rgPixelParameterHandles[0]
            );
    }
    else
    {
        Assert(eFunction == ShaderFunctions::MultiplyRadialGradientNonCentered);

        pRadialGradient->SetNonCenteredRadialGradientParamData(
            rgPixelParameterHandles[0],
            rgPixelParameterHandles[1],
            rgPixelParameterHandles[2],
            rgPixelParameterHandles[3]
            );
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipelineBuilder::Mul_BlendColorsInternal
//
//  Synopsis:
//      Multiplies the pipeline by a set of blend colors.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::Mul_BlendColorsInternal(
    __in_ecount(1) CHwColorComponentSource *pBlendColorSource
    )
{
    HRESULT hr = S_OK;

    ShaderFunctions::Enum oShaderFunction;

    if (m_eAlphaMultiplyOp == HBO_Multiply)
    {
        oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse;
    }
    else
    {
        Assert(m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly);
        oShaderFunction = ShaderFunctions::MultiplyByInputDiffuse_NonPremultipledInput;
    }

    IFC(AddShaderPipelineItem(
        pBlendColorSource,
        INVALID_PIPELINE_SAMPLER,
        MILVFAttrDiffuse,
        oShaderFunction
        ));

    //
    // Use this function to assert that there are no parameters we have to
    // set for the shader item.
    //
    GetShaderParameterHandles(
        oShaderFunction,
        0,
        DBG_PARAM_COMMA(NULL)
        NULL,
        0,
        DBG_PARAM_COMMA(NULL)
        NULL
        );

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::Add_Lighting
//
//  Synopsis:
//      Adds an adds a lighting colorsource.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::Add_Lighting(
    __inout_ecount(1) CHwLightingColorSource *pLightingSource
    )
{
    HRESULT hr = S_OK;

    IFC(SetupHwLighting(
        pLightingSource->GetNormalScale(),
        pLightingSource->GetLightingPass(),
        pLightingSource->GetNumDirectionalLights(),
        pLightingSource->GetNumPointLights(),
        pLightingSource->GetNumSpotLights(),
        pLightingSource
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipelineBuilder::SetupHwLighting
//
//  Synopsis:
//      Sets up hw lighting.
//
//------------------------------------------------------------------------------
HRESULT
CHwShaderPipelineBuilder::SetupHwLighting(
    float flNormalScale,
    CHwShader::LightingValues eLightingPass,
    UINT uNumDirectionalLights,
    UINT uNumPointLights,
    UINT uNumSpotLights,
    __inout_ecount(1) CHwLightingColorSource *pLightingSource
    )
{
    HRESULT hr = S_OK;

    Assert(!m_fHwLightingAdded);

    MILSPHandle hFirstLightingConstant = MILSP_INVALID_HANDLE;

    //
    // NOTICE-2006/05/05-milesc If the lighting color source is moved to a
    // fragment other than ShaderFunctions::SystemVertexBuilderPassDiffuse, aka
    // the NullFunction, we must modify the TransparencyEffect of the shader
    // functions appropriately. Right now we put BlendsColorSource on the Null
    // function. See the comment in hwhlslshaderfragments.cpp
    //

    IFC(AddShaderPipelineItem(
        pLightingSource,
        INVALID_PIPELINE_SAMPLER,
        MILVFAttrNone,
        ShaderFunctions::SystemVertexBuilderPassDiffuse
        ));

    if (flNormalScale == -1.0)
    {
        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            ShaderFunctions::FlipNormal
            )); 
    }

    if (   eLightingPass == CHwShader::LV_Diffuse 
        || eLightingPass == CHwShader::LV_Specular
           )
    {   
        enum Operation
        {
            O_Setup = 0,
            O_Directional,
            O_Point,
            O_Spot,
            O_Total
        };

        enum IlluminationType
        {
            IT_Diffuse = 0,
            IT_Specular,
            IT_Total
        };

        static const ShaderFunctions::Enum rgShaderFuncs[O_Total][IT_Total] =
            {
                {
                    ShaderFunctions::CalcAmbientLighting,
                    ShaderFunctions::GetSpecularPower
                },
                {
                    ShaderFunctions::CalcDiffuseDirectionalLighting,
                    ShaderFunctions::CalcSpecularDirectionalLighting
                },
                {
                    ShaderFunctions::CalcDiffusePointLighting,
                    ShaderFunctions::CalcSpecularPointLighting
                },
                {
                    ShaderFunctions::CalcDiffuseSpotLighting,
                    ShaderFunctions::CalcSpecularSpotLighting
                },
            };

        IlluminationType itIllum;
        if (eLightingPass == CHwShader::LV_Diffuse)
        {
            itIllum = IT_Diffuse;   
        }
        else
        {
            itIllum = IT_Specular;
        }

        // ORDER
        //
        // 1. ambient light or get spec power
        // 2. diffuse or spec directional lights
        // 3. diffuse or spec point lights
        // 4. diffuse or spec spot lights

        // 1. ambient light or get spec power
        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            rgShaderFuncs[O_Setup][itIllum]
            ));   

        {
            const UINT c_uNumParams = 1;
            MILSPHandle rghVertexShaderHandles[c_uNumParams];
#if DBG
            ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];
            oDbgTypes[0] = ShaderFunctionConstantData::Float4;
#endif

            GetShaderParameterHandles(
                rgShaderFuncs[O_Setup][itIllum],
                c_uNumParams,
                DBG_PARAM_COMMA(oDbgTypes)
                rghVertexShaderHandles,
                0,
                DBG_PARAM_COMMA(NULL)
                NULL
                );

            hFirstLightingConstant = rghVertexShaderHandles[0];
        }

        // 2. diffuse or spec directional lights
        for (UINT i = 0; i < uNumDirectionalLights; i++)
        {
            IFC(AddShaderPipelineItem(
                NULL,
                INVALID_PIPELINE_SAMPLER,
                MILVFAttrNone,
                rgShaderFuncs[O_Directional][itIllum]
                ));

            {
                const UINT c_uNumParams = 2;

#if DBG
                ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];

                oDbgTypes[0] = ShaderFunctionConstantData::Float4;
                oDbgTypes[1] = ShaderFunctionConstantData::Float3;
#endif

                // We don't care about the handles because we'll index off of
                // the first handle. We still need to call this method to
                // let the builder know what handles are in use.
                GetShaderParameterHandles(
                    rgShaderFuncs[O_Directional][itIllum],
                    c_uNumParams,
                    DBG_PARAM_COMMA(oDbgTypes)
                    NULL,
                    0,
                    DBG_PARAM_COMMA(NULL)
                    NULL
                    );    
            }
        }

        // 3. diffuse or spec point lights
        for (UINT i = 0; i < uNumPointLights; i++)
        {
            IFC(AddShaderPipelineItem(
                NULL,
                INVALID_PIPELINE_SAMPLER,
                MILVFAttrNone,
                rgShaderFuncs[O_Point][itIllum]
                ));  

            {
                const UINT c_uNumParams = 3;

#if DBG
                ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];

                oDbgTypes[0] = ShaderFunctionConstantData::Float4;
                oDbgTypes[1] = ShaderFunctionConstantData::Float4;
                oDbgTypes[2] = ShaderFunctionConstantData::Float4;
#endif

                // We don't care about the handles because we'll index off of
                // the first handle. We still need to call this method to
                // let the builder know what handles are in use.
                GetShaderParameterHandles(
                    rgShaderFuncs[O_Point][itIllum],
                    c_uNumParams,
                    DBG_PARAM_COMMA(oDbgTypes)
                    NULL,
                    0,
                    DBG_PARAM_COMMA(NULL)
                    NULL
                    );    
            }
        }

        // 4. diffuse or spec spot lights
        for (UINT i = 0; i < uNumSpotLights; i++)
        {
            IFC(AddShaderPipelineItem(
                NULL,
                INVALID_PIPELINE_SAMPLER,
                MILVFAttrNone,
                rgShaderFuncs[O_Spot][itIllum]
                ));  

            {
                const UINT c_uNumParams = 5;

#if DBG
                ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];

                oDbgTypes[0] = ShaderFunctionConstantData::Float4;
                oDbgTypes[1] = ShaderFunctionConstantData::Float4;
                oDbgTypes[2] = ShaderFunctionConstantData::Float4;
                oDbgTypes[3] = ShaderFunctionConstantData::Float3;
                oDbgTypes[4] = ShaderFunctionConstantData::Float4;
#endif

                // We don't care about the handles because we'll index off of
                // the first handle. We still need to call this method to
                // let the builder know what handles are in use.
                GetShaderParameterHandles(
                    rgShaderFuncs[O_Spot][itIllum],
                    c_uNumParams,
                    DBG_PARAM_COMMA(oDbgTypes)
                    NULL,
                    0,
                    DBG_PARAM_COMMA(NULL)
                    NULL
                    );    
            }
        }
    }
    else if (eLightingPass == CHwShader::LV_Emissive)
    {
        // Emissive and Ambient lighting calculations are equivalent.
        // In one the color source is the sum of ambient lights in the
        // scene.  In the other it comes from the material.
        IFC(AddShaderPipelineItem(
            NULL,
            INVALID_PIPELINE_SAMPLER,
            MILVFAttrNone,
            ShaderFunctions::CalcAmbientLighting
            ));   

        {
            const UINT c_uNumParams = 1;
            MILSPHandle rghVertexShaderHandles[c_uNumParams];
#if DBG
            ShaderFunctionConstantData::Enum oDbgTypes[c_uNumParams];
            oDbgTypes[0] = ShaderFunctionConstantData::Float4;
#endif
            GetShaderParameterHandles(
                ShaderFunctions::CalcAmbientLighting,
                c_uNumParams,
                DBG_PARAM_COMMA(oDbgTypes)
                rghVertexShaderHandles,
                0,
                DBG_PARAM_COMMA(NULL)
                NULL
                );

            hFirstLightingConstant = rghVertexShaderHandles[0];
        }

    }
    // else LV_None

    pLightingSource->SetFirstConstantParameter(hFirstLightingConstant);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::AddPipelineItem
//
//  Synopsis:
//      Adds an item to the pipeline as well as incrementing the cache.
//

HRESULT
CHwShaderPipelineBuilder::AddShaderPipelineItem(
    __in_ecount_opt(1) CHwColorSource *pHwColorSource,
    int iSampler,
    MilVertexFormatAttribute mvfaSourceLocation,
    ShaderFunctions::Enum oFunction
    )
{
    HRESULT hr = S_OK;

    HwPipelineItem *pEntry = NULL;

    IFC(m_pHPNoRef->AddPipelineItem(&pEntry));

    pEntry->dwStage             = ReserveCurrentStage();
    pEntry->pHwColorSource      = pHwColorSource;
    pEntry->dwSampler           = iSampler;
    pEntry->pFragment           = g_pHwHLSLShaderFunctions[oFunction];
    pEntry->mvfaTextureCoordinates  = mvfaSourceLocation;

    if (pEntry->pHwColorSource)
    {
        pEntry->pHwColorSource->AddRef();

        pEntry->pHwColorSource->ResetForPipelineReuse();
    }

    if (GetAvailableForGeneration() & pEntry->mvfaTextureCoordinates)
    {
        GenerateVertexAttribute(pEntry->mvfaTextureCoordinates);
    }

    IFC(m_pCache->AddOperation(
        *pEntry
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::IncrementShaderConstantPositions
//
//  Synopsis:
//      Returns handles to the shader parameters, updates the shader constant
//      registers, and verifies the types and expected number of parameters
//      match. function.
//

void
CHwShaderPipelineBuilder::GetShaderParameterHandles(
    __in_ecount(1) ShaderFunctions::Enum oFunction,
    UINT uNumVertexParameters,
    DBG_PARAM_COMMA(__in_ecount_opt(uNumVertexParameters) const ShaderFunctionConstantData::Enum *rgDbgVertexConstantDataTypes)
    __out_ecount_opt(uNumVertexParameters) MILSPHandle *rgVertexParameterHandles,
    UINT uNumPixelParameters,
    DBG_PARAM_COMMA(__in_ecount_opt(uNumPixelParameters) const ShaderFunctionConstantData::Enum *rgDbgPixelConstantDataTypes)
    __out_ecount_opt(uNumPixelParameters) MILSPHandle *rgPixelParameterHandles
    )
{
    const ShaderFunction *pShaderFunction = g_pHwHLSLShaderFunctions[oFunction];

    const VertexShaderFunction &oVertexFunc = pShaderFunction->VertexShader;
    const PixelShaderFunction  &oPixelFunc  = pShaderFunction->PixelShader;

    Assert(oVertexFunc.NumConstDataParameters == uNumVertexParameters);
    Assert(oPixelFunc.NumConstDataParameters == uNumPixelParameters);

    //
    // Vertex Function Parameters
    //
    for (UINT uVertexItem = 0; 
              uVertexItem < uNumVertexParameters; 
              uVertexItem++
              )
    {
        #if DBG
        Assert(oVertexFunc.rgConstDataParameter[uVertexItem].Type == rgDbgVertexConstantDataTypes[uVertexItem]);
        #endif

        if (rgVertexParameterHandles)
        {
            rgVertexParameterHandles[uVertexItem] = m_uVertexShaderCurrentConstantPosition;
        }

        m_uVertexShaderCurrentConstantPosition += 
            GetShaderConstantRegisterSize(oVertexFunc.rgConstDataParameter[uVertexItem].Type);
    }

    //
    // Pixel Function Parameters
    //
    for (UINT uPixelItem = 0;
              uPixelItem < uNumPixelParameters;
              uPixelItem++
              )
    {
        #if DBG
        Assert(oPixelFunc.rgConstDataParameter[uPixelItem].Type == rgDbgPixelConstantDataTypes[uPixelItem]);
        #endif

        if (rgPixelParameterHandles)
        {
            rgPixelParameterHandles[uPixelItem] = m_uPixelShaderCurrentConstantPosition;
        }

        m_uPixelShaderCurrentConstantPosition += 
            GetShaderConstantRegisterSize(oPixelFunc.rgConstDataParameter[uPixelItem].Type);
    }
}


#if NEVER
//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShaderPipelineBuilder::ReplacePiggybackedItem
//
//  Synopsis:
//      Find an alternate attribute to use for the piggybacking item to input
//      into the pixel shader.
//
//  Future Consideration:   Move calls to GetShaderParameterHandles post pipeline build
//   to enable pipeline functions to be changed.
//

void
CHwShaderPipelineBuilder::ReplacePiggybackedItem(
    )
{
    INT iPiggybackItem = GetAAPiggybackItem();

    Assert(iPiggybackItem != INVALID_PIPELINE_ITEM);

    HwPipelineItem *pEntry = m_pHPNoRef->m_rgItem[iPiggybackItem];

    CHwConstantColorSource *pConstantSource =
        DYNCAST(CHwConstantColorSource, pEntry->pHwColorSource);

    ShaderFunctions::Enum eFunction;

    // Cache operation can be left intact
    //HwPipelineOperation::Enum eCacheOperation;

    if (m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly)
    {
        eFunction  = ShaderFunctions::MultiplyAlpha_NonPremultiplied;
        //eCacheOperation = HwPipelineOperation::MultiplyAlphaOnly_NonPremultiplied;
    }
    else
    {
        eFunction  = ShaderFunctions::MultiplyAlpha;
        //eCacheOperation = HwPipelineOperation::MultiplyAlphaOnly;
    }

    pEntry->pFragment = g_pHwHLSLShaderFunctions[eFunction];

    Assert(pEntry->mvfaTextureCoordinates == m_mvfaAntiAliasScaleLocation);
    pEntry->mvfaTextureCoordinates = MILVFAttrNone;

    return;
}

#endif





