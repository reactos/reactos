// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       ImageDrawing Duce resource implementation.
//

#include "precomp.hpp"

MtDefine(CMilImageDrawingDuce, CMilDrawing, "CMilImageDrawingDuce");

//+------------------------------------------------------------------------
//
//  Member:    CMilImageDrawingDuce::Draw
//
//  Synopsis:  Draw the image held onto by this object to the drawing
//             context.
//
//-------------------------------------------------------------------------
HRESULT 
CMilImageDrawingDuce::Draw(
    __in_ecount(1) CDrawingContext *pDrawingContext  // Drawing context to draw to
    )
{
    HRESULT hr = S_OK;

    Assert(pDrawingContext);

    // Must apply the render state before drawing
    pDrawingContext->ApplyRenderState();

    // Draw the image referenced by this Drawing
    IFC(pDrawingContext->DrawImage(
        m_data.m_pImageSource,
        &m_data.m_Rect,
        m_data.m_pRectAnimation
        ));

Cleanup:

    RRETURN(hr);
}


