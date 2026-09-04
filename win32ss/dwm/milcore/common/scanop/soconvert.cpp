// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      The "Convert" scan operation. See scanoperation.h.
//
//      This module implements scan operations for converting pixels from one
//      format, to another of equal or greater color precision. (Conversion to a
//      lesser color precision is done with either a "Quantize" operation or a
//      "Halftone" operation.)
//
// Notes:
//
//      If the source format doesn't have alpha, we assume an alpha of 1.
//
//      If the source format has a palette, it is supplied in pScanOpParam
//      (SOPPalette).
//
//      When converting to greater color precision, we need to be careful.
//      The operation must:
//        + Map 0 to 0
//        + Map the maximum value to the maxmimum value (e.g. in 555->32bpp, it
//          must map 31 to 255).
//
//      In addition, we desire that the mapping is as close to linear as
//      possible.
//
//      Currently (12/16/1999), our 16bpp->32bpp code does have slight rounding
//      errors. e.g. we get a different value from "round(x*31/255)" when x is
//      3, 7, 24, or 28. This is probably acceptable. We could also speed the
//      code up by using byte lookup tables. (From an unpublished paper by Blinn
//      & Marr of MSR.)
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      Convert: Binary operation; converts pixel format "upwards" (often to
//               32bppARGB).
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan.
//      pSOP->m_pvSrc1:   The source scan.
//      pPP->m_uiCount:    Scan length, in pixels.
//
//      OSDPalette:       Used for palettized formats.
//
//  Return Value:
//
//      None
//
//  Notes:
//
//      The output format is non-premultiplied. But if we're sure the input data
//      is opaque, we can act as if the output format is PARGB or RGB. Beware,
//      though - palettes can have alpha.
//

// Convert from 1bpp indexed to 32bppARGB

VOID FASTCALL
Convert_1_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    const ColorPalette *ppal = DYNCAST(OSDPalette, pSOP->m_posd)->m_pPalette;
    Assert(ppal);
    Assert(ppal->Count >= 2);

    UINT n, bits;

    ARGB c0 = ppal->Entries[0];
    ARGB c1 = ppal->Entries[1];

    // NOTE: We choose code size over speed here

    while (uiCount)
    {
        bits = *pSrc++;
        n = uiCount > 8 ? 8 : uiCount;
        uiCount -= n;

        while (n--)
        {
            *pDest++ = (bits & 0x80) ? c1 : c0;
            bits <<= 1;
        }
    }
}

VOID FASTCALL
Convert_1BW_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    UINT n, bits;

    const ARGB c0 = 0xff000000; // black (opaque)
    const ARGB c1 = 0xffffffff; // white

    // NOTE: We choose code size over speed here

    while (uiCount)
    {
        bits = *pSrc++;
        n = uiCount > 8 ? 8 : uiCount;
        uiCount -= n;

        while (n--)
        {
            *pDest++ = (bits & 0x80) ? c1 : c0;
            bits <<= 1;
        }
    }
}


// Convert from 4bpp indexed to 32bppARGB

VOID FASTCALL
Convert_4_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    const ColorPalette *ppal = DYNCAST(OSDPalette, pSOP->m_posd)->m_pPalette;
    Assert(ppal);

    const ARGB* colors = ppal->Entries;
    UINT n = uiCount >> 1;

    // Handle whole bytes

    while (n--)
    {
        UINT bits = *pSrc++;

        Assert((bits >> 4)  < ppal->Count);
        Assert((bits & 0xf) < ppal->Count);

        pDest[0] = colors[bits >> 4];
        pDest[1] = colors[bits & 0xf];

        pDest += 2;
    }

    // Handle the last odd nibble, if any

    if (uiCount & 1)
        *pDest = colors[*pSrc >> 4];
}


// Convert from 2bpp indexed to 32bppARGB

VOID FASTCALL
Convert_2_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    const ColorPalette *ppal = DYNCAST(OSDPalette, pSOP->m_posd)->m_pPalette;
    Assert(ppal);

    const ARGB* colors = ppal->Entries;

    while (uiCount)
    {
        BYTE bits = *pSrc++;

        UINT c = min(4U, uiCount);

        uiCount -= c;

        while (c)
        {
            UINT i = bits >> 6;

            bits <<= 2;

            Assert(i  < ppal->Count);

            * pDest++ = colors[i];

            c --;
        }
    }
}


// Convert from 8bpp indexed to 32bppARGB

VOID FASTCALL
Convert_8_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    const ColorPalette *ppal = DYNCAST(OSDPalette, pSOP->m_posd)->m_pPalette;
    Assert(ppal);

    const ARGB* colors = ppal->Entries;

    while (uiCount--)
    {
#if DBG
        if (*pSrc >= ppal->Count)
        {
            TraceTag((tagMILWarning,
                "Palette missing entries on conversion from 8bpp to 32bppARGB"));
        }
#endif
        *pDest++ = colors[*pSrc++];
    }
}

// Convert 16bpp RGB555 to 32bppARGB

VOID FASTCALL
Convert_555_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB v = *pSrc++;
        ARGB r = (v >> 10) & 0x1f;
        ARGB g = (v >>  5) & 0x1f;
        ARGB b = (v      ) & 0x1f;

        *pDest++ = MIL_ALPHA_MASK |
               (((r << 3) | (r >> 2)) << MIL_RED_SHIFT) |
               (((g << 3) | (g >> 2)) << MIL_GREEN_SHIFT) |
               (((b << 3) | (b >> 2)) << MIL_BLUE_SHIFT);
    }
}

// Convert from 16bppRGB565 to 32bppARGB

VOID FASTCALL
Convert_565_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB v = *pSrc++;
        ARGB r = (v >> 11) & 0x1f;
        ARGB g = (v >>  5) & 0x3f;
        ARGB b = (v      ) & 0x1f;

        *pDest++ = MIL_ALPHA_MASK |
              (((r << 3) | (r >> 2)) << MIL_RED_SHIFT) |
              (((g << 2) | (g >> 4)) << MIL_GREEN_SHIFT) |
              (((b << 3) | (b >> 2)) << MIL_BLUE_SHIFT);
    }
}

// Convert from 16bppARGB1555 to 32bppARGB

VOID FASTCALL
Convert_1555_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(WORD, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        ARGB v = *pSrc++;
        ARGB a = (v & 0x8000) ? MIL_ALPHA_MASK : 0;
        ARGB r = (v >> 10) & 0x1f;
        ARGB g = (v >>  5) & 0x1f;
        ARGB b = (v      ) & 0x1f;

        *pDest++ = a |
               (((r << 3) | (r >> 2)) << MIL_RED_SHIFT) |
               (((g << 3) | (g >> 2)) << MIL_GREEN_SHIFT) |
               (((b << 3) | (b >> 2)) << MIL_BLUE_SHIFT);
    }
}

// Convert from 24bppRGB to 32bppARGB

VOID FASTCALL
Convert_24_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        *pDest++ = MIL_ALPHA_MASK |
               ((ARGB) pSrc[0] << MIL_BLUE_SHIFT) |
               ((ARGB) pSrc[1] << MIL_GREEN_SHIFT) |
               ((ARGB) pSrc[2] << MIL_RED_SHIFT);

        pSrc += 3;
    }
}

// Convert from 24bppBGR to 32bppARGB

VOID FASTCALL
Convert_24BGR_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        *pDest++ = MIL_ALPHA_MASK |
               ((ARGB) pSrc[0] << MIL_RED_SHIFT) |
               ((ARGB) pSrc[1] << MIL_GREEN_SHIFT) |
               ((ARGB) pSrc[2] << MIL_BLUE_SHIFT);

        pSrc += 3;
    }
}

// Convert from 32bppRGB to 32bppARGB

VOID FASTCALL
Convert_32RGB_32bppARGB(
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

// Convert from 48bppRGB to 64bppARGB

VOID FASTCALL
Convert_48_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(INT16, ARGB64)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.a = 0xffff;
        c.b = pSrc[0];
        c.g = pSrc[1];
        c.r = pSrc[2];

        *pDest++ = c.argb;

        pSrc += 3;
    }
}

// Convert from 16bppGray to 64bppARGB

VOID FASTCALL
Convert_16bppGray_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(INT16, ARGB64)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.a = 0xffff;
        c.b =
        c.g =
        c.r = *pSrc++;

        //  This format is said to be
        // "useful for monochrome images and alpha channels"
        // (
        // Do we need separate routine to treat source following way:
        //c.a = *pSrc++;
        //c.b =
        //c.g =
        //c.r = SRGB_ONE;
        // Or to have special format, say 16bppAlpha?

        *pDest++ = c.argb;
    }
}

// Convert from 128bppBGR to 128bppABGR

VOID FASTCALL
Convert_128RGB_128bppABGR(
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

// Convert from 32bppRGB101010 to 128bppABGR

VOID FASTCALL
Convert_32bppRGB101010_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(DWORD, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        MilColorF colDest;
        DWORD dwSrc = *pSrc++;

        colDest.b = (dwSrc & 1023) / 1023.0f;
        dwSrc = dwSrc >> 10;
        colDest.g = (dwSrc & 1023) / 1023.0f;
        dwSrc = dwSrc >> 10;
        colDest.r = (dwSrc & 1023) / 1023.0f;
        colDest.a = 1.0f;

        *pDest++ = colDest;
    }
}

// 64bppARGB to 48bppRGBInt

VOID FASTCALL
Convert_64bppARGB_48bppRGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        pDest[0] = pSrc[0];
        pDest[1] = pSrc[1];
        pDest[2] = pSrc[2];

        pSrc += 4;
        pDest += 3;
    }
}

// 48bppRGBInt to 64bppARGB

VOID FASTCALL
Convert_48bppRGB_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(UINT16, UINT16)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        pDest[0] = pSrc[0];
        pDest[1] = pSrc[1];
        pDest[2] = pSrc[2];
        pDest[3] = 0xffff;
        
        pDest += 4;
        pSrc += 3;
    }
}

// 32bppARGB to 64bppARGB

VOID FASTCALL
Convert_32bppARGB_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(UINT8, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        (*pDest++) = ((pSrc[2]) << 8) + (pSrc[2]);
        (*pDest++) = ((pSrc[1]) << 8) + (pSrc[1]);
        (*pDest++) = ((pSrc[0]) << 8) + (pSrc[0]);
        (*pDest++) = ((pSrc[3]) << 8) + (pSrc[3]);
        pSrc+=4;
    }
}

// 64bppARGB to 32bppARGB

VOID FASTCALL
Convert_64bppARGB_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(UINT16, UINT8)
    UINT uiCount = pPP->m_uiCount;    
    
    while (uiCount--)
    {
        *pDest++ = (pSrc[2]) >> 8;
        *pDest++ = (pSrc[1]) >> 8;
        *pDest++ = (pSrc[0]) >> 8;
        *pDest++ = (pSrc[3]) >> 8;
        pSrc+=4;
    }
}


//+---------------------------------------------------------------------------
//
// Function:    Convert_32bppCMYK_64bppARGB
//
// Synopsis:    ***  STUB  ***
//
//      Converts scanline from 32bpp CMYK to 64bpp sRGB.
//      This is not yet real routine, but it supposed
//      to produce recognizable image.
//          
//----------------------------------------------------------------------------
VOID FASTCALL
Convert_32bppCMYK_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(BYTE, ARGB64)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        UINT C = pSrc[0];   // cyan
        UINT M = pSrc[1];   // magenta
        UINT Y = pSrc[2];   // yellow
        UINT K = pSrc[3];   // black

        UINT R = (255-C)*(255-K);
        UINT G = (255-M)*(255-K);
        UINT B = (255-Y)*(255-K);

        // stretch 255*255 -> 0x2000
        UINT ratio = 0x2041; 

        GpCC64 c;
        c.a = SRGB_ONE;
        c.b = (B*ratio)>>16;
        c.g = (G*ratio)>>16;
        c.r = (R*ratio)>>16;

        *pDest++ = c.argb;
        pSrc += 4;
    }
}





