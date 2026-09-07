// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Contains the definition of CBrushRealizer
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CBrushResourceRealizer, MILApi, "CBrushResourceRealizer");
MtDefine(CImmediateBrushRealizer, MILApi, "CImmediateBrushRealizer");

#if DBG
void DbgAssertEffectWellKnown(__in_ecount_opt(1) IMILEffectList *pEffect);
#else
void DbgAssertEffectWellKnown(__in_ecount_opt(1) IMILEffectList *) {}
#endif

//+-----------------------------------------------------------------------------
//
//  Function:
//      GetAlphaScaleFromEffect
//
//  Synopsis:
//      Utility method to get an alpha scale from the effect list. this method
//      is simplified by assuming that the effect is an alpha scale and nothing
//      more. Callers should beware that anything else will assert.
//

HRESULT
GetAlphaScaleFromEffect(
    __in_ecount_opt(1) const IMILEffectList *pEffect,
    __out_ecount(1) float *pflAlphaScale
    )
{
    HRESULT hr = S_OK;

    UINT cEntries = 0;
    *pflAlphaScale = 1.0f;

    if (pEffect == NULL)
    {
        goto Cleanup;
    }

    // Get the count of the transform blocks in the effect object.
    IFC(pEffect->GetCount(&cEntries));
    Assert(cEntries <= 1);

    if (cEntries == 1)
    {
        #if DBG
        {
            CLSID clsid;
            IGNORE_HR(pEffect->GetCLSID(0, &clsid));
            Assert(clsid == CLSID_MILEffectAlphaScale);
        }
        #endif DBG
    
        AlphaScaleParams alphaScale;

        // check the parameter size
        IFC(pEffect->GetParameters(0, sizeof(alphaScale), &alphaScale));

        *pflAlphaScale = alphaScale.scale;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::CreateResourceRealizer
//
//  Synopsis:
//      Creates a brush realizer from a UCE resource and brush context
//

HRESULT
CBrushRealizer::CreateResourceRealizer(
    __inout_ecount(1) CMilBrushDuce *pBrushResource,
    __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    CBrushResourceRealizer *pBrushRealizer = NULL;

    pBrushRealizer = new CBrushResourceRealizer(
        pBrushResource
        );
    IFCOOM(pBrushRealizer);
    pBrushRealizer->AddRef();

    *ppBrushRealizer = pBrushRealizer;
    pBrushRealizer = NULL; // steal ref

Cleanup:
    ReleaseInterfaceNoNULL(pBrushRealizer);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::CreateImmediateRealizer
//
//  Synopsis:
//      Creates a brush realizer from a mil brush and effect list
//
//      Callers who know that their brush has no meta-intermediate can set the
//      "fSkipMetaFixups" flag to true to skip the QueryInterface
//

HRESULT
CBrushRealizer::CreateImmediateRealizer(
    __in_ecount(1) CMILBrush *pMILBrush,
    __in_ecount_opt(1) IMILEffectList *pIEffect,
    bool fSkipMetaFixups,
    __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    CImmediateBrushRealizer *pBrushRealizer = NULL;

    pBrushRealizer = new CImmediateBrushRealizer();
    IFCOOM(pBrushRealizer);
    pBrushRealizer->AddRef();

    pBrushRealizer->SetMILBrush(
        pMILBrush,
        pIEffect,
        fSkipMetaFixups
        );

    *ppBrushRealizer = pBrushRealizer;
    pBrushRealizer = NULL; // steal ref

Cleanup:
    ReleaseInterfaceNoNULL(pBrushRealizer);

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::CreateImmediateRealizer
//
//  Synopsis:
//      Creates a brush realizer from a solid color
//

HRESULT
CBrushRealizer::CreateImmediateRealizer(
    __in_ecount(1) const MilColorF *pColor,
    __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    CImmediateBrushRealizer *pBrushRealizer = NULL;

    pBrushRealizer = new CImmediateBrushRealizer();
    IFCOOM(pBrushRealizer);
    pBrushRealizer->AddRef();
    
    pBrushRealizer->SetSolidColorBrush(pColor);

    *ppBrushRealizer = pBrushRealizer;
    pBrushRealizer = NULL; // steal ref

Cleanup:
    ReleaseInterfaceNoNULL(pBrushRealizer);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::CreateNullBrush
//
//  Synopsis:
//      Creates a brush which will skip realization, producing a NULL brush
//      instead. This is useful for the bounds render target.
//

HRESULT
CBrushRealizer::CreateNullBrush(
    __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    CImmediateBrushRealizer *pBrushRealizer = NULL;

    pBrushRealizer = new CImmediateBrushRealizer();
    IFCOOM(pBrushRealizer);
    pBrushRealizer->AddRef();

    *ppBrushRealizer = pBrushRealizer;
    pBrushRealizer = NULL; // steal ref

Cleanup:
    ReleaseInterfaceNoNULL(pBrushRealizer);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::GetOpacityFromRealizedBrush
//
//  Synopsis:
//      Gets the opacity from the realized brush.
//

float
CBrushRealizer::GetOpacityFromRealizedBrush()
{
    float opacity;

    if (m_pRealizedBrush && m_pRealizedBrush->GetType() == BrushBitmap)
    {
        opacity = DYNCAST(CMILBrushBitmap, m_pRealizedBrush)->GetOpacity();
    }
    else
    {
        opacity = 1.0f;
    }

    return opacity;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::CBrushRealizer
//
//  Synopsis:
//      ctor
//

CBrushRealizer::CBrushRealizer()
{
    m_pRealizedBrush = NULL;
    m_pBrushMetaBitmapRT = NULL;

    MilColorF colTransparent = {0.0f, 0.0f, 0.0f, 0.0f};

    m_solidColorBrush.SetColor(&colTransparent); 
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::~CBrushRealizer
//
//  Synopsis:
//      dtor
//

CBrushRealizer::~CBrushRealizer()
{
    ReleaseInterfaceNoNULL(m_pRealizedBrush);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::SetRealizedBrush
//
//  Synopsis:
//      Sets the realized brush, preparing for meta intermediate adjustment
//
//      Callers who know that their brush has no meta-intermediate can set the
//      "fSkipMetaFixups" flag to true to skip the QueryInterface
//

void
CBrushRealizer::SetRealizedBrush(
    __inout_ecount_opt(1) CMILBrush *pRealizedBrush,
    bool fSkipMetaFixups
    )
{
    // Eliminate members linked to old realized brush
    Assert(m_pBrushMetaBitmapRT == NULL);
    m_pBrushMetaBitmapRT = NULL;

    // Set the new realized brush
    ReplaceInterface(m_pRealizedBrush, pRealizedBrush);

#if DBG
    // Validate that the fSkipMetaFixups flag is being used correctly... The
    // flag should not be true when there are indeed meta fixups to be done.
    bool fDBGSkipMetaFixupsOriginalValue = fSkipMetaFixups;
    fSkipMetaFixups = false;
#endif

    // Prepare for meta intermediate adjustment
    if (   !fSkipMetaFixups
        && m_pRealizedBrush != NULL
        && m_pRealizedBrush->GetType() == BrushBitmap)
    {
        CMILBrushBitmap *pBrushBitmap = DYNCAST(CMILBrushBitmap, m_pRealizedBrush);
        Assert(pBrushBitmap);

        // Figure out if the bitmap supports meta-RT internal bitmaps.
        HRESULT hrQueryInterface = pBrushBitmap->GetTextureNoAddRef()->QueryInterface(
            IID_CMetaBitmapRenderTarget,
            reinterpret_cast<void **>(&m_pBrushMetaBitmapRT)
            );

    #if DBG
        if (fDBGSkipMetaFixupsOriginalValue)
        {
            Assert(m_pBrushMetaBitmapRT == NULL);
        }
    #endif
    
        if (FAILED(hrQueryInterface))
        {
            Assert(hrQueryInterface == E_NOINTERFACE);
            Assert(m_pBrushMetaBitmapRT == NULL);
        }
    }
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::GetRealizedBrushNoRef
//
//  Synopsis:
//      Gets the brush that was realized, optionally returning a transparent
//      brush instead of NULL for callers that need this
//

__out_ecount_opt(1) CMILBrush *
CBrushRealizer::GetRealizedBrushNoRef(
    bool fConvertNULLToTransparent
    )
{
    CMILBrush *pBrush = m_pRealizedBrush;

    if (   fConvertNULLToTransparent
        && pBrush == NULL
       )
    {
        pBrush = &m_solidColorBrush;
    }

    return pBrush;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::FreeRealizationResources
//
//  Synopsis:
//      Free realization resources that should not last longer than a primitive
//

void
CBrushRealizer::FreeRealizationResources()
{
    //
    // Before releasing this reference to the brush (see below), we must
    // restore it to its original state
    //
    RestoreMetaIntermediates();

    // Note that for brush resource realizers, this may not actually delete
    // the realized brush. There could be another reference in the
    // m_pBrushRealization member.
    ReleaseInterface(m_pRealizedBrush);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::RestoreMetaIntermediates
//
//  Synopsis:
//      Restores meta intermediates within realized brushes
//
//      This method should be called in the meta render target during cleanup,
//      after the drawing operations are complete.
//

void
CBrushRealizer::RestoreMetaIntermediates()
{
    PutBackBrushMetaIntermediate();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::ReplaceBrushMetaIntermedateWithInternalIntermediate
//
//  Synopsis:
//      Replaces a meta-intermediate RT within a realized bitmap brush with the
//      internal RT designed for the given cache index and display id.
//
//      The display id is optional, but if it exists it overrides the cache
//      index as a lookup mechanism.
//
//  Note:
//      We can remove this method/functionality when meta bitmaps are no
//      realized in bitmap brushes. Currently these realizations come
//      from three places:
//         1) CMilCachedVisualImageDuce
//         2) layers created for render targets containing alpha
//         3) dev-test code
//

HRESULT
CBrushRealizer::ReplaceBrushMetaIntermedateWithInternalIntermediate(
    IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
    DisplayId realizationDestination
    )
{
    HRESULT hr = S_OK;

    if (m_pBrushMetaBitmapRT)
    {
        Assert(m_pRealizedBrush->GetType() == BrushBitmap);

        CMILBrushBitmap *pBitmapBrush = DYNCAST(CMILBrushBitmap, m_pRealizedBrush);
        IWGXBitmapSource *pILocalBitmap = NULL;
        IMILRenderTargetBitmap *pIRenderTargetNoRef = NULL;
        
        Assert(pBitmapBrush);

        IFC(m_pBrushMetaBitmapRT->GetCompatibleSubRenderTargetNoRef(
            uOptimalRealizationCacheIndex,
            realizationDestination,
            &pIRenderTargetNoRef
            ));
        
        IFC(pIRenderTargetNoRef->GetBitmapSource(
            &pILocalBitmap
            ));

        pBitmapBrush->GetTextureNoAddRef()->Release();
        pBitmapBrush->SetTextureNoAddRef(pILocalBitmap); // steal ref
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushRealizer::PutBackBrushMetaIntermediate
//
//  Synopsis:
//      puts back the meta-intermediate that we sneakily replaced with an
//      internal-intemediate earlier in
//      ReplaceBrushMetaIntermedateWithInternalIntermediate
//

void
CBrushRealizer::PutBackBrushMetaIntermediate()
{
    if (m_pBrushMetaBitmapRT)
    {
        Assert(m_pRealizedBrush->GetType() == BrushBitmap);

        CMILBrushBitmap *pBitmapBrush = DYNCAST(CMILBrushBitmap, m_pRealizedBrush);
        Assert(pBitmapBrush);

        pBitmapBrush->GetTextureNoAddRef()->Release();
        pBitmapBrush->SetTextureNoAddRef(m_pBrushMetaBitmapRT);
        m_pBrushMetaBitmapRT = NULL; // steal ref
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::CBrushResourceRealizer
//
//  Synopsis:
//      ctor
//

CBrushResourceRealizer::CBrushResourceRealizer(
    __inout_ecount(1) CMilBrushDuce *pBrushResource
    )
{
    m_pBrushResourceNoRef = pBrushResource;
    m_pBrushEffects = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::~CBrushResourceRealizer
//
//  Synopsis:
//      dtor
//

CBrushResourceRealizer::~CBrushResourceRealizer()
{
    ReleaseInterfaceNoNULL(m_pBrushEffects);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::EnsureRealization
//
//  Synopsis:
//      Realizes this brush for the given realization cache index. The
//      realization cache index may be CMILResourceCache::InvalidToken which
//      will be treated the same as as
//      CMILResourceCache::SwRealizationCacheIndex (produces a software
//      relization.)  Otherwise the cache index should be specific to the
//      hardware device.
//

HRESULT
CBrushResourceRealizer::EnsureRealization(
    UINT uAdapterIndex,
    DisplayId realizationDestination,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount(1) CIntermediateRTCreator *pIRenderTargetCreator
    )
{
    HRESULT hr = S_OK;

    CMILBrush *pRealizedBrushNoRef = NULL;

    //
    // A brush context is necessary to realize brush resources. The parameter
    // is only optional in this interface because CImmediateBrushRealizers do not
    // need it. Callers who pass NULL must know that they are not dealing with
    // a CBrushResourceRealizer.
    //
    Assert(pBrushContext);

    //
    // Update brush context given the context state
    //
    pIRenderTargetCreator->ResetUsedState();    
    pBrushContext->pRenderTargetCreator = pIRenderTargetCreator;
    pBrushContext->compositingMode = pContextState->RenderState->CompositingMode;
    if (!pBrushContext->fBrushIsUsedFor3D)
    {
        pBrushContext->matWorldToSampleSpace = pContextState->WorldToDevice;
        pContextState->AliasedClip.GetAsCMilRectF(&pBrushContext->rcSampleSpaceClip);
    }

    pBrushContext->uAdapterIndex = uAdapterIndex;

    //
    // The cached realization is reusable only if the realization is not
    // dependent on hardware or if the hardware realization is dependent on
    // the same cache index as requested.
    //
    // Note that this reuse logic will cause software intermediates to be
    // used in hardware in multimon scenarios. The expectation is that this
    // is faster than using a hardware intermediate.
    //
    // This logic will cause
    // differences in performance based on the order of various devices in
    // the meta render target. When this bug is fixed, "PERF: reorder
    // internal render targets within the meta render target in order to
    // avoid realizing extra hardware intermediates" then this will no
    // longer be an issue
    //

    //
    // Free the reference to realized brush. Note that this will not free
    // the entire realization because there is another reference in
    // m_pBrushRealization. The reference to the realized brush will be set
    // back in SetRealizedBrush below. This call is necessary to avoid
    // keeping an expensive brush realization when we don't need it
    // anymore.
    //

    CBrushRealizer::FreeRealizationResources();
    
    // Get current brush realizer
    IFC(m_pBrushResourceNoRef->GetBrushRealizationNoRef(
        pBrushContext,
        &pRealizedBrushNoRef
        ));

    // Note: We need to set the realized brush even when we did not need to
    //       re-realize because we might have released the reference to it
    //       during FreeRealizationResources
    SetRealizedBrush(
        pRealizedBrushNoRef,
        false // don't skip meta fixups
        );

    IFC(ReplaceBrushMetaIntermedateWithInternalIntermediate(
        uAdapterIndex, // uOptimalRealizationCacheIndex
        realizationDestination
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::RealizedBrushMayNeedNonPow2Tiling
//
//  Synopsis:
//      Check whether realized brush will be tiled. This routine can be called
//      before EnsureRealization.
//

bool
CBrushResourceRealizer::RealizedBrushMayNeedNonPow2Tiling(
    __in_ecount(1) const BrushContext *pBrushContext
    ) const
{
    return m_pBrushResourceNoRef->RealizationMayNeedNonPow2Tiling(
        pBrushContext
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::RealizedBrushWillHaveSourceClip
//
//  Synopsis:
//      Check whether realized brush will need a source clip. This routine can
//      be called before EnsureRealization.
//
//------------------------------------------------------------------------------
bool
CBrushResourceRealizer::RealizedBrushWillHaveSourceClip() const
{
    return m_pBrushResourceNoRef->RealizationWillHaveSourceClip();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::RealizedBrushSourceClipMayBeEntireSource
//
//  Synopsis:
//      Check whether realized brush might have a source clip that might be the
//      entire source. The caller must ensure that the brush will have a source
//      clip (see RealizedBrushWillHaveSourceClip) before calling this method.
//      This routine can be called before EnsureRealization.
//

bool
CBrushResourceRealizer::RealizedBrushSourceClipMayBeEntireSource(
    __in_ecount_opt(1) const BrushContext *pBrushContext
    ) const
{
    //
    // A brush context is necessary to realize brush resources. The parameter
    // is only optional in this interface because CImmediateBrushRealizers do not
    // need it. Callers who pass NULL must know that they are not dealing with
    // a CBrushResourceRealizer.
    //
    Assert(pBrushContext);

    return m_pBrushResourceNoRef->RealizationSourceClipMayBeEntireSource(
        pBrushContext
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::FreeRealizationResources
//
//  Synopsis:
//      Free realization resources that should not last longer than a primitive
//

void
CBrushResourceRealizer::FreeRealizationResources()
{
    CBrushRealizer::FreeRealizationResources();

    m_pBrushResourceNoRef->FreeRealizationResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushResourceRealizer::GetRealizedEffectsNoRef
//
//  Synopsis:
//      returns the realized IMILEffectList
//

HRESULT
CBrushResourceRealizer::GetRealizedEffectsNoRef(
    __deref_out_ecount_opt(1) IMILEffectList **ppIEffectList
    )
{
    HRESULT hr = S_OK;

    FLOAT opacity = GetOpacityFromRealizedBrush();

    if (opacity == 1.0f)
    {
        *ppIEffectList = NULL;
    }
    else
    {
        //
        // Ensure that the effect list is there
        // 
        if (m_pBrushEffects == NULL)
        {
            IMILEffectList *pIEffectTemp = NULL;

            IFC(MILCreateEffectList(
                &pIEffectTemp
                ));
    
            // Cast to the type (CEffectList) that we store
            m_pBrushEffects = DYNCAST(CEffectList, pIEffectTemp);
        }

        //
        // Clear the effect list of its old alpha scale and replace it with the
        // new one.
        //
        //   Note that the effect list will be cleared
        // every time the realized effect list is demanded. It would be more
        // efficient to generate this effect list whenever it changes. Long
        // term we are trying to transition code into avoiding the effect list
        // altogether.
        //
        m_pBrushEffects->Clear();

        AlphaScaleParams alphaParams;
        alphaParams.scale = opacity;

        // Add AlphaScale effect
        IFC(m_pBrushEffects->Add(
            CLSID_MILEffectAlphaScale,
            sizeof(alphaParams),
            &alphaParams
            ));

        *ppIEffectList = m_pBrushEffects;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::CImmediateBrushRealizer
//
//  Synopsis:
//      ctor
//

CImmediateBrushRealizer::CImmediateBrushRealizer()
{
    m_pEffectMetaBitmapRT = NULL;

    m_pIEffectList = NULL;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::~CImmediateBrushRealizer
//
//  Synopsis:
//      dtor
//

CImmediateBrushRealizer::~CImmediateBrushRealizer()
{
    ReleaseInterfaceNoNULL(m_pIEffectList);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::SetMILBrush
//
//  Synopsis:
//      sets the members of a CImmediateBrushRealizer
//
//------------------------------------------------------------------------------
void
CImmediateBrushRealizer::SetMILBrush(
    __inout_ecount(1) CMILBrush *pMILBrush,
    __inout_ecount_opt(1) IMILEffectList *pIEffectList,
    bool fSkipMetaFixups
    )
{
    Assert(m_pIEffectList == NULL);
    DbgAssertEffectWellKnown(pIEffectList);

    SetRealizedBrush(pMILBrush, fSkipMetaFixups);
    SetEffect(pIEffectList, fSkipMetaFixups);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::SetSolidColorBrush
//
//  Synopsis:
//      sets this brush to represent a solid color brush
//

void
CImmediateBrushRealizer::SetSolidColorBrush(
    __in_ecount(1) const MilColorF *pColor
    )
{
    Assert(m_pIEffectList == NULL);

    m_solidColorBrush.SetColor(pColor);
    SetRealizedBrush(
        &m_solidColorBrush,
        true // skip meta fixups
        );
    m_pIEffectList = NULL;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::RealizedBrushMayNeedNonPow2Tiling
//
//  Synopsis:
//      Check whether realized brush will be tiled. This routine can be called
//      before EnsureRealization.
//

bool
CImmediateBrushRealizer::RealizedBrushMayNeedNonPow2Tiling(
    __in_ecount(1) const BrushContext *pBrushContext
    ) const
{
    UNREFERENCED_PARAMETER(pBrushContext);

    bool fWillBeTiled = false;

    const CMILBrush *pBrush = GetRealizedBrushNoRef();
    if (pBrush)
    {
        fWillBeTiled = pBrush->MayNeedNonPow2Tiling();
    }

    return fWillBeTiled;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::RealizedBrushWillHaveSourceClip
//
//  Synopsis:
//      Check whether realized brush will need a source clip. This routine can
//      be called before EnsureRealization.
//
//------------------------------------------------------------------------------
bool
CImmediateBrushRealizer::RealizedBrushWillHaveSourceClip() const
{
    bool fHasClip = false;

    const CMILBrush *pBrush = GetRealizedBrushNoRef();
    if (pBrush)
    {
        if (pBrush->GetType() == BrushBitmap)
        {
            const CMILBrushBitmap *pBitmapBrush = DYNCAST(const CMILBrushBitmap, pBrush);
            Assert(pBitmapBrush);

            fHasClip = !!pBitmapBrush->HasSourceClip();
        }
    }

    return fHasClip;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::RealizedBrushSourceClipMayBeEntireSource
//
//  Synopsis:
//      Check whether realized brush will have a source clip that will be the
//      entire source. The caller must ensure that the brush will have a source
//      clip (see RealizedBrushWillHaveSourceClip) before calling this method.
//      This routine can be called before EnsureRealization.
//

bool
CImmediateBrushRealizer::RealizedBrushSourceClipMayBeEntireSource(
    __in_ecount_opt(1) const BrushContext *pBrushContext
    ) const
{
    UNREFERENCED_PARAMETER(pBrushContext);

    Assert(RealizedBrushWillHaveSourceClip());

    const CMILBrushBitmap *pBitmapBrush = DYNCAST(const CMILBrushBitmap, GetRealizedBrushNoRef());
    Assert(pBitmapBrush);

    return !!pBitmapBrush->SourceClipIsEntireSource();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::EnsureRealization
//
//  Synopsis:
//      Realizes this brush for the given realization cache index. The
//      realization cache index may be CMILResourceCache::InvalidToken which
//      will be treated the same as as
//      CMILResourceCache::SwRealizationCacheIndex (produces a software
//      relization.)  Otherwise the cache index should be specific to the
//      hardware device.
//

HRESULT
CImmediateBrushRealizer::EnsureRealization(
    UINT uAdapterIndex,
    DisplayId realizationDestination,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __in_ecount(1) const CContextState *pContextState,
    __inout_ecount(1) CIntermediateRTCreator *pIRenderTargetCreator
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pBrushContext);
    UNREFERENCED_PARAMETER(pContextState);

    if (uAdapterIndex == CMILResourceCache::InvalidToken)
    {
        uAdapterIndex = CMILResourceCache::SwRealizationCacheIndex;
    }

    // Adjust the brush
    IFC(ReplaceBrushMetaIntermedateWithInternalIntermediate(
        uAdapterIndex, // uOptimalRealizationCacheIndex
        realizationDestination
        ));

    // Adjust the effect
    IFC(ReplaceEffectMetaIntermedateWithInternalIntermediate(
        uAdapterIndex, // uOptimalRealizationCacheIndex
        realizationDestination
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::RestoreMetaIntermediates
//
//  Synopsis:
//      Restores meta intermediates within realized brushes
//
//      This method should be called in the meta render target during cleanup,
//      after the drawing operations are complete.
//

void
CImmediateBrushRealizer::RestoreMetaIntermediates()
{
    CBrushRealizer::RestoreMetaIntermediates();
    
    PutBackEffectMetaIntermediate();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::SetEffect
//
//  Synopsis:
//      Sets the effect, preparing for meta intermediate adjustment
//
//      Callers who know that their effect has no meta-intermediate can set the
//      "fSkipMetaFixups" flag to true to skip the QueryInterface
//

void
CImmediateBrushRealizer::SetEffect(
    __inout_ecount_opt(1) IMILEffectList *pIEffect,
    bool fSkipMetaFixups
    )
{
    IUnknown *pIUnknown = NULL;

    // The effect should only be set once, so this should always be NULL
    Assert(m_pEffectMetaBitmapRT == NULL);

    // Set the effect
    SetInterface(m_pIEffectList, pIEffect);

#if DBG
    // Validate that the fSkipMetaFixups flag is being used correctly... The
    // flag should not be true when there are indeed meta fixups to be done.
    bool fDBGSkipMetaFixupsOriginalValue = fSkipMetaFixups;
    fSkipMetaFixups = false;
#endif

    if (   !fSkipMetaFixups
        && m_pIEffectList
       )
    {
        // Prepare for meta intermediate adjustment
        UINT cResources;
        Verify(SUCCEEDED(m_pIEffectList->GetTotalResourceCount(&cResources)));

        //
        // The effect should either contain nothing (it is empty) or it should
        // contain 1 alpha mask and/or 1 alpha scale.
        //
        Assert(cResources <= 1);
        
        if (cResources == 1)
        {
            Verify(SUCCEEDED(m_pIEffectList->GetResource(0, &pIUnknown)));
    
            // Figure out if the bitmap supports meta-RT internal bitmaps.
            HRESULT hrQueryInterface = pIUnknown->QueryInterface(
                IID_CMetaBitmapRenderTarget,
                (void **)&m_pEffectMetaBitmapRT
                );

        #if DBG
            if (fDBGSkipMetaFixupsOriginalValue)
            {
                Assert(m_pEffectMetaBitmapRT == NULL);
            }
        #endif
            
            if (FAILED(hrQueryInterface))
            {
                Assert(hrQueryInterface == E_NOINTERFACE);
                Assert(m_pEffectMetaBitmapRT == NULL);
            }
        } 
    }

    ReleaseInterfaceNoNULL(pIUnknown);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::ReplaceEffectMetaIntermedateWithInternalIntermediate
//
//  Synopsis:
//      Replaces a meta-intermediate RT within a realized effect with the
//      internal RT designed for the given cache index and display target.
//
//      The display target is optional, but if it exists it overrides the cache
//      index as a lookup mechanism..
//
//  Note:
//      We can remove this method/functionality when meta bitmaps are no
//      realized in effect lists. Currently these realizations come
//      from two places:
//         1) PushEffects with an alpha mask
//         2) dev-test code
//

HRESULT
CImmediateBrushRealizer::ReplaceEffectMetaIntermedateWithInternalIntermediate(
    IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
    DisplayId realizationDestination
    )
{
    HRESULT hr = S_OK;
    IWGXBitmapSource *pILocalBitmap = NULL;

    if (m_pEffectMetaBitmapRT)
    {   
        IMILRenderTargetBitmap *pIRenderTargetNoRef = NULL;
        Assert(m_pIEffectList);

        IFC(m_pEffectMetaBitmapRT->GetCompatibleSubRenderTargetNoRef(
            uOptimalRealizationCacheIndex,
            realizationDestination,
            &pIRenderTargetNoRef
            ));
         
        IFC(pIRenderTargetNoRef->GetBitmapSource(
            &pILocalBitmap
            ));

        IFC(m_pIEffectList->ReplaceResource(0, pILocalBitmap));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pILocalBitmap);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CImmediateBrushRealizer::PutBackEffectMetaIntermediate
//
//  Synopsis:
//      puts back the meta-intermediate that we sneakily replaced with an
//      internal-intemediate earlier in
//      ReplaceEffectMetaIntermedateWithInternalIntermediate
//

void
CImmediateBrushRealizer::PutBackEffectMetaIntermediate()
{
    if (m_pEffectMetaBitmapRT)
    {
        Assert(m_pIEffectList);

        IUnknown *pIUnknownNoAddRef = NULL;    
        pIUnknownNoAddRef = static_cast<IUnknown *>(static_cast<CMILCOMBase *>(m_pEffectMetaBitmapRT));
    
        Verify(SUCCEEDED(m_pIEffectList->ReplaceResource(0, pIUnknownNoAddRef)));
        
        ReleaseInterface(m_pEffectMetaBitmapRT);
    }
}



#if DBG
//+-----------------------------------------------------------------------------
//
//  Function:
//      DbgAssertEffectWellKnown
//
//  Synopsis:
//      Helper method for checking to make sure we don't have any weird
//      combinations of effects
//

void
DbgAssertEffectWellKnown(
    __in_ecount_opt(1) IMILEffectList *pEffect
    )
{
    HRESULT hr = S_OK;
    UINT cEntries = 0;

    if (pEffect == NULL)
    {
        // No effects are okay
        goto Cleanup;
    }

    // Get the count of the transform blocks in the effect object.
    IFC( pEffect->GetCount(&cEntries) );
    Assert(cEntries <= 3);

    UINT uNumAlphaScales = 0;
    UINT uNumAlphaMasks = 0;

    for (UINT i = 0; i < cEntries; i++)
    {
        CLSID clsid;

        IFC( pEffect->GetCLSID(i, &clsid) );

        if (clsid == CLSID_MILEffectAlphaScale)
        {
            // one alpha scale is okay
            uNumAlphaScales++;
            Assert(uNumAlphaScales == 1);
        }
        else if (clsid == CLSID_MILEffectAlphaMask)
        {
            // one alpha mask is okay
            uNumAlphaMasks++;
            Assert(uNumAlphaMasks == 1);
        }
        else
        {
            RIPW(L"Unknown effect found!");
        }
    }

Cleanup:
    IGNORE_HR(hr);
}
#endif




