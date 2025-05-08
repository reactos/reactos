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
//      implement the classes CD3DGlyphRun and CD3DSubGlyph
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


// Meters ----------------------------------------------------------------------
MtDefine(CD3DGlyphRun, CD3DDeviceLevel1, "CD3DGlyphRun");


//==================================== CD3DSubGlyph ============================

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DSubGlyph::~CD3DSubGlyph
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DSubGlyph::~CD3DSubGlyph()
{
    FreeAlphaMap();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DSubGlyph::ValidateAlphaMap
//
//  Synopsis:
//      allocate rectangle in container texture and fill it with glyph run shape
//      data
//
//------------------------------------------------------------------------------
HRESULT
CD3DSubGlyph::ValidateAlphaMap(CD3DGlyphRunPainter* pPainter)
{
    Assert (!IsAlphaMapValid());

    HRESULT hr = S_OK;
    
    if (m_pTank) FreeAlphaMap();

    IFC(pPainter->EnsureAlphaMap());

    int wid = m_rcFiltered.right - m_rcFiltered.left;
    int hei = m_rcFiltered.bottom - m_rcFiltered.top;

    const CD3DGlyphRun* pRun = pPainter->GetGlyphRun();
    Assert(pRun);

    POINT tankLocation;
    Assert(m_pTank == NULL);
    IFC( pPainter->GetBank()->AllocRect(
        wid,
        hei,
        (BOOL)pRun->IsPersistent(),
        &m_pTank,
        &tankLocation
        ) );
    Assert(m_pTank);
    m_pTank->AddRef();

    m_offset.cx = tankLocation.x - m_rcFiltered.left;
    m_offset.cy = tankLocation.y - m_rcFiltered.top;

    {
        BYTE* pAlphaArray = NULL;
        UINT32 alphaArraySize = 0;
        pPainter->GetAlphaArray(&pAlphaArray, &alphaArraySize);

        // GSchneid: The following call seems super shady since there seems to be no notion of the size of the 
        // alpha array, stride of the alpha array, etc. being passed along. Need to make this more obvious to make
        // security review easier...
        IFC(pPainter->GetBank()->RectFillAlpha(
            m_pTank,
            tankLocation,
            pAlphaArray, 
            pRun->GetFilteredRect(),
            m_rcFiltered
            ));
    }
Cleanup:
    if (FAILED(hr)) 
    { 
        FreeAlphaMap(); 
    }
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DSubGlyph::FreeAlphaMap
//
//  Synopsis:
//      detach from glyph tank, let it reclaim used area
//
//------------------------------------------------------------------------------
void
CD3DSubGlyph::FreeAlphaMap()
{
    if (m_pTank)
    {
        int wid = m_rcFiltered.right - m_rcFiltered.left;
        int hei = m_rcFiltered.bottom - m_rcFiltered.top;

        POINT tankLocation;
        tankLocation.x = m_offset.cx + m_rcFiltered.left;
        tankLocation.y = m_offset.cy + m_rcFiltered.top;

        m_pTank->FreeRect(wid, hei, tankLocation);
        m_pTank->Release();
        m_pTank = NULL;
    }
}

//==================================== CD3DGlyphRun ===============================

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRun::ValidateGeometry
//
//  Synopsis:
//      extracts glyph shape from given glyph run and stores data in this
//      instance
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphRun::ValidateGeometry(CD3DGlyphRunPainter* pPainter)
{
    HRESULT hr = S_OK;
    
    IFC( pPainter->PrepareTransforms());

    if (!IsGeomValid())
    {
        IFC( pPainter->EnsureAlphaMap() );
        if (!IsEmpty())
        {
            IFC( MakeGeometry(pPainter) );
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DGlyphRun::MakeGeometry
//
//  Synopsis:
//      Generate the chain of subglyphs. Given data are glyph bounding rectangle
//      and maximum available for hardware sizes of texture. When given
//      rectangle is too big to fit in HW, it is split into smaller
//      rectangles, and subglyph is created for each one. On success, the chain
//      will be filled with subglyphs, and each subglyph will have valid
//      m_rcFiltered.
//
//      Every rectangle is treated in filtered space
//
//------------------------------------------------------------------------------
HRESULT
CD3DGlyphRun::MakeGeometry(CD3DGlyphRunPainter* pPainter)
{
    HRESULT hr = S_OK;

    RECT const& rcFiltered = GetFilteredRect();
    RECT rcFull;
    rcFull.left   = rcFiltered.left   - DX9_SUBGLYPH_OVERLAP_X;
    rcFull.right  = rcFiltered.right  + DX9_SUBGLYPH_OVERLAP_X;
    rcFull.top    = rcFiltered.top    - DX9_SUBGLYPH_OVERLAP_Y;
    rcFull.bottom = rcFiltered.bottom + DX9_SUBGLYPH_OVERLAP_Y;


    int wid = rcFull.right - rcFull.left;
    int hei = rcFull.bottom - rcFull.top;

    int dx = pPainter->GetBank()->GetMaxSubGlyphWidth() - DX9_SUBGLYPH_OVERLAP_X;
    int dy = pPainter->GetBank()->GetMaxSubGlyphHeight() - DX9_SUBGLYPH_OVERLAP_Y;

    // calculate total amount of subglyphs
    int nx = (wid - DX9_SUBGLYPH_OVERLAP_X + dx - 1)/dx;
    int ny = (hei - DX9_SUBGLYPH_OVERLAP_Y + dy - 1)/dy;
    int n = nx*ny;
    Assert(n > 0);
    Assert(m_subglyphs.IsEmpty());

    RECT r;
    for (r.top = rcFull.top + dy*(ny-1), r.bottom = rcFull.bottom;
            r.top >= rcFull.top;
            r.bottom = r.top + DX9_SUBGLYPH_OVERLAP_Y, r.top -= dy)
    {
        for (r.left = rcFull.left + dx*(nx-1), r.right = rcFull.right;
                r.left >= rcFull.left;
                r.right = r.left + DX9_SUBGLYPH_OVERLAP_X, r.left -= dx)
        {
            Assert(r.left < r.right);
            Assert(r.top < r.bottom);

            CD3DSubGlyph* pSubGlyph = new CD3DSubGlyph();
            IFCOOM(pSubGlyph);
            m_subglyphs.AddAsFirst(pSubGlyph);
            pSubGlyph->m_rcFiltered = r;
        }
    }

    SetGeomValid();
    SetBig(n > 1);

Cleanup:
    if (FAILED(hr))
    {
        m_subglyphs.Clean();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
// Marks all alpha surfaces and glyph tanks as invalid
//
//------------------------------------------------------------------------------

void 
CD3DGlyphRun::DiscardAlphaArrayAndResources()
{
    CD3DSubGlyph* pSubGlyph = NULL;

    for (pSubGlyph = GetFirstSubGlyph(); pSubGlyph != NULL; pSubGlyph = pSubGlyph->GetNext())
    {
        pSubGlyph->FreeAlphaMap();
    }
}




