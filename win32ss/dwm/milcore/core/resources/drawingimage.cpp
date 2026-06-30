// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(DrawingImageResource, MILRender, "DrawingImage Resource");

MtDefine(CMilDrawingImageDuce, DrawingImageResource, "CMilDrawingImageDuce");

//+------------------------------------------------------------------------
//
//  Member:  
//      GetBounds
//
//  Synopsis:  
//      Obtains the bounds of the source image in device-independent 
//      content units.
//
//-------------------------------------------------------------------------  
__override
HRESULT 
CMilDrawingImageDuce::GetBounds(
    __in_ecount_opt(1) CContentBounder* pBounder,
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;

    Assert(prcBounds);

    CHECKPTRARG(pBounder);

    IFC(pBounder->GetContentBounds(m_data.m_pDrawing, prcBounds));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  
//      Draw
//
//  Synopsis:  
//      Draws the drawing with pDC
//
//-------------------------------------------------------------------------  
__override
HRESULT 
CMilDrawingImageDuce::Draw(
    __in_ecount(1) CDrawingContext *pDC,
    MilBitmapWrapMode::Enum wrapMode
    )
{
    UNREFERENCED_PARAMETER(wrapMode);

    Assert(m_data.m_pDrawing);

    RRETURN(m_data.m_pDrawing->Draw(pDC));
}

