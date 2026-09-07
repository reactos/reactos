// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       GeometryDrawing Duce resource implementation.
//

#include "precomp.hpp"

MtDefine(CMilGeometryDrawingDuce, CMilDrawing, "CMilGeometryDrawingDuce");

//+------------------------------------------------------------------------
//
//  Member:    CMilGeometryDrawingDuce::Draw
//
//  Synopsis:  Draw the geometry held onto by this object to the drawing
//             context using it's brush and pen
//
//-------------------------------------------------------------------------
HRESULT 
CMilGeometryDrawingDuce::Draw(
    __in_ecount(1) CDrawingContext *pDrawingContext  // Drawing context to draw to
    )
{
    HRESULT hr = S_OK;

    Assert(pDrawingContext);

    // Must apply the render state before drawing
    pDrawingContext->ApplyRenderState();

    // Draw the geometry referenced by this Drawing
    IFC(pDrawingContext->DrawGeometry(
        m_data.m_pBrush,
        m_data.m_pPen,
        m_data.m_pGeometry
        ));

Cleanup:

    RRETURN(hr);
}


