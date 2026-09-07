// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//      The "Dither" scan operation. See scanoperation.h.
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      Dither: Binary operation; dithers from 32bppARGB to 16bpp.
//
//  Inputs:
//
//      pSOP->m_pvDest:        The destination scan.
//      pSOP->m_pvSrc1:        The source scan.
//      pPP->m_uiCount:        Scan length, in pixels.
//      pPP->m_fDither16bpp:   Whether to dither or just quantize.
//      pPP->m_iX, pPP->m_iY:  Position of scan, used to position dither matrix.
//
//  Return Value:
//
//      None
//

const UINT32 Saturate5Bit[] = { 0,  1,  2,  3,  4,  5,  6,  7,
                          8,  9,  10, 11, 12, 13, 14, 15,
                          16, 17, 18, 19, 20, 21, 22, 23,
                          24, 25, 26, 27, 28, 29, 30, 31,
                          31 };

const UINT32 Saturate6Bit[] = { 0,  1,  2,  3,  4,  5,  6,  7,
                          8,  9,  10, 11, 12, 13, 14, 15,
                          16, 17, 18, 19, 20, 21, 22, 23,
                          24, 25, 26, 27, 28, 29, 30, 31,
                          32, 33, 34, 35, 36, 37, 38, 39,
                          40, 41, 42, 43, 44, 45, 46, 47,
                          48, 49, 50, 51, 52, 53, 54, 55,
                          56, 57, 58, 59, 60, 61, 62, 63,
                          63 };

const UINT32 Dither5BitR[16] = { 0x00000000, 0x00040000, 0x00010000, 0x00050000,
                           0x00060000, 0x00020000, 0x00070000, 0x00030000,
                           0x00010000, 0x00050000, 0x00000000, 0x00040000,
                           0x00070000, 0x00030000, 0x00060000, 0x00020000 };

const UINT32 Dither5BitG[16] = { 0x00000000, 0x00000400, 0x00000100, 0x00000500,
                           0x00000600, 0x00000200, 0x00000700, 0x00000300,
                           0x00000100, 0x00000500, 0x00000000, 0x00000400,
                           0x00000700, 0x00000300, 0x00000600, 0x00000200 };

const UINT32 Dither6BitG[16] = { 0x00000000, 0x00000200, 0x00000000, 0x00000200,
                           0x00000300, 0x00000100, 0x00000300, 0x00000100,
                           0x00000000, 0x00000200, 0x00000000, 0x00000200, 
                           0x00000300, 0x00000100, 0x00000300, 0x00000100 };

const UINT32 Dither5BitB[16] = { 0x00000000, 0x00000004, 0x00000001, 0x00000005,
                           0x00000006, 0x00000002, 0x00000007, 0x00000003, 
                           0x00000001, 0x00000005, 0x00000000, 0x00000004,
                           0x00000007, 0x00000003, 0x00000006, 0x00000002 };

// The following 'Dither565' and 'Dither555' matrices are 4 by 4
// arrays for adding straight to an ARGB dword value.  Every row
// is repeated to allow us to do 128-bit reads with wrapping.

const UINT32 Dither565[32] = { 0x00000000, 0x00040204, 0x00010001, 0x00050205,
                         0x00000000, 0x00040204, 0x00010001, 0x00050205,
                         0x00060306, 0x00020102, 0x00070307, 0x00030103,
                         0x00060306, 0x00020102, 0x00070307, 0x00030103,
                         0x00010001, 0x00050205, 0x00000000, 0x00040204,
                         0x00010001, 0x00050205, 0x00000000, 0x00040204,
                         0x00070307, 0x00030103, 0x00060306, 0x00020102,
                         0x00070307, 0x00030103, 0x00060306, 0x00020102 };

const UINT32 Dither555[32] = { 0x00000000, 0x00040404, 0x00010101, 0x00050505, 
                         0x00000000, 0x00040404, 0x00010101, 0x00050505, 
                         0x00060606, 0x00020202, 0x00070707, 0x00030303,
                         0x00060606, 0x00020202, 0x00070707, 0x00030303,
                         0x00010101, 0x00050505, 0x00000000, 0x00040404,
                         0x00010101, 0x00050505, 0x00000000, 0x00040404,
                         0x00070707, 0x00030303, 0x00060606, 0x00020202,
                         0x00070707, 0x00030303, 0x00060606, 0x00020202 };

// The 'DitherNone' matrix allows us to disable dithering in a dithering
// routine:

const UINT32 DitherNone[4] = { 0, 0, 0, 0 };

// Dither to 16bppRGB565

VOID FASTCALL
Dither_32bppARGB_565(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // Since the MMX versions easily handle both dithering and non-dithering,
    // it makes it simpler if all the 16bpp functions handle both.

    if (!pPP->m_fDither16bpp)
    {
        Quantize_32bppARGB_565(pPP, pSOP);
        return;
    }

    DEFINE_POINTERS(ARGB, WORD);
    UINT uiCount = pPP->m_uiCount;

    Assert(uiCount != 0);
    
    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

    // !!![andrewgo] Are we getting the window-relative (x, y)?  (Don't think so!)

    INT startDitherIndex = (y & 3) * 4;

    do {
        UINT32 uiSrc = *pSrc;
        x = (x & 3) + startDitherIndex;

        *pDest = static_cast<WORD>(
             (Saturate5Bit[((uiSrc & 0xff0000) + Dither5BitR[x]) >> 19] << 11) +
             (Saturate6Bit[((uiSrc & 0x00ff00) + Dither6BitG[x]) >> 10] << 5) +
             (Saturate5Bit[((uiSrc & 0x0000ff) + Dither5BitB[x]) >> 3]));

        pSrc++;
        pDest++;
        x++;
    } while (--uiCount != 0);
}

// Dither to 16bppRGB555

VOID FASTCALL
Dither_32bppARGB_555(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // Since the MMX versions easily handle both dithering and non-dithering,
    // it makes it simpler if all the 16bpp functions handle both.

    if (!pPP->m_fDither16bpp)
    {
        Quantize_32bppARGB_555(pPP, pSOP);
        return;
    }

    DEFINE_POINTERS(ARGB, WORD);
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount != 0);
    
    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

    INT startDitherIndex = (y & 3) * 4;

    do {
        UINT32 uiSrc = *pSrc;
        x = (x & 3) + startDitherIndex;

        *pDest = static_cast<WORD>(
             (Saturate5Bit[((uiSrc & 0xff0000) + Dither5BitR[x]) >> 19] << 10) +
             (Saturate5Bit[((uiSrc & 0x00ff00) + Dither5BitG[x]) >> 11] << 5) +
             (Saturate5Bit[((uiSrc & 0x0000ff) + Dither5BitB[x]) >> 3]));

        pSrc++;
        pDest++;
        x++;
    } while (--uiCount != 0);
}

// SrcOverAL 32bppPARGB over 16bppRGB565.

VOID FASTCALL
SrcOverAL_32bppPARGB_565(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount>0);

    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

    INT startDitherIndex = (y & 3) * 4;
    
    do {
        UINT32 uiSrc = *pSrc;
        UINT32 uiSrcAlpha = uiSrc >> 24;
        x = (x & 3) + startDitherIndex;

        if (uiSrcAlpha != 0)
        {
            UINT32 uiDestIn;
            UINT r, g, b;

            r = uiSrc & 0xff0000;
            g = uiSrc & 0x00ff00;
            b = uiSrc & 0x0000ff;
            
            if (uiSrcAlpha != 255)
            {
                uiDestIn = *pDestIn;
        
                UINT sr = (uiDestIn >> 11) & 0x1f;
                UINT sg = (uiDestIn >>  5) & 0x3f;
                UINT sb = (uiDestIn      ) & 0x1f;

                sr = (sr << 3) | (sr >> 2);
                sg = (sg << 2) | (sg >> 4);
                sb = (sb << 3) | (sb >> 2);

                //
                // Dst = B + (1-Alpha) * S
                //

                ULONG Multa = 255 - uiSrcAlpha;
                ULONG _D1_000000GG = sg;
                ULONG _D1_00RR00BB = sb | (sr << 16);

                ULONG _D2_0000GGGG = _D1_000000GG * Multa + 0x00000080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_000000GG = (_D2_0000GGGG & 0x0000ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

                ULONG _D4_0000GG00 = (_D2_0000GGGG + _D3_000000GG) & 0x0000FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                r += _D4_00RR00BB; // The BB part will be shifted off
                g += _D4_0000GG00;
                b += _D4_00RR00BB & 0x0000ff;
            }

            *pDestOut = static_cast<UINT16>(
                 (Saturate5Bit[(r + Dither5BitR[x]) >> 19] << 11) +
                 (Saturate6Bit[(g + Dither6BitG[x]) >> 10] << 5) +
                 (Saturate5Bit[(b + Dither5BitB[x]) >> 3]));
        }

        pSrc++;
        pDestIn++;
        pDestOut++;
        x++;
    } while (--uiCount != 0);
}

// SrcOverAL 32bppPARGB over 16bppRGB555.

VOID FASTCALL
SrcOverAL_32bppPARGB_555(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS_BLEND(ARGB, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    Assert(uiCount>0);

    INT x = pPP->m_iX;
    INT y = pPP->m_iY;

    INT startDitherIndex = (y & 3) * 4;
    
    do {
        UINT32 uiSrc = *pSrc;
        UINT32 uiSrcAlpha = uiSrc >> 24;
        x = (x & 3) + startDitherIndex;

        if (uiSrcAlpha != 0)
        {
            UINT32 uiDestIn;
            UINT r, g, b;

            r = uiSrc & 0xff0000;
            g = uiSrc & 0x00ff00;
            b = uiSrc & 0x0000ff;
            
            if (uiSrcAlpha != 255)
            {
                uiDestIn = *pDestIn;

                UINT sr = (uiDestIn >> 10) & 0x1f;
                UINT sg = (uiDestIn >>  5) & 0x1f;
                UINT sb = (uiDestIn      ) & 0x1f;

                sr = (sr << 3) | (sr >> 2);
                sg = (sg << 3) | (sg >> 2);
                sb = (sb << 3) | (sb >> 2);
                
                //
                // Dst = B + (1-Alpha) * S
                //

                ULONG Multa = 255 - uiSrcAlpha;
                ULONG _D1_000000GG = sg;
                ULONG _D1_00RR00BB = sb | (sr << 16);

                ULONG _D2_0000GGGG = _D1_000000GG * Multa + 0x00000080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_000000GG = (_D2_0000GGGG & 0x0000ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;

                ULONG _D4_0000GG00 = (_D2_0000GGGG + _D3_000000GG) & 0x0000FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                r += _D4_00RR00BB; // The BB part will be shifted off
                g += _D4_0000GG00;
                b += _D4_00RR00BB & 0x0000ff;
            }

            *pDestOut = static_cast<UINT16>(
                 (Saturate5Bit[(r + Dither5BitR[x]) >> 19] << 10) +
                 (Saturate5Bit[(g + Dither5BitG[x]) >> 11] << 5) +
                 (Saturate5Bit[(b + Dither5BitB[x]) >> 3]));
        }

        pSrc++;
        pDestIn++;
        pDestOut++;
        x++;
    } while (--uiCount != 0);
}

// Generate 555 versions of the routines defined in 'sodither.inc'

#define DITHER_BLEND_555 1

#include "SODither.inc"

// Generate 565 versions of the routines defined in 'sodither.inc'

#undef DITHER_BLEND_555 
#define DITHER_BLEND_555 0

#include "SODither.inc"




