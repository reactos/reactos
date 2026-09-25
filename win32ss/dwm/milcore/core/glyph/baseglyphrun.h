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
//      Class CBaseGlyphRun:
//          common stuff for device dependent glyph run rendering objects.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#define SCALE_RATIO_MAX 1.414f
#define SCALE_RATIO_MIN (1.f/SCALE_RATIO_MAX)

class CBaseGlyphRunPainter;

class CBaseGlyphRun
{
public:
    CBaseGlyphRun()
    {
        // DECLARE_METERHEAP_CLEAR assumed in all derived classes
    }

    // Accessors
    bool IsAlphaValid()        const {return (m_flags & mIsAlphaValid       ) != 0;}
    bool IsGeomValid()         const {return (m_flags & mIsGeomValid        ) != 0;}
    bool IsEmpty()             const {return (m_flags & mIsEmpty            ) != 0;}
    bool IsPersistent()        const {return (m_flags & mIsPersistent       ) != 0;}
    bool IsBig()               const {return (m_flags & mIsBig              ) != 0;}

    RECT const& GetFilteredRect() const {return m_rcFiltered;}
    int GetPitch() const {return m_rcFiltered.right - m_rcFiltered.left;}
    int GetHeight() const {return m_rcFiltered.bottom - m_rcFiltered.top;}

protected:
    friend class CBaseGlyphRunPainter;
    friend class CD3DGlyphRunPainter;
    void SetAlphaValid        (bool yes = true) {SetFlag(yes, mIsAlphaValid       );}
    void SetGeomValid         (bool yes = true) {SetFlag(yes, mIsGeomValid        );}
    void SetEmpty             (bool yes = true) {SetFlag(yes, mIsEmpty            );}
    void SetPersistent        (bool yes = true) {SetFlag(yes, mIsPersistent       );}
    void SetBig               (bool yes = true) {SetFlag(yes, mIsBig              );}

    RECT m_rcFiltered;    // glyphrun bounds in filtered space

private:
    UINT m_flags;

    void SetFlag(bool yes, UINT mask) {if(yes) m_flags |= mask; else m_flags &= ~mask;}
    void SetFlags(UINT value, UINT mask) { m_flags ^= (m_flags ^ value) & mask;}

    // m_flags constants
    enum {
          mIsAlphaValid         = 0x00000004,
          mIsGeomValid          = 0x00000008,
          mIsEmpty              = 0x00000010,
          mIsPersistent         = 0x00000020,
          mIsBig                = 0x00000040,
    };

    // Flags meaning:
    //
    // AlphaValid:   rasterization was requested and finished successfully.
    //               Applicable in SW rendering that use to cache the array of alpha values.
    //               Not used in HW branch sinse it keeps these data in bank's surfaces
    //               and releases alpha map immediately.
    //
    // GeomValid:    for HW only.
    //               The glyph run area has been splitted into pieces (subglyphs)
    //               small enough to fit in HW surfaces. The chain of subglyphs has
    //               been allocated (although alpha data may not yet been constructed
    //               for each subglyph).
    //
    // Empty:        rasterization was requested and returned empty bitmap.
    //               This happens, for instance, when glyph run consists of blanks.
    //
    // Persistent:   (HW only) Has been rendered once, and requested to be rendered again.
    //
    // Big:          (HW only) Consist of more than one subglyph.

    //
    // subpixel animation variables
    //

    ULONG m_uLastBumpTime;
    float m_rLastGivenY;
    float m_rLastActualY;

    friend class CBaseGlyphRunPainter;
};


