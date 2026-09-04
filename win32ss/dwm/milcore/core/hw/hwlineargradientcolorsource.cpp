// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwLinearGradientColorSource implementation
//

#include "precomp.hpp"

MtDefine(CHwLinearGradientColorSource, MILRender, "CHwLinearGradientColorSource");

//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::Create
//
//  Synopsis:  Creates a HW linear gradient color source
//
//-----------------------------------------------------------------------------
HRESULT
CHwLinearGradientColorSource::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CHwLinearGradientColorSource **ppHwLinGradCS
    )
{
    HRESULT hr = S_OK;

    *ppHwLinGradCS = new CHwLinearGradientColorSource(pDevice);
    IFCOOM(*ppHwLinGradCS);
    (*ppHwLinGradCS)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        Assert(*ppHwLinGradCS == NULL);
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::CHwLinearGradientColorSource
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CHwLinearGradientColorSource::CHwLinearGradientColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) :
    CHwTexturedColorSource(pDevice)
{
    m_cDesiredTextureWidth = UINT_MAX;  // Unreasonable->invalid default
    m_cRealizedTextureWidth = UINT_MAX; // Unreasonable->invalid default
    m_pGradBrushNoRef = NULL;
    m_fColorsNeedUpdating = true; 
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::~CHwLinearGradientColorSource
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CHwLinearGradientColorSource::~CHwLinearGradientColorSource()
{
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::SetBrushAndContext
//
//  Synopsis:  Set the current context and brush this color source is to
//             realize
//
//-----------------------------------------------------------------------------
HRESULT
CHwLinearGradientColorSource::SetBrushAndContext(
    __in_ecount(1) CMILBrushGradient *pGradBrush,
    __in_ecount(1) const CBaseMatrix *pmatWorld2DToSampleSpace,
    __in_ecount(1) const CContextState *pContextState
    )
{
    HRESULT hr = S_OK;

    CMILMatrix matSampleSpaceToTexture;
    CMILMatrix matXSpaceToTexture;

    D3DTEXTUREADDRESS taU;

    //
    // This brush is not referenced- we expect its lifetime to be
    // longer than it's use here the linear gradient color source.
    //
    m_pGradBrushNoRef = pGradBrush;

    //
    // Calculate & Set XSpace->Source Matrix
    //

    // Calculate matrix & size
    MilPoint2F ptsGradient[3];
    pGradBrush->GetEndPoints(
        &ptsGradient[0],
        &ptsGradient[1],
        &ptsGradient[2]
        );

    IFC(CGradientTextureGenerator::CalculateTextureSizeAndMapping(
        &ptsGradient[0],
        &ptsGradient[1],
        &ptsGradient[2],
        CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::ReinterpretBase(pmatWorld2DToSampleSpace),
        pGradBrush->IsRadial(),
        m_pGradBrushNoRef->GetWrapMode(),
        TRUE,   // HW uses normalized [0,1] texture coordinates
        &m_gradientSpanInfo,
        &matSampleSpaceToTexture // pmatSampleSpaceToTextureMaybeNormalized
        ));

    m_cDesiredTextureWidth = m_gradientSpanInfo.GetTexelCount();

    Assert(m_cDesiredTextureWidth <= MAX_GRADIENTTEXEL_COUNT);

    // Calculate World2DToTexture
    m_matWorld2DToTexture.SetToMultiplyResult(    // World->Texture = 
        *pmatWorld2DToSampleSpace,                // World->Sample *
        matSampleSpaceToTexture                   // Sample->Texture
        );

    // Calculate XSpaceToTexture
    const CoordinateSpaceId::Enum SourceCoordSpace =
        pContextState->GetSamplingSourceCoordSpace();
    if (SourceCoordSpace == CoordinateSpaceId::Device)
    {
        matXSpaceToTexture = matSampleSpaceToTexture;
    }   
    else
    {
        Assert(SourceCoordSpace == CoordinateSpaceId::BaseSampling);
        matXSpaceToTexture = m_matWorld2DToTexture;
    }

    //
    // Set m_matXSpaceToTextureUV
    //

    // Convert CMILMatrix to MILMatrix3x2
    {
        m_matXSpaceToTextureUV.m_00 = matXSpaceToTexture._11;
        m_matXSpaceToTextureUV.m_10 = matXSpaceToTexture._21;
        m_matXSpaceToTextureUV.m_20 = matXSpaceToTexture._41;
    
        if (pGradBrush->IsRadial())
        {
            // We need the V coordinate
            m_matXSpaceToTextureUV.m_01 = matXSpaceToTexture._12;
            m_matXSpaceToTextureUV.m_11 = matXSpaceToTexture._22;
            m_matXSpaceToTextureUV.m_21 = matXSpaceToTexture._42;
        }
        else
        {
            // V coordinate is always 0 since the texture's height is always one texel
            m_matXSpaceToTextureUV.m_01 = 0.0f;
            m_matXSpaceToTextureUV.m_11 = 0.0f;
            m_matXSpaceToTextureUV.m_21 = 0.0f;
        }
    }

    // Reset shader handle for this context use
    ResetShaderTextureTransformHandle();

    // Mark matrix as set
#if DBG
    XSpaceDefinition dbgXSpaceDefinition = (SourceCoordSpace == CoordinateSpaceId::Device)
        ? XSpaceIsSampleSpace
        : XSpaceIsWorldSpace;
    DbgMarkXSpaceToTextureUVAsSet(dbgXSpaceDefinition);
#endif

    //
    // Set filter & wrap modes
    //
    
    switch (m_pGradBrushNoRef->GetWrapMode())
    {

    // Investigate using flip mode instead of duplicating texels.
    case MilGradientWrapMode::Flip:
    case MilGradientWrapMode::Tile:
        taU = D3DTADDRESS_WRAP;
        break;

    case MilGradientWrapMode::Extend:
        taU = D3DTADDRESS_CLAMP;
        break;

    default:
        NO_DEFAULT("Unrecognized wrap mode");
    }

    //
    // Future Consideration:  PERF: Investigate setting v sampler state only when necessary
    // Linear gradients still render fine when rendered with a v sampler state of anything but
    // border. They would render correctly with border as well so long as we could guarantee
    // that the v coordinate was always exactly 0.5. This would be risky though.
    //
    // Non-clamp would also be a problem if the U mode were clamp, but not a
    // power of two, because D3D/drivers don't support conditional non-power of
    // two in one texture direction, but not the other.  This is found per
    // testing by DanWo around 2005/05/24 - see
    // CHwBitmapColorSource::ReconcileLayouts.
    //

    SetFilterAndWrapModes(
        MilBitmapInterpolationMode::Linear,
        taU,
        D3DTADDRESS_CLAMP
        );

Cleanup:

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::FillGradientTexture
//
//  Synopsis:  Copies the gradient texels over to the texture
//
//-----------------------------------------------------------------------------
HRESULT
CHwLinearGradientColorSource::FillGradientTexture(
    __in_ecount(1) CGradientColorData const &oColorData,
    MilGradientWrapMode::Enum wrapMode,
    MilColorInterpolationMode::Enum colorInterpolationMode,
    BOOL fIsRadial
    )
{
    HRESULT hr = S_OK;
    D3DLOCKED_RECT rectTexture;

    BOOL fLockedTexture = FALSE;

    Assert(m_cRealizedTextureWidth >= 1);
    Assert(m_cRealizedTextureWidth <= MAX_GRADIENTTEXEL_COUNT);

    IFC(m_vidMemManager.ReCreateAndLockSysMemSurface(
        &rectTexture
        ));

    fLockedTexture = TRUE;
    if (rectTexture.Pitch < static_cast<INT>(m_cRealizedTextureWidth * D3DFormatSize(D3DFMT_A8R8G8B8)))
    {
        RIPW(L"Unexpected: The texture created is not big enough for the gradient.");
        IFC(WGXERR_INSUFFICIENTBUFFER);
    }

    IFC(CGradientTextureGenerator::GenerateGradientTexture(
        oColorData.GetColorsPtr(),
        oColorData.GetPositionsPtr(),
        oColorData.GetCount(),
        fIsRadial,
        wrapMode,
        colorInterpolationMode,
        &m_gradientSpanInfo,
        m_cRealizedTextureWidth,
        static_cast<MilColorB*>(rectTexture.pBits)
        ));

Cleanup:

    if (fLockedTexture)
    {
        MIL_THR_SECONDARY(m_vidMemManager.UnlockSysMemSurface());
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::IsOpaque
//
//  Synopsis:  Does the source contain alpha?  This method tells you.
//
//-----------------------------------------------------------------------------

bool
CHwLinearGradientColorSource::IsOpaque(
    ) const
{
    CGradientColorData const *pColorData = m_pGradBrushNoRef->GetColorData();
    MilColorF const *pColor = pColorData->GetColorsPtr();
    UINT uColors = pColorData->GetCount();

    bool fOpaque = true;

    while (uColors > 0)
    {
        // Note this comparison is too restrictive for sRGB which has less
        // granularity and is considered opaque at values less than 1.
        if (pColor->a < 1.0f)
        {
            fOpaque = false;
            break;
        }

        pColor++;
        uColors--;
    }

    return fOpaque;
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::InvalidateRealization
//
//  Synopsis:  Mark realization as invalid; simply release any HW resource
//
//-----------------------------------------------------------------------------
void
CHwLinearGradientColorSource::InvalidateRealization()
{
    m_fColorsNeedUpdating = true;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::Realize
//
//  Synopsis:  Create or get a realization of the current device independent
//             brush.  If already in the cache, just make sure the current
//             realization still works in this context.
//
//-----------------------------------------------------------------------------
HRESULT
CHwLinearGradientColorSource::Realize(
    )
{
    HRESULT hr = S_OK;

    bool fVidMemTextureNeedsUpdating = false;

    Assert(m_pGradBrushNoRef);

    //
    // Check to see if the size changed
    //
    // Creating a texture the exact size calculated by
    // CGradientTextureGenerator::CalculateTextureSizeAndMapping is required to
    // avoid more texels mapping to a pixel than can be properly handled by
    // bilinear filtering.
    //
    if (   m_vidMemManager.HasRealizationParameters()
        && m_cDesiredTextureWidth != m_cRealizedTextureWidth
       )
    {
        // In order to resize the video memory manager we need to prepare for a
        // new realization and then call SetRealizationParameters again
        m_vidMemManager.PrepareForNewRealization();
    }

    if (!m_vidMemManager.HasRealizationParameters())
    {
        //
        // Create the texture manager
        //
        
        TextureMipMapLevel eMipMapLevel = TMML_One; // for now...

        m_vidMemManager.SetRealizationParameters(
            m_pDevice,
            D3DFMT_A8R8G8B8,
            m_cDesiredTextureWidth,
            1, // uHeight
            eMipMapLevel
            DBG_COMMA_PARAM(TextureAddressingAllowsConditionalNonPower2Usage(GetTAU(), GetTAV()))
            );

        m_cRealizedTextureWidth = m_cDesiredTextureWidth;
    }

    // We should have created the texture manager by now.
    Assert(m_vidMemManager.HasRealizationParameters());

    if (m_fColorsNeedUpdating
        || !m_vidMemManager.IsSysMemSurfaceValid()
        )
    {
        //
        // Populate the texture
        //

        IFC(FillGradientTexture(
            *m_pGradBrushNoRef->GetColorData(),
            m_pGradBrushNoRef->GetWrapMode(),
            m_pGradBrushNoRef->GetColorInterpolationMode(),
            m_pGradBrushNoRef->IsRadial()
            ));

        fVidMemTextureNeedsUpdating = true;

        // Successful population means that the colors no longer need updating
        m_fColorsNeedUpdating = false;
    }

    // We should have ensured that the system memory surface is valid by
    // now.
    Assert(!m_fColorsNeedUpdating);

    //
    // Check to see if we need to re-realize the video memory texture
    //
    if (   fVidMemTextureNeedsUpdating
        || m_vidMemManager.GetVidMemTextureNoRef() == NULL)
    {
        IFC(m_vidMemManager.PushBitsToVidMemTexture());
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientColorSource::SendDeviceStates
//
//  Synopsis:  Send related texture states to device
//
//-----------------------------------------------------------------------------
HRESULT
CHwLinearGradientColorSource::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    Assert(!m_fColorsNeedUpdating);
    Assert(m_vidMemManager.HasRealizationParameters());
    Assert(m_vidMemManager.GetVidMemTextureNoRef());

    IFC(CHwTexturedColorSource::SendDeviceStates(
        dwStage,
        dwSampler
        ));

    IFC(m_pDevice->SetTexture(dwSampler, m_vidMemManager.GetVidMemTextureNoRef()));

Cleanup:
    RRETURN(hr);
}




