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
//      Class CBaseGlyphRunPainter:
//              Base class for SW & HW glyph painters.
//
//  $ENDTAG
//
//-------------- Coordinate spaces comments ------------------------------------
//
//  Text rendering involves many coordinate spaces:
//
//  U - user space (referres also as local space and LocalRendering)
//      the space where glyph positions are defined. Glyph positions array came
//      into scope when CMILGlyphRun is created.
//
//  R - render space (referred also as global space and Device)
//      the space where pixels live.
//      The transformation U->R should be given for rendering.
//
//  G - glyphrun space
//      this is normalized space independent of font size.
//      Origin of glyphrun space corresponds to anchor point.
//      Anchor point is the very first glyph position.
//      It is assumed to be on base line.
//      We need anchor point to execute snapping to pixel grid.
//
//  W - work space
//      glyphrun space scaled by font size, measured in conventional unit = 1/96 of inch.
//      Origin of work space corresponds to anchor point.
//
//  F - filtered space
//      work space scaled 3 times by X.
//      Origin of filtered space corresponds to anchor point.
//      The unit of filtered space in the textel size of filtered alpha texture.
//
//------------------------------------------------------------------------------

class CMILGlyphRun;
class CGlyphRunRealization;

struct CContextState;
class CGlyphRunRealization;

// runtime class: lives in stack frame only
class CBaseGlyphRunPainter
{
private:
    // The instances of this class live in stack frame only,
    // so no MtDefines and DECLARE_METERHEAP required.
    // Following unimplemented "operator new" is to prevent
    // an attempt to create an instance other way.
    void * __cdecl operator new(size_t cb);

protected:
    CBaseGlyphRunPainter()
    {
        m_pRealization = NULL;
    }
    
    virtual ~CBaseGlyphRunPainter();
    
public:
    bool Init(
        __inout_ecount(1) CGlyphPainterMemory* pGlyphPainterMemory,
        __inout_ecount(1) CGlyphRunResource* pGlyphRunResource,
        __inout_ecount(1) CContextState* pContextState
        );
    MILMatrix3x2 const& InitForPrecomp(
        __inout_ecount(1) CGlyphPainterMemory* pGlyphPainterMemory,
        __inout_ecount(1) CGlyphRunResource* pGlyphRunResource,
        float scaleX,
        float scaleY,
        FontFaceHandle faceHandle
        );

    float GetScaleX() const {return m_scaleX;}
    float GetScaleY() const {return m_scaleY;}

    HRESULT PrepareTransforms();
    CGlyphRunRealization* GetRealizationNoRef() const {return m_pRealization;}
    CGlyphRunResource const* GetGlyphRunResource() const { return m_pGlyphRunResource; }
    CGlyphPainterMemory* GetGlyphPainterMemory() const {return m_pGlyphPainterMemory;}
    
    void GetAlphaArray(
        __deref_out_ecount(*pAlphaArraySize) BYTE **ppAlphaArray,
        __out UINT32 *pAlphaArraySize
        ) const 
    {
        *ppAlphaArray = m_pAlphaArray;
        *pAlphaArraySize = m_alphaArraySize;
    }

    bool HasAlphaArray()
    {
        return (m_pAlphaArray != NULL);
    }

    void MakeAlphaMap(__inout_ecount(1) CBaseGlyphRun *pRun);

    __ecount(1) CContextState* GetContextState() const
    {
        return m_pContextState;
    }

protected:
    CGlyphRunResource* m_pGlyphRunResource;
    // <CoordinateSpace::LocalRendering, CoordinateSpace::Device>
    MILMatrix3x2 m_xfGlyphUR;

    CContextState* m_pContextState;
    UINT m_uTextContrastLevel;

    //
    // transformation matrices:
    //  U - user space - CoordinateSpace::LocalRendering (=BaseSampling)
    //  R - render space - CoordinateSpace::Device, except in case of InitForPrecomp when it is simply CoordinateSpace::LocalRendering
    //  G - glyphrun space - CoordinateSpace::BrushSampling
    //  W - work space - CoordinateSpace::RealizationSampling  (GivenSourceSampling?)
    //  F - filtered space - CoordinateSpace::TexelSampling (just GivenSource w/ x*3?)
    //

    MILMatrix3x2 m_xfGlyphGR;
    MILMatrix3x2 m_xfGlyphGW;
    MILMatrix3x2 m_xfGlyphWR;
    MILMatrix3x2 m_xfGlyphRW;

    float m_scaleX, m_scaleY;
    UINT m_uFaceFlags;

    CGlyphRunRealization* m_pRealization;

    CGlyphPainterMemory* m_pGlyphPainterMemory;
    BYTE* m_pAlphaArray;
    UINT32 m_alphaArraySize;

    bool m_fDisableClearType;

    RenderingMode m_recommendedBlendMode;

public:
    // When given realization is too big and requires scaling down
    // more than this value/100% (by area) then we'll switch off clear type.
    // This value should not exceed 50% because onanimation we allow
    // sqrt(1/2) scaling by both X- and Y-coordinates.
    static const UINT sc_uCriticalScaleDown = 45;
};


