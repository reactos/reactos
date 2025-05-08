// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    wim_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Contains CHwRadialGradientBrush implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwRadialGradientBrush, MILRender, "CHwRadialGradientBrush");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientBrush::CHwRadialGradientBrush
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwRadialGradientBrush::CHwRadialGradientBrush(
    __in_ecount(1) IMILPoolManager *pManager,
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : CHwLinearGradientBrush(pManager, pDevice)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientBrush::dtor
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwRadialGradientBrush::~CHwRadialGradientBrush()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientBrush::SetBrushAndContext
//
//  Synopsis:
//      Called at the beginning of a rendering operation to set context and
//      device independent brush to realize.
//
//------------------------------------------------------------------------------
HRESULT
CHwRadialGradientBrush::SetBrushAndContext(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext
    )
{
    HRESULT hr = S_OK;
    
    if (!m_pLinGradSource)
    {
        CHwRadialGradientColorSource *pRadialGradientColorSource = NULL;

        IFC(CHwRadialGradientColorSource::Create(
            m_pDevice,
            &pRadialGradientColorSource
            ));

        m_pLinGradSource = pRadialGradientColorSource;
        pRadialGradientColorSource = NULL; // steal ref
    }

    IFC(CHwLinearGradientBrush::SetBrushAndContextInternal(
        pBrush,
        hwBrushContext
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwRadialGradientBrush::SendOperations
//
//  Synopsis:
//      Send primary blend operations and color source(s) to builder
//

HRESULT
CHwRadialGradientBrush::SendOperations(
    __inout_ecount(1) CHwPipelineBuilder *pBuilder
    )
{
    HRESULT hr = S_OK;

    Assert(m_pLinGradSource);

    CHwRadialGradientColorSource *pRadialGradientColorSource = 
        DYNCAST(CHwRadialGradientColorSource, m_pLinGradSource);
    Assert(pRadialGradientColorSource);

    IFC(pBuilder->Set_RadialGradient(pRadialGradientColorSource));

Cleanup:
    RRETURN(hr);
}





