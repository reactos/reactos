// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Contains definition of the CSwIntermediateRTCreator class
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

//+----------------------------------------------------------------------------
//
//  Member:    CSwIntermediateRTCreator::CSwIntermediateRTCreator
//
//-----------------------------------------------------------------------------

CSwIntermediateRTCreator::CSwIntermediateRTCreator(
    MilPixelFormat::Enum fmtTarget,
    DisplayId associatedDisplay
    DBG_STEP_RENDERING_COMMA_PARAM(__in_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
    )
{
    m_fmtTarget = fmtTarget;
    m_associatedDisplay = associatedDisplay;

#if DBG_STEP_RENDERING
    m_pDisplayRTParent = pDisplayRTParent;
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:    CSwIntermediateRTCreator::CreateRenderTargetBitmap
//
//  Synopsis:  Create a SW bitmap Render target with the format compatible with
//             the fmt specified in the constructor
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CSwIntermediateRTCreator::CreateRenderTargetBitmap(
    UINT width,
    UINT height,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pActiveDisplays);
    
    MilPixelFormat::Enum fmtCompatible;
    DisplayId associatedDisplay;

    // The width and height are converted to floats when clipping,
    // make sure we don't expect values TOO big as input.
    if (width > MAX_INT_TO_FLOAT || height > MAX_INT_TO_FLOAT)
    {
        IFC(WGXERR_UNSUPPORTEDTEXTURESIZE);
    }    

    if (usageInfo.flags & IntermediateRTUsage::ForBlending)
    {
        IFC(GetBestBlendingFormat(m_fmtTarget, &fmtCompatible));
    }
    else
    {
        fmtCompatible = m_fmtTarget;
    }

    IFC(CSwRenderTargetBitmap::Create(
        width,
        height,
        fmtCompatible,
        96.0f,
        96.0f,
        m_associatedDisplay,
        ppIRenderTargetBitmap
        DBG_STEP_RENDERING_COMMA_PARAM(m_pDisplayRTParent)
        ));

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CSwIntermediateRTCreator::ReadEnabledDisplays
//
//  Synopsis:  Returns the displays for which this intermediate RT creator is
//             enabled.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CSwIntermediateRTCreator::ReadEnabledDisplays (
    __inout DynArray<bool> *pEnabledDisplays
    )
{
    HRESULT hr = S_OK;

    const CDisplaySet *pDisplaySet;
    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

    // Mark the display we are associated with as true, all others false.
    // If our surf RT is not associated with a display, we will report no
    // displays enabled.
    UINT uDisplayIndex;
    if (m_associatedDisplay.IsNone())
    {
        uDisplayIndex = UINT_MAX;
    }
    else
    {
        IFC(pDisplaySet->GetDisplayIndexFromDisplayId(m_associatedDisplay, uDisplayIndex));
    }

    Assert(uDisplayIndex < pDisplaySet->GetDisplayCount() || uDisplayIndex == UINT_MAX);
    Assert(pDisplaySet->GetDisplayCount() == pEnabledDisplays->GetCount());
    for (UINT i = 0; i < pEnabledDisplays->GetCount(); i++)
    {       
        (*pEnabledDisplays)[i] = (i == uDisplayIndex);
    }

Cleanup:
    ReleaseInterface(pDisplaySet);
    RRETURN(hr);
}

