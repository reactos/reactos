// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwLinearGradientBrush implementation
//

#include "precomp.hpp"

MtDefine(CHwLinearGradientBrush, MILRender, "CHwLinearGradientBrush");


//+------------------------------------------------------------------------
//
//  Member:  CHwLinearGradientBrush::CHwLinearGradientBrush
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CHwLinearGradientBrush::CHwLinearGradientBrush(
    __in_ecount(1) IMILPoolManager *pManager,
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : CHwCacheablePoolBrush(pManager, pDevice)
{
    m_pLinGradSource = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:  CHwLinearGradientBrush::~CHwLinearGradientBrush
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CHwLinearGradientBrush::~CHwLinearGradientBrush()
{
    ReleaseInterfaceNoNULL(m_pLinGradSource);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientBrush::IsValid
//
//  Synopsis:  Determine if this is valid; simply check if there is a color
//             source.
//
//-----------------------------------------------------------------------------
bool
CHwLinearGradientBrush::IsValid() const
{
    // This shouldn't be called if SetBrushAndContext wasn't ever successfully
    // called.  This isn't a foolproof check as allocating a color source is
    // only the first step in successfully initializing this brush.
    Assert(m_pLinGradSource != NULL);
    return m_fValid && (m_pLinGradSource != NULL);
}

//+------------------------------------------------------------------------
//
//  Member:  CHwLinearGradientBrush::SetBrushAndContext
//
//  Synopsis:  Called at the beginning of a rendering operation to
//             set context and device independent brush to realize.
//             Checks that the color source has been created and 
//             then forwards to the internal call.
//
//-------------------------------------------------------------------------
HRESULT
CHwLinearGradientBrush::SetBrushAndContext(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure a color source is available
    //

    if (!m_pLinGradSource)
    {
        IFC(CHwLinearGradientColorSource::Create(
            m_pDevice,
            &m_pLinGradSource
            ));

        // Note that when caching is available we call Invalidate on this newly
        // created color source, but that is okay.
    }

    IFC(SetBrushAndContextInternal(
        pBrush,
        hwBrushContext
        ));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  CHwLinearGradientBrush::SetBrushAndContextInternal
//
//  Synopsis:  Called at the beginning of a rendering operation to
//             set context and device independent brush to realize.
//
//-------------------------------------------------------------------------
HRESULT
CHwLinearGradientBrush::SetBrushAndContextInternal(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext
    )
{
    HRESULT hr = S_OK;

    CMILBrushGradient *pGradBrush =
        DYNCAST(CMILBrushGradient, pBrush);
    Assert(pGradBrush);

    //
    // Check for caching availability on DI brush
    //

    bool fStoreInCache = false;
    UINT uBrushUniquenessToken;

    pGradBrush->GetUniquenessToken(&uBrushUniquenessToken);

    //
    // Check if this brush is already in use and simply being reused from
    // the cache or if it is not in use and needs added to the cache.
    // If this brush is in use then the reference count will be non-zero.
    //

    if (m_cRef)
    {
        //
        // Check if the uniqueness token match.  If so, then a current
        // color source realization would have the right color properties.
        // Note that the context may still increase the number of texels
        // needed, but that resolution is currently handled by the color
        // source.
        //

        if (m_uCachedUniquenessToken != uBrushUniquenessToken)
        {
            // Update uniqueness token
            m_uCachedUniquenessToken = uBrushUniquenessToken;

            // If there was an existing color source realization then it is
            // useless as the brush colors have changed; mark it as such.
            m_pLinGradSource->InvalidateRealization();
        }
    }
    else
    {
        // Try to store this brush in the cache if everything else succeeds.
        fStoreInCache = true;

        // Initialize uniqueness for this new use.
        m_uCachedUniquenessToken = uBrushUniquenessToken;

        // If there was an existing color source realization then it is
        // definitely useless as this is a completely different brush from
        // last use; mark it as such.
        m_pLinGradSource->InvalidateRealization();
    }

    //
    // Set context and brush.  They may be the first to be set, the same as
    // currently set, or different than what was set previously.
    //

    IFC(m_pLinGradSource->SetBrushAndContext(
        pGradBrush,
        &hwBrushContext.GetWorld2DToIdealSamplingSpace(),
        hwBrushContext.GetContextStatePtr()
        ));

    //
    // Check if the cache needs updated
    //
    // This is only done after success since there is no point in caching if
    // we've failed.  Additionally, out callers (the brush pool manager) assume
    // there is no reference count on this object when this object is first
    // used or it is being taken from the unused pool.
    //

    if (fStoreInCache)
    {
        Assert(m_cRef == 0);

        //
        // Try to cache the HW brush - failure is not fatal to actual rendering
        //
        // Note that the caching is attempted independent of whether we can
        // realize the color source or not.  In most cases, we will be able
        // to realize a color source.  In the unusual case that we can not
        // it is still useful to have the light weight HW brush and color
        // source objects allocated as long as the brush is in use as heap
        // allocations show up on performance profiles for simple paths.
        // For example allocating a solid brush and matching color source
        // accounted for 12% of the time when rendering a PPAA rectangle.
        //

        IMILResourceCache::ValidIndex uCacheIndex;

        if (SUCCEEDED(THR(m_pDevice->GetCacheIndex(&uCacheIndex))) &&
            SUCCEEDED(THR(pGradBrush->SetResource(uCacheIndex, this))))
        {
            Assert(m_cRef > 0);
        }
        else
        {
            Assert(m_cRef == 0);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientBrush::SendOperations
//
//  Synopsis:  Send primary blend operations and color source(s) to builder
//

HRESULT
CHwLinearGradientBrush::SendOperations(
    __inout_ecount(1) CHwPipelineBuilder *pBuilder
    )
{
    HRESULT hr = S_OK;

    Assert(m_pLinGradSource);

    IFC(pBuilder->Set_Texture(m_pLinGradSource));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:    CHwLinearGradientBrush::GetHwTexturedColorSource
//
//  Synopsis:  Retrieve the color source derived from this brush
//

HRESULT
CHwLinearGradientBrush::GetHwTexturedColorSource(
    __deref_out_ecount(1) CHwTexturedColorSource ** const ppColorSource
    )
{
    HRESULT hr = S_OK;

    Assert(m_pLinGradSource);

    *ppColorSource = m_pLinGradSource;

    m_pLinGradSource->AddRef();

    RRETURN(hr);
}




