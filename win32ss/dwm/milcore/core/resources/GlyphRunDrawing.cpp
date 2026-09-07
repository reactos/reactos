// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      GlyphRunDrawing Duce resource implementation.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilGlyphRunDrawingDuce, CMilDrawing, "CMilGlyphRunDrawingDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilGlyphRunDrawingDuce::Draw
//
//  Synopsis:
//      Draw the GlyphRun held onto by this object to the drawing context.
//
//------------------------------------------------------------------------------
HRESULT 
CMilGlyphRunDrawingDuce::Draw(
    __in_ecount(1) CDrawingContext *pDrawingContext  // Drawing context to draw to
    )
{
    HRESULT hr = S_OK;

    Assert(pDrawingContext);

    // Must apply the render state before drawing
    pDrawingContext->ApplyRenderState();

    // Draw the glyph run referenced by this Drawing   
    IFC(pDrawingContext->DrawGlyphRun(
        m_data.m_pForegroundBrush,    
        m_data.m_pGlyphRun
        ));

Cleanup:

    RRETURN(hr);
}


