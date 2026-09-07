// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      The "GammaConvert" scan operation. See Gdiplus\Specs\ScanOperation.doc
//      for an overview.
//
//      These operations convert from one format to another, accounting for
//      differing gamma ramps.
//

#include "precomp.hpp"

extern FLOAT c_rWeightRed;
extern FLOAT c_rWeightGreen;
extern FLOAT c_rWeightBlue;  


//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      GammaConvert: Binary operation; converts between an sRGB format and an
//                    scRGB format, accounting for differing gamma ramps.
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
//      Gamma conversion must be done between non-premultiplied formats.
//      Premultiplied data must have AlphaDivide applied to it (and
//      AlphaMultiply afterwards, if necessary.)
//

// Convert from 64bppARGB to 128bppABGR

VOID FASTCALL
GammaConvert_64bppARGB_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(INT16, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        MilColorF c;

        c.r = Convert_sRGB_UINT16_To_scRGB_float(pSrc[0]);
        c.g = Convert_sRGB_UINT16_To_scRGB_float(pSrc[1]);
        c.b = Convert_sRGB_UINT16_To_scRGB_float(pSrc[2]);
        c.a = Convert_sRGB_UINT16_To_scRGB_float(pSrc[3]);

        *pDest++ = c;
        pSrc += 4;
    }
}

// 16bppGrayInt to 128bppABGR

VOID FASTCALL
GammaConvert_16bppGrayInt_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(UINT16, MilColorF)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        pDest->a = 1.f;
        pDest->b =
        pDest->g =
        pDest->r = Convert_sRGB_UINT16_To_scRGB_float(*pSrc++);

        pDest++;
    }
}

// 32bppGrayFloat to 128bppABGR

VOID FASTCALL
GammaConvert_32bppGrayFloat_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(float, MilColorF)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        pDest->a = 1.f;
        pDest->b =
        pDest->g =
        pDest->r = *pSrc++;     // This is already a float

        pDest++;
    }
}

// 128bppABGR to 16bppGrayInt

VOID FASTCALL
GammaConvert_128bppABGR_16bppGrayInt(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        float f = (pSrc->g*c_rWeightGreen + 
             pSrc->b*c_rWeightBlue + 
             pSrc->r*c_rWeightRed);        
        // we might prefer (pSrc->r + 2*pSrc->g + pSrc->b)*.25
        *pDest++ = Convert_scRGB_float_To_sRGB_UINT16(f);
        pSrc++;
    }
}

// 128bppABGR to 32bppGrayFloat

VOID FASTCALL
GammaConvert_128bppABGR_32bppGrayFloat(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, float)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        float f = (float)((pSrc->g*c_rWeightGreen + 
             pSrc->b*c_rWeightBlue + 
             pSrc->r*c_rWeightRed));
        f = max(min(f, 1.0f), 0.0f);
        *pDest++ = f;
        pSrc++;
    }
}

// 128bppABGR to 64bppRGBInt

VOID FASTCALL
GammaConvert_128bppABGR_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, UINT16)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        pDest[0] = Convert_scRGB_float_To_sRGB_UINT16(pSrc->r);
        pDest[1] = Convert_scRGB_float_To_sRGB_UINT16(pSrc->g);
        pDest[2] = Convert_scRGB_float_To_sRGB_UINT16(pSrc->b);
        pDest[3] = Convert_scRGB_float_To_sRGB_UINT16(pSrc->a);

        pSrc++;
        pDest += 4;
    }
}





