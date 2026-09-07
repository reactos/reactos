// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      The "AlphaMultiply" and "AlphaDivide" scan operations. See
//      scanoperation.h.
//
//      These scan operations multiply/divide the color components by the alpha
//      component. API-level input colors are (usually) specified in
//      'non-premultiplied'. Given a non-premultiplied color (R, G, B, A), its
//      'premultiplied' form is (RA, GA, BA, A).
//
//  Notes:
//
//      Since "AlphaMultiply" loses information, "AlphaDivide" is not a true
//      inverse operation. (But it is an inverse if all pixels have an alpha of
//      1.)
//
//      If the alpha is 0, the output pixel will be all 0.
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      AlphaMultiply/AlphaDivide: Binary operation; converts between
//                                 premultiplied and non-premultiplied alpha.
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan.
//      pSOP->m_pvSrc1:   The source scan. May equal m_pvDest.
//      pPP->m_uiCount:    Scan length, in pixels.
//
//  Return Value:
//
//      None
//

// AlphaDivide from 32bppPARGB (to 32bppARGB)

VOID FASTCALL
AlphaDivide_32bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC c;
        c.argb = *pSrc;
        if (c.a != 255)
        {
            if (c.a != 0)
            {
                c.argb = Unpremultiply(c.argb);
            }
            else
            {
                c.argb = 0;
            }
        }
        *pDest = c.argb;
        pDest++;
        pSrc++;
    }
}

// AlphaMultiply from 32bppARGB (to 32bppPARGB)

VOID FASTCALL
AlphaMultiply_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC c;
        c.argb = *pSrc;
        ARGB alpha = c.argb & 0xff000000;
        
        if (alpha != 0xff000000)
        {
            if (alpha != 0x00000000)
            {
                c.argb = MyPremultiply(c.argb);
            }
            else
            {
                c.argb = 0;
            }
        }
        *pDest = c.argb;
        pDest++;
        pSrc++;
    }
}

// !!![agodfrey] We may want to round off, in both AlphaDivide_64bppPARGB and
//     AlphaMultiply_64bppARGB.

// AlphaDivide from 64bppPARGB (to 64bppARGB)

VOID FASTCALL
AlphaDivide_64bppPARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.argb = *pSrc;
        if (c.a != 0xffff)
        {
            if (c.a != 0)
            {
                c.r = (UINT16)min((UINT)0xffff, ((UINT) c.r << 16) / c.a);
                c.g = (UINT16)min((UINT)0xffff, ((UINT) c.g << 16) / c.a);
                c.b = (UINT16)min((UINT)0xffff, ((UINT) c.b << 16) / c.a);
            }
            else
            {
                c.argb = 0;
            }
        }
        *pDest = c.argb;
        pDest++;
        pSrc++;
    }
}

// AlphaMultiply from 64bppARGB (to 64bppPARGB)

VOID FASTCALL
AlphaMultiply_64bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        GpCC64 c;
        c.argb = *pSrc;
        if (c.a != 0xffff)
        {
            if (c.a != 0)
            {
                c.r = ((INT) c.r * c.a) >> 16;
                c.g = ((INT) c.g * c.a) >> 16;
                c.b = ((INT) c.b * c.a) >> 16;
            }
            else
            {
                c.argb = 0;
            }
        }
        *pDest = c.argb;
        pDest++;
        pSrc++;
    }
}

// AlphaDivide from 128bppPABGR (to 128bppABGR)

VOID FASTCALL
AlphaDivide_128bppPABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        MilColorF c = *pSrc;
        if (c.a != 1.0f)
        {
            if (c.a != 0.0f)
            {
                //   FP overflow in AlphaDivide
                //  If c.a is very close to zero, this can overflow. I know the
                //  quick fix (use an epsilon), but that's not right either.
                //
                //  This code also needs to do the right thing regarding > 1.0
                //  and < 0.0 values - whatever that is. Maybe the alpha channel
                //  should be clamped to the [0, 1] range. But that's not right
                //  for the color channels.
                
                FLOAT invA = 1.0f / c.a;

                c.r = c.r * invA;
                c.g = c.g * invA;
                c.b = c.b * invA;
            }
            else
            {
                c.r = c.g = c.b = 0.0f;
            }
        }
        *pDest = c; 
        pDest++;
        pSrc++;
    }
}

// AlphaMultiply from 128bppABGR (to 128bppPABGR)

VOID FASTCALL
AlphaMultiply_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(MilColorF, MilColorF)
    UINT uiCount = pPP->m_uiCount;

    while (uiCount--)
    {
        MilColorF c = *pSrc;
        if (c.a != 1.0f)
        {
            if (c.a != 0.0f)
            {
                c.r = c.r * c.a;
                c.g = c.g * c.a;
                c.b = c.b * c.a;
            }
            else
            {
                c.r = c.g = c.b = 0.0f;
            }
        }
        *pDest = c; 
        pDest++;
        pSrc++;
    }
}





