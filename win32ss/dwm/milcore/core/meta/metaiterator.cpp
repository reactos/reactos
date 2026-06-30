// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_meta
//      $Keywords:
//
//  $Description:
//          Contains class definition for the render target iterator used in the
//      implementation of the meta render target primitive drawing functions.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::CMetaIterator
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CMetaIterator::CMetaIterator(
    __in_ecount(cRT) MetaData *prgMetaData,
    UINT cRT,
    UINT idxFirstEnabledRT,
    bool fUseRTOffset,
    __in_ecount_opt(1) CDisplaySet const *pDisplaySet,
    __in_ecount_opt(1) CAliasedClip *pAliasedClip,
    __deref_opt_inout_ecount(1) CMilRectF const **ppBoundsToAdjust,
    __in_ecount_opt(1) CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> *pTransform,
    __in_ecount_opt(1) CContextState *pContextState,
    __in_ecount_opt(1) IWGXBitmapSource **ppIBitmapSource
    ) :
        m_firstTransformAdjustor(
            !fUseRTOffset           ? NULL :
            (pTransform != NULL)    ? &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(*pTransform) :
            (pContextState == NULL) ? NULL :
            pContextState->In3D     ? &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(pContextState->ViewportProjectionModifier3D) :
                                      &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(pContextState->WorldToDevice)
            ),
        m_boundsAdjustor(ppBoundsToAdjust),
        m_aliasedClipAdjustor(
            !fUseRTOffset           ? NULL :
            pContextState           ? &pContextState->AliasedClip :
                                      pAliasedClip
            ),
        m_bitmapSourceAdjustor(ppIBitmapSource)
#if DBG_ANALYSIS
        ,
        // If transform adjust is not needed only because of fUseRTOffset being
        // false, then just change the Out space while iterating.
        m_pDbgToPageOrDeviceTransform(
            fUseRTOffset            ? NULL :
            (pTransform != NULL)    ? &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(*pTransform) :
            (pContextState == NULL) ? NULL :
            pContextState->In3D     ? &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(pContextState->ViewportProjectionModifier3D) :
                                      &static_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> &>(pContextState->WorldToDevice)
            )
#endif
{
    Assert(prgMetaData[idxFirstEnabledRT].fEnable);
    Assert(idxFirstEnabledRT < cRT);
    // Don't expect both a transform and context state
    Assert(!pTransform || !pContextState);
    
    // Don't expect both an aliased clip and context state
    Assert(!pContextState || !pAliasedClip);

    m_prgMetaData = prgMetaData;
    m_cRT = cRT;
    m_idxCurrent = idxFirstEnabledRT;
    m_pContextState = pContextState;
    m_pDisplaySet = pDisplaySet;
    AssertMsg(m_pDisplaySet || !m_pContextState, "Must have display set when there is a ContextState");

    m_pFirstTransformAdjustor = &m_firstTransformAdjustor;
    m_pAliasedClipAdjustor = &m_aliasedClipAdjustor;
    m_pBitmapSourceAdjustor = &m_bitmapSourceAdjustor;

#if DBG_ANALYSIS
    if (m_pDbgToPageOrDeviceTransform)
    {
        m_pDbgToPageOrDeviceTransform->DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();
    }
#endif
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::~CMetaIterator
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CMetaIterator::~CMetaIterator()
{
#if DBG_ANALYSIS
    if (m_pDbgToPageOrDeviceTransform)
    {
        m_pDbgToPageOrDeviceTransform->DbgChangeToSpace<CoordinateSpace::Device,CoordinateSpace::PageInPixels>();
    }
#endif
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::PrepareForIteration
//
//  Synopsis:
//      Calls BeginPrimitiveAdjust on all adjustors, gathering pointers to the
//      objects that require adjustment
//
//------------------------------------------------------------------------------
HRESULT CMetaIterator::PrepareForIteration()
{
    HRESULT hr = S_OK;

    {
        bool fRequiresAdjustment;
    
        IFC(m_firstTransformAdjustor.BeginPrimitiveAdjust(
            &fRequiresAdjustment
            ));
        if (!fRequiresAdjustment)
        {
            m_pFirstTransformAdjustor = NULL;
        }
    }

    m_fAdjustBounds = m_boundsAdjustor.BeginPrimitiveAdjust();

    {
        bool fRequiresAdjustment;

        IFC(m_aliasedClipAdjustor.BeginPrimitiveAdjust(
            &fRequiresAdjustment
            ));
        if (!fRequiresAdjustment)
        {
            m_pAliasedClipAdjustor = NULL;
        }
    }

    {
        bool fRequiresAdjustment;

        IFC(m_pBitmapSourceAdjustor->BeginPrimitiveAdjust(
            &fRequiresAdjustment
            ));
        if (!fRequiresAdjustment)
        {
            m_pBitmapSourceAdjustor = NULL;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::SetupForNextInternalRT
//
//  Synopsis:
//      retrieves the next internal render target, performing any setup work
//      necessary to draw to this render target
//
//------------------------------------------------------------------------------
HRESULT CMetaIterator::SetupForNextInternalRT(
    __deref_out_ecount(1) IRenderTargetInternal **ppRTInternalNoAddRef
    )
{
    HRESULT hr = S_OK;

    Assert(m_idxCurrent < m_cRT);
    Assert(m_prgMetaData[m_idxCurrent].fEnable);

    // Active internal render target found.
    // Perform BeginDeviceAdjustment and remember this index
    // such that post adjustment will occur
    IFC(BeginDeviceAdjust(
        m_idxCurrent
        ));

    *ppRTInternalNoAddRef = m_prgMetaData[m_idxCurrent].pInternalRT;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaIterator::BeginDeviceAdjust
//
//  Synopsis:
//      Performs the adjustment. Any data that is changed here should be
//      restored in EndPrimitiveAdjust
//
//------------------------------------------------------------------------------
HRESULT CMetaIterator::BeginDeviceAdjust(
    UINT idx
    )
{
    HRESULT hr = S_OK;
    
    if (m_pFirstTransformAdjustor)
    {
        IFC(m_pFirstTransformAdjustor->BeginDeviceAdjust(
            m_prgMetaData,
            idx
            ));
    }

    if (m_fAdjustBounds)
    {
        m_boundsAdjustor.BeginDeviceAdjust(
            m_prgMetaData,
            idx
            );
    }

    if (m_pAliasedClipAdjustor)
    {
        IFC(m_pAliasedClipAdjustor->BeginDeviceAdjust(
            m_prgMetaData,
            idx
            ));
    }

    if (m_pBitmapSourceAdjustor)
    {
        IFC(m_pBitmapSourceAdjustor->BeginDeviceAdjust(
            m_prgMetaData,
            idx
            ));
    }

    if (m_pContextState)
    {
        // let rendering objects to know which display is served
        m_pContextState->GetDisplaySettingsFromDisplaySet(m_pDisplaySet, idx);
    }

Cleanup:
    RRETURN(hr);
}



