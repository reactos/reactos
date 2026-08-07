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
//      Class CSWGlyphRunPainter implementation.
//
//      See comments in header file.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

DeclareTag(tagShowGlyphAreaBase, "MIL_SW", "Show glyph area");

#define DBG_CORRECT(alpha) IF_DBG(if (IsTagEnabled(tagShowGlyphAreaBase) && alpha < 50) alpha = 50)


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::Init
//
//  Synopsis:
//      Prepare to rendering: store painting arguments; check glyphrun
//      visibility (i.e. intersection with clip rect); ensure that given
//      pGlyphRunResource contains correct CSWGlyphRun.
//
//------------------------------------------------------------------------------
HRESULT
CSWGlyphRunPainter::Init(
    __inout_ecount(1) DrawGlyphsParameters &pars,
    FLOAT flEffectAlpha,
    __inout_ecount(1) CGlyphPainterMemory* pGlyphPainterMemory,
    BOOL fTargetSupportsClearType,
    __out_ecount(1) BOOL* pfVisible
    )
{
    HRESULT hr = S_OK;
    Assert(pfVisible);
    Assert(pars.pContextState);
    DisplaySettings const * pDisplaySettings = pars.pContextState->GetCurrentOrDefaultDisplaySettings();
    Assert(pDisplaySettings);

    m_flEffectAlpha = flEffectAlpha;

    *pfVisible = super::Init(
        pGlyphPainterMemory,
        pars.pGlyphRun,
        pars.pContextState
        );

    m_fIsClearType = (m_recommendedBlendMode == ClearType) && fTargetSupportsClearType;

    if (!*pfVisible) goto Cleanup;

    m_pSWGlyph   = 0;

    m_pSWGlyph = GetRealizationNoRef()->GetSWGlyphRun();

    if (!m_pSWGlyph)
    {
        m_pSWGlyph = new CSWGlyphRun();
        IFCOOM(m_pSWGlyph);

        GetRealizationNoRef()->SetSWGlyphRun(m_pSWGlyph);
    }

    IFC( m_pSWGlyph->Validate(this));
    if (m_pSWGlyph->IsEmpty())
    {
        *pfVisible = FALSE;
        goto Cleanup;
    }

    RECT const &rcf = m_pSWGlyph->GetFilteredRect();
    m_uFilteredWidth = rcf.right - rcf.left;
    m_uFilteredHeight = rcf.bottom - rcf.top;

    // Inspect given transformation and settings.
    // When only translation is required, we'l go thru faster branch.

    // When clear type applied to BGR display, or when clear type level
    // is not 100%, then alpha texture is not mapped regularly onto
    // output surface, so we can't get profit of fast branches.

    bool fTranslation = !(m_fIsClearType && !pars.pGlyphRun->IsRGBFullCleartype(pDisplaySettings))
                        && m_xfGlyphWR.m_00 == 1 && m_xfGlyphWR.m_01 == 0
                        && m_xfGlyphWR.m_10 == 0 && m_xfGlyphWR.m_11 == 1;

    int dy = CFloatFPU::SmallRound(m_xfGlyphWR.m_21);
    bool fOffsetYIsInteger = fabs(m_xfGlyphWR.m_21 - float(dy)) < .01;

    if (!fTranslation || !fOffsetYIsInteger || m_fDisableClearType)
    {
        // Complex transformation handling.

        //
        // calculate the matrix to transform rendering space to glyph texture
        // 
        MILMatrix3x2 xfGlyphRT;
        {
            MILMatrix3x2 xfRW;
            xfRW.SetInverse(m_xfGlyphWR);

            MILMatrix3x2 xfWT(3               , 0              ,
                              0               , 1              ,
                              -float(rcf.left), -float(rcf.top));
            
            xfGlyphRT.SetProduct(xfRW, xfWT);
        }

        //
        // Convert xfGlyphRT matrix floating point values to fixed 16.16,
        // taking into account .5 pixel centers offsets both in render and
        // glyph texture space.

        // This is not a violation of upper-left corner convention.
        // Here we have a pixel indexed by a pair of integers (x,y)
        // and we are interested in it's center coordinates that are
        // (xc, yc) = (x+.5, y+.5).
        // We need to convert this point to corresponding (uc,vc)
        // on the texture, using regular matrix-by-vector multiplication.
        // Then, we are interested in four neighjbouring texels
        // that have centers closest to this point (uc, uv).
        // These texels are indexed by interer pair (u,v):
        // (u,v), (u+1,v), (u,v+1), (u+1,v+1).
        // Centers of these four texels lay at:
        // (u+.5,v+.5), (u+1.5,v+.5), (u+.5,v+1.5), (u+1.5,v+1.5).
        // So (u+.5) should be < uc and uc < (u+1.5).
        // This means u = floor(uc - .5), and the fractional part of
        // (uc - .5) means how close we are to next-right texel.
        // The matrix represented by values m_mxx, so far, provides
        // conversion from pixel indices to texels:
        // (x,y) -> (xc,yx) -> (uc,vc) -> (u,v),
        // as marked below.

        m_m00 = CFloatFPU::Trunc(xfGlyphRT.m_00*float(0x10000)),
        m_m10 = CFloatFPU::Trunc(xfGlyphRT.m_10*float(0x10000)),
        m_m20 = CFloatFPU::Trunc(
                        xfGlyphRT.m_20                   * float(0x10000)
                     + (xfGlyphRT.m_00 + xfGlyphRT.m_10) * float(0x8000)  // x,y) -> (xc,yx)
                   ) - 0x8000, // uc -> u
        m_m01 = CFloatFPU::Trunc(xfGlyphRT.m_01*float(0x10000)),
        m_m11 = CFloatFPU::Trunc(xfGlyphRT.m_11*float(0x10000)),
        m_m21 = CFloatFPU::Trunc(
                        xfGlyphRT.m_21                   * float(0x10000)
                     + (xfGlyphRT.m_01 + xfGlyphRT.m_11) * float(0x8000) // x,y) -> (xc,yx)
                   ) - 0x8000; // vc -> v

        if (m_fDisableClearType)
        {
            m_ds = 0;
            m_dt = 0;
        }
        else
        {
            float blueSubpixelOffset = pars.pGlyphRun->BlueSubpixelOffset()*float(0x10000);
            m_ds = CFloatFPU::Trunc(xfGlyphRT.m_00*blueSubpixelOffset);
            m_dt = CFloatFPU::Trunc(xfGlyphRT.m_01*blueSubpixelOffset);
        }

        if (m_fIsClearType)
        {
            m_pfnScanOpFuncCopyBGR = sc_pfnClearTypeBilinear32bppBGRCopy;
            m_pfnScanOpFuncOverBGR = sc_pfnClearTypeBilinear32bppBGROver;
            m_pfnScanOpFuncCopyPBGRA = sc_pfnClearTypeBilinear32bppPBGRACopy;
            m_pfnScanOpFuncOverPBGRA = sc_pfnClearTypeBilinear32bppPBGRAOver;
        }
        else
        {
            m_pfnScanOpFuncCopyBGR = sc_pfnGreyScaleBilinear32bppBGRCopy;
            m_pfnScanOpFuncOverBGR = sc_pfnGreyScaleBilinear32bppBGROver;
            m_pfnScanOpFuncCopyPBGRA = sc_pfnGreyScaleBilinear32bppPBGRACopy;
            m_pfnScanOpFuncOverPBGRA = sc_pfnGreyScaleBilinear32bppPBGRAOver;
        }
    }
    else
    {
        // Simple transformation handling.
        // We know it is translation-only, and offset along Y axis is integer.
        // Fraction of X-offset still can be nonzero, it is taken to interpolate between
        // two nearest values in alpha array.


        //
        // calculate offsets to transform rendering space to glyph texture
        // 

        float offsetS = -3*m_xfGlyphWR.m_20 - float(rcf.left) + 1;
        m_offsetS = CFloatFPU::SmallFloor(offsetS);
        m_fractionS = CFloatFPU::SmallFloor((offsetS - float(m_offsetS))*0x10000);
        Assert(m_fractionS >= 0 && m_fractionS < 0x10000);

        m_offsetT = - dy - rcf.top;

        if (m_fIsClearType)
        {
            m_pfnScanOpFuncCopyBGR = sc_pfnClearTypeLinear32bppBGRCopy;
            m_pfnScanOpFuncOverBGR = sc_pfnClearTypeLinear32bppBGROver;
            m_pfnScanOpFuncCopyPBGRA = sc_pfnClearTypeLinear32bppPBGRACopy;
            m_pfnScanOpFuncOverPBGRA = sc_pfnClearTypeLinear32bppPBGRAOver;
        }
        else
        {
            m_pfnScanOpFuncCopyBGR = sc_pfnGreyScaleLinear32bppBGRCopy;
            m_pfnScanOpFuncOverBGR = sc_pfnGreyScaleLinear32bppBGROver;
            m_pfnScanOpFuncCopyPBGRA = sc_pfnGreyScaleLinear32bppPBGRACopy;
            m_pfnScanOpFuncOverPBGRA = sc_pfnGreyScaleLinear32bppPBGRAOver;
        }
    }


    {
        // setup outline rectangle
        m_rcfGlyphRun.left = static_cast<float>(rcf.left * (1./3));
        m_rcfGlyphRun.right = static_cast<float>(rcf.right * (1./3));

        m_rcfGlyphRun.top = static_cast<float>(rcf.top);
        m_rcfGlyphRun.bottom = static_cast<float>(rcf.bottom);
    }

    IFC(pars.pGlyphRun->GetGammaTable(pDisplaySettings, &m_pGammaTable));
    Assert(m_pGammaTable);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::GetScanOpCopy
//
//  Synopsis:
//      Get the pointer to no-blending scan operation, depending on given source
//      pixel format.
//
//------------------------------------------------------------------------------
ScanOpFunc
CSWGlyphRunPainter::GetScanOpCopy(MilPixelFormat::Enum fmtColorSource)
{
    if (fmtColorSource == MilPixelFormat::BGR32bpp)
    {
        return m_pfnScanOpFuncCopyBGR;
    }
    else
    {
        Assert(fmtColorSource == MilPixelFormat::PBGRA32bpp);
        return m_pfnScanOpFuncCopyPBGRA;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::GetScanOpOver
//
//  Synopsis:
//      Get the pointer to blending scan operation, depending on given source
//      pixel format.
//
//------------------------------------------------------------------------------
ScanOpFunc
CSWGlyphRunPainter::GetScanOpOver(MilPixelFormat::Enum fmtColorSource)
{
    if (fmtColorSource == MilPixelFormat::BGR32bpp)
    {
        return m_pfnScanOpFuncOverBGR;
    }
    else
    {
        Assert(fmtColorSource == MilPixelFormat::PBGRA32bpp);
        return m_pfnScanOpFuncOverPBGRA;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::GetOutlineRect
//
//  Synopsis:
//      Provides glyphrun outlining rectange in local space, and the
//      transformation matrix from local space to device space.
//
//------------------------------------------------------------------------------
__outro_ecount(1) const CRectF<CoordinateSpace::Shape> &
CSWGlyphRunPainter::GetOutlineRect(
    __out_ecount(1) CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> & mat
    ) const
{
    MILMatrix3x2 const& m  = m_xfGlyphWR;
    mat._11 = m.m_00; mat._12 = m.m_01; mat._13 = 0; mat._14 = 0;
    mat._21 = m.m_10; mat._22 = m.m_11; mat._23 = 0; mat._24 = 0;
    mat._31 =      0; mat._32 =      0; mat._33 = 1; mat._34 = 0;
    mat._41 = m.m_20; mat._42 = m.m_21; mat._43 = 0; mat._44 = 1;

    return m_rcfGlyphRun;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::GetAlphaBilinear
//
//  Synopsis:
//      Fetch the value from 2D alpha texture, providing bilinear interpolation
//      based on fractional parts of given 16.16 fixed point coordinates
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __int32
CSWGlyphRunPainter::GetAlphaBilinear(__int32 s, __int32 t) const
{
    __int32 is = s>>16;
    __int32 it = t>>16;

    unsigned width  = m_uFilteredWidth;
    unsigned height = m_uFilteredHeight;
    unsigned char const *pAlpha = m_pSWGlyph->GetAlphaArray() + it*(int)width + is;

    __int32 alpha00, alpha01, alpha10, alpha11;
    if ((unsigned)is < width-1 && (unsigned)it < height-1)
    {
        alpha00 = pAlpha[        0];
        alpha01 = pAlpha[        1];
        alpha10 = pAlpha[width    ];
        alpha11 = pAlpha[width + 1];
    }
    else
    {
        alpha00 = (unsigned)(is    ) < width && (unsigned)(it    ) < height ? pAlpha[        0] : 0;
        alpha01 = (unsigned)(is + 1) < width && (unsigned)(it    ) < height ? pAlpha[        1] : 0;
        alpha10 = (unsigned)(is    ) < width && (unsigned)(it + 1) < height ? pAlpha[width    ] : 0;
        alpha11 = (unsigned)(is + 1) < width && (unsigned)(it + 1) < height ? pAlpha[width + 1] : 0;
    }

    __int32
        dads0 = alpha01 - alpha00,
        dads1 = alpha11 - alpha10,
        rs = s & 0xFFFF,
        alpha0s = alpha00 + (dads0 * rs >> 16),
        alpha1s = alpha10 + (dads1 * rs >> 16),
        dadt = alpha1s - alpha0s,
        rt = t & 0xFFFF;

    return alpha0s + (dadt * rt >> 16);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::GetReciprocal
//
//  Synopsis:
//      Calculate reciprocal of given number, scaled by ratio 0xFF0000. Used to
//      avoid divisions on unpremultiplying color values.
//
//------------------------------------------------------------------------------
unsigned __int32
CSWGlyphRunPainter::GetReciprocal(unsigned __int32 alpha)
{

    Assert(alpha > 0 && alpha < 256);
    return UnpremultiplyTable[alpha];
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSWGlyphRunPainter::ApplyAlphaCorrection
//
//  Synopsis:
//      Apply gamma correction for a given pair (alpha, color component
//      luminance). See comments on CGammaHandler::CalculateGammaTable().
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE unsigned __int32
CSWGlyphRunPainter::ApplyAlphaCorrection(
    unsigned __int32 alpha,
    unsigned __int32 color
    ) const
{
    GammaTable::Row const &row = m_pGammaTable->Polynom[alpha];

    unsigned __int32 alphaCorrected = row.f1 + ((row.f2*color) >> 8);

    Assert(alphaCorrected < 256);

    return alphaCorrected;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ApplyGreyScaleCopy
//
//  Synopsis:
//      Apply scalar alpha to given color. Consider given color premultiplied if
//      fSrcHasAlpha == true.
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
MIL_FORCEINLINE void
CSWGlyphRunPainter::ApplyGreyScaleCopy(
    unsigned __int32 alpha,
    unsigned __int32 src,
    unsigned __int32 &dst
    )
{
    DBG_CORRECT(alpha);

    if (alpha == 0)
    {
        dst =   0;
        return;
    }

    if (alpha == 0xFF)
    {
        dst = fSrcHasAlpha ? src : (src | 0xFF000000);
        return;
    }

    // unpack colors
    unsigned __int32 colorR = (src >> 16) & 0xFF;
    unsigned __int32 colorG = (src >>  8) & 0xFF;
    unsigned __int32 colorB = (src      ) & 0xFF;

    unsigned __int32 alphaCombined = alpha;

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA = src >> 24;
        if (colorA == 0) { dst = 0; return;}

        unsigned __int32 colorA_rc = GetReciprocal(colorA);

        // unpremultiply colors
        colorR = (colorR * colorA_rc) >> 16;
        colorG = (colorG * colorA_rc) >> 16;
        colorB = (colorB * colorA_rc) >> 16;

        // combine glyph alpha with brush alpha
        alphaCombined = (alphaCombined * colorA) >> 8;
    }

    // For non-clear type smoothing we use average luminance.
    // Green value is duplicated to avoid division by 3, and taking
    // into account that green is most important for human vision.
    unsigned __int32 colorAverage = (colorR + colorG + colorG + colorB) >> 2;

    // apply alpha correction, using average color luminance
    unsigned __int32 alphaCorrected = ApplyAlphaCorrection(alphaCombined, colorAverage);

    // premultiply colors
    colorR = (colorR * alphaCorrected) >> 8;
    colorG = (colorG * alphaCorrected) >> 8;
    colorB = (colorB * alphaCorrected) >> 8;

    // pack results
    dst =  (alphaCorrected  << 24)
         | (colorR          << 16)
         | (colorG          <<  8)
         | (colorB               );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ApplyGreyScaleOver
//
//  Synopsis:
//      Apply scalar alpha to given color and blend the result to the
//      destination. Consider given color premultiplied if fSrcHasAlpha == true.
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
MIL_FORCEINLINE void
CSWGlyphRunPainter::ApplyGreyScaleOver(
    unsigned __int32 alpha,
    unsigned __int32 src,
    unsigned __int32 &dst
    )
{
    DBG_CORRECT(alpha);

    if (alpha == 0) return;

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA = src >> 24;

        if (colorA == 0) return;

        if ((alpha & colorA) == 0xFF)
        {
            dst = src;
            return;
        }
    }
    else
    {
        if (alpha == 0xFF)
        {
            dst = src;
            return;
        }
    }

    // unpack colors
    unsigned __int32 colorR = (src >> 16) & 0xFF;
    unsigned __int32 colorG = (src >>  8) & 0xFF;
    unsigned __int32 colorB = (src      ) & 0xFF;

    unsigned __int32 alphaCombined = alpha;

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA = src >> 24;
        unsigned __int32 colorA_rc = GetReciprocal(colorA);

        // unpremultiply colors
        colorR = (colorR * colorA_rc) >> 16;
        colorG = (colorG * colorA_rc) >> 16;
        colorB = (colorB * colorA_rc) >> 16;

        // combine glyph alpha with brush alpha
        alphaCombined = (alphaCombined * colorA) >> 8;
    }

    // For non-clear type smoothing we use average luminance.
    // Green value is duplicated to avoid division by 3, and taking
    // into account that green is most important for human vision.
    unsigned __int32 colorAverage = (colorR + colorG + colorG + colorB) >> 2;

    // apply alpha correction, using average color luminance
    unsigned __int32 alphaCorrected = ApplyAlphaCorrection(alphaCombined, colorAverage);

    // premultiply colors
    colorR = (colorR * alphaCorrected) >> 8;
    colorG = (colorG * alphaCorrected) >> 8;
    colorB = (colorB * alphaCorrected) >> 8;

    // unpack destination pixel
    unsigned __int32 dst_a  = (dst >> 24)     ;
    unsigned __int32 dst_ar = (dst >> 16) & 0xFF;
    unsigned __int32 dst_ag = (dst >>  8) & 0xFF;
    unsigned __int32 dst_ab = (dst      ) & 0xFF;

    // do blending
    unsigned __int32 a_inv = 0xFF - alphaCorrected;

    dst_a  = ((dst_a  * a_inv) >> 8) + alphaCorrected;
    dst_ar = ((dst_ar * a_inv) >> 8) + colorR;
    dst_ag = ((dst_ag * a_inv) >> 8) + colorG;
    dst_ab = ((dst_ab * a_inv) >> 8) + colorB;

    dst =  (dst_a  << 24 )
         | (dst_ar << 16 )
         | (dst_ag <<  8 )
         | (dst_ab       );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ApplyClearTypeCopy
//
//  Synopsis:
//      Apply vector alpha to given color. Consider given color premultiplied if
//      fSrcHasAlpha == true.
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
MIL_FORCEINLINE void
CSWGlyphRunPainter::ApplyClearTypeCopy(
    unsigned __int32 alphaR,
    unsigned __int32 alphaG,
    unsigned __int32 alphaB,
    unsigned __int32 src,
    unsigned __int32 &dstAlpha,
    unsigned __int32 &dstColor
    )
{
    DBG_CORRECT(alphaR);
    DBG_CORRECT(alphaG);
    DBG_CORRECT(alphaB);

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA = src >> 24;

        if ((alphaR | alphaG | alphaB) == 0 || colorA == 0)
        {
            dstAlpha = 0;
            dstColor = 0;
            return;
        }

        if ((alphaR & alphaG & alphaB & colorA) == 0xFF)
        {
            dstAlpha = 0xFFFFFF;
            dstColor = src & 0xFFFFFF;
            return;
        }
    }
    else
    {
        if ((alphaR & alphaG & alphaB) == 0xFF)
        {
            dstAlpha = 0xFFFFFF;
            dstColor = src & 0xFFFFFF;
            return;
        }
    }

    // unpack brush colors
    unsigned __int32 colorR = (src >> 16) & 0xFF;
    unsigned __int32 colorG = (src >>  8) & 0xFF;
    unsigned __int32 colorB = (src      ) & 0xFF;

    unsigned __int32 alphaRCombined = alphaR;
    unsigned __int32 alphaGCombined = alphaG;
    unsigned __int32 alphaBCombined = alphaB;

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA = src >> 24;
        unsigned __int32 colorA_rc = GetReciprocal(colorA);

        // unpremultiply colors
        colorR = (colorR * colorA_rc) >> 16;
        colorG = (colorG * colorA_rc) >> 16;
        colorB = (colorB * colorA_rc) >> 16;

        // combine glyph alpha with brush alpha
        alphaRCombined = (alphaRCombined * colorA) >> 8;
        alphaGCombined = (alphaGCombined * colorA) >> 8;
        alphaBCombined = (alphaBCombined * colorA) >> 8;
    }

    // apply alpha correction
    unsigned __int32 alphaRCorrected = ApplyAlphaCorrection(alphaRCombined, colorR);
    unsigned __int32 alphaGCorrected = ApplyAlphaCorrection(alphaGCombined, colorG);
    unsigned __int32 alphaBCorrected = ApplyAlphaCorrection(alphaBCombined, colorB);
    
    // premultiply colors
    colorR = (colorR*alphaRCorrected)>>8;
    colorG = (colorG*alphaGCorrected)>>8;
    colorB = (colorB*alphaBCorrected)>>8;

    // pack results
    dstColor =  (colorR << 16 )
              | (colorG <<  8 )
              | (colorB       );

    dstAlpha =  (alphaRCorrected << 16 )
              | (alphaGCorrected <<  8 )
              | (alphaBCorrected       );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ApplyClearTypeOver
//
//  Synopsis:
//      Apply vector alpha to given color and blend the result to the
//      destination. Consider given color premultiplied if fSrcHasAlpha == true.
//
//      This method preserves the alpha channel of the render target.
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
MIL_FORCEINLINE void
CSWGlyphRunPainter::ApplyClearTypeOver(
    unsigned __int32 alphaR,
    unsigned __int32 alphaG,
    unsigned __int32 alphaB,
    unsigned __int32 src,
    unsigned __int32 &dst
    )
{
    DBG_CORRECT(alphaR);
    DBG_CORRECT(alphaG);
    DBG_CORRECT(alphaB);

    unsigned __int32 opaque_a_shifted = 0xFF000000;
    unsigned __int32 colorA = src >> 24;

    if (fSrcHasAlpha)
    {
        if ((alphaR | alphaG | alphaB) == 0 || colorA == 0)
        {
            return;
        }

        if ((alphaR & alphaG & alphaB & colorA) == 0xFF)
        {
            dst = opaque_a_shifted | (src & 0xFFFFFF);
            return;
        }
    }
    else
    {
        if ((alphaR & alphaG & alphaB) == 0xFF)
        {
            dst = opaque_a_shifted | (src & 0xFFFFFF);
            return;
        }
    }

    // unpack colors
    unsigned __int32 colorR = (src >> 16) & 0xFF;
    unsigned __int32 colorG = (src >>  8) & 0xFF;
    unsigned __int32 colorB = (src      ) & 0xFF;

    //
    // The greyscale version has a single glyph alpha value, which it takes from
    // the green channel. We do the same here for our overall alpha value to be
    // consistent.
    //
    unsigned __int32 alphaACombined = alphaG;
    unsigned __int32 alphaRCombined = alphaR;
    unsigned __int32 alphaGCombined = alphaG;
    unsigned __int32 alphaBCombined = alphaB;

    if (fSrcHasAlpha)
    {
        unsigned __int32 colorA_rc = GetReciprocal(colorA);

        // unpremultiply colors
        colorR = (colorR * colorA_rc) >> 16;
        colorG = (colorG * colorA_rc) >> 16;
        colorB = (colorB * colorA_rc) >> 16;

        // combine overall alpha and glyph alpha with brush alpha
        alphaACombined = (alphaACombined * colorA) >> 8;
        alphaRCombined = (alphaRCombined * colorA) >> 8;
        alphaGCombined = (alphaGCombined * colorA) >> 8;
        alphaBCombined = (alphaBCombined * colorA) >> 8;
    }

    // apply alpha correction
    unsigned __int32 alphaRCorrected = ApplyAlphaCorrection(alphaRCombined, colorR);
    unsigned __int32 alphaGCorrected = ApplyAlphaCorrection(alphaGCombined, colorG);
    unsigned __int32 alphaBCorrected = ApplyAlphaCorrection(alphaBCombined, colorB);
    
    // premultiply colors
    colorR = (colorR*alphaRCorrected)>>8;
    colorG = (colorG*alphaGCorrected)>>8;
    colorB = (colorB*alphaBCorrected)>>8;

    // unpack destination pixel
    unsigned __int32 dst_aa = (dst >> 24)&0xFF;
    unsigned __int32 dst_ar = (dst >> 16)&0xFF;
    unsigned __int32 dst_ag = (dst >>  8)&0xFF;
    unsigned __int32 dst_ab = (dst      )&0xFF;

    // do blending
    unsigned __int32 alphaA_inv = 0xFF - alphaACombined;
    unsigned __int32 alphaR_inv = 0xFF - alphaRCorrected;
    unsigned __int32 alphaG_inv = 0xFF - alphaGCorrected;
    unsigned __int32 alphaB_inv = 0xFF - alphaBCorrected;

    dst_aa = ((dst_aa*alphaA_inv)>>8) + alphaACombined;
    dst_ar = ((dst_ar*alphaR_inv)>>8) + colorR;
    dst_ag = ((dst_ag*alphaG_inv)>>8) + colorG;
    dst_ab = ((dst_ab*alphaB_inv)>>8) + colorB;

    dst =  (dst_aa << 24 )
         | (dst_ar << 16 )
         | (dst_ag <<  8 )
         | (dst_ab       );
}

//+========================================================================
//+
//+                         SCAN OPERATIONS
//+
//+========================================================================

// Pointers to available scan operations.

//
// [pfx_parse] - workaround for PREfix parse problems
//
#if !ANALYSIS

const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeLinear32bppBGRCopy       = &CSWGlyphRunPainter::ScanOpClearTypeLinearCopy<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeLinear32bppPBGRACopy     = &CSWGlyphRunPainter::ScanOpClearTypeLinearCopy<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeBilinear32bppBGRCopy     = &CSWGlyphRunPainter::ScanOpClearTypeBilinearCopy<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeBilinear32bppPBGRACopy   = &CSWGlyphRunPainter::ScanOpClearTypeBilinearCopy<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleLinear32bppBGRCopy       = &CSWGlyphRunPainter::ScanOpGreyScaleLinearCopy<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleLinear32bppPBGRACopy     = &CSWGlyphRunPainter::ScanOpGreyScaleLinearCopy<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleBilinear32bppBGRCopy     = &CSWGlyphRunPainter::ScanOpGreyScaleBilinearCopy<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleBilinear32bppPBGRACopy   = &CSWGlyphRunPainter::ScanOpGreyScaleBilinearCopy<true>;

const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeLinear32bppBGROver       = &CSWGlyphRunPainter::ScanOpClearTypeLinearOver<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeLinear32bppPBGRAOver     = &CSWGlyphRunPainter::ScanOpClearTypeLinearOver<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeBilinear32bppBGROver     = &CSWGlyphRunPainter::ScanOpClearTypeBilinearOver<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnClearTypeBilinear32bppPBGRAOver   = &CSWGlyphRunPainter::ScanOpClearTypeBilinearOver<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleLinear32bppBGROver       = &CSWGlyphRunPainter::ScanOpGreyScaleLinearOver<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleLinear32bppPBGRAOver     = &CSWGlyphRunPainter::ScanOpGreyScaleLinearOver<true>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleBilinear32bppBGROver     = &CSWGlyphRunPainter::ScanOpGreyScaleBilinearOver<false>;
const ScanOpFunc CSWGlyphRunPainter::sc_pfnGreyScaleBilinear32bppPBGRAOver   = &CSWGlyphRunPainter::ScanOpGreyScaleBilinearOver<true>;

#endif // !ANALYSIS

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpGreyScaleBilinearCopy
//
//  Synopsis:
//      Generate scan line for grey scale glyph run, using bilinear alpha
//      sampling.
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpGreyScaleBilinearCopy(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    const void* pSrc = pSOP->m_pvSrc1;
    void* pDst = pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    __int32 s = x*pThis->m_m00 + y*pThis->m_m10 + pThis->m_m20;
    __int32 t = x*pThis->m_m01 + y*pThis->m_m11 + pThis->m_m21;

    for (UINT i = 0; i < count; i++, s += pThis->m_m00, t += pThis->m_m01)
    {
        unsigned __int32  src = ((unsigned __int32*)pSrc)[i];
        unsigned __int32& dst = ((unsigned __int32*)pDst)[i];
        unsigned __int32 alpha = pThis->GetAlphaBilinear(s, t);

        pThis->ApplyGreyScaleCopy<fSrcHasAlpha>(alpha, src, dst);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpGreyScaleBilinearOver
//
//  Synopsis:
//      Generate scan line for grey scale glyph run, using bilinear alpha
//      sampling.
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//      pSOP->m_pvDest = premultiplied backbuffer color data
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpGreyScaleBilinearOver(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    const void* pSrc = pSOP->m_pvSrc1;
    void* pDst = pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    __int32 s = x*pThis->m_m00 + y*pThis->m_m10 + pThis->m_m20;
    __int32 t = x*pThis->m_m01 + y*pThis->m_m11 + pThis->m_m21;

    for (UINT i = 0; i < count; i++, s += pThis->m_m00, t += pThis->m_m01)
    {
        unsigned __int32  src = ((unsigned __int32*)pSrc)[i];
        unsigned __int32& dst = ((unsigned __int32*)pDst)[i];
        unsigned __int32 alpha = pThis->GetAlphaBilinear(s, t);

        pThis->ApplyGreyScaleOver<fSrcHasAlpha>(alpha, src, dst);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpGreyScaleLinearCopy
//
//  Synopsis:
//      Generate scan line for grey scale glyph run, using linear alpha sampling
//      (interpolated between two nearest values allong X axis).
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpGreyScaleLinearCopy(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    unsigned __int32 const * pSrc = (unsigned __int32*)pSOP->m_pvSrc1;
    unsigned __int32       * pDst = (unsigned __int32*)pSOP->m_pvDest;

    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    int s = x*3 + pThis->m_offsetS;
    int t = y   + pThis->m_offsetT;

    // if given scan line is above or below glyph area, just fill it with zeros
    if ((unsigned)t >= pThis->m_uFilteredHeight)
    {
        while (count--)
        {
            pDst[count] = 0;
        }
        return;
    }

    UINT width  = pThis->m_uFilteredWidth;
    const BYTE *pAlphaRow = pThis->m_pSWGlyph->GetAlphaArray() + t*width;
    int fractionS = pThis->m_fractionS;

    // Regular loop:
    //for (int i = 0; i < count; i++, s += 3)
    //{
    //    __int32 alpha0 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
    //    __int32 alpha1 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
    //    __int32 alpha = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
    //
    //    pThis->ApplyGreyScaleCopy<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    //}

    // Optimized loop that excludes some consition checks:
    int sEnd = s + count*3;
    int sMin = min(sEnd, 0);
    int sMax = min(sEnd, (int)width-1);

    int i = 0;
    for (; s < sMin; i++, s += 3)
    {
        __int32 alpha1 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha = alpha1*fractionS >> 16;
    
        pThis->ApplyGreyScaleCopy<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }

    for (; s < sMax; i++, s += 3)
    {
        __int32 alpha0 = pAlphaRow[s  ];
        __int32 alpha1 = pAlphaRow[s+1];
        __int32 alpha = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
    
        pThis->ApplyGreyScaleCopy<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }

    for (; s < sEnd; i++, s += 3)
    {
        __int32 alpha0 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha = alpha0 + ((-alpha0)*fractionS >> 16);
    
        pThis->ApplyGreyScaleCopy<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpGreyScaleLinearOver
//
//  Synopsis:
//      Generate scan line for grey scale glyph run, using linear alpha sampling
//      (interpolated between two nearest values allong X axis).
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//      pSOP->m_pvDest = premultiplied backbuffer color data
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpGreyScaleLinearOver(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    unsigned __int32 const * pSrc = (unsigned __int32*)pSOP->m_pvSrc1;
    unsigned __int32       * pDst = (unsigned __int32*)pSOP->m_pvDest;

    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    int s = x*3 + pThis->m_offsetS;
    int t = y   + pThis->m_offsetT;

    // if given scan line is above or below glyph area, we are done
    if ((unsigned)t >= pThis->m_uFilteredHeight)
    {
        return;
    }

    UINT width  = pThis->m_uFilteredWidth;
    const BYTE *pAlphaRow = pThis->m_pSWGlyph->GetAlphaArray() + t*width;
    int fractionS = pThis->m_fractionS;

    // Regular loop:
    //for (int i = 0; i < count; i++, s += 3)
    //{
    //    __int32 alpha0 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
    //    __int32 alpha1 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
    //    __int32 alpha = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);

    //    pThis->ApplyGreyScaleOver<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    //}

    // Optimized loop that excludes some condition checks:
    int sEnd = s + count*3;
    int sMin = min(sEnd, 0);
    int sMax = min(sEnd, (int)width-1);

    int i = 0;
    for (; s < sMin; i++, s += 3)
    {
        __int32 alpha1 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha = alpha1*fractionS >> 16;
    
        pThis->ApplyGreyScaleOver<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }

    for (; s < sMax; i++, s += 3)
    {
        __int32 alpha0 = pAlphaRow[s  ];
        __int32 alpha1 = pAlphaRow[s+1];
        __int32 alpha = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
    
        pThis->ApplyGreyScaleOver<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }

    for (; s < sEnd; i++, s += 3)
    {
        __int32 alpha0 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha = alpha0 + ((-alpha0)*fractionS >> 16);
    
        pThis->ApplyGreyScaleOver<fSrcHasAlpha>(alpha, pSrc[i], pDst[i]);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpClearTypeBilinearCopy
//
//  Synopsis:
//      Generate scan line for clear type glyph run, using bilinear alpha
//      sampling.
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//
//  Output:
//      pSOP->m_pvSrc1 = premultiplied output color data with no alpha
//      pSOP->m_pvDest = premultiplied output alpha data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpClearTypeBilinearCopy(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    void* pSrc = (void*)pSOP->m_pvSrc1;
    void* pDstAlpha = pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    __int32 s = x*pThis->m_m00 + y*pThis->m_m10 + pThis->m_m20;
    __int32 t = x*pThis->m_m01 + y*pThis->m_m11 + pThis->m_m21;

    for (UINT i = 0; i < count; i++, s += pThis->m_m00, t += pThis->m_m01)
    {
        unsigned __int32& dstAlpha = ((unsigned __int32*)pDstAlpha)[i];
        unsigned __int32& dstColor = ((unsigned __int32*)pSrc)[i];
        unsigned __int32  src = dstColor;

        unsigned __int32 alphaR = pThis->GetAlphaBilinear(s - pThis->m_ds, t - pThis->m_dt);
        unsigned __int32 alphaG = pThis->GetAlphaBilinear(s              , t              );
        unsigned __int32 alphaB = pThis->GetAlphaBilinear(s + pThis->m_ds, t + pThis->m_dt);

        pThis->ApplyClearTypeCopy<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dstAlpha, dstColor);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpClearTypeBilinearOver
//
//  Synopsis:
//      Generate scan line for clear type glyph run, using bilinear alpha
//      sampling.
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//      pSOP->m_pvDest = premultiplied backbuffer color data
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpClearTypeBilinearOver(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    void* pSrc = (void*)pSOP->m_pvSrc1;
    void* pDst = pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    __int32 s = x*pThis->m_m00 + y*pThis->m_m10 + pThis->m_m20;
    __int32 t = x*pThis->m_m01 + y*pThis->m_m11 + pThis->m_m21;

    for (UINT i = 0; i < count; i++, s += pThis->m_m00, t += pThis->m_m01)
    {
        unsigned __int32& dst = ((unsigned __int32*)pDst)[i];
        unsigned __int32& src = ((unsigned __int32*)pSrc)[i];

        unsigned __int32 alphaR = pThis->GetAlphaBilinear(s - pThis->m_ds, t - pThis->m_dt);
        unsigned __int32 alphaG = pThis->GetAlphaBilinear(s              , t              );
        unsigned __int32 alphaB = pThis->GetAlphaBilinear(s + pThis->m_ds, t + pThis->m_dt);

        pThis->ApplyClearTypeOver<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dst);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpClearTypeLinearCopy
//
//  Synopsis:
//      Generate scan line for clear type glyph run, using linear alpha sampling
//      (interpolated between two nearest values allong X axis).
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//
//  Output:
//      pSOP->m_pvSrc1 = premultiplied output color data with no alpha
//      pSOP->m_pvDest = premultiplied output alpha data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpClearTypeLinearCopy(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    unsigned __int32* pSrc      = (unsigned __int32*)pSOP->m_pvSrc1;
    unsigned __int32* pDstAlpha = (unsigned __int32*)pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = static_cast<int>(pPP->m_uiCount);

    int s = x*3 + pThis->m_offsetS;
    int t = y   + pThis->m_offsetT;

    // if given scan line is above or below glyph area, just fill it with zeros
    if ((unsigned)t >= pThis->m_uFilteredHeight)
    {
        while (count--)
        {
            pDstAlpha[count] = 0;
        }
        return;
    }

    UINT width  = pThis->m_uFilteredWidth;
    const BYTE *pAlphaRow = pThis->m_pSWGlyph->GetAlphaArray() + t*width;
    int fractionS = pThis->m_fractionS;

    // Regular loop:
    //for (int i = 0; i < count; i++, s += 3)
    //{
    //    unsigned __int32& dstAlpha = pDstAlpha[i];
    //    unsigned __int32& dstColor = pSrc[i];
    //    unsigned __int32  src = dstColor;
    //
    //    __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
    //    __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
    //    __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
    //    __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;
    //
    //    __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
    //    __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
    //    __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);
    //
    //    pThis->ApplyClearTypeCopy<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dstAlpha, dstColor);
    //}

    // Optimized loop that excludes some consition checks:
    int sEnd = s + count*3;
    int sMin = min(sEnd, 1);
    int sMax = min(sEnd, (int)width-2);

    int i = 0;
    for (; s < sMin; i++, s += 3)
    {
        unsigned __int32& dstAlpha = pDstAlpha[i];
        unsigned __int32& dstColor = pSrc[i];
        unsigned __int32  src = dstColor;

        __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
        __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeCopy<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dstAlpha, dstColor);
    }

    for (; s < sMax; i++, s += 3)
    {
        unsigned __int32& dstAlpha = pDstAlpha[i];
        unsigned __int32& dstColor = pSrc[i];
        unsigned __int32  src = dstColor;

        __int32 alpha0 = pAlphaRow[s-1];
        __int32 alpha1 = pAlphaRow[s  ];
        __int32 alpha2 = pAlphaRow[s+1];
        __int32 alpha3 = pAlphaRow[s+2];

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeCopy<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dstAlpha, dstColor);
    }

    for (; s < sEnd; i++, s += 3)
    {
        unsigned __int32& dstAlpha = pDstAlpha[i];
        unsigned __int32& dstColor = pSrc[i];
        unsigned __int32  src = dstColor;

        __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
        __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeCopy<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dstAlpha, dstColor);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      static CSWGlyphRunPainter::ScanOpClearTypeLinearOver
//
//  Synopsis:
//      Generate scan line for clear type glyph run, using linear alpha sampling
//      (interpolated between two nearest values allong X axis).
//
//  Input:
//      pSOP->m_pvSrc1 = brush color data, using format
//          MilPixelFormat::PBGRA32bpp when fSrcHasAlpha == true
//          or MilPixelFormat::BGR32bpp otherwise
//      pSOP->m_pvDest = premultiplied backbuffer color data
//
//  Output:
//      pSOP->m_pvDest = premultiplied output color data
//
//------------------------------------------------------------------------------
template<bool fSrcHasAlpha>
void
CSWGlyphRunPainter::ScanOpClearTypeLinearOver(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    CSWGlyphRunPainter* pThis = DYNCAST(CSWGlyphRunPainter, pSOP->m_posd);
    Assert(pThis);

    unsigned __int32* pSrc = (unsigned __int32*)pSOP->m_pvSrc1;
    unsigned __int32* pDst = (unsigned __int32*)pSOP->m_pvDest;
    int x = pPP->m_iX;
    int y = pPP->m_iY;
    UINT count = pPP->m_uiCount;

    int s = x*3 + pThis->m_offsetS;
    int t = y   + pThis->m_offsetT;

    // if given scan line is above or below glyph area, we are done
    if ((unsigned)t >= pThis->m_uFilteredHeight)
    {
        return;
    }

    UINT width  = pThis->m_uFilteredWidth;
    const BYTE *pAlphaRow = pThis->m_pSWGlyph->GetAlphaArray() + t*width;
    int fractionS = pThis->m_fractionS;

    // Regular loop:
    //for (int i = 0; i < count; i++, s += 3)
    //{
    //    unsigned __int32& dst = pDst[i];
    //    unsigned __int32& src = pSrc[i];
    //
    //    __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
    //    __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
    //    __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
    //    __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;
    //
    //    __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
    //    __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
    //    __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);
    //
    //    pThis->ApplyClearTypeOver<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dst);
    //}

    // Optimized loop that excludes some consition checks:
    int sEnd = s + count*3;
    int sMin = min(sEnd, 1);
    int sMax = min(sEnd, (int)width-2);

    int i = 0;
    for (; s < sMin; i++, s += 3)
    {
        unsigned __int32& dst = pDst[i];
        unsigned __int32& src = pSrc[i];

        __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
        __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeOver<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dst);
    }

    for (; s < sMax; i++, s += 3)
    {
        unsigned __int32& dst = pDst[i];
        unsigned __int32& src = pSrc[i];

        __int32 alpha0 = pAlphaRow[s-1];
        __int32 alpha1 = pAlphaRow[s  ];
        __int32 alpha2 = pAlphaRow[s+1];
        __int32 alpha3 = pAlphaRow[s+2];

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeOver<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dst);
    }

    for (; s < sEnd; i++, s += 3)
    {
        unsigned __int32& dst = pDst[i];
        unsigned __int32& src = pSrc[i];

        __int32 alpha0 = unsigned(s-1) < width ? pAlphaRow[s-1] : 0;
        __int32 alpha1 = unsigned(s  ) < width ? pAlphaRow[s  ] : 0;
        __int32 alpha2 = unsigned(s+1) < width ? pAlphaRow[s+1] : 0;
        __int32 alpha3 = unsigned(s+2) < width ? pAlphaRow[s+2] : 0;

        __int32 alphaR = alpha0 + ((alpha1 - alpha0)*fractionS >> 16);
        __int32 alphaG = alpha1 + ((alpha2 - alpha1)*fractionS >> 16);
        __int32 alphaB = alpha2 + ((alpha3 - alpha2)*fractionS >> 16);

        pThis->ApplyClearTypeOver<fSrcHasAlpha>(alphaR, alphaG, alphaB, src, dst);
    }
}


