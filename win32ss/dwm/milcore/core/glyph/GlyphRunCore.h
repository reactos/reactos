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
//      Definitions for class CGlyphRunStorage that stores glyph run data in a
//      compact form.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#pragma once

extern volatile LONG g_uiCMILGlyphRunCount;

class CGlyphRunResource;

typedef UINT FontFaceHandle;

 
//+-----------------------------------------------------------------------------
//
//  Class:
//      CGlyphRunStorage
//
//  Synopsis:
//      Stores glyph run data in a compact form.
//
//------------------------------------------------------------------------------

class CGlyphRunStorage
{
public:
    CGlyphRunStorage()
    {
        // instance is created zero-filled
        Assert (m_pGlyphIndices == NULL);
        InterlockedIncrement(&g_uiCMILGlyphRunCount);
    }
    ~CGlyphRunStorage();

    HRESULT InitStorage(
        MILCMD_GLYPHRUN_CREATE const* pPacket,
        UINT cbSize
        );

private:
    HRESULT SecurityCheck();

public:
    // ============== data accessors
    // the number of glyphs in the run
    UINT16 GetGlyphCount() const
    {
        return m_usGlyphCount;
    }

    // returns the array of indices in the font for glyphs in the run
    const USHORT * GetGlyphIndices() const
    {
        return m_pGlyphIndices;
    }

    bool HasOffsets() const
    {
        return (m_glyphRunFlags & MilGlyphRun::HasOffsets) != 0;
    }

    // font rendering em size in MIL units
    double GetMUSize() const {return m_muSize;}

    // glyph run logical baseline origin in MIL units
    MilPoint2F const & GetOrigin() const {return m_origin;}

    // returns whether the glyph run is sideways
    bool IsSideways() const {return (m_glyphRunFlags & MilGlyphRun::Sideways ) != 0;}

    /// Returns true if the glyph run is right-to-left.
    bool IsRightToLeft() const {return m_bidiLevel % 2 != 0;}

    bool IsDisplayMeasured()
    {
        return (m_measuringMethod == DWRITE_MEASURING_MODE_GDI_CLASSIC) ||
               (m_measuringMethod == DWRITE_MEASURING_MODE_GDI_NATURAL);
    }

 protected:
    UINT16 m_usGlyphCount;
    UINT16 m_glyphRunFlags;

    /// Odd levels indicate right-to-left languages like Hebrew and Arabic, 
    /// while even levels indicate left-to-right languages like English and Japanese (when
    /// written horizontally). 
    UINT16 m_bidiLevel;

    MilPoint2F m_origin;

    // font's point size is measured in MIL units (1/96 inch)
    float m_muSize;

    DWRITE_MEASURING_MODE m_measuringMethod;
    
    USHORT    * m_pGlyphIndices;
    float     * m_pGlyphAdvances;
    float     * m_pGlyphOffsets;

    // WARNING very brittle code - relies on particular field (m_pFontFileName)
    // being first in alloc order for this class.    

    IDWriteFont *m_pIDWriteFont;

    // Variable length data are packed in following order:
    //
    // m_pFontFileName
    // m_pGlyphPositions       [see below], must present
    // m_pGlyphIndices         [m_usGlyphCount], must present
    //
    // THE ORDER IS IMPORTANT BECAUSE OF 64 BIT ALIGNMENT REQUIREMENTS.

    // m_pGlyphPositions size depends on MilGlyphRun::HasYPositions flag.
    // When set, m_pGlyphPositions array contains (m_usGlyphCount) coordinate pairs,
    // that are interpretable either as MilPoint2F[m_usGlyphCount] or float[2*m_usGlyphCount].
    // Otherwise, m_pGlyphPositions (m_usGlyphCount-1) contains X-coordinates for all the glyphs
    // except the very first, that assumed to be zero, as well as all Y-coordinates.

    CRectF<CoordinateSpace::LocalRendering> m_boundingRect;  // Precomputed from managed side
};



