// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:  Defines CHalftone Object Class
//

#include "precomp.hpp"

// CCIR601 Luminosity Coefficients
// 0.299R + 0.587G + 0.114B
FLOAT c_rWeightRed   = 0.299f;
FLOAT c_rWeightGreen = 0.587f;
FLOAT c_rWeightBlue  = 0.114f;

/**************************************************************************\
*
* Function Description:
*
*   Copy a scanline from an unaligned source buffer to
*   an aligned destination buffer.
*
* Arguments:
*
*   dst - Pointer to the destination buffer
*   src - Pointer to the source buffer
*   totalBits - Total number of bits for the scanline
*   startBit - Number of source bits to skip
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
ReadUnalignedScanline(
    __out_bcount_full((totalBits + 7) >> 3) BYTE* dst,
    __in_bcount(((totalBits + startBit) >> 3) + 1) const BYTE* src,
    UINT totalBits,
    __in_range(1, 7) UINT startBit
    )
{
#if DBG_ANALYSIS
    UINT dbgAnalysisTotalBitsOrig = totalBits;
#endif

    // Process the whole bytes in the destination
    // NOTE: we probably could be faster doing DWORD reads/writes
    // at the expense of more complicated code. Since this code
    // path is rare, we'll take the simple route.

    UINT bytecnt = totalBits >> 3;
    UINT rem = 8 - startBit;


    while (bytecnt--)
    {
        *dst++ = (src[0] << startBit) | (src[1] >> rem);
        src++;
    }

    // Handle the last partial byte

    if ((totalBits &= 7) != 0)
    {
    #if DBG_ANALYSIS
        // These bit operations are too fancy for prefast. We help it out with
        // this Assert.
        Assert(   ((dbgAnalysisTotalBitsOrig + 7) >> 3)
               == ((dbgAnalysisTotalBitsOrig >> 3) + 1));
    #endif

        BYTE mask = ~(0xff >> totalBits);
        BYTE val = (src[0] << startBit);

        if (totalBits > rem)
        {
        #if DBG_ANALYSIS
            // These bit operations are too fancy for prefast. We help it out with
            // this Assert.
            //
            Assert(   ((dbgAnalysisTotalBitsOrig + startBit) >> 3)
                   == ((dbgAnalysisTotalBitsOrig >> 3) + 1));
        #endif

            val |= (src[1] >> rem);
        }

        *dst = (*dst & ~mask) | (val & mask);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Copy a scanline from an aligned source buffer to
*   an unaligned destination buffer.
*
* Arguments:
*
*   dst - Pointer to the destination buffer
*   src - Pointer to the source buffer
*   totalBits - Total number of bits for the scanline
*   startBit - Number of destination bits to skip
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
WriteUnalignedScanline(
    __inout_bcount(((totalBits + startBit) >> 3) + 1) BYTE* dst,
    __in_bcount((totalBits + 7) >> 3) const BYTE* src,
    UINT totalBits,
    __in_range(1, 7) UINT startBit
    )
{
#if DBG_ANALYSIS
    UINT dbgAnalysisTotalBitsOrig = totalBits;
#endif

    UINT rem = 8-startBit;
    BYTE mask, val;

    // Special case: startBit+totalBits < 8
    //  i.e. destination fits entirely in a partial byte

    if (totalBits < rem)
    {
        mask = (0xff >> startBit);
        mask ^= (mask >> totalBits);

        *dst = (*dst & ~mask) | ((*src >> startBit) & mask);
        return;
    }

    // Handle the first partial destination byte

    *dst = (*dst & ~(0xff >> startBit)) | (*src >> startBit);
    dst++;
    totalBits -= rem; // totalBits = dbgAnalysisTotalBitsOrig + startBit - 8

    // Handle the whole destination bytes

    UINT bytecnt = totalBits >> 3;


    while (bytecnt--)
    {
        *dst++ = (src[0] << rem) | (src[1] >> startBit);
        src++;
    }


    // Handle the last partial destination byte
    if ((totalBits &= 7) != 0)
    {
        mask = ~(0xff >> totalBits);
        val = src[0] << rem;

        if (totalBits > startBit)
        {
        #if DBG_ANALYSIS
            // These bit operations are too fancy for prefast. We help it out with
            // this Assert.
            //
            Assert(   ((dbgAnalysisTotalBitsOrig + 7) >> 3)
                   == (((dbgAnalysisTotalBitsOrig + startBit - 8) >> 3) + 2));
        #endif
            val |= src[1] >> startBit;
        }

        *dst = (*dst & ~mask) | (val & mask);
    }
}

/**************************************************************************
*
* Function Description:
*
*   Convert 32bpp PARGB to grayscale.
*
* Created:
*
*   10/10/2001 asecchia
*      Created it.
*
**************************************************************************/

void FASTCALL
Convert_32bppARGB_8Gray(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const GpCC *pSrc = static_cast<const GpCC*>(pSOP->m_pvSrc1);
    BYTE *pbDest = static_cast<BYTE*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    for (UINT i=0; i<uiCount; i++)
    {
        // ignore alpha, convert RGB to grayscale using ITU-R BT.709/3

        *pbDest = static_cast<BYTE>(GpRound(
            pSrc->b * c_rWeightBlue +
            pSrc->g * c_rWeightGreen +
            pSrc->r * c_rWeightRed
            ));

        // next pixel.

        pSrc++;
        pbDest++;
    }
}

/**************************************************************************
*
* Function Description:
*
*   Convert 32bpp (P)ARGB to the same, but in grayscale, so all the channels
*   have uniform intensity.
*
*
**************************************************************************/

void FASTCALL
Convert_32bppARGB_Grayscale(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const GpCC *pSrc = static_cast<const GpCC*>(pSOP->m_pvSrc1);
    GpCC *pbDest = static_cast<GpCC *>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    for (UINT i=0; i<uiCount; i++)
    {
        //
        // convert RGB to grayscale (but keep it in 4 channels) using ITU-R
        // BT.709/3
        //

        BYTE grayValue = static_cast<BYTE>(GpRound(
            pSrc->b * c_rWeightBlue +
            pSrc->g * c_rWeightGreen +
            pSrc->r * c_rWeightRed
            ));

        pbDest->a = pSrc->a;
        pbDest->b = pbDest->g = pbDest->r = grayValue;

        // next pixel.

        pSrc++;
        pbDest++;
    }
}

/**************************************************************************
*
* Function Description:
*
*   Convert grayscale to 32bpp (P)ARGB.
*   note - alpha channel is 0xff so the output can be considered premultiplied
*   or not - whichever is more important.
*
*
**************************************************************************/

void FASTCALL
Convert_4Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const BYTE *pbSrc = static_cast<const BYTE*>(pSOP->m_pvSrc1);
    GpCC *pDest = static_cast<GpCC*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    while (uiCount > 0)
    {
        const BYTE b = *pbSrc ++;

        // scale from 0..15 to 0..255 (255 / 15 == 17)
        BYTE bGray = (b >> 4) * 17; // high nibble

        pDest->b = bGray;
        pDest->g = bGray;
        pDest->r = bGray;
        pDest->a = 0xff;
        pDest++;

        uiCount --;

        if (uiCount > 0)
        {
            bGray = (b & 0x0F) * 17; // low nibble

            pDest->b = bGray;
            pDest->g = bGray;
            pDest->r = bGray;
            pDest->a = 0xff;
            pDest++;

            uiCount --;
        }
    }
}


/**************************************************************************
*
* Function Description:
*
*   Convert grayscale to 32bpp (P)ARGB.
*   note - alpha channel is 0xff so the output can be considered premultiplied
*   or not - whichever is more important.
*
*
**************************************************************************/

void FASTCALL
Convert_2Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const BYTE *pbSrc = static_cast<const BYTE*>(pSOP->m_pvSrc1);
    GpCC *pDest = static_cast<GpCC*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    while (uiCount > 0)
    {
        BYTE b = *pbSrc ++;

        UINT c = min(4U, uiCount);

        uiCount -= c;

        while (c)
        {
            // scale from 0..3 to 0..255 (255 / 3 == 85)
            BYTE bGray = (b >> 6) * 85;
            b <<= 2;

            pDest->b = bGray;
            pDest->g = bGray;
            pDest->r = bGray;
            pDest->a = 0xff;
            pDest++;

            c --;
        }
    }
}



/**************************************************************************
*
* Function Description:
*
*   Convert grayscale to 32bpp (P)ARGB.
*   note - alpha channel is 0xff so the output can be considered premultiplied
*   or not - whichever is more important.
*
* Created:
*
*   12/28/2001 asecchia
*      Created it.
*
**************************************************************************/

void FASTCALL
Convert_8Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const BYTE *pbSrc = static_cast<const BYTE*>(pSOP->m_pvSrc1);
    GpCC *pDest = static_cast<GpCC*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    for (UINT i=0; i<uiCount; i++)
    {
        const BYTE bGray = *pbSrc;

        pDest->b = bGray;
        pDest->g = bGray;
        pDest->r = bGray;
        pDest->a = 0xff;

        // next pixel.

        pbSrc++;
        pDest++;
    }
}

/**************************************************************************
*
* Function Description:
*
*   Gamma-convert 128bppABGR to 32bppARGB.
*
* Created:
*
*   06/04/2002 asecchia
*      Created it.
*
**************************************************************************/

void FASTCALL
GammaConvert_128bppABGR_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const MilColorF *pSrc = static_cast<const MilColorF*>(pSOP->m_pvSrc1);
    GpCC *pccDest = static_cast<GpCC*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    for (UINT i=0; i<uiCount; i++)
    {
        // Gamma convert the color channels to 2.2 space from the linear
        // space used by the float formats.

        pccDest->b = Convert_scRGB_Channel_To_sRGB_Byte(pSrc->b);
        pccDest->g = Convert_scRGB_Channel_To_sRGB_Byte(pSrc->g);
        pccDest->r = Convert_scRGB_Channel_To_sRGB_Byte(pSrc->r);

        // Alpha is always linear, even in 32bpp.

        pccDest->a = ByteSaturate(GpRound(255.0f*pSrc->a));

        // next pixel.

        pccDest++;
        pSrc++;
    }
}


/**************************************************************************
*
* Function Description:
*
*   Gamma-convert 32bppARGB to 128bppABGR.
*
* Created:
*
*   06/04/2002 asecchia
*      Created it.
*
**************************************************************************/

void FASTCALL
GammaConvert_32bppARGB_128bppABGR(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    // source and dest cast to an appropriate format.

    const GpCC *pccSrc = static_cast<const GpCC*>(pSOP->m_pvSrc1);
    MilColorF *pDest = static_cast<MilColorF*>(pSOP->m_pvDest);
    UINT uiCount = pPP->m_uiCount;

    for (UINT i=0; i<uiCount; i++)
    {
        // Gamma convert the color channels to 1.0 space from the 2.2
        // space used by the 32bpp formats.

        pDest->b = GammaLUT_sRGB_to_scRGB[pccSrc->b] / 255.0f;
        pDest->g = GammaLUT_sRGB_to_scRGB[pccSrc->g] / 255.0f;
        pDest->r = GammaLUT_sRGB_to_scRGB[pccSrc->r] / 255.0f;

        // Alpha is always linear, even in 32bpp.

        pDest->a = float(pccSrc->a) / 255.0f;

        // next pixel.

        pDest++;
        pccSrc++;
    }
}





