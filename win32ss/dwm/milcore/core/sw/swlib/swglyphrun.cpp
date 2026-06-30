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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

// Meters ----------------------------------------------------------------------
MtDefine(CSWGlyphRun, MILRender, "CSWGlyphRun");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRun::DiscardAlphaArray
//
//  Synopsis:
//      Destroy bitmap data
//
//------------------------------------------------------------------------------
void
CSWGlyphRun::DiscardAlphaArray()
{
    SetEmpty(false);
    SetAlphaValid(false);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRun::Validate
//
//  Synopsis:
//      extracts glyph shape from given glyph run and stores data
//
//------------------------------------------------------------------------------
HRESULT
CSWGlyphRun::Validate(
    __inout_ecount(1) CSWGlyphRunPainter* pPainter
    )
{
    HRESULT hr = S_OK;
    
    IFC( pPainter->PrepareTransforms());

    if (!IsAlphaValid())
    {
        // need [re]rasterize
        DiscardAlphaArray();

        pPainter->MakeAlphaMap(this);
        pPainter->GetAlphaArray(&m_pAlphaArray, &m_alphaArraySize);

        SetAlphaValid();
    }

Cleanup:
    return hr;
}

