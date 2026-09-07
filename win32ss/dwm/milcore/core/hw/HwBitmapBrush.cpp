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
//      Contains CHwBitmapBrush implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwBitmapBrush, MILRender, "CHwBitmapBrush");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::CHwBitmapBrush
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwBitmapBrush::CHwBitmapBrush(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : CHwBrush(pDevice),
        m_pTexturedSource(NULL),
        m_pBumpMapSource(NULL)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::~CHwBitmapBrush
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CHwBitmapBrush::~CHwBitmapBrush()
{
    Assert(m_pTexturedSource == NULL);
    ReleaseInterfaceNoNULL(m_pBumpMapSource);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::AddRef
//
//  Synopsis:
//      Illegal method for this type of brush
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CHwBitmapBrush::AddRef()
{
    AssertMsg(FALSE, "CHwBitmapBrush should not be AddRef'ed.");
    return 1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::Release
//
//  Synopsis:
//             Release resources used by most recent rendering operation.
//
//      Note: This should be called by the caller of CHwBrushPool::GetHwBrush
//            when it is no longer needed.
//
//------------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CHwBitmapBrush::Release()
{
    ReleaseInterface(m_pTexturedSource);
    Assert(m_pBumpMapSource == NULL);
    return 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::SetBrushAndContext
//
//  Synopsis:
//              Called at the beginning of a rendering operation to set context
//              and device independent brush to realize.
//
//      Notes:  This object is expected to have a clean slate before being
//              called.  This means that either it is newly created or Release
//              has been called.
//

HRESULT
CHwBitmapBrush::SetBrushAndContext(
    __inout_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) const CHwBrushContext &hwBrushContext
    )
{
    HRESULT hr = S_OK;

    Assert(m_pTexturedSource == NULL);
    Assert(m_pBumpMapSource == NULL);

    CMILBrushBitmap *pBitmapBrush =
        DYNCAST(CMILBrushBitmap, pBrush);
    Assert(pBitmapBrush);

    IFC(CHwBitmapColorSource::DeriveFromBrushAndContext(
        m_pDevice,
        pBitmapBrush,
        hwBrushContext,
        &m_pTexturedSource
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapBrush::SendOperations
//
//  Synopsis:
//      Send primary blend operations color source(s) to builder
//

HRESULT
CHwBitmapBrush::SendOperations(
    __inout_ecount(1) CHwPipelineBuilder *pBuilder
    )
{
    HRESULT hr = S_OK;

    CHwBoxColorSource *pMaskColorSource = NULL;
    
    Assert(m_pTexturedSource);

    if (m_pBumpMapSource)
    {
        IFC(pBuilder->Set_BumpMap(m_pBumpMapSource));
    }

    IFC(pBuilder->Set_Texture(m_pTexturedSource));

    IFC(m_pTexturedSource->GetMaskColorSource(&pMaskColorSource));
    
    if (pMaskColorSource)
    {
        pMaskColorSource->ResetAlphaScaleFactor();

        IFC(pBuilder->Mul_AlphaMask(
            pMaskColorSource
            ));
    }
    
Cleanup:
    ReleaseInterfaceNoNULL(pMaskColorSource);
    
    RRETURN(hr);
}





