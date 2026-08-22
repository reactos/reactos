// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      The "Quantize" scan operation. See scanoperation.h.
//
//      This module implements scan operations for converting pixels from one
//      format, to another of less color precision. "Quantize" uses a simple,
//      fixed mapping, which maps each source color level to a particular
//      destination color level.
//
//  Notes:
//
//      The "Quantize" operation is fast but can cause Mach banding. An
//      alternative is the "Halftone" operation, in SOHalftone.cpp.
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      Quantize: Binary operation; Quickly convert format down from 32bpp ARGB.
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan.
//      pSOP->m_pvSrc1:   The source scan.
//      pPP->m_uiCount:   Scan length, in pixels.
//
//  Return Value:
//
//      None
//
//  Notes:
//
//      These functions convert from non-premultiplied formats (e.g. 32bppARGB).
//      If the input data is known to be opaque, then they can also be used to
//      convert from premultiplied formats.
//

// Quantize from 32bppARGB to 16bppRGB555

extern FLOAT c_rWeightRed;
extern FLOAT c_rWeightGreen;
extern FLOAT c_rWeightBlue;  

VOID FASTCALL
Quantize_32bppARGB_555(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB argb = *pSrc++;

        *pDest++ = (WORD) ((((argb >> (MIL_RED_SHIFT+3))   & 0x1f) << 10) |
                           (((argb >> (MIL_GREEN_SHIFT+3)) & 0x1f) << 5) |
                            ((argb >> (MIL_BLUE_SHIFT+3))  & 0x1f));
    }
}

// Quantize from 32bppARGB to 16bppRGB565

VOID FASTCALL
Quantize_32bppARGB_565(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB argb = *pSrc++;

        *pDest++ = (WORD) ((((argb >> (MIL_RED_SHIFT+3))   & 0x1f) << 11) |
                           (((argb >> (MIL_GREEN_SHIFT+2)) & 0x3f) << 5) |
                            ((argb >> (MIL_BLUE_SHIFT+3))  & 0x1f));
    }
}

// Quantize from 32bppARGB to 16bppRGB1555

VOID FASTCALL
Quantize_32bppARGB_1555(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, WORD)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB argb = *pSrc++;

        // NOTE: Very crude conversion of alpha data
        // from 8bpp down to 1bpp

        *pDest++ = (WORD) ((((argb >> MIL_ALPHA_SHIFT) >= 128) ? 0x8000 : 0) |
                           (((argb >> (MIL_RED_SHIFT+3))   & 0x1f) << 10) |
                           (((argb >> (MIL_GREEN_SHIFT+3)) & 0x1f) << 5) |
                            ((argb >> (MIL_BLUE_SHIFT+3))  & 0x1f));
    }
}

// Quantize from 32bppARGB to 24bppRGB

VOID FASTCALL
Quantize_32bppARGB_24(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, BYTE)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB argb = *pSrc++;

        pDest[0] = (BYTE) (argb >> MIL_BLUE_SHIFT);
        pDest[1] = (BYTE) (argb >> MIL_GREEN_SHIFT);
        pDest[2] = (BYTE) (argb >> MIL_RED_SHIFT);
        pDest += 3;
    }
}

// Quantize from 32bppARGB to 24bppBGR

VOID FASTCALL
Quantize_32bppARGB_24BGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, BYTE)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB argb = *pSrc++;

        pDest[0] = (BYTE) (argb >> MIL_RED_SHIFT);
        pDest[1] = (BYTE) (argb >> MIL_GREEN_SHIFT);
        pDest[2] = (BYTE) (argb >> MIL_BLUE_SHIFT);
        pDest += 3;
    }
}

// Quantize from 32bppARGB to 32bppRGB

VOID FASTCALL
Quantize_32bppARGB_32RGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        *pDest++ = *pSrc++ | MIL_ALPHA_MASK;
    }
}

// Quantize from 64bppARGB to 48bppRGB

VOID FASTCALL
Quantize_64bppARGB_48(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB64, INT16)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.argb = *pSrc++;

        pDest[0] = c.b;
        pDest[1] = c.g;
        pDest[2] = c.r;
        pDest += 3;
    }
}

// Quantize from 64bppARGB to 16bppGray

VOID FASTCALL
Quantize_64bppARGB_16bppGray(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB64, UINT16)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.argb = *pSrc++;

        *pDest++ = (UINT16)((float)c.g*c_rWeightGreen + 
                            (float)c.b*c_rWeightBlue + 
                            (float)c.r*c_rWeightRed);
        //  This format is said to be
        // "useful for monochrome images and alpha channels"
        // (
        // Do we need separate routine to treat source following way:
        // *pDest++ = c.a;
        // Or to have special format, say 16bppAlpha?
        //
        // 2) When converting real colored data to gray we may want to
        // average color components following way:
        // *pDest++ = (INT16)(((INT)c.b + 2*(INT)c.g + (INT)c.r) / 4);
        // and/or multiply result by alpha.
    }
}

MIL_FORCEINLINE UINT16 Saturate16bpc(INT i)
{
    if (i > SRGB_MAX)
    {
        i = SRGB_MAX;
    }
    else if (i < SRGB_MIN)
    {
        i = SRGB_MIN;
    }

    return(UINT16)i;
}


// Quantize from 128bppABGR to 128bppBGR

VOID FASTCALL
Quantize_128bppABGR_128RGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        MilColorF c = *pSrc++;
        c.a = 1.0f;
        *pDest++ = c;
    }
}

inline INT Saturate10Bit(INT x)
{
    if (x < 0)
    {
        return 0;
    }
    if (x > 1023)
    {
        return 1023;
    }
    return x;
}

inline INT Saturate3Bit(INT x)
{
    if (x < 0)
    {
        return 0;
    }
    if (x > 3)
    {
        return 3;
    }
    return x;
}

// Quantize from 128bppABGR to 32bppRGB101010

VOID FASTCALL
Quantize_128bppABGR_32bppRGB101010(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, DWORD)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        DWORD dwDest;
        MilColorF colSrc = *pSrc++;

        DWORD dwR = Saturate10Bit(GpRound(colSrc.r * 1023.0f));
        DWORD dwG = Saturate10Bit(GpRound(colSrc.g * 1023.0f));
        DWORD dwB = Saturate10Bit(GpRound(colSrc.b * 1023.0f));

        dwDest =  (  3 << 30)  // For compatibility with D3D's 2-10-10-10 format.
                | (dwR << 20)
                | (dwG << 10)
                |  dwB;

        *pDest++ = dwDest;
    }
}

//+---------------------------------------------------------------------------
//
// Function:    Quantize_64bppARGB_32bppCMYK
//
// Synopsis:    ***  STUB  ***
//
//      Converts scanline from 64bpp sRGB format to 32bpp CMYK.
//      This is not yet real routine, but it supposed
//      to produce recognizable image.
//          
//----------------------------------------------------------------------------
VOID FASTCALL
Quantize_64bppARGB_32bppCMYK(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(GpCC64, BYTE)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c = *pSrc++;

        UINT sR = (UINT)c.r*(UINT)c.a;
        UINT sG = (UINT)c.g*(UINT)c.a;
        UINT sB = (UINT)c.b*(UINT)c.a;

        // scale SRGB_ONE*SRGB_ONE -> 255 and clamp;

#define CLAMP_SCALE(x) (BYTE)((x) >= SRGB_ONE*SRGB_ONE ? 255 : ((x)-((x)>>8))>>(SRGB_FRACTIONBITS*2 - 8))

        BYTE R = CLAMP_SCALE(sR);
        BYTE G = CLAMP_SCALE(sG);
        BYTE B = CLAMP_SCALE(sB);

        BYTE C = ~R;
        BYTE M = ~G;
        BYTE Y = ~B;
        BYTE K = 0;

        pDest[0] = C;
        pDest[1] = M;
        pDest[2] = Y;
        pDest[3] = K;

        *pDest += 4;
    }
}







