// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       VideoDrawing Duce resource implementation.
//

#include "precomp.hpp"

MtDefine(CMilVideoDrawingDuce, CMilDrawing, "CMilVideoDrawingDuce");

//+------------------------------------------------------------------------
//
//  Member:    CMilVideoDrawingDuce::Draw
//
//  Synopsis:  Draw the video held onto by this object to the drawing
//             context.
//
//-------------------------------------------------------------------------
HRESULT
CMilVideoDrawingDuce::Draw(
    __in_ecount(1) CDrawingContext *pDrawingContext  // Drawing context to draw to
    )
{
    HRESULT hr = S_OK;

    Assert(pDrawingContext);

    // Must apply the render state before drawing
    pDrawingContext->ApplyRenderState();

    // Draw the video referenced by this Drawing
    IFC(pDrawingContext->DrawVideo(
        m_data.m_pPlayer,
        &(m_data.m_Rect),
        m_data.m_pRectAnimation));

Cleanup:

    RRETURN(hr);
}



