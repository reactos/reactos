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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(BrushResource, MILRender, "Brush Resource");

MtDefine(CMilBrushDuce, BrushResource, "CMilBrushDuce");

CMilBrushDuce::~CMilBrushDuce()
{
    ReleaseInterfaceNoNULL(m_pBrushRealizer);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilBrushDuce::GetRealizer
//
//  Synopsis:
//      Gets a reference to an object which can be used to obtain a realization
//      of this brush
//

HRESULT
CMilBrushDuce::GetRealizer(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount(1) CBrushRealizer** ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pBrushContext);

    if (!m_pBrushRealizer)
    {
        IFC(CBrushRealizer::CreateResourceRealizer(
            this,
            &m_pBrushRealizer
            ));
    }

    *ppBrushRealizer = m_pBrushRealizer;
    m_pBrushRealizer->AddRef();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilBrushDuce::GetBrushRealization
//
//  Synopsis:
//      Returns a realized brush for this resource that can be used for
//      immediate rendering.
//
//      This is a non-virtual driving method that delegates type-specific
//      creation & realization logic to derived classes via
//      GetBrushRealizationInternal.
//

HRESULT
CMilBrushDuce::GetBrushRealizationNoRef(
    __in_ecount(1) const BrushContext *pBrushContext,      
        // Brush context
    __deref_out_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        // Output realization of this brush
    )
{
    HRESULT hr = S_OK;

    if (IsDirty() ||
        HasRealizationContextChanged(pBrushContext)
        )
    {
        // Update brush realization
        IFC(GetBrushRealizationInternal(
            pBrushContext,
            &m_pCurrentRealizationNoRef
            ));

        // Realization has been updated.  This brush is no longer dirty.
        SetDirty(FALSE);
    }


    //
    // Optimize away rendering for zero-alpha brushes
    //
    // For source over, it is really easy to say that a zero alpha brush can be
    // skipped. This optimization may apply to compositing modes other than
    // source over, but for now we just optimize for source over as it is the
    // only mode we need the optimization for.
    //
    //
    if (   m_pCurrentRealizationNoRef == NULL
        || (   pBrushContext->compositingMode == MilCompositingMode::SourceOver
            && m_pCurrentRealizationNoRef->ObviouslyHasZeroAlpha()
           )
       )
    {
        *ppBrushRealizationNoRef = NULL;
    }
    else
    {
        *ppBrushRealizationNoRef = m_pCurrentRealizationNoRef;
    }

Cleanup:

    if (FAILED(hr))
    {
        // NULL-out out-param
        *ppBrushRealizationNoRef = NULL;

        // Set the dirty flag so that we try to recreate the realization next time.
        // Doing this prevents us from handing out an unset realization next time if
        // the brush is never marked "Dirty".  This is an unlikely  corner case
        // where IsDirty() returns FALSE and HasRealizationContextChanged returns TRUE
        // during this call, but HasRealizationContextChanged returns FALSE during a future
        // call.  Thus, the realization isn't recreated/updated during the future call,
        // even though we just free'd it due to a failure.
        SetDirty(TRUE);
    }

    RRETURN(hr);                
}


