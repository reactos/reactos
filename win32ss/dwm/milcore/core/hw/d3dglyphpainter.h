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
//      Definitions for the helper class CD3DGlyphRunPainter.
//
//      CD3DGlyphRunPainter is short-term life time class that is instantiated
//      in a stack frame. It encapsulates all temporary data needed for
//      rendering one glyph run and executes glyph rendering.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Structure:
//      VertexFillData
//
//  Synopsis:
//      contains various data used in CUVertex* derivatives "Set" methods
//
//------------------------------------------------------------------------------
struct VertexFillData
{
    // matrix transform from work space to render space
    const MILMatrix3x2* pxfGlyphWR;

    // scaling transform from work space to masking texture space
    float kxWT, kyWT, dxWT, dyWT;

    // mask texture container
    IDirect3DTexture9* pMaskTexture;

    // offsets in texture space corresponding to shift by 1/3 pixel
    // along X-coordinate in render space
    float ds, dt;

    // solid brush color
    DWORD color;

    // brush texture
    CHwTexturedColorSource *m_pHwColorSource;

    // matrix transform from render space to brush texture space
    MILMatrix3x2 xfBrushRT;

    float blueOffset;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DGlyphRunPainter
//
//  Synopsis:
//      Executes everything to render the glyph run in D3D-based environment
//
//------------------------------------------------------------------------------
class CD3DGlyphRunPainter : public CBaseGlyphRunPainter
{
    // The instances of this class live in stack frame only,
    // so no MtDefines and DECLARE_METERHEAP required.
    // Following unimplemented "operator new" is to prevent
    // an attempt to create an instance other way.
    void * __cdecl operator new(size_t cb);
public:
    CD3DGlyphRunPainter();
    ~CD3DGlyphRunPainter();

    HRESULT Paint(
        __in_ecount(1) DrawGlyphsParameters &pars,
        bool fTargetSupportsClearType,
        __inout_ecount(1) CD3DDeviceLevel1* pDevice,
        MilPixelFormat::Enum fmtTargetSurface
        );

    HRESULT EnsureAlphaMap();

    // Accessors
    CD3DDeviceLevel1* GetDevice() const {return m_pDevice;}
    CD3DGlyphBank* GetBank() const {return m_pDevice->GetGlyphBank();}
    CD3DGlyphRun* GetGlyphRun() const {return m_pGlyphRun;}

private:
    CD3DDeviceLevel1* m_pDevice;    // not addreffed
    CD3DGlyphRun* m_pGlyphRun;      // not addreffed
    CD3DSubGlyph* m_pSubGlyph;      // not addreffed

    VertexFillData m_data;

    // rectangle limits in work space
    float m_xMin, m_xMax;
    float m_yMin, m_yMax;

    HRESULT (CD3DGlyphRunPainter::*m_pfnDrawRectangle)();

private:
    bool IsSubglyphClippedOut(MilPointAndSizeL const* prcClip) const;
    HRESULT ValidateGlyphRun();
    HRESULT InspectBrush(
        __in_ecount(1) DrawGlyphsParameters &pars,
        MilPixelFormat::Enum fmtTargetSurface
        );

    template<class TVertex, class TRender> HRESULT TDrawRectangle();
    typedef HRESULT (CD3DGlyphRunPainter::*const pfnDrawRectangle)();
    static pfnDrawRectangle sc_pfnDrawRectangle_CVertM1_1Pass;
    static pfnDrawRectangle sc_pfnDrawRectangle_CVertBM_1Pass;
    static pfnDrawRectangle sc_pfnDrawRectangle_CVertM3_1Pass;
    static pfnDrawRectangle sc_pfnDrawRectangle_CVertBM_3Pass;

    static pfnDrawRectangle sc_pfnDrawRectangle_CVertM1_CT_1Pass;
};


