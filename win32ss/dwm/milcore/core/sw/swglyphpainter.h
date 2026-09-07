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
//      Class CSWGlyphRunPainter:
//
//      Provides SW pipeline scan operations for text rendering. These
//      operations need a number of variables that are wrapped in
//      CSWGlyphRunPainter instance. The life time of the instance is short. It
//      is created in stack frame and exist while glyph run is being rendered.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CSWGlyphRunPainter : public CBaseGlyphRunPainter, public OpSpecificData
{
typedef CBaseGlyphRunPainter super;

public:
    // No DECLARE_METERHEAP_*: instances are allocated only in stack frame.

    HRESULT Init(
        __inout_ecount(1) DrawGlyphsParameters &pars,
        FLOAT flEffectAlpha,
        __inout_ecount(1) CGlyphPainterMemory* pGlyphPainterMemory,
        BOOL fTargetSupportsClearType,
        __out_ecount(1) BOOL* pfVisible
        );

    __outro_ecount(1) const CRectF<CoordinateSpace::Shape> & GetOutlineRect(
        __out_ecount(1) CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> & mat
        ) const;

    ScanOpFunc GetScanOpCopy(MilPixelFormat::Enum fmtColorSource);
    ScanOpFunc GetScanOpOver(MilPixelFormat::Enum fmtColorSource);

    BOOL IsClearType() const {return m_fIsClearType;}

    float GetEffectAlpha() const {return m_flEffectAlpha;}

private:

    //
    // helper routines
    //

    MIL_FORCEINLINE unsigned __int32
    ApplyAlphaCorrection(
        unsigned __int32 alpha,
        unsigned __int32 color
        ) const;

    static unsigned __int32 GetReciprocal(unsigned __int32 alpha);

    //
    // scan operations
    //
    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpGreyScaleBilinearCopy(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpGreyScaleBilinearOver(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpGreyScaleLinearCopy(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpGreyScaleLinearOver(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpClearTypeBilinearCopy(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpClearTypeBilinearOver(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpClearTypeLinearCopy(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    template<bool fSrcHasAlspa>
    static VOID FASTCALL ScanOpClearTypeLinearOver(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP
        );

    //
    // scan operation helpers
    //
    template<bool fSrcHasAlspa>
    void ApplyGreyScaleCopy(
        unsigned __int32 alpha,
        unsigned __int32 src,
        unsigned __int32 &dst
        );

    template<bool fSrcHasAlspa>
    void ApplyGreyScaleOver(
        unsigned __int32 alpha,
        unsigned __int32 src,
        unsigned __int32 &dst
        );

    template<bool fSrcHasAlspa>
    void ApplyClearTypeCopy(
        unsigned __int32 alphaR,
        unsigned __int32 alphaG,
        unsigned __int32 alphaB,
        unsigned __int32 src,
        unsigned __int32 &dstAlpha,
        unsigned __int32 &dstColor
        );

    template<bool fSrcHasAlspa>
    void ApplyClearTypeOver(
        unsigned __int32 alphaR,
        unsigned __int32 alphaG,
        unsigned __int32 alphaB,
        unsigned __int32 src,
        unsigned __int32 &dst
        );

    MIL_FORCEINLINE __int32 GetAlphaBilinear(__int32 s, __int32 t) const;

private:

    CSWGlyphRun* m_pSWGlyph;           // not addreffed
    BOOL m_fIsClearType;

    UINT m_uFilteredWidth, m_uFilteredHeight; // size of filtered rectangle

    //
    // variables for simplified translation-only rendering
    //

    int m_dy;
    int m_offsetS, m_fractionS, m_offsetT;

    //
    // variables for arbitrary transformed rendering
    //

    // conversion from render space to glyph texture
    // (16.16 fixed point representation of MILMatrix3x2)
    __int32 m_m00, m_m10, m_m20, m_m01, m_m11, m_m21;


    // Adjustable pointers to scan operations
    ScanOpFunc m_pfnScanOpFuncCopyBGR;
    ScanOpFunc m_pfnScanOpFuncOverBGR;
    ScanOpFunc m_pfnScanOpFuncCopyPBGRA;
    ScanOpFunc m_pfnScanOpFuncOverPBGRA;

    // Pointers to available scan operations.
    static const ScanOpFunc sc_pfnClearTypeLinear32bppBGRCopy;
    static const ScanOpFunc sc_pfnClearTypeLinear32bppPBGRACopy;
    static const ScanOpFunc sc_pfnClearTypeBilinear32bppBGRCopy;
    static const ScanOpFunc sc_pfnClearTypeBilinear32bppPBGRACopy;
    static const ScanOpFunc sc_pfnGreyScaleLinear32bppBGRCopy;
    static const ScanOpFunc sc_pfnGreyScaleLinear32bppPBGRACopy;
    static const ScanOpFunc sc_pfnGreyScaleBilinear32bppBGRCopy;
    static const ScanOpFunc sc_pfnGreyScaleBilinear32bppPBGRACopy;

    static const ScanOpFunc sc_pfnClearTypeLinear32bppBGROver;
    static const ScanOpFunc sc_pfnClearTypeLinear32bppPBGRAOver;
    static const ScanOpFunc sc_pfnClearTypeBilinear32bppBGROver;
    static const ScanOpFunc sc_pfnClearTypeBilinear32bppPBGRAOver;
    static const ScanOpFunc sc_pfnGreyScaleLinear32bppBGROver;
    static const ScanOpFunc sc_pfnGreyScaleLinear32bppPBGRAOver;
    static const ScanOpFunc sc_pfnGreyScaleBilinear32bppBGROver;
    static const ScanOpFunc sc_pfnGreyScaleBilinear32bppPBGRAOver;

    // Glyph run outline rectangle (work space)
    CRectF<CoordinateSpace::Shape> m_rcfGlyphRun;

    // offset in glyph texture space,
    // corresponding to (1/3,0) offset in render space
    __int32 m_ds, m_dt;

    //
    // Gamma handling
    //
    GammaTable const* m_pGammaTable;

    float m_flEffectAlpha;
};


