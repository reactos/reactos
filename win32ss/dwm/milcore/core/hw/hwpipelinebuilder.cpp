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
//      Contains implementation for CHwPipelineBuilder class.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//  Table:
//      sc_PipeOpProperties
//
//  Synopsis:
//      Table of HwBlendOp properties
//
//------------------------------------------------------------------------------

static const 
struct BlendOperationProperties {
    bool AllowsAlphaMultiplyInEarlierStage;
} sc_BlendOpProperties[] =
{   // HBO_SelectSource
    {
/* AllowsAlphaMultiplyInEarlierStage */ false
    },

    // HBO_Multiply
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },

    // HBO_SelectSourceColorIgnoreAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ false
    },

    // HBO_MultiplyColorIgnoreAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },

    // HBO_BumpMap
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
    
    // HBO_MultiplyByAlpha
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
    
    // HBO_MultiplyAlphaOnly
    {
/* AllowsAlphaMultiplyInEarlierStage */ true
    },
};

C_ASSERT(ARRAYSIZE(sc_BlendOpProperties)==HBO_Total);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwPipelineBuilder
//
//  Synopsis:
//      Helper class for CHwPipeline that does the actual construction of the
//      pipeline and to which other components interface
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Builder
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CHwPipelineBuilder::CHwPipelineBuilder(
    __in_ecount(1) CHwPipeline * const pHP,
    HwPipeline::Type oType
    )
    : m_pHP(pHP),
      m_oPipelineType(oType)
{
    Assert(pHP);

    m_iCurrentSampler = INVALID_PIPELINE_SAMPLER;
    m_iCurrentStage   = INVALID_PIPELINE_STAGE;

    m_mvfIn = MILVFAttrNone;
    m_mvfGenerated = MILVFAttrNone;

    m_fAntiAliasUsed = false;

    m_eAlphaMultiplyOp = HBO_Nop;

    m_iAlphaMultiplyOkayAtItem = INVALID_PIPELINE_STAGE;
    m_iLastAlphaScalableItem   = INVALID_PIPELINE_ITEM;

    m_iAntiAliasingPiggybackedByItem = INVALID_PIPELINE_ITEM;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::InitializePipelineMembers
//
//  Synopsis:
//      Figure out the alpha multiply operation and obtain vertex info.
//

void 
CHwPipelineBuilder::InitializePipelineMembers(
    MilCompositingMode::Enum eCompositingMode,
    __in_ecount(1) IGeometryGenerator const *pIGeometryGenerator
    )
{
    Assert(m_iCurrentSampler == INVALID_PIPELINE_SAMPLER);
    Assert(m_iCurrentStage   == INVALID_PIPELINE_STAGE);
    Assert(m_iAlphaMultiplyOkayAtItem  == INVALID_PIPELINE_STAGE);
    Assert(m_iLastAlphaScalableItem == INVALID_PIPELINE_STAGE);

    if (eCompositingMode == MilCompositingMode::SourceOverNonPremultiplied ||
        eCompositingMode == MilCompositingMode::SourceInverseAlphaOverNonPremultiplied)
    {
        m_eAlphaMultiplyOp = HBO_MultiplyAlphaOnly;
    }
    else
    {
        m_eAlphaMultiplyOp = HBO_Multiply;
    }    

    pIGeometryGenerator->GetPerVertexDataType(
        OUT m_mvfIn
        );

    m_mvfAvailable = MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrSpecular | MILVFAttrUV4;
    m_mvfAvailable &= ~m_mvfIn;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::SendPipelineOperations
//
//  Synopsis:
//      Construct a full rendering pipeline for the given context from scratch
//

HRESULT
CHwPipelineBuilder::SendPipelineOperations(
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext,
    __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator
    )
{
    HRESULT hr = S_OK;

    // Determine incoming per vertex data included with geometry.

    // Request primary color source to send primary rendering operations

    IFC(pIPCS->SendOperations(this));

    // Setup effects operations if any

    if (pIEffects)
    {
        IFC(ProcessEffectList(
            pIEffects,
            pEffectContext
            ));
    }

    IFC(pIGeometryGenerator->SendGeometryModifiers(this));
    IFC(pIGeometryGenerator->SendLighting(this));

    // Setup operations to handle clipping
    IFC(ProcessClip());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Set_BumpMap
//
//  Synopsis:
//      Take the given color source and set it as a bump map for the first
//      texture color source
//
//      This call must be followed by a Set_Texture call specifying the first
//      real color source.
//

HRESULT
CHwPipelineBuilder::Set_BumpMap(
    __in_ecount(1) CHwTexturedColorSource *pBumpMap
    )
{
    HRESULT hr = S_OK;

    // Parameter Assertions
    Assert(pBumpMap->GetSourceType() != CHwColorSource::Constant);

    IFC(E_NOTIMPL);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Mul_AlphaMask
//
//  Synopsis:
//      Add a blend operation that uses the given color source's alpha
//      components to scale previous rendering results
//

HRESULT
CHwPipelineBuilder::Mul_AlphaMask(
    __in_ecount(1) CHwTexturedColorSource *pAlphaMaskColorSource
    )
{
    HRESULT hr = S_OK;

    IFC(E_NOTIMPL);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessClip
//
//  Synopsis:
//      Set up clipping operations and/or resources
//

HRESULT
CHwPipelineBuilder::ProcessClip(
    )
{
    HRESULT hr = S_OK;


    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessEffectList
//
//  Synopsis:
//      Read the effect list and add pipeline operations for each one
//
//      This method and the ProcessXxxEffect helper methods make up the logical
//      Hardware Effects Processor component.
//
//  Responsibilities:
//      - Decode effects list to create color sources and specify operation
//        needed to pipeline
//
//  Not responsible for:
//      - Determining operation order or combining operations
//
//  Inputs required:
//      - Effects list
//      - Pipeline builder object (this)
//

HRESULT
CHwPipelineBuilder::ProcessEffectList(
    __in_ecount(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext *pEffectContext
    )
{
    HRESULT hr = S_OK;

    UINT cEntries = 0;

    // Get the count of the transform blocks in the effect object.
    IFC(pIEffects->GetCount(&cEntries));

    // Handle only alpha effects

    for (UINT uIndex = 0; uIndex < cEntries; uIndex++)
    {
        CLSID clsid;
        UINT cbSize;
        UINT cResources;

        IFC(pIEffects->GetCLSID(uIndex, &clsid));
        IFC(pIEffects->GetParameterSize(uIndex, &cbSize));
        IFC(pIEffects->GetResourceCount(uIndex, &cResources));

        if (clsid == CLSID_MILEffectAlphaScale)
        {
            IFC(ProcessAlphaScaleEffect(pIEffects, uIndex, cbSize, cResources));
        }
        else if (clsid == CLSID_MILEffectAlphaMask)
        {
            IFC(ProcessAlphaMaskEffect(pEffectContext, pIEffects, uIndex, cbSize, cResources));
        }
        else
        {
            IFC(WGXERR_UNSUPPORTED_OPERATION);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessAlphaScaleEffect
//
//  Synopsis:
//      Decode an alpha scale effect and add to pipeline
//

HRESULT
CHwPipelineBuilder::ProcessAlphaScaleEffect(
    __in_ecount(1) const IMILEffectList *pIEffects,
    UINT uIndex,
    UINT cbSize,
    UINT cResources
    )
{
    HRESULT hr = S_OK;

    CHwConstantAlphaScalableColorSource *pNewAlphaColorSource = NULL;

    AlphaScaleParams alphaScale;

    // check the parameter size
    if (cbSize != sizeof(alphaScale))
    {
        AssertMsg(FALSE, "AlphaScale parameter has unexpected size.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else if (cResources != 0)
    {
        AssertMsg(FALSE, "AlphaScale has unexpected number of resources.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    IFC(pIEffects->GetParameters(uIndex, cbSize, &alphaScale));

    if (0.0f > alphaScale.scale || alphaScale.scale > 1.0f)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else
    {

        IFC(CHwConstantAlphaScalableColorSource::Create(
            m_pHP->m_pDevice,
            alphaScale.scale,
            NULL,
            &m_pHP->m_dbScratch,
            &pNewAlphaColorSource
            ));

        IFC(Mul_ConstAlpha(pNewAlphaColorSource));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pNewAlphaColorSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ProcessAlphaMaskEffect
//
//  Synopsis:
//      Decode an alpha mask effect and add to pipeline
//

HRESULT
CHwPipelineBuilder::ProcessAlphaMaskEffect(
    __in_ecount(1) const CHwBrushContext *pEffectContext,
    __in_ecount(1) const IMILEffectList *pIEffects,
    UINT uIndex,
    UINT cbSize,
    UINT cResources
    )
{
    HRESULT hr = S_OK;

    AlphaMaskParams alphaMaskParams;

    IUnknown *pIUnknown = NULL;
    IWGXBitmapSource *pMaskBitmap = NULL;
    CHwTexturedColorSource *pMaskColorSource = NULL;
    
    CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> matBitmapToIdealRealization;

    CDelayComputedBounds<CoordinateSpace::RealizationSampling> rcRealizationBounds;

    BitmapToXSpaceTransform matRealizationToGivenSampleSpace;
    
    // check the parameter size
    if (cbSize != sizeof(alphaMaskParams))
    {
        AssertMsg(FALSE, "AlphaMask parameter has unexpected size.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }
    else if (cResources != 1)
    {
        AssertMsg(FALSE, "AlphaMask has unexpected number of resources.");
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    IFC(pIEffects->GetParameters(uIndex, cbSize, &alphaMaskParams));

    IFC(pIEffects->GetResources(uIndex, cResources, &pIUnknown));

    IFC(pIUnknown->QueryInterface(
            IID_IWGXBitmapSource,
            reinterpret_cast<void **>(&pMaskBitmap)));

    pEffectContext->GetRealizationBoundsAndTransforms(
        CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Effect>::ReinterpretBase(alphaMaskParams.matTransform),
        OUT matBitmapToIdealRealization,
        OUT matRealizationToGivenSampleSpace,
        OUT rcRealizationBounds
        );

    {
        CHwBitmapColorSource::CacheContextParameters oContextCacheParameters(
            MilBitmapInterpolationMode::Linear,
            pEffectContext->GetContextStatePtr()->RenderState->PrefilterEnable,
            pEffectContext->GetFormat(),
            MilBitmapWrapMode::Extend
            );

        IFC(CHwBitmapColorSource::DeriveFromBitmapAndContext(
            m_pHP->m_pDevice,
            pMaskBitmap,
            NULL,
            NULL,
            rcRealizationBounds,
            &matBitmapToIdealRealization,
            &matRealizationToGivenSampleSpace,
            pEffectContext->GetContextStatePtr()->RenderState->PrefilterThreshold,
            pEffectContext->CanFallback(),
            NULL,
            oContextCacheParameters,
            &pMaskColorSource
            ));

        IFC(Mul_AlphaMask(pMaskColorSource));
    }

Cleanup:

    ReleaseInterfaceNoNULL(pIUnknown);
    ReleaseInterfaceNoNULL(pMaskBitmap);
    ReleaseInterfaceNoNULL(pMaskColorSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::ChooseVertexBuilder
//
//  Synopsis:
//      Create a vertex builder for the current pipeline
//

HRESULT
CHwPipelineBuilder::ChooseVertexBuilder(
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
    )
{
    HRESULT hr = S_OK;

    MilVertexFormatAttribute mvfaAALocation = MILVFAttrNone;

    if (m_fAntiAliasUsed)
    {
        mvfaAALocation   = HWPIPELINE_ANTIALIAS_LOCATION;
    }

    Assert((m_mvfIn & m_mvfGenerated) == 0);

    IFC(CHwVertexBuffer::Builder::Create(
        m_mvfIn,
        m_mvfIn | m_mvfGenerated,
        mvfaAALocation,
        m_pHP,
        m_pHP->m_pDevice,
        &m_pHP->m_dbScratch,
        ppVertexBuilder
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::TryToMultiplyConstantAlphaToExistingStage
//
//  Synopsis:
//      Tries to find an existing stage it can use to drop it's alpha multiply
//      into.  Should work on both shader and fixed function pipelines.
//
//------------------------------------------------------------------------------
bool
CHwPipelineBuilder::TryToMultiplyConstantAlphaToExistingStage(
    __in_ecount(1) const CHwConstantAlphaColorSource *pAlphaColorSource
    )
{
    HRESULT hr = S_OK;

    float flAlpha = pAlphaColorSource->GetAlpha();
    CHwConstantAlphaScalableColorSource *pScalableAlphaSource = NULL;
    bool fStageToMultiplyFound = false;
    INT iItemCount = static_cast<INT>(m_pHP->m_rgItem.GetCount());

    // Parameter Assertions
    Assert(flAlpha >= 0.0f);
    Assert(flAlpha <= 1.0f);

    // Member Assertions

    // There should be at least one stage
    Assert(iItemCount > 0);
    Assert(GetNumReservedStages() > 0);

    // An alpha scale of 1.0 is a nop; do nothing
    if (flAlpha == 1.0f)
    {
        fStageToMultiplyFound = true;
        goto Cleanup;
    }

    int iLastAlphaScalableItem = GetLastAlphaScalableItem();
    int iItemAvailableForAlphaMultiply = GetEarliestItemAvailableForAlphaMultiply();

    //  We can add logic to recognize that an alpha scale of 0 would give us a
    //  completely transparent result and then "compress" previous stages.

    // Check for existing stage at which constant alpha scale may be applied
    if (iItemAvailableForAlphaMultiply < iItemCount)
    {
        // Check for existing color source that will handle the alpha scale
        if (iLastAlphaScalableItem >= iItemAvailableForAlphaMultiply)
        {
            Assert(m_pHP->m_rgItem[iLastAlphaScalableItem].pHwColorSource);

            // Future Consideration:   Shader pipe issue
            // The if statement around the Assert is to prevent the Assert from
            // firing on the shader path because the shader path does not set
            // eBlendOp. We can remove this if in the future when the shader
            // shader path uses the blend args.
            HwBlendOp hwBlendOp = m_pHP->m_rgItem[iLastAlphaScalableItem].eBlendOp;
            if (hwBlendOp == HBO_MultiplyAlphaOnly || hwBlendOp == HBO_Multiply)
            {
                Assert(hwBlendOp == m_eAlphaMultiplyOp);
            }
            
            // Multiply with new scale factor
            m_pHP->m_rgItem[iLastAlphaScalableItem].pHwColorSource->AlphaScale(flAlpha);

            fStageToMultiplyFound = true;
        }
        else
        {
            //
            // Check for existing color source that can be reused to handle the
            // alpha scale.  Alpha scale can be applied to any constant color
            // source using the ConstantAlphaScalable class.
            //
            // The scale should technically come at the end of the current
            // operations; so, try to get as close to the end as possible.
            //
    
            for (INT iLastConstant = iItemCount-1;
                 iLastConstant >= iItemAvailableForAlphaMultiply;
                 iLastConstant--)
            {
                CHwColorSource *pHCS =
                    m_pHP->m_rgItem[iLastConstant].pHwColorSource;
    
                if (pHCS && (pHCS->GetSourceType() & CHwColorSource::Constant))
                {
                    // The ConstantAlphaScalable class only supports
                    // HBO_Multiply because it assumes premulitplied colors come
                    // in and go out.
                    Assert(m_eAlphaMultiplyOp == HBO_Multiply);
    
                    //
                    // Inject an alpha scalable color source in place
                    // of the current constant color source.
                    //
    
                    IFC(CHwConstantAlphaScalableColorSource::Create(
                        m_pHP->m_pDevice,
                        flAlpha,
                        DYNCAST(CHwConstantColorSource, pHCS),
                        &m_pHP->m_dbScratch,
                        &pScalableAlphaSource
                        ));
    
                    // Transfer pScalableAlphaSource reference
                    m_pHP->m_rgItem[iLastConstant].pHwColorSource =
                        pScalableAlphaSource;
                    pHCS->Release();

                    //
                    // Color Sources being added to a pipeline are
                    // required to have their mappings reset.  This
                    // normally happens when items are added to the
                    // pipeline, but since this is replacing an item
                    // we need to call it ourselves.
                    //
                    pScalableAlphaSource->ResetForPipelineReuse();

                    pScalableAlphaSource = NULL;

                    // Remember this location now holds an
                    // alpha scalable color source
                    SetLastAlphaScalableStage(iLastConstant);

                    fStageToMultiplyFound = true;
                    break;
                }
            }
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(pScalableAlphaSource);

    //
    // We only want to consider success if our HRESULT is S_OK.
    //
    return (hr == S_OK && fStageToMultiplyFound);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::CheckForBlendAlreadyPresentAtAALocation
//
//  Synopsis:
//      We may have already added a blend operation using the location we're
//      going to generate anti-aliasing in.  If this is the case we don't need
//      to add another blend operation.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipelineBuilder::CheckForBlendAlreadyPresentAtAALocation(
    bool *pfNeedToAddAnotherStageToBlendAntiAliasing
    )
{
    HRESULT hr = S_OK;

    INT iAAPiggybackItem = GetAAPiggybackItem();

    *pfNeedToAddAnotherStageToBlendAntiAliasing = false;

    //
    // Validate that any AA piggybacking is okay.  If first location (item)
    // available for alpha multiply is greater than location of piggyback item,
    // then piggybacking is not allowed.
    //
    // AA piggyback item is -1 when not set so that case will also be detected.
    //

    if (iAAPiggybackItem < GetEarliestItemAvailableForAlphaMultiply())
    {
        //
        // Check if there was a piggyback item
        //
        if (iAAPiggybackItem != INVALID_PIPELINE_ITEM)
        {
            // Future Consideration:   Find new attribute for AA piggybacker
            //  and modify pipeline item with new properties.
            RIP("Fixed function pipeline does not expect invalid piggybacking");
            IFC(WGXERR_NOTIMPLEMENTED);
        }

        *pfNeedToAddAnotherStageToBlendAntiAliasing = true;
    }
    else
    {
        Assert(GetGeneratedComponents() & MILVFAttrDiffuse);
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::SetupVertexBuilder
//
//  Synopsis:
//      Choose the appropriate vertex builder class for the pipeline that has
//      just been set up and initialize the vertex builder
//

HRESULT
CHwPipelineBuilder::SetupVertexBuilder(
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBuilder
    )
{
    HRESULT hr = S_OK;

    // Select a vertex builder
    IFC(ChooseVertexBuilder(ppVertexBuilder));

    // Send vertex mappings for each color source
    HwPipelineItem *pItem = m_pHP->m_rgItem.GetDataBuffer();

    CHwVertexBuffer::Builder *pVertexBuilder = *ppVertexBuilder;

    if (VerticesArePreGenerated())
    {
        // Pass NULL builder to color source to indicate that vertices are
        // pre-generated and should not be modified.
        pVertexBuilder = NULL;
    }

    if (m_oPipelineType == HwPipeline::FixedFunction)
    {
        for (UINT uItem = 0; uItem < m_pHP->m_rgItem.GetCount(); uItem++, pItem++)
        {
            if (pItem->pHwColorSource)
            {
                IFC(pItem->pHwColorSource->SendVertexMapping(
                    pVertexBuilder,
                    pItem->mvfaSourceLocation
                    ));
            }
        }
    }
    else
    {
        for (UINT uItem = 0; uItem < m_pHP->m_rgItem.GetCount(); uItem++, pItem++)
        {
            if (pItem->pHwColorSource && pItem->mvfaTextureCoordinates != MILVFAttrNone)
            {
                IFC(pItem->pHwColorSource->SendVertexMapping(
                    pVertexBuilder,
                    pItem->mvfaTextureCoordinates
                    ));
            }
        }
    }

    // Let vertex builder know that is the end of the vertex mappings
    IFC((*ppVertexBuilder)->FinalizeMappings());

Cleanup:

    if (FAILED(hr))
    {
        delete *ppVertexBuilder;
        *ppVertexBuilder = NULL;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Mul_BlendColorsInternal
//
//  Synopsis:
//      Multiplies the pipeline by a set of blend colors.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipelineBuilder::Mul_BlendColors(
    __in_ecount(1) CHwColorComponentSource *pBlendColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(GetAvailableForReference() & MILVFAttrDiffuse);
    Assert(GetAAPiggybackItem() < GetEarliestItemAvailableForAlphaMultiply());

    IFC(Mul_BlendColorsInternal(
        pBlendColorSource
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipelineBuilder::Set_AAColorSource
//
//  Synopsis:
//      Adds an antialiasing colorsource.
//
//------------------------------------------------------------------------------
HRESULT
CHwPipelineBuilder::Set_AAColorSource(
    __in_ecount(1) CHwColorComponentSource *pAAColorSource
    )
{
    HRESULT hr = S_OK;

    //
    // Use Geometry Generator specified AA location (none, falloff, UV) to
    //   1) Append blend operation as needed
    //   2) Otherwise set proper indicators to vertex builder
    //

    Assert(pAAColorSource->GetComponentLocation() == CHwColorComponentSource::Diffuse);

    bool fNeedToAddAnotherStageToBlendAntiAliasing = true;

    IFC(CheckForBlendAlreadyPresentAtAALocation(
        &fNeedToAddAnotherStageToBlendAntiAliasing
        ));

    if (fNeedToAddAnotherStageToBlendAntiAliasing)
    {
        IFC(Mul_BlendColorsInternal(
            pAAColorSource
            ));
    }

    m_fAntiAliasUsed = true;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::Builder
//
//  Synopsis:
//      Create the fixed function pipeline builder.
//

CHwFFPipelineBuilder::CHwFFPipelineBuilder(
    __inout_ecount(1) CHwFFPipeline *pHP
    )
    : CHwPipelineBuilder(
        pHP,
        HwPipeline::FixedFunction
        )
{
    m_pHP = pHP;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::Setup
//
//  Synopsis:
//      Setup the fixed function pipeline for rendering
//

HRESULT
CHwFFPipelineBuilder::Setup(
    MilCompositingMode::Enum eCompositingMode,
    __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
    __inout_ecount(1) IHwPrimaryColorSource *pIPCS,
    __in_ecount_opt(1) const IMILEffectList *pIEffects,
    __in_ecount(1) const CHwBrushContext    *pEffectContext
    )
{
    HRESULT hr = S_OK;

    CHwPipelineBuilder::InitializePipelineMembers(
        eCompositingMode,
        pIGeometryGenerator
        );

    IFC(CHwPipelineBuilder::SendPipelineOperations(
        pIPCS,
        pIEffects,
        pEffectContext,
        pIGeometryGenerator
        ));

    FinalizeBlendOperations(
        eCompositingMode
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipeline::FinalizeBlendOperations
//
//  Synopsis:
//      Examine the pipeline after all the basic operations have been added and
//      make any adjustments to yield a valid pipeline
//

void
CHwFFPipelineBuilder::FinalizeBlendOperations(
    MilCompositingMode::Enum eCompositingMode
    )
{
    //
    // Assertions for the currently very limited pipeline
    //
    // Currently implemented pipeline operations are:
    //  Primary operation - from primary color source (required)
    //      Set_Constant                    
    //          or
    //      Set_Texture
    //
    //  Secondary operations - from secondary color source (optional)
    //      Mul_ConstAlpha 
    //      
    //  Tertiary operations
    //      SetupPerPrimitiveAntialiasingBlend (optional)
    //
    
    // There is always a primary operation so there should always be something
    // in the pipeline
    Assert(m_pHP->m_rgItem.GetCount() > 0);

    Assert(m_mvfIn == MILVFAttrXY 
        || m_mvfIn == (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV1) 
        || m_mvfIn == (MILVFAttrXYZ | MILVFAttrUV1)
        || m_mvfIn == (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV1 | MILVFAttrUV2)
        || m_mvfIn == (MILVFAttrXYZ | MILVFAttrUV1 | MILVFAttrUV2));

#if DBG
    MilVertexFormat mvfDbgUsed = GetAvailableForReference() | GetGeneratedComponents();

    Assert(
        // Set_Constant (+ Antialias)
        (mvfDbgUsed == (MILVFAttrXY | MILVFAttrDiffuse))
        // or Set_Texture
        || (mvfDbgUsed == (MILVFAttrXY | MILVFAttrUV1))       
        // or Set_Texture + (Mul_ConstAlpha | Antialias)
        || (mvfDbgUsed == (MILVFAttrXY | MILVFAttrDiffuse | MILVFAttrUV1))
        || (mvfDbgUsed == (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV1))
        // or Set_Texture + Mul_AlphaMask (with texture coords)
        || (mvfDbgUsed == (MILVFAttrXY | MILVFAttrUV1 | MILVFAttrUV2))
        // or Set_Texture + Mul_AlphaMask (with texture coords) + Antialias
        || (mvfDbgUsed == (MILVFAttrXY | MILVFAttrDiffuse | MILVFAttrUV1 | MILVFAttrUV2))
        );
#endif

    // At least one stage is guaranteed by the primary color source
    Assert(GetNumReservedStages() > 0);
    Assert(GetNumReservedSamplers() >= 0);

    if (GetNumReservedStages() == 1)
    {
        //
        // There is only one pipeline operation- (coming from Set_Constant or Set_Texture)
        //
        
        Assert(m_pHP->m_rgItem[0].dwStage == 0);
        if (m_pHP->m_rgItem[0].oBlendParams.hbaSrc1 == HBA_Texture)
        {
            Assert(m_pHP->m_rgItem[0].dwSampler == 0);
        }
        else
        {
            Assert(m_pHP->m_rgItem[0].dwSampler == INVALID_PIPELINE_SAMPLER);
        }

        Assert(m_pHP->m_rgItem[0].oBlendParams.hbaSrc2 == HBA_None);
        Assert(m_pHP->m_rgItem[0].eBlendOp == HBO_SelectSource 
            || m_pHP->m_rgItem[0].eBlendOp == HBO_SelectSourceColorIgnoreAlpha );
    }
    else
    {
        //
        // There are multiple pipeline items- see if we can combine several
        // color sources into the same stage
        //

        // This combination is much easier if we can assume that
        // the pipeline items are all re-orderable
        Assert(GetEarliestItemAvailableForAlphaMultiply() == 0);

        // The combination is further simplified knowing that we only
        // have two items to deal with
        Assert(GetNumReservedStages() <= 3);

        int iFirstNonTextureStage = INVALID_PIPELINE_STAGE;

#if DBG
        int iDbgNumTexturesEncountered = 0;
#endif

        //
        // Verifying the pipeline and looking for opportunities to consolidate it.
        // 
        // All items after the first stage are going to involve some sort of multiply, which
        // is going to take the current value and multiply it with another argument.
        // 
        // The first stage however, is going to be selecting one parameter.  This gives us
        // an opportunity to collapse one of the later stages into the first stage, taking it
        // from:
        // 
        // Stage diagram
        //                  Before           
        //           Stage 0                   Stage N            
        // Input 1      texture                    Diffuse        
        // Input 2      irrelevant                 Current        
        // Blend op     SelectSource (1)           Multiply       
        //
        //                  After           
        //           Stage 0
        // Input 1       Diffuse
        // Input 2       Texture
        // Blend op      Multiply
        //
        // It's easier for us to collapse a non-texture argument, because we don't have to
        // worry about setting another texture stage.  So while we validate the pipeline
        // we search for a non-texture argument.
        //

        // Future Consideration:  Could do further consolidation if stage 0 = diffuse && stage 1 = texture
        for (int iStage = 0; iStage < GetNumReservedStages(); iStage++)
        {
            HwPipelineItem const &curItem = m_pHP->m_rgItem[iStage];

            if (iStage == 0)
            {
                //
                // Our First stage should be selecting the source.
                //

                Assert(   curItem.eBlendOp == HBO_SelectSource
                       || curItem.eBlendOp == HBO_SelectSourceColorIgnoreAlpha
                          );
            }
            else
            {
                //
                // All non-first stages should involve a multiply
                //
                Assert(   curItem.eBlendOp == HBO_Multiply
                       || curItem.eBlendOp == HBO_MultiplyAlphaOnly
                       || curItem.eBlendOp == HBO_MultiplyColorIgnoreAlpha
                       || curItem.eBlendOp == HBO_MultiplyByAlpha
                          );

                Assert(curItem.oBlendParams.hbaSrc2 == HBA_Current);
            }

            if (curItem.oBlendParams.hbaSrc1 != HBA_Texture)
            {
                if (iFirstNonTextureStage == INVALID_PIPELINE_STAGE)
                {
                    iFirstNonTextureStage = iStage;
                }

                Assert(curItem.oBlendParams.hbaSrc1 == HBA_Diffuse);
            }
            else
            {
#if DBG
                Assert(curItem.dwSampler == static_cast<DWORD>(iDbgNumTexturesEncountered));                
                iDbgNumTexturesEncountered++;
#endif
            }
        }

        //
        // If we found a non-texture stage we can combine it with the 1st ("select")
        // stage.
        //
        
        if (iFirstNonTextureStage != INVALID_PIPELINE_STAGE)
        {
            HwBlendOp eNewBlendOp;
            HwBlendArg eNewBlendArg1;
            HwBlendArg eNewBlendArg2;

            HwPipelineItem &oFirstItem = m_pHP->m_rgItem[0];
            HwPipelineItem &oCollapsableItem = m_pHP->m_rgItem[iFirstNonTextureStage];

            //
            // We're taking the first stage from a select source to a multiply, so
            // determine which kind of multiply we need to do.
            //
            if (oFirstItem.eBlendOp == HBO_SelectSourceColorIgnoreAlpha)
            {
                Assert(oCollapsableItem.eBlendOp == HBO_Multiply);
                eNewBlendOp = HBO_MultiplyColorIgnoreAlpha;
            }
            else
            {
                eNewBlendOp = oCollapsableItem.eBlendOp;
            }

            eNewBlendArg1 = oCollapsableItem.oBlendParams.hbaSrc1;
            eNewBlendArg2 = oFirstItem.oBlendParams.hbaSrc1;

            oFirstItem.eBlendOp = eNewBlendOp;
            oFirstItem.oBlendParams.hbaSrc1 = eNewBlendArg1;
            oFirstItem.oBlendParams.hbaSrc2 = eNewBlendArg2;

            oCollapsableItem.eBlendOp = HBO_Nop;

            //
            // Decrease the stage number since we are using one less stage now
            //
            for (UINT i = iFirstNonTextureStage; static_cast<INT>(i) < GetNumReservedStages(); i++)
            {
                m_pHP->m_rgItem[i].dwStage--;
            }

            DecrementNumStages();
        }
    }
    
    //
    // Fix-up the need of SelectTextureIgnoreAlpha to have white as diffuse color
    // The vertex builder is required (expected) to have white as the
    // default value if nothing else has been specified.  We could
    // eliminate that requirement by adding a new solid white color
    // source to the pipe line item list.
    //

    if (    m_pHP->m_rgItem[0].eBlendOp == HBO_SelectSourceColorIgnoreAlpha 
         && m_pHP->m_rgItem[0].oBlendParams.hbaSrc1 == HBA_Texture
            )
    {
        if (GetAvailableForGeneration() & MILVFAttrDiffuse)
        {
            //
            // Make sure diffuse value gets set.  No color source should try
            // to use this location so it should default to solid white.
            //
            // We should only be here if we're rendering 2D aliased.
            //
            GenerateVertexAttribute(MILVFAttrDiffuse);
        }
    }

    //
    // Set first blend stage that should be disabled
    //

    m_pHP->m_dwFirstUnusedStage = GetNumReservedStages();

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
    // Setup composition mode
    //
    // Source over without transparency is equivalent to source copy, but
    // source copy is faster, so we check for it and promote the mode to
    // sourcecopy.
    //

    if (   eCompositingMode == MilCompositingMode::SourceOver
        && !m_fAntiAliasUsed
        && m_pHP->m_dwFirstUnusedStage == 1
        && (   (   m_pHP->m_rgItem[0].eBlendOp == HBO_SelectSource
                && m_pHP->m_rgItem[0].pHwColorSource->IsOpaque())
            || m_pHP->m_rgItem[0].eBlendOp == HBO_SelectSourceColorIgnoreAlpha)
       )
    {
        eCompositingMode = MilCompositingMode::SourceCopy;
    }

    m_pHP->SetupCompositionMode(
        eCompositingMode
        );

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::AddPipelineItem
//
//  Synopsis:
//      Adds a new pipeline item to the pipeline
//
//------------------------------------------------------------------------------
HRESULT
CHwFFPipelineBuilder::AddFFPipelineItem(
    HwBlendOp eBlendOp,
    HwBlendArg hbaSrc1,
    HwBlendArg hbaSrc2,
    MilVertexFormatAttribute mvfaSourceLocation,
    __in_ecount_opt(1) CHwColorSource *pHwColorSource
    )
{
    HRESULT hr = S_OK;

    // No-op is designed for use in and after finalize blend operations only
    Assert(eBlendOp != HBO_Nop);

    // If we not performing a blend, there is no need for src 2
    if (   eBlendOp == HBO_SelectSource 
        || eBlendOp == HBO_SelectSourceColorIgnoreAlpha)
    {
        Assert(hbaSrc2 == HBA_None);
    }

    // It is not possible to put two textures in one pipeline item
    // so let us enforce a convention that textures go in src 1
    Assert(hbaSrc2 != HBA_Texture);

    HwPipelineItem *pItem = NULL;

    IFC(m_pHP->AddPipelineItem(&pItem));
    Assert(pItem);

    pItem->dwStage = ReserveCurrentStage();

    if (hbaSrc1 == HBA_Texture)
    {
        // samplers are only needed for textures
        pItem->dwSampler = ReserveCurrentTextureSampler();
    }
    else
    {
        pItem->dwSampler = UINT_MAX;    // No sampler
    }

    pItem->eBlendOp = eBlendOp;
    pItem->oBlendParams.hbaSrc1 = hbaSrc1;
    pItem->oBlendParams.hbaSrc2 = hbaSrc2;

    // If the operation does not allow alpha multiply in earlier stage advance
    // tracking marker to this item (independent of whether the color sources
    // support alpha scaling.)
    if (!sc_BlendOpProperties[eBlendOp].AllowsAlphaMultiplyInEarlierStage)
    {
        SetLastItemAsEarliestAvailableForAlphaMultiply();
    }

    // Assert that the vertex attribute is not in use OR that we have special
    // case of reuse when the attribute is for texture and is already provided.
    // Having a constant source that does not truly require particular
    // coordinates is not good enough because the pipeline builder just isn't
    // prepared for the situation, which will likely result in three texture
    // stages and require TexCoordinateIndex different than stage.
    Assert(   (GetAvailableForGeneration() & mvfaSourceLocation)
           || (GetAvailableForReference()  & mvfaSourceLocation)
              );

    if (   (HWPIPELINE_ANTIALIAS_LOCATION == mvfaSourceLocation)
           // NULL pHwColorSource indicates addition of AA scale factor; so
           // skip piggyback marking for it.
        && pHwColorSource
        )
    {
        SetLastItemAsAAPiggyback();
    }

    if (GetAvailableForGeneration() & mvfaSourceLocation)
    {
        Assert(!(GetAvailableForReference() & mvfaSourceLocation));
        GenerateVertexAttribute(mvfaSourceLocation);
    }

    pItem->mvfaSourceLocation = mvfaSourceLocation;

    pItem->pHwColorSource = pHwColorSource;


    // This Addref will be handled by the base pipeline builder

    if (pHwColorSource)
    {
        pHwColorSource->AddRef();
        pHwColorSource->ResetForPipelineReuse();
    }


Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::Set_Constant
//
//  Synopsis:
//      Takes the given color source and sets it as the first color source for
//      the hardware blending pipeline
//

HRESULT
CHwFFPipelineBuilder::Set_Constant(
    __in_ecount(1) CHwConstantColorSource *pConstant
    )
{
    HRESULT hr = S_OK;

    // Parameter Assertions
    Assert(pConstant->GetSourceType() & CHwColorSource::Constant);

    // Member Assertions

    // There shouldn't be any items or stages yet
    Assert(m_pHP->m_rgItem.GetCount() == 0);
    Assert(GetEarliestItemAvailableForAlphaMultiply() == INVALID_PIPELINE_ITEM);

    Assert(GetNumReservedStages() == 0);
    Assert(GetNumReservedSamplers() == 0);

    MilVertexFormatAttribute mvfa;
    HwBlendArg hba;

    //
    // Find an acceptable vertex field
    //

    if (GetAvailableForGeneration() & MILVFAttrDiffuse)
    {
        mvfa = MILVFAttrDiffuse;
        hba = HBA_Diffuse;
    }
    else
    {
        //
        // Future Consideration:   Use a alpha scale texture stage instead.
        //
        // Setting the a texture stage to be an alpha scale value should be
        // supported on all our hardware and should be more efficient than
        // using a texture.
        //

        // Required for logic to work
        Assert(GetAvailableForReference() & MILVFAttrUV1);

        mvfa = MILVFAttrUV1;
        hba = HBA_Texture;
    }

    //
    // Add the first color source
    //        

    IFC(AddFFPipelineItem(
        HBO_SelectSource,
        hba,
        HBA_None,
        mvfa,
        pConstant
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::Set_Texture
//
//  Synopsis:
//      Takes the given color source and sets it as the first color source for
//      the hardware blending pipeline
//
//      If it is to be bump mapped the bump map operation has to specified by a
//      call to Set_BumpMap just before this call.
//

HRESULT
CHwFFPipelineBuilder::Set_Texture(
    __in_ecount(1) CHwTexturedColorSource *pTexture
    )
{
    HRESULT hr = S_OK;

    // Parameter Assertions
    Assert(pTexture->GetSourceType() != CHwColorSource::Constant);

    // Member Assertions

    // There shouldn't be any items or stages yet
    Assert(m_pHP->m_rgItem.GetCount() == 0);
    Assert(GetEarliestItemAvailableForAlphaMultiply() == INVALID_PIPELINE_ITEM);
    Assert(GetNumReservedStages() == 0);
    Assert(GetNumReservedSamplers() == 0);

    //
    // Add the first color source
    //

    //
    // Future Consideration:  Seperate IgnoreAlpha BlendOp into multiple items
    // 
    // This is dangerous.  Select Source Color Ignore Alpha says it's the first stage,
    // but its texture states specify that it's going to grab alpha from current. 
    // This works because specifying current on stage 0 will draw from diffuse, and
    // we make sure to always fill diffuse.
    // 
    // If the pipeline supports more rendering operations especially ones that don't
    // allow re-ordering of the stages, we may have to break 
    // HBO_SelectSourceColorIgnoreAlpha into more than one stage.
    //

    IFC(AddFFPipelineItem(
        HBO_SelectSource,
        HBA_Texture,
        HBA_None,
        MILVFAttrUV1,
        pTexture
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipelineBuilder::Set_RadialGradient
//
//  Synopsis:
//      Not implemented in the fixed function pipeline
//
//------------------------------------------------------------------------------
HRESULT
CHwFFPipelineBuilder::Set_RadialGradient(
    __in_ecount(1) CHwRadialGradientColorSource *pRadialGradient
    )
{
    RRETURN(E_NOTIMPL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwPipeline::FFBuilder::Mul_ConstAlpha
//
//  Synopsis:
//      Add a blend operation that scales all previous rendering by the given
//      alpha value
//
//      This operation may be added as a modifier to an existing color source or
//      as an independent operation.  If added via modification to an existing
//      color source then the results of the pipeline should be respected just
//      as if it were added as a new operation.
//

HRESULT
CHwFFPipelineBuilder::Mul_ConstAlpha(
    CHwConstantAlphaColorSource *pAlphaColorSource
    )
{
    HRESULT hr = S_OK;
    float flAlpha = pAlphaColorSource->GetAlpha();

    // There should be at least one item that has marked available alpha mul
    Assert(m_pHP->m_rgItem.GetCount() > 0);
    Assert(GetEarliestItemAvailableForAlphaMultiply() >= 0);

    CHwConstantAlphaScalableColorSource *pScalableAlphaSource = NULL;

    if (TryToMultiplyConstantAlphaToExistingStage(pAlphaColorSource))
    {
        //
        // We've succeeded in multiplying the alpha color source to an existing
        // stage, so early out.
        //
        goto Cleanup;
    }

    //
    // There is no color source available to apply this scale to directly.
    // Add an additional blending stage.
    //

    MilVertexFormatAttribute mvfa = MILVFAttrNone;
    HwBlendArg hba = HBA_None;

    //
    // Find an acceptable vertex field
    //

    if (GetAvailableForGeneration() & MILVFAttrDiffuse)
    {
        mvfa = MILVFAttrDiffuse;
        hba = HBA_Diffuse;
    }
    else if (GetAvailableForReference() & MILVFAttrUV1)
    {
        // Piggyback on a texture coordinate set that is already requested.
        mvfa = MILVFAttrUV1;
        hba = HBA_Texture;
    }
    else if (GetAvailableForGeneration() & MILVFAttrSpecular)
    {
        mvfa = MILVFAttrSpecular;
        hba = HBA_Specular;
    }

    if (mvfa != MILVFAttrNone)
    {
        //
        // Append alpha scale blend operation
        //

        IFC(CHwConstantAlphaScalableColorSource::Create(
            m_pHP->m_pDevice,
            flAlpha,
            NULL,   // No orignal color source
            &m_pHP->m_dbScratch,
            &pScalableAlphaSource
            ));

        IFC(AddFFPipelineItem(
            m_eAlphaMultiplyOp,
            hba,
            HBA_Current,
            mvfa,
            pScalableAlphaSource
            ));

        // Remember this location holds an alpha scalable color source
        SetLastItemAsAlphaScalable();
    }
    else
    {
        // No suitable vertex location could be found
        IFC(E_NOTIMPL);
    }

Cleanup:
    ReleaseInterfaceNoNULL(pScalableAlphaSource);
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipelineBuilder::Mul_AlphaMask
//
//  Synopsis:
//

HRESULT
CHwFFPipelineBuilder::Mul_AlphaMask(
    __in_ecount(1) CHwTexturedColorSource *pAlphaMask
    )
{
    HRESULT hr = S_OK;

    // There should be at least one item that has marked available alpha mul
    Assert(m_pHP->m_rgItem.GetCount() > 0);
    Assert(GetEarliestItemAvailableForAlphaMultiply() >= 0);

    Assert(    m_eAlphaMultiplyOp == HBO_Multiply
            || m_eAlphaMultiplyOp == HBO_MultiplyAlphaOnly );

    HwBlendOp blendop = m_eAlphaMultiplyOp;
    if (blendop == HBO_Multiply)
    {
        blendop = HBO_MultiplyByAlpha;
    }

    MilVertexFormatAttribute mvfaSource = VerticesArePreGenerated() ?
                                          MILVFAttrUV1 :
                                          MILVFAttrUV2;
    IFC(AddFFPipelineItem(
        blendop,
        HBA_Texture,
        HBA_Current,
        mvfaSource,
        pAlphaMask
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
//      CHwFFPipelineBuilder::Add_Lighting
//
//  Synopsis:
//      Adds an adds a lighting colorsource.
//
//------------------------------------------------------------------------------
HRESULT
CHwFFPipelineBuilder::Add_Lighting(
    __in_ecount(1) CHwLightingColorSource *pLightingSource
    )
{
    HRESULT hr = S_OK;

    IFC(AddFFPipelineItem(
        m_eAlphaMultiplyOp,
        HBA_Diffuse,
        HBA_Current,
        MILVFAttrDiffuse,
        pLightingSource
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFPipelineBuilder::Mul_BlendColors
//
//  Synopsis:
//      Multiplies the pipeline by a set of blend colors.
//
//------------------------------------------------------------------------------
HRESULT
CHwFFPipelineBuilder::Mul_BlendColorsInternal(
    __in_ecount(1) CHwColorComponentSource *pBlendColorSource
    )
{
    HRESULT hr = S_OK;

    HwBlendArg hbaParam1;
    MilVertexFormatAttribute mvfaSource;

    CHwColorComponentSource::VertexComponent eLocation = pBlendColorSource->GetComponentLocation();

    switch(eLocation)
    {
        case CHwColorComponentSource::Diffuse:
        {
            hbaParam1  = HBA_Diffuse;
            mvfaSource = MILVFAttrDiffuse;
        }
        break;

        case CHwColorComponentSource::Specular:
        {
            hbaParam1  = HBA_Specular;
            mvfaSource = MILVFAttrSpecular;
        }
        break;

        default:
            NO_DEFAULT("Unknown Color Component Source");
    }

    IFC(AddFFPipelineItem(
        m_eAlphaMultiplyOp,
        hbaParam1,
        HBA_Current,
        mvfaSource,
        pBlendColorSource
        ));

Cleanup:
    RRETURN(hr);
}






