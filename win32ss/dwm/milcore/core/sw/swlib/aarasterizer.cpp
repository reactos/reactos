// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:  Code for rasterizing the fill of a path.
//
//  >>>> Note that some of this code is duplicated in hw\hwrasterizer.cpp,
//  >>>> so changes to this file may need to propagate.
//
//   pursue reduced code duplication
//

#include "precomp.hpp"

MtDefine(MAARasterizerEdge, MILRawMemory, "MAARasterizerEdge");
MtDefine(MAARasterizerInterval, MILRawMemory, "MAARasterizerInterval");

// This option may potentially increase performance for many
// paths that have edges adjacent at their top point and cover
// more than one span.  The code has been tested, but performance
// has not been thoroughly investigated.
#define SORT_EDGES_INCLUDING_SLOPE  0


/////////////////////////////////////////////////////////////////////////
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.
//
// QUOTIENT_REMAINDER_64_32 takes a 64-bit numerator and produces 32-bit
// results.

#if defined(_USE_X86_ASSEMBLY)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#define QUOTIENT_REMAINDER_64_32(ullNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                                            \
    ULONG ulNumeratorLow = *((ULONG*) &ullNumerator);                        \
    ULONG ulNumeratorHigh = *((ULONG*) &ullNumerator + 1);                   \
    __asm mov eax, ulNumeratorLow                                            \
    __asm mov edx, ulNumeratorHigh                                           \
    __asm div ulDenominator                                                  \
    __asm mov ulQuotient, eax                                                \
    __asm mov ulRemainder, edx                                               \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#define QUOTIENT_REMAINDER_64_32(ullNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                                            \
    ulQuotient = (ULONG) ((ULONGLONG) ullNumerator / (ULONG) ulDenominator); \
    ulRemainder = (ULONG) ((ULONGLONG) ullNumerator % (ULONG) ulDenominator);\
}

#endif

// SWAP macro:

#define SWAP(temp, a, b) { temp = a; a = b; b = temp; }

/**************************************************************************\
*
* Function Description:
*
*   The edge initializer is out of room in its current 'store' buffer;
*   get it a new one.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

HRESULT
CEdgeStore::NextAddBuffer(
    __deref_out_ecount(*puRemaining) CEdge **ppCurrentEdge,
    __inout_ecount(1) UINT *puRemaining
    )
{
    HRESULT hr = S_OK;

    UINT cNewTotalCount = 0;

    // The caller has completely filled up this chunk:

    Assert(*puRemaining == 0);

    // Check to make sure that "TotalCount" will be able to represent the current capacity
    cNewTotalCount = TotalCount + CurrentBuffer->Count;

    if (cNewTotalCount < TotalCount)
    {
        IFC(WINCODEC_ERR_VALUEOVERFLOW);
    }

    // And that it can represent the new capacity as well, with at least 2 to spare.
    // This "magic" 2 comes from the fact that the usage pattern of this class has callers
    // needing to allocate space for TotalCount + 2 edges.
    if (cNewTotalCount + (UINT)(EDGE_STORE_ALLOCATION_NUMBER + 2) < cNewTotalCount)
    {
        IFC(WINCODEC_ERR_VALUEOVERFLOW);
    }

    // We have to grow our data structure by adding a new buffer
    // and adding it to the list:

    CEdgeAllocation *newBuffer = static_cast<CEdgeAllocation*>
        (GpMalloc(Mt(MAARasterizerEdge),
                  sizeof(CEdgeAllocation) +
                  sizeof(CEdge) * (EDGE_STORE_ALLOCATION_NUMBER
                                  - EDGE_STORE_STACK_NUMBER)));
    IFCOOM(newBuffer);

    newBuffer->Next = NULL;
    newBuffer->Count = EDGE_STORE_ALLOCATION_NUMBER;

    TotalCount = cNewTotalCount;

    CurrentBuffer->Next = newBuffer;
    CurrentBuffer = newBuffer;

    *ppCurrentEdge = CurrentEdge = &newBuffer->EdgeArray[0];
    *puRemaining = CurrentRemaining = EDGE_STORE_ALLOCATION_NUMBER;

Cleanup:
    RRETURN(hr);
}

#ifdef DBG

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the state of the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
AssertActiveList(
    __in_ecount(1) const CEdge *list,
    INT yCurrent
    )
{
    BOOL b = TRUE;
    INT activeCount = 0;

    Assert(list->X == INT_MIN);
    b &= (list->X == INT_MIN);

    // Skip the head sentinel:

    list = list->Next;

    while (list->X != INT_MAX)
    {
        Assert(list->X != INT_MIN);
        b &= (list->X != INT_MIN);

        Assert(list->X <= list->Next->X);
        b &= (list->X <= list->Next->X);

        Assert((list->StartY <= yCurrent) && (yCurrent < list->EndY));
        b &= ((list->StartY <= yCurrent) && (yCurrent < list->EndY));

        activeCount++;
        list = list->Next;
    }

    Assert(list->X == INT_MAX);
    b &= (list->X == INT_MAX);

    // There should always be a multiple of 2 edges in the active list.
    //
    // NOTE: If you hit this assert, do NOT simply comment it out!
    //       It usually means that all the edges didn't get initialized
    //       properly.  For every scan-line, there has to be a left edge
    //       and a right edge (or a multiple thereof).  So if you give
    //       even a single bad edge to the edge initializer (or you miss
    //       one), you'll probably hit this assert.

    Assert((activeCount & 1) == 0);
    b &= ((activeCount & 1) == 0);

    return(b);
}

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the state of the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
AssertActiveListOrder(
    __in_ecount(1) const CEdge *list
    )
{
    INT activeCount = 0;

    Assert(list->X == INT_MIN);

    // Skip the head sentinel:

    list = list->Next;

    while (list->X != INT_MAX)
    {
        Assert(list->X != INT_MIN);
        Assert(list->X <= list->Next->X);

        activeCount++;
        list = list->Next;
    }

    Assert(list->X == INT_MAX);
}

#endif

//+-----------------------------------------------------------------------------
//
//  Class:     CAntialiasedFiller
//
//  Synopsis:  Antialised filler state.
//

class CAntialiasedFiller : public OpSpecificData
{
private:
    CScanPipeline *m_pScanPipeline;
    CCoverageBuffer m_coverageBuffer;

    COutputSpan *m_pOutputSpan;

    CMILSurfaceRect m_rcComplementBounds;
    float m_rComplementFactor;


    friend VOID FASTCALL ScalePPAACoverage_128bppPRGBA(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP);

    friend VOID FASTCALL ScalePPAACoverage_32bppPBGRA(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP);

    friend VOID FASTCALL ScalePPAACoverage_32bppBGR(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP);

    friend VOID FASTCALL ScalePPAACoverage_Complement_32bppBGR(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP);

    friend VOID FASTCALL ScalePPAACoverage_Complement_32bppBGRA(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP);

    // Implementation of the last 4 32 bit ScalePPAACoverage variants.
    // Note that the input pixel formats are different for complement
    // & non-complement
    //
    //                 HasAlpha          NoAlpha
    // Complement        BGRA              BGR
    // Non-complement    PBGRA             BGR
    //
    // In all cases the output is PBGRA.
    friend static VOID MIL_FORCEINLINE ScalePPAACoverage_32bppPBGRA_Out_Slow(
        __in_ecount(1) const PipelineParams *pPP,
        __in_ecount(1) const ScanOpParams *pSOP,
        bool fHasAlpha,
        bool fUseComplementFactor
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CreateComplementGeometry
    //
    //  Synopsis:  True if we are in complement mode.
    //
    //-------------------------------------------------------------------------
    bool CreateComplementGeometry() const
    {
        return m_rComplementFactor >= 0;
    }

    // Disable instrumentation checks within all methods of this class
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);

public:

    CAntialiasedFiller(
        __in_ecount(1) COutputSpan *pOutputSpan,
        MilAntiAliasMode::Enum antiAliasMode
        )
    {
        m_pOutputSpan = pOutputSpan;

        m_coverageBuffer.Initialize();

        m_rComplementFactor = -1;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    SetComplementFactor
    //
    //  Synopsis:  Enables rendering the inverse shape according to the following
    //             rule:  If rComplementFactor is negative just do render normally.
    //             If rComplementFactor is non-negative then render the complement
    //             of the shape (but in prcBounds) with alpha 1 & the inside of
    //             the shape with alpha 1 - rComplementFactor.
    //
    //                 1          +---------------+
    //                            |               |
    //  NORMAL                    |               |
    //  RENDERING                 |   INSIDE OF   |
    //                            |   THE SHAPE   |
    //                            |               |
    //                 0 ---------+               +------------
    //
    //
    //
    //
    //                 1 ---------+               +------------
    //  COMPLEMENTED              |   ORIGINAL    |
    //  RENDERING                 |    INSIDE     |
    //                 1-factor.. +---------------+
    //
    //
    //                 0 . . . . . . . . . . . . . . . . . . . .
    //
    //-------------------------------------------------------------------------
    void SetComplementFactor(
        float rComplementFactor,
        __in_ecount_opt(1) const CMILSurfaceRect *prcComplementBounds
        )
    {
        Assert(rComplementFactor < 0 || prcComplementBounds != NULL);
        m_rComplementFactor = rComplementFactor;
        if (prcComplementBounds)
        {
            m_rcComplementBounds = *prcComplementBounds;
        }
    }

    ~CAntialiasedFiller()
    {
        // Free the coverage buffer
        m_coverageBuffer.Destroy();
    }

    HRESULT RasterizeEdges(
        __inout_ecount(1) CEdge *pEdgeActiveList,
        __inout_xcount(array terminated by an edge with StartY >= nSubpixelYBottom)
            CInactiveEdge *pInactiveEdgeArray,
        INT nSubpixelYCurrent,
        INT nSubpixelYBottom,
        MilFillMode::Enum fillMode
    );

    // Like RasterizeEdges but for geometry with no edges.  Useful for complement only.
    HRESULT RasterizeNoEdges();

    HRESULT FillEdges(
        MilFillMode::Enum fillMode,
        __in_ecount(1) const CEdge *active,
        INT iCurrentY
        );

    VOID GenerateOutput(INT iCurrentY);
};

//+-----------------------------------------------------------------------------
//
//  Function:  DowncastFiller
//
//  Synopsis:  Casts the given CAntialiasedFiller down to an OpSpecificData.
//             This function is provided so that
//             CScanPipeline::SetAntialiasedFiller can do this cast without
//             requiring CAntialiasedFiller to be in a header file.
//

__ecount(1) OpSpecificData *
DowncastFiller(
    __in_ecount(1) CAntialiasedFiller *pFiller
    )
{
    return static_cast<OpSpecificData *>(pFiller);
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do an alternate-mode
*   antialiased fill.
*
* Created:
*
*   07/20/2003 ashrafm
*
\**************************************************************************/

HRESULT
MIL_FORCEINLINE
CAntialiasedFiller::FillEdges(
    MilFillMode::Enum fillMode,
    __in_ecount(1) const CEdge *activeList,
    INT iCurrentY
    )
{
    HRESULT hr = S_OK;

    // Fill edges using proper fill mode

    if (fillMode == MilFillMode::Winding)
    {
        IFC(m_coverageBuffer.FillEdgesWinding(activeList, iCurrentY));
    }
    else
    {
        Assert(fillMode == MilFillMode::Alternate);
        IFC(m_coverageBuffer.FillEdgesAlternating(activeList, iCurrentY));
    }

    // If the next scan is done, output what's there:

    if (((iCurrentY + 1) & c_nShiftMask) == 0)
    {
        GenerateOutput(iCurrentY);
        m_coverageBuffer.Reset();
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      ScalePPAACoverage: Unary operation: Scales the color data according to
//                         "Per-Primitive AntiAliasing" coverage information.
//
//  Notes:
//
//      Per-primitive antialiasing (PPAA) is a limited but fast AA technique.
//      Unlike full-scene antialiasing (FSAA), PPAA converts coverage
//      information immediately into alpha information. Hence, it assumes that:
//
//      1) SrcOver blend mode is being used.
//      2) Whenever two separate primitives share a pixel, the two edges are
//         "independent" of each other - i.e. edge A evenly divides the area
//         covered by edge B.
//
//      The worst case of (2) occurs when two adjacent primitives share a common
//      edge. The result is that the background "shows through" where it
//      shouldn't, causing a seam.
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan.
//      pPP->m_uiCount:   Scan length, in pixels.
//
//  Return Value:
//
//      None
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Function:  GetOp_ScalePPAACoverage
//
//  Synopsis:  Return a ScalePPAACoverage operation for the given format.
//

ScanOpFunc GetOp_ScalePPAACoverage(
    MilPixelFormat::Enum fmtColorData,
        // Color data format, either 32bppPBGRA, 32bppBGR, or 128bppPABGR, or
        // 32bppBGRA
    bool fComplementAlpha,
        // Should the operation support using complement factor to rescale
        // coverage values
    __out_ecount(1) MilPixelFormat::Enum *pFmtColorOut
        // Color data format after operation
    )
{
    switch (fmtColorData)
    {
    case MilPixelFormat::PBGRA32bpp:
        *pFmtColorOut = MilPixelFormat::PBGRA32bpp;
        if (fComplementAlpha)
        {
            RIP("Don't support complement MilPixelFormat::PBGRA32bpp because it's not currently used.");
        }
        else
        {
            return ScalePPAACoverage_32bppPBGRA;
        }

    case MilPixelFormat::PRGBA128bppFloat:
        *pFmtColorOut = MilPixelFormat::PRGBA128bppFloat;
        // Floating point always supports complement
        return ScalePPAACoverage_128bppPRGBA;

    case MilPixelFormat::BGR32bpp:
        *pFmtColorOut = MilPixelFormat::PBGRA32bpp;
        if (fComplementAlpha)
        {
            return ScalePPAACoverage_Complement_32bppBGR;
        }
        else
        {
            return ScalePPAACoverage_32bppBGR;
        }

    case MilPixelFormat::BGRA32bpp:
        if (fComplementAlpha)
        {
            // Complement can handle non-premultiplied BGRA for handling input with an alpha
            // mask applied.
            *pFmtColorOut = MilPixelFormat::PBGRA32bpp;
            return ScalePPAACoverage_Complement_32bppBGRA;
        }
        else
        {
            RIP("Don't support non-complement MilPixelFormat::BGRA32bpp.");
        }


    default:
        RIP("Unexpected pixel format");
        return NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_32bppPBGRA_Out_Slow
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.
//
//             This function shouldn't be called.  It supports PBGRA and BGR
//             and should be inlined into the two functions which call with
//             fHasAlpha set as a constant.
//
//------------------------------------------------------------------------------
static VOID MIL_FORCEINLINE ScalePPAACoverage_32bppPBGRA_Out_Slow(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP,
    bool fHasAlpha,
    bool fComplementAlpha
    )
{
    BYTE *pbDest = static_cast<BYTE*>(pSOP->m_pvDest);

    UINT nCount = pPP->m_uiCount;

    INT nCurrent = pPP->m_iX;
    INT nRight = nCurrent + nCount;
    UINT uiConsecutivePixels;

    Assert(nRight > nCurrent);

    const CAntialiasedFiller *pAF = DYNCAST(CAntialiasedFiller, pSOP->m_posd);
    Assert(pAF);

    CCoverageInterval *pCoverage = const_cast<CCoverageInterval *>(pAF->m_coverageBuffer.m_pIntervalStart);
    float rComplementFactor = pAF->m_rComplementFactor;

    const INT c_nCoverageComplete = fComplementAlpha ? 0 : c_nShiftSizeSquared;

    // In the case of complement with input alpha we need to process the per-pixel
    // alpha & can't do the full coverage optimization.
    const bool c_fCoverageCompleteValid = !(fHasAlpha && fComplementAlpha);

    //
    // Find the coverage information for the first pixel
    //

    while (pCoverage->m_pNext->m_nPixelX <= nCurrent)
    {
        pCoverage = pCoverage->m_pNext;
    }

    //
    // Modify the destination pixels
    //

    while (nCurrent < nRight)
    {
        uiConsecutivePixels = min(nRight, pCoverage->m_pNext->m_nPixelX) - nCurrent;

        INT nCoverage = pCoverage->m_nCoverage;

        if ((nCoverage == c_nCoverageComplete) && c_fCoverageCompleteValid)
        {
            // All these pixels are completely covered.
            if (fHasAlpha)
            {
                // No work needed.
                pbDest += (4 * uiConsecutivePixels);
            }
            else
            {
                for (UINT i = 0; i < uiConsecutivePixels; i++)
                {
                    UINT uColorSource = *(UINT *)pbDest;
                    uColorSource = uColorSource | 0xff000000;

                    *(UINT *)pbDest = uColorSource;

                    pbDest += 4;
                }
            }
        }
        else
        {
            // We only get spans that have non-zero coverage at this level unless we're
            // doing a complement render.
            Assert(fComplementAlpha || nCoverage > 0);


            // Convert our coverage from [0, c_nShiftSize*c_nShiftSize] to [0, 256]
            Assert(c_nShiftSize <= 16);
            C_ASSERT(256 % (c_nShiftSize*c_nShiftSize) == 0);
            C_ASSERT(c_nShiftSize == 8);

            // NOTE (EXTREMELY TRICKY) coverage values are 1/256ths, while color values
            // are 1/255ths.  The math in this function handles this correctly.
            //  Let x be in 1/255ths and y be in 1/256ths.  To multiply them & get a value
            //  back in 255ths is just
            //   result = (x * y + 128) >> 8
            //  Where result is x if y == 256 and result is 0 if x * y == 0.
            //
            //  To get a 1/256th result is
            //   temp = (x * y + 128);
            //   temp += temp >> 8;
            //   result = temp >> 8;
            //  Where result is y if x == 255 and result is 0 if x * y == 0.
            //  Note that for this case temp could be 17 bits (because 256 is 9 bits)
            UINT uCoverage = pCoverage->m_nCoverage*(256/(c_nShiftSize*c_nShiftSize));

            if (fComplementAlpha)
            {
                uCoverage = GpRound(uCoverage * rComplementFactor);
                if (!fHasAlpha)
                {
                    // Do complement now.  Only if we have alpha do we need to
                    // complement per-pixel.
                    uCoverage = 256 - uCoverage;
                }
            }
            UINT uScale = uCoverage;

            // For the non-complement case or the complement case without alpha
            // the scale factor is constant.  For complement with incoming alpha
            // we have to multiply uCoveragePart by the per-pixel alpha, then
            // complement to get the scale value.

            // Go through the run and multiply the alpha values by the run's
            // pCoverage:

            for (UINT i = 0; i < uiConsecutivePixels; i++)
            {
                UINT uColorSource = *(UINT *)pbDest;
                if (!fHasAlpha)
                {
                    uColorSource = uColorSource | 0xff000000;
                }
                else if (fComplementAlpha)
                {
                    uScale = (uCoverage * (uColorSource >> 24)) + 0x80;
                    uScale = (uScale + (uScale >> 8)) >> 8;
                    uScale = 256 - uScale;
                    // Now set alpha to 255 because uScale is the new alpha & we're
                    // going to scale the whole color by it next.
                    uColorSource = uColorSource | 0xff000000;
                }

                // Read color and convert to our fast blending format
                UINT uColorSource00aa00gg = (uColorSource >> 8) & 0x00ff00ff;
                UINT uColorSource00rr00bb = uColorSource & 0x00ff00ff;

                // Blend with coverage
                UINT uBlendedColoraa00gg00 = ((uColorSource00aa00gg * uScale + 0x00800080) & 0xff00ff00);
                UINT uBlendedColor00rr00bb = (((uColorSource00rr00bb * uScale + 0x00800080) >> 8) & 0x00ff00ff);

                // Write color
                *(UINT *)pbDest = uBlendedColoraa00gg00 | uBlendedColor00rr00bb;

                // Advance
                pbDest += 4;
            }
        }

        pCoverage = pCoverage->m_pNext;
        nCurrent += uiConsecutivePixels;
    }
}


//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_32bppPARGB
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.
//

VOID FASTCALL ScalePPAACoverage_32bppPBGRA(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ScalePPAACoverage_32bppPBGRA_Out_Slow(
        pPP,
        pSOP,
        true,   // Has alpha
        false   // Doesn't support complement
        );
}


//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_32bppBGR
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.
//

VOID FASTCALL ScalePPAACoverage_32bppBGR(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ScalePPAACoverage_32bppPBGRA_Out_Slow(
        pPP,
        pSOP,
        false,  // Has no alpha
        false   // Doesn't support complement
        );
}

//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_Complement_32bppBGR
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.  Supports complement.
//

VOID FASTCALL ScalePPAACoverage_Complement_32bppBGR(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ScalePPAACoverage_32bppPBGRA_Out_Slow(
        pPP,
        pSOP,
        false,  // Has no alpha
        true    // Supports complement
        );
}

//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_Complement_32bppBGRA
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.  Supports complement &
//             input alpha (non-premultiplied.)
//

VOID FASTCALL ScalePPAACoverage_Complement_32bppBGRA(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ScalePPAACoverage_32bppPBGRA_Out_Slow(
        pPP,
        pSOP,
        true,  // Has alpha
        true   // Supports complement
        );
}

//+-----------------------------------------------------------------------------
//
//  Function:  ScalePPAACoverage_128bppPRGBA
//
//  Synopsis:  Unary operation - modify the source colors according to the
//             per-primitive antialiasing coverage.
//

VOID FASTCALL ScalePPAACoverage_128bppPRGBA(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    FLOAT *prDest = static_cast<FLOAT*>(pSOP->m_pvDest);

    UINT nCount = pPP->m_uiCount;

    INT nCurrent = pPP->m_iX;
    INT nRight = nCurrent + nCount;
    UINT uiConsecutivePixels;

    Assert(nRight > nCurrent);

    const CAntialiasedFiller *pAF = DYNCAST(CAntialiasedFiller, pSOP->m_posd);
    Assert(pAF);

    CCoverageInterval *pCoverage = const_cast<CCoverageInterval *>(pAF->m_coverageBuffer.m_pIntervalStart);
    float rComplementFactor = pAF->m_rComplementFactor;
    bool fComplementAlpha = rComplementFactor >= 0;

    INT nCoverageComplete = fComplementAlpha
                              ? -1 // Not a valid coverage so we won't do optimization
                              : c_nShiftSizeSquared;

    // If we are doing complement we can't skip any pixels because we
    // have to process the input alpha.


    float rCoverageFactor = 1 / (FLOAT)(c_nShiftSize*c_nShiftSize);
    if (fComplementAlpha)
    {
        rCoverageFactor *= rComplementFactor;
    }

    //
    // Find the coverage information for the first pixel
    //

    while (pCoverage->m_pNext->m_nPixelX <= nCurrent)
    {
        pCoverage = pCoverage->m_pNext;
    }

    //
    // Modify the destination pixels
    //

    while (nCurrent < nRight)
    {
        // We only get spans that have non-zero coverage at this level unless
        // we're complementing.
        Assert(fComplementAlpha || pCoverage->m_nCoverage > 0);

        uiConsecutivePixels = min(nRight, pCoverage->m_pNext->m_nPixelX) - nCurrent;

        if (pCoverage->m_nCoverage == nCoverageComplete)
        {
            // All these pixels are completely covered.

            prDest += (4 * uiConsecutivePixels);
        }
        else
        {
            // Go through the run and multiply the alpha values by the run's
            // pCoverage:

            for (UINT i = 0; i < uiConsecutivePixels; i++)
            {
                FLOAT rAAFactor = (FLOAT) pCoverage->m_nCoverage * rCoverageFactor;
                if (fComplementAlpha)
                {
                    rAAFactor *= prDest[3];
                    rAAFactor = 1 - rAAFactor;
                    prDest[0] = prDest[0] * rAAFactor;
                    prDest[1] = prDest[1] * rAAFactor;
                    prDest[2] = prDest[2] * rAAFactor;
                    prDest[3] = 1;
                }
                else
                {
                    *(prDest+0) = *(prDest+0) * rAAFactor;
                    *(prDest+1) = *(prDest+1) * rAAFactor;
                    *(prDest+2) = *(prDest+2) * rAAFactor;
                    *(prDest+3) = *(prDest+3) * rAAFactor;
                }

                prDest += 4;
            }
        }

        pCoverage = pCoverage->m_pNext;
        nCurrent += uiConsecutivePixels;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Given complete interval data for a scan, find runs of touched pixels
*   and then call the clipper (or directly to the rendering routine if
*   there's no clipping).
*
* Created:
*
*   03/17/2000 andrewgo
*
\**************************************************************************/

MIL_FORCEINLINE VOID
CAntialiasedFiller::GenerateOutput(
    INT yScaled
    )
{
    INT y = yScaled >> c_nShift;

    if (CreateComplementGeometry())
    {
        // For complemented geometry just output the entire width of the complement bounds
        // and CAntialiasedFiller::OutputSpan will take care of producing correct
        // coverage values.
        m_pOutputSpan->OutputSpan(y, m_rcComplementBounds.left, m_rcComplementBounds.right);
    }
    else
    {
        CCoverageInterval *pIntervalSpanStart = m_coverageBuffer.m_pIntervalStart->m_pNext;
        CCoverageInterval *pIntervalSpanEnd;

        while (pIntervalSpanStart->m_nPixelX != INT_MAX)
        {
            Assert(pIntervalSpanStart->m_nCoverage != 0);

            // Here we determine the length of a continuous run of covered
            // pixels.  For the case where the user has set the mode to
            // SRCCOPY, it's very important that we don't accidentally pass
            // off as 'covered' a pixel that we later realize wasn't covered.

            pIntervalSpanEnd = pIntervalSpanStart->m_pNext;
            while (pIntervalSpanEnd->m_nCoverage > 0)
            {
                pIntervalSpanEnd = pIntervalSpanEnd->m_pNext;
            }

            //   Avoid clipping where not necessary
            //   The comment below *should* be right, but it's currently not true -
            //   we always jump to the clipper. "surface rect" clipping should be
            //   done earlier, inside the rasterizer code. Then, the common case
            //   will be for there to be no additional clipping, so we should indeed
            //   usually jump directly to CAntialiasedFiller::OutputSpan.
            // If there's no clip region, this jumps to
            // CAntialiasedFiller::OutputSpan:

            m_pOutputSpan->OutputSpan(y, pIntervalSpanStart->m_nPixelX, pIntervalSpanEnd->m_nPixelX);

            // Advance to after the gap:

            pIntervalSpanStart = pIntervalSpanEnd->m_pNext;
        }
    }
}

/**************************************************************************\
*
* Class Description:
*
*   Aliased filler state.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class CAliasedFiller
{
private:
    COutputSpan *m_pOutputSpan;

public:
    CAliasedFiller(
        __in_ecount(1) COutputSpan *pOutputSpan
        )
    {
        m_pOutputSpan = pOutputSpan;
    }

    void RasterizeEdges(
        __inout_ecount(1) CEdge *activeList,
        __inout_xcount(array terminated by an edge with StartY != iCurrentY)
            CInactiveEdge *inactiveArray,
        INT iCurrentY,
        INT yBottom,
        MilFillMode::Enum fillMode
    );

    friend VOID FASTCALL FillEdges_Aliased_Alternate(
        __inout_ecount(1) CAliasedFiller *filler,
        __in_ecount(1) const CEdge *active,
        INT iCurrentY
        );
    friend VOID FASTCALL FillEdges_Aliased_Winding(
        __inout_ecount(1) CAliasedFiller *filler,
        __in_ecount(1) const CEdge *active,
        INT iCurrentY
        );

private:
    VOID FillEdges(
        MilFillMode::Enum fillMode,
        __in_ecount(1) const CEdge *active,
        INT iCurrentY
        );
};

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do an alternate-mode
*   aliased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
FillEdges_Aliased_Alternate(
    __inout_ecount(1) CAliasedFiller *pFiller,
    __in_ecount(1) const CEdge *activeList,
    INT iCurrentY
    )
{
    const CEdge *startEdge = activeList->Next;
    const CEdge *endEdge;
    INT left;
    INT right;

    ASSERTACTIVELIST(activeList, iCurrentY);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        Assert(endEdge->X != INT_MAX);

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
                endEdge = endEdge->Next->Next;

            Assert((left < right) && (right < INT_MAX));

            pFiller->m_pOutputSpan->OutputSpan(iCurrentY, left, right);
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do a winding-mode
*   aliased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
FillEdges_Aliased_Winding(
    __inout_ecount(1) CAliasedFiller *pFiller,
    __in_ecount(1) const CEdge *activeList,
    INT iCurrentY
    )
{
    const CEdge *startEdge = activeList->Next;
    const CEdge *endEdge;
    INT left;
    INT right;
    INT windingValue;

    ASSERTACTIVELIST(activeList, iCurrentY);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        windingValue = startEdge->WindingDirection;
        while ((windingValue += endEdge->WindingDirection) != 0)
            endEdge = endEdge->Next;

        Assert(endEdge->X != INT_MAX);

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
            {
                startEdge = endEdge->Next;
                endEdge = startEdge->Next;

                windingValue = startEdge->WindingDirection;
                while ((windingValue += endEdge->WindingDirection) != 0)
                    endEdge = endEdge->Next;
            }

            Assert((left < right) && (right < INT_MAX));

            pFiller->m_pOutputSpan->OutputSpan(iCurrentY, left, right);
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do an antialiased
*   fill.
*
* Created:
*
*   07/20/2003 ashrafm
*
\**************************************************************************/

VOID
MIL_FORCEINLINE
CAliasedFiller::FillEdges(
    MilFillMode::Enum fillMode,
    __in_ecount(1) const CEdge *activeList,
    INT iCurrentY
    )
{
    if (fillMode == MilFillMode::Winding)
    {
        FillEdges_Aliased_Winding(this, activeList, iCurrentY);
    }
    else
    {
        Assert(fillMode == MilFillMode::Alternate);
        FillEdges_Aliased_Alternate(this, activeList, iCurrentY);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Clip the edge vertically.
*
*   We've pulled this routine out-of-line from InitializeEdges mainly
*   because it needs to call inline Asm, and when there is in-line
*   Asm in a routine the compiler generally does a much less efficient
*   job optimizing the whole routine.  InitializeEdges is rather
*   performance critical, so we avoid polluting the whole routine
*   by having this functionality out-of-line.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
ClipEdge(
    __inout_ecount(1) CEdge *edgeBuffer,
    INT yClipTopInteger,
    INT dMOriginal
    )
{
    INT xDelta;
    INT error;

    // Cases where bigNumerator will exceed 32-bits in precision
    // will be rare, but could happen, and we can't fall over
    // in those cases.

    INT dN = edgeBuffer->ErrorDown;
    LONGLONG bigNumerator = Int32x32To64(dMOriginal,
                                         yClipTopInteger - edgeBuffer->StartY)
                          + (edgeBuffer->Error + dN);
    if (bigNumerator >= 0)
    {
        QUOTIENT_REMAINDER_64_32(bigNumerator, dN, xDelta, error);
    }
    else
    {
        bigNumerator = -bigNumerator;
        QUOTIENT_REMAINDER_64_32(bigNumerator, dN, xDelta, error);

        xDelta = -xDelta;
        if (error != 0)
        {
            xDelta--;
            error = dN - error;
        }
    }

    // Update the edge data structure with the results:

    edgeBuffer->StartY  = yClipTopInteger;
    edgeBuffer->X      += xDelta;
    edgeBuffer->Error   = error - dN;      // Renormalize error
}

//+-----------------------------------------------------------------------------
//
//  Function:  TransformRasterizerPointsTo28_4
//
//  Synopsis:
//      Transform rasterizer points to 28.4.  If overflow occurs, return that
//      information.
//
//------------------------------------------------------------------------------
HRESULT
TransformRasterizerPointsTo28_4(
    __in_ecount(1) const CMILMatrix *pmat,
        // Transform to take us to 28.4
    __in_ecount(cPoints) const MilPoint2F *pPtsSource,
        // Source points
    UINT cPoints,
        // Count of points
    __out_ecount(cPoints) POINT *pPtsDest
        // Destination points
    )
{
    HRESULT hr = S_OK;

    Assert(cPoints > 0);

    //
    // We want coordinates in the 28.4 range in the end.  The matrix we get
    // as input includes the scale by 16 to get to 28.4, so we want to
    // ensure that we are in integer range.  Assuming a sign bit and
    // five bits for the rasterizer working range, we want coordinates in the
    // -2^26 to 2^26.
    //
    // Note that the 5-bit requirement comes from the
    // implementation of InitializeEdges.
    // (See line with "error -= dN * (16 - (xStart & 15))")
    //
    // Anti-aliasing uses another c_nShift bits, so we get a
    // desired range of -2^(26-c_nShift) to 2^(26-c_nShift)
    //

    float rPixelCoordinateMax = static_cast<float>(1 << (26 - c_nShift));
    float rPixelCoordinateMin = -rPixelCoordinateMax;

    do
    {
        //
        // Transform coordinates
        //

        float rPixelX = (pmat->GetM11() * pPtsSource->X) + (pmat->GetM21() * pPtsSource->Y) + pmat->GetDx();
        float rPixelY = (pmat->GetM12() * pPtsSource->X) + (pmat->GetM22() * pPtsSource->Y) + pmat->GetDy();

        //
        // Check for NaNs or overflow
        //

        if (!(rPixelX <= rPixelCoordinateMax &&
            rPixelX >= rPixelCoordinateMin &&
            rPixelY <= rPixelCoordinateMax &&
            rPixelY >= rPixelCoordinateMin))
        {
            hr = THR(WGXERR_BADNUMBER);
            goto Cleanup;
        }

        //
        // Assign coordinates
        //

        pPtsDest->x = CFloatFPU::Round(rPixelX);
        pPtsDest->y = CFloatFPU::Round(rPixelY);
    }
    while (pPtsDest++, pPtsSource++, --cPoints != 0);


Cleanup:
    RRETURN(hr);
}

VOID AppendScaleToMatrix(
    __inout_ecount(1) CMILMatrix *pmat,
    REAL scaleX,
    REAL scaleY
    )
{
    pmat->SetM11(pmat->GetM11() * scaleX);
    pmat->SetM21(pmat->GetM21() * scaleX);
    pmat->SetM12(pmat->GetM12() * scaleY);
    pmat->SetM22(pmat->GetM22() * scaleY);
    pmat->SetDx(pmat->GetDx() * scaleX);
    pmat->SetDy(pmat->GetDy() * scaleY);
}

/**************************************************************************\
*
* Function Description:
*
*   Add edges to the edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

HRESULT
InitializeEdges(
    __inout_ecount(1) CInitializeEdgesContext *pEdgeContext,
    __inout_ecount(vertexCount) POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
                                                   //   Note that we may modify the contents!
    __in_range(>=, 2) UINT vertexCount
    )
{
    // Disable instrumentation checks for this function
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);

    HRESULT hr = S_OK;

    INT xStart;
    INT yStart;
    INT yStartInteger;
    INT yEndInteger;
    INT dMOriginal;
    INT dM;
    INT dN;
    INT dX;
    INT errorUp;
    INT quotient;
    INT remainder;
    INT error;
    INT windingDirection;
    CEdge *edgeBuffer;
    UINT bufferCount;
    INT yClipTopInteger;
    INT yClipTop;
    INT yClipBottom;
    INT xClipLeft;
    INT xClipRight;

    INT yMax              = pEdgeContext->MaxY;
    CEdgeStore *store     = pEdgeContext->Store;
    const RECT *clipRect  = pEdgeContext->ClipRect;

    INT edgeCount = vertexCount - 1;
    Assert(edgeCount >= 1);

    if (clipRect == NULL)
    {
        yClipBottom = 0;
        yClipTopInteger = INT_MIN >> c_nShift;

        // These 3 values are only used when clipRect is non-NULL
        yClipTop = 0;
        xClipLeft = 0;
        xClipRight = 0;

    }
    else
    {
        yClipTopInteger = clipRect->top >> 4;
        yClipTop = clipRect->top;
        yClipBottom = clipRect->bottom;
        xClipLeft = clipRect->left;
        xClipRight = clipRect->right;

        Assert(yClipBottom > 0);
        Assert(yClipTop <= yClipBottom);
    }

    if (pEdgeContext->AntiAliasMode != MilAntiAliasMode::None)
    {
        // If antialiasing, apply the supersampling scaling here before we
        // calculate the DDAs.  We do this here and not in the Matrix
        // transform we give to FixedPointPathEnumerate mainly so that the
        // Bezier flattener can continue to operate in its optimal 28.4
        // format.
        //
        // PS#856364-2003/07/01-JasonHa  Remove pixel center fixup
        //
        // We also apply a half-pixel offset here so that the antialiasing
        // code can assume that the pixel centers are at half-pixel
        // coordinates, not on the integer coordinates.

        POINT *point = pointArray;
        INT i = vertexCount;

        do {
            point->x = (point->x + 8) << c_nShift;
            point->y = (point->y + 8) << c_nShift;

        } while (point++, --i != 0);

        yClipTopInteger <<= c_nShift;
        yClipTop <<= c_nShift;
        yClipBottom <<= c_nShift;
        xClipLeft <<= c_nShift;
        xClipRight <<= c_nShift;
    }

    // Make 'yClipBottom' inclusive by subtracting off one pixel
    // (keeping in mind that we're in 28.4 device space):

    yClipBottom -= 16;

    // Warm up the store where we keep the edge data:

    store->StartAddBuffer(&edgeBuffer, &bufferCount);

    do {
        // Handle trivial rejection:

        if (yClipBottom >= 0)
        {
            // Throw out any edges that are above or below the clipping.
            // This has to be a precise check, because we assume later
            // on that every edge intersects in the vertical dimension
            // with the clip rectangle.  That asssumption is made in two
            // places:
            //
            // 1.  When we sort the edges, we assume either zero edges,
            //     or two or more.
            // 2.  When we start the DDAs, we assume either zero edges,
            //     or that there's at least one scan of DDAs to output.
            //
            // Plus, of course, it's less efficient if we let things
            // through.
            //
            // Note that 'yClipBottom' is inclusive:

            BOOL clipHigh = ((pointArray)->y <= yClipTop) &&
                            ((pointArray + 1)->y <= yClipTop);

            BOOL clipLow = ((pointArray)->y > yClipBottom) &&
                             ((pointArray + 1)->y > yClipBottom);

            #if DBG
            {
                INT yRectTop, yRectBottom, y0, y1, yTop, yBottom;

                // Getting the trivial rejection code right is tricky.
                // So on checked builds let's verify that we're doing it
                // correctly, using a different approach:

                BOOL clipped = FALSE;
                if (clipRect != NULL)
                {
                    yRectTop = clipRect->top >> 4;
                    yRectBottom = clipRect->bottom >> 4;
                    if (pEdgeContext->AntiAliasMode != MilAntiAliasMode::None)
                    {
                        yRectTop <<= c_nShift;
                        yRectBottom <<= c_nShift;
                    }
                    y0 = ((pointArray)->y + 15) >> 4;
                    y1 = ((pointArray + 1)->y + 15) >> 4;
                    yTop = min(y0, y1);
                    yBottom = max(y0, y1);

                    clipped = ((yTop >= yRectBottom) || (yBottom <= yRectTop));
                }

                Assert(clipped == (clipHigh || clipLow));
            }
            #endif

            if (clipHigh || clipLow)
                continue;               // ======================>

            if (edgeCount > 1)
            {
                // Here we'll collapse two edges down to one if both are
                // to the left or to the right of the clipping rectangle.

                if (((pointArray)->x < xClipLeft) &&
                    ((pointArray + 1)->x < xClipLeft) &&
                    ((pointArray + 2)->x < xClipLeft))
                {
                    // Note this is one reason why 'pointArray' can't be 'const':

                    *(pointArray + 1) = *(pointArray);

                    continue;           // ======================>
                }

                if (((pointArray)->x > xClipRight) &&
                    ((pointArray + 1)->x > xClipRight) &&
                    ((pointArray + 2)->x > xClipRight))
                {
                    // Note this is one reason why 'pointArray' can't be 'const':

                    *(pointArray + 1) = *(pointArray);

                    continue;           // ======================>
                }
            }

        }

        dM = (pointArray + 1)->x - (pointArray)->x;
        dN = (pointArray + 1)->y - (pointArray)->y;

        if (dN >= 0)
        {
            // The vector points downward:

            xStart = (pointArray)->x;
            yStart = (pointArray)->y;

            yStartInteger = (yStart + 15) >> 4;
            yEndInteger   = ((pointArray + 1)->y + 15) >> 4;

            windingDirection = 1;
        }
        else
        {
            // The vector points upward, so we have to essentially
            // 'swap' the end points:

            dN = -dN;
            dM = -dM;

            xStart = (pointArray + 1)->x;
            yStart = (pointArray + 1)->y;

            yStartInteger = (yStart + 15) >> 4;
            yEndInteger   = ((pointArray)->y + 15) >> 4;

            windingDirection = -1;
        }

        // The edgeBuffer must span an integer y-value in order to be
        // added to the edgeBuffer list.  This serves to get rid of
        // horizontal edges, which cause trouble for our divides.

        if (yEndInteger > yStartInteger)
        {
            yMax = max(yMax, yEndInteger);

            dMOriginal = dM;
            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)            // Can't be '<='
                {
                    dX      = -1;
                    errorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, quotient, remainder);

                    dX      = -quotient;
                    errorUp = remainder;
                    if (remainder > 0)
                    {
                        dX      = -quotient - 1;
                        errorUp = dN - remainder;
                    }
                }
            }
            else
            {
                if (dM < dN)
                {
                    dX      = 0;
                    errorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, quotient, remainder);

                    dX      = quotient;
                    errorUp = remainder;
                }
            }

            error = -1;     // Error is initially zero (add dN - 1 for
                            //   the ceiling, but subtract off dN so that
                            //   we can check the sign instead of comparing
                            //   to dN)

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (INT i = 16 - (yStart & 15); i != 0; i--)
                {
                    xStart += dX;
                    error += errorUp;
                    if (error >= 0)
                    {
                        error -= dN;
                        xStart++;
                    }
                }
            }

            if ((xStart & 15) != 0)
            {
                error -= dN * (16 - (xStart & 15));
                xStart += 15;       // We'll want the ceiling in just a bit...
            }

            xStart >>= 4;
            error >>= 4;

            if (bufferCount == 0)
            {
                IFC(store->NextAddBuffer(&edgeBuffer, &bufferCount));
            }

            edgeBuffer->X                = xStart;
            edgeBuffer->Dx               = dX;
            edgeBuffer->Error            = error;
            edgeBuffer->ErrorUp          = errorUp;
            edgeBuffer->ErrorDown        = dN;
            edgeBuffer->WindingDirection = windingDirection;
            edgeBuffer->StartY           = yStartInteger;
            edgeBuffer->EndY             = yEndInteger;       // Exclusive of end

            Assert(error < 0);

            // Here we handle the case where the edge starts above the
            // clipping rectangle, and we need to jump down in the 'y'
            // direction to the first unclipped scan-line.
            //
            // Consequently, we advance the DDA here:

            if (yClipTopInteger > yStartInteger)
            {
                Assert(edgeBuffer->EndY > yClipTopInteger);

                ClipEdge(edgeBuffer, yClipTopInteger, dMOriginal);
            }

            // Advance to handle the next edge:

            edgeBuffer++;
            bufferCount--;
        }
    } while (pointArray++, --edgeCount != 0);

    // We're done with this batch.  Let the store know how many edges
    // we ended up with:

    store->EndAddBuffer(edgeBuffer, bufferCount);

    pEdgeContext->MaxY = yMax;

Cleanup:
    RRETURN(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE if the line from point[1] to point[2] turns 'left'
*   from the line from point[0] to point[1].  Uses the sign of the
*   cross product.
*
*   Remember that we're in device space, where positive 'y' is down!
*
* Created:
*
*   04/09/2000 andrewgo
*
\**************************************************************************/

inline
BOOL
TurnLeft(
    __in_ecount(3) const POINT *points
    )
{
    LONGLONG ad = Int32x32To64(points[1].x - points[0].x,
                               points[2].y - points[1].y);
    LONGLONG bc = Int32x32To64(points[1].y - points[0].y,
                               points[2].x - points[1].x);

    return(ad < bc);
}

/**************************************************************************\
*
* Function Description:
*
*   Computes the index of the NominalDrawVertex table to be use as the
*   drawing vertex.  The result is numbered such that a traversal using
*   an increasing pointer will go counter-clockwise around the pen.
*
* Created:
*
*   04/09/2000 andrewgo
*
\**************************************************************************/

const POINT NominalDrawVertex[] =
{
    // Don't forget that in device space, positive 'y' is down:

    {0,  -8},
    {-8, 0},
    {0,  8},
    {8,  0}
};

/**************************************************************************\
*
* Function Description:
*
*   Does complete parameter checking on the 'types' array of a path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
ValidatePathTypes(
    __in_ecount(count) const BYTE *typesArray,
    INT count
    )
{
    const BYTE *types = typesArray;

    if (count == 0)
        return(TRUE);

    for ( ; ; )
    {
        // The first point in every subpath has to be an unadorned
        // 'start' point:

        if ((*types & PathPointTypePathTypeMask) != PathPointTypeStart)
        {
            TraceTag((tagMILWarning, "Bad subpath start"));
            return(FALSE);
        }

        // Advance to the first point after the 'start' point:

        types++;
        if (--count == 0)
        {
            TraceTag((tagMILWarning, "Path ended after start-path"));
            return(FALSE);
        }

        if ((*types & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            TraceTag((tagMILWarning, "Can't have a start followed by a start!"));
            return(FALSE);
        }

        // Process runs of lines and Bezier curves:

        do {
            switch(*types & PathPointTypePathTypeMask)
            {
            case PathPointTypeLine:
                types++;
                if (--count == 0)
                    return(TRUE);

                break;

            case PathPointTypeBezier:
                if(count < 3)
                {
                    TraceTag((tagMILWarning,
                        "Path ended before multiple of 3 Bezier points"));
                    return(FALSE);
                }

                if((*types & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    TraceTag((tagMILWarning,
                        "Bad subpath start"));
                    return(FALSE);
                }

                if((*(types + 1) & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    TraceTag((tagMILWarning,
                        "Expected plain Bezier control point for 3rd vertex"));
                    return(FALSE);
                }

                if((*(types + 2) & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    TraceTag((tagMILWarning,
                        "Expected Bezier control point for 4th vertex"));
                    return(FALSE);
                }

                types += 3;
                if ((count -= 3) == 0)
                    return(TRUE);

                break;

            default:
                TraceTag((tagMILWarning, "Illegal type"));
                return(FALSE);
            }

            // A close-subpath marker or a start-subpath marker marks the
            // end of a subpath:

        } while (!(*(types - 1) & PathPointTypeCloseSubpath) &&
                  ((*types & PathPointTypePathTypeMask) != PathPointTypeStart));
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
AssertPath(
    __in_ecount(cPoints) const BYTE *rgTypes,
    const UINT cPoints
    )
{
    // Make sure that the 'types' array is well-formed, otherwise we
    // may fall over in the FixedPointPathEnumerate function.
    //
    // NOTE: If you hit this assert, DO NOT SIMPLY COMMENT THIS Assert OUT!
    //
    //       Instead, fix the ValidatePathTypes code if it's letting through
    //       valid paths, or (more likely) fix the code that's letting bogus
    //       paths through.  The FixedPointPathEnumerate routine has some
    //       subtle assumptions that require the path to be perfectly valid!
    //
    //       No internal code should be producing invalid paths, and all
    //       paths created by the application must be parameter checked!

    Assert(ValidatePathTypes(rgTypes, cPoints));
}


//+----------------------------------------------------------------------------
//
//  Member:
//      FixedPointPathEnumerate
//
//  Synopsis:
//
//      Enumerate the path.
//
//      NOTE: The 'enumerateFunction' function is allowed to modify the
//            contents of our call-back buffer!  (This is mainly done to allow
//            'InitializeEdges' to be simpler for some clipping trivial
//            rejection cases.)
//
//      NOTICE-2006/03/22-milesc  This function was initially built to be a
//      general path enumeration function. However, we were only using it for
//      one specific purpose... for Initializing edges of a path to be filled.
//      In doing security work, I simplified this function to just do edge
//      initialization. The name is therefore now overly general. I have kept
//      the name to be a reminder that this function has been written to be
//      more general than would otherwise be evident.
//

HRESULT
FixedPointPathEnumerate(
    __in_ecount(cPoints) const MilPoint2F *rgpt,
    __in_ecount(cPoints) const BYTE *rgTypes,
    UINT cPoints,
    __in_ecount(1) const CMILMatrix *matrix,
    __in_ecount(1) const RECT *clipRect,       // In scaled 28.4 format
    __inout_ecount(1) CInitializeEdgesContext *enumerateContext
    )
{
    HRESULT hr = S_OK;
    POINT bufferStart[ENUMERATE_BUFFER_NUMBER];
    POINT bezierBuffer[4];
    POINT *buffer;
    UINT bufferSize;
    POINT startFigure;
    UINT iStart;
    UINT iEnd;
    UINT runSize;
    UINT thisCount;
    BOOL isMore;
    INT xLast;
    INT yLast;

    ASSERTPATH(rgTypes, cPoints);

    // Every valid subpath has at least two vertices in it, hence the
    // check of 'cPoints - 1':

    iStart = 0;

    Assert(cPoints > 1);
    while (iStart < cPoints - 1)
    {
        Assert((rgTypes[iStart] & PathPointTypePathTypeMask)
                    == PathPointTypeStart);
        Assert((rgTypes[iStart + 1] & PathPointTypePathTypeMask)
                    != PathPointTypeStart);

        // Add the start point to the beginning of the batch, and
        // remember it for handling the close figure:

        IFC(TransformRasterizerPointsTo28_4(matrix, &rgpt[iStart], 1, &startFigure));

        bufferStart[0].x = startFigure.x;
        bufferStart[0].y = startFigure.y;
        buffer = bufferStart + 1;
        bufferSize = ENUMERATE_BUFFER_NUMBER - 1;

        // We need to enter our loop with 'iStart' pointing one past
        // the start figure:

        iStart++;

        do {
            // Try finding a run of lines:

            if ((rgTypes[iStart] & PathPointTypePathTypeMask)
                                == PathPointTypeLine)
            {
                iEnd = iStart + 1;

                while ((iEnd < cPoints) &&
                       ((rgTypes[iEnd] & PathPointTypePathTypeMask)
                                == PathPointTypeLine))
                {
                    iEnd++;
                }

                // Okay, we've found a run of lines.  Break it up into our
                // buffer size:

                runSize = (iEnd - iStart);
                do {
                    thisCount = min(bufferSize, runSize);

                    IFC(TransformRasterizerPointsTo28_4(matrix, &rgpt[iStart], thisCount, buffer));

                    __analysis_assume(buffer + bufferSize == bufferStart + ENUMERATE_BUFFER_NUMBER);
                    Assert(buffer + bufferSize == bufferStart + ENUMERATE_BUFFER_NUMBER);

                    iStart += thisCount;
                    buffer += thisCount;
                    runSize -= thisCount;
                    bufferSize -= thisCount;

                    if (bufferSize > 0)
                        break;

                    xLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].x;
                    yLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].y;
                    IFC(InitializeEdges(
                        enumerateContext,
                        bufferStart,
                        ENUMERATE_BUFFER_NUMBER
                        ));

                    // Continue the last vertex as the first in the new batch:

                    bufferStart[0].x = xLast;
                    bufferStart[0].y = yLast;
                    buffer = bufferStart + 1;
                    bufferSize = ENUMERATE_BUFFER_NUMBER - 1;

                } while (runSize != 0);
            }
            else
            {
                Assert(iStart + 3 <= cPoints);
                Assert((rgTypes[iStart] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);
                Assert((rgTypes[iStart + 1] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);
                Assert((rgTypes[iStart + 2] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);

                IFC(TransformRasterizerPointsTo28_4(matrix, &rgpt[iStart - 1], 4, bezierBuffer));

                // Prepare for the next iteration:

                iStart += 3;

                // Process the Bezier:

                CMILBezier bezier(bezierBuffer, clipRect);
                do {
                    thisCount = bezier.Flatten(buffer, bufferSize, &isMore);

                    __analysis_assume(buffer + bufferSize == bufferStart + ENUMERATE_BUFFER_NUMBER);
                    Assert(buffer + bufferSize == bufferStart + ENUMERATE_BUFFER_NUMBER);

                    buffer += thisCount;
                    bufferSize -= thisCount;

                    if (bufferSize > 0)
                        break;

                    xLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].x;
                    yLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].y;
                    IFC(InitializeEdges(
                        enumerateContext,
                        bufferStart,
                        ENUMERATE_BUFFER_NUMBER
                        ));

                    // Continue the last vertex as the first in the new batch:

                    bufferStart[0].x = xLast;
                    bufferStart[0].y = yLast;
                    buffer = bufferStart + 1;
                    bufferSize = ENUMERATE_BUFFER_NUMBER - 1;

                } while (isMore);
            }

        } while ((iStart < cPoints) &&
                 ((rgTypes[iStart] & PathPointTypePathTypeMask)
                    != PathPointTypeStart));

        // Okay, the subpath is done.  But we still have to handle the
        // 'close figure' (which is implicit for a fill):
        // Add the close-figure point:

        buffer->x = startFigure.x;
        buffer->y = startFigure.y;
        bufferSize--;

        // We have to flush anything we might have in the batch, unless
        // there's only one vertex in there!  (The latter case may happen
        // for the stroke case with no close figure if we just flushed a
        // batch.)
        // If we're flattening, we must call the one additional time to
        // correctly handle closing the subpath, even if there is only
        // one entry in the batch. The flattening callback handles the
        // one point case and closes the subpath properly without adding
        // extraneous points.

        INT verticesInBatch = ENUMERATE_BUFFER_NUMBER - bufferSize;
        if (verticesInBatch > 1)
        {
            IFC(InitializeEdges(
                enumerateContext,
                bufferStart,
                static_cast<UINT>(verticesInBatch)
                ));
        }
    }

Cleanup:
    RRETURN(hr);
}

/**************************************************************************\
*
* Function Description:
*
*   We want to sort in the inactive list; the primary key is 'y', and
*   the secondary key is 'x'.  This routine creates a single LONGLONG
*   key that represents both.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

inline VOID YX(
    INT x,
    INT y,
    __out_ecount(1) LONGLONG *p
    )
{
    // Bias 'x' by INT_MAX so that it's effectively unsigned:

    reinterpret_cast<LARGE_INTEGER*>(p)->HighPart = y;
    reinterpret_cast<LARGE_INTEGER*>(p)->LowPart = x + INT_MAX;
}

/**************************************************************************\
*
* Function Description:
*
*   Recursive function to quick-sort our inactive edge list.  Note that
*   for performance, the results are not completely sorted; an insertion
*   sort has to be run after the quicksort in order to do a lighter-weight
*   sort of the subtables.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

#define QUICKSORT_THRESHOLD 8

VOID
QuickSortEdges(
    __inout_xcount(f - l + 1 elements) CInactiveEdge *f,
    __inout_xcount(array starts at f) CInactiveEdge *l
    )
{
    CEdge *e;
    LONGLONG y;
    LONGLONG first;
    LONGLONG second;
    LONGLONG last;

    // Find the median of the first, middle, and last elements:

    CInactiveEdge *m = f + ((l - f) >> 1);

    SWAP(y, (f + 1)->Yx, m->Yx);
    SWAP(e, (f + 1)->Edge, m->Edge);

    if ((second = (f + 1)->Yx) > (last = l->Yx))
    {
        (f + 1)->Yx = last;
        l->Yx = second;

        SWAP(e, (f + 1)->Edge, l->Edge);
    }
    if ((first = f->Yx) > (last = l->Yx))
    {
        f->Yx = last;
        l->Yx = first;

        SWAP(e, f->Edge, l->Edge);
    }
    if ((second = (f + 1)->Yx) > (first = f->Yx))
    {
        (f + 1)->Yx = first;
        f->Yx = second;

        SWAP(e, (f + 1)->Edge, f->Edge);
    }

    // f->Yx is now the desired median, and (f + 1)->Yx <= f->Yx <= l->Yx

    Assert(((f + 1)->Yx <= f->Yx) && (f->Yx <= l->Yx));

    LONGLONG median = f->Yx;

    CInactiveEdge *i = f + 2;
    while (i->Yx < median)
        i++;

    CInactiveEdge *j = l - 1;
    while (j->Yx > median)
        j--;

    while (i < j)
    {
        SWAP(y, i->Yx, j->Yx);
        SWAP(e, i->Edge, j->Edge);

        do {
            i++;
        } while (i->Yx < median);

        do {
            j--;
        } while (j->Yx > median);
    }

    SWAP(y, f->Yx, j->Yx);
    SWAP(e, f->Edge, j->Edge);

    size_t a = j - f;
    size_t b = l - j;

    // Use less stack space by recursing on the shorter subtable.  Also,
    // have the less-overhead insertion-sort handle small subtables.

    if (a <= b)
    {
        if (a > QUICKSORT_THRESHOLD)
        {
            // 'a' is the smallest, so do it first:

            QuickSortEdges(f, j - 1);
            QuickSortEdges(j + 1, l);
        }
        else if (b > QUICKSORT_THRESHOLD)
        {
            QuickSortEdges(j + 1, l);
        }
    }
    else
    {
        if (b > QUICKSORT_THRESHOLD)
        {
            // 'b' is the smallest, so do it first:

            QuickSortEdges(j + 1, l);
            QuickSortEdges(f, j - 1);
        }
        else if (a > QUICKSORT_THRESHOLD)
        {
            QuickSortEdges(f, j- 1);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Do a sort of the inactive table using an insertion-sort.  Expects
*   large tables to have already been sorted via quick-sort.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
InsertionSortEdges(
    __inout_xcount(count forward & -1 back) CInactiveEdge *inactive,
    INT count
    )
{
    CInactiveEdge *p;
    CEdge *e;
    LONGLONG y;
    LONGLONG yPrevious;

    Assert((inactive - 1)->Yx == _I64_MIN);
    Assert(count >= 2);

    inactive++;     // Skip first entry (by definition it's already in order!)
    count--;

    do {
        p = inactive;

        // Copy the current stuff to temporary variables to make a hole:

        e = inactive->Edge;
        y = inactive->Yx;

        // Shift everything one slot to the right (effectively moving
        // the hole one position to the left):

        while (y < (yPrevious = (p - 1)->Yx))
        {
            p->Yx = yPrevious;
            p->Edge = (p - 1)->Edge;
            p--;
        }

        // Drop the temporary stuff into the final hole:

        p->Yx = y;
        p->Edge = e;

        // The quicksort should have ensured that we don't have to move
        // any entry terribly far:

        Assert(inactive - p <= QUICKSORT_THRESHOLD);

    } while (inactive++, --count != 0);
}

#if DBG
/**************************************************************************\
*
* Function Description:
*
*   Assert the state of the inactive array.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
AssertInactiveArray(
    __in_ecount(count) const CInactiveEdge *inactive,   // Annotation should allow the -1 element
    INT count
    )
{
    // Verify the head:

#if !ANALYSIS
    // #if needed because prefast don't know that the -1 element is avaliable
    Assert((inactive - 1)->Yx == _I64_MIN);
#endif
    Assert(inactive->Yx != _I64_MIN);

    do {
        LONGLONG yx;
        YX(inactive->Edge->X, inactive->Edge->StartY, &yx);

        Assert(inactive->Yx == yx);
    #if !ANALYSIS
        // #if needed because tools don't know that the -1 element is avaliable
        Assert(inactive->Yx >= (inactive - 1)->Yx);
    #endif

    } while (inactive++, --count != 0);

    // Verify that the tail is setup appropriately:

    Assert(inactive->Edge->StartY == INT_MAX);
}

#endif

/**************************************************************************\
*
* Function Description:
*
*   Initialize and sort the inactive array.
*
* Returns:
*
*   'y' value of topmost edge.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

INT
InitializeInactiveArray(
    __in_ecount(1) CEdgeStore *pEdgeStore,
    __in_ecount(count+2) CInactiveEdge *rgInactiveArray,
    UINT count,
    __in_ecount(1) CEdge *tailEdge                    // Tail sentinel for inactive list
    )
{
    BOOL isMore;
    CEdge *pActiveEdge;
    CEdge *pActiveEdgeEnd;

    // First initialize the inactive array.  Skip the first entry,
    // which we reserve as a head sentinel for the insertion sort:

    CInactiveEdge *pInactiveEdge = rgInactiveArray + 1;

    do {
        isMore = pEdgeStore->Enumerate(&pActiveEdge, &pActiveEdgeEnd);

        while (pActiveEdge != pActiveEdgeEnd)
        {
            pInactiveEdge->Edge = pActiveEdge;
            YX(pActiveEdge->X, pActiveEdge->StartY, &pInactiveEdge->Yx);
            pInactiveEdge++;
            pActiveEdge++;
        }
    } while (isMore);

    Assert(static_cast<UINT>(pInactiveEdge - rgInactiveArray) == count + 1);

    // Add the tail, which is used when reading back the array.  This
    // is why we had to allocate the array as 'count + 1':

    pInactiveEdge->Edge = tailEdge;

    // Add the head, which is used for the insertion sort.  This is why
    // we had to allocate the array as 'count + 2':

    rgInactiveArray->Yx = _I64_MIN;

    // Only invoke the quicksort routine if it's worth the overhead:

    if (count > QUICKSORT_THRESHOLD)
    {
        // Quick-sort this, skipping the first and last elements,
        // which are sentinels.
        //
        // We do 'inactiveArray + count' to be inclusive of the last
        // element:

        QuickSortEdges(rgInactiveArray + 1, rgInactiveArray + count);
    }

    // Do a quick sort to handle the mostly sorted result:

    InsertionSortEdges(rgInactiveArray + 1, count);

    ASSERTINACTIVEARRAY(rgInactiveArray + 1, count);

    // Return the 'y' value of the topmost edge:

    return(rgInactiveArray[1].Edge->StartY);
}

/**************************************************************************\
*
* Function Description:
*
*   Insert edges into the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
InsertNewEdges(
    __inout_ecount(1) CEdge *pActiveList,
    INT iCurrentY,
    __deref_inout_xcount(array terminated by an edge with StartY != iCurrentY)
        CInactiveEdge **ppInactiveEdge,
    __out_ecount(1) INT *pYNextInactive
        // will be INT_MAX when no more
    )
{
    CInactiveEdge *inactive = *ppInactiveEdge;

    Assert(inactive->Edge->StartY == iCurrentY);

    do {
        CEdge *newActive = inactive->Edge;

        // The activeList edge list sentinel has X = INT_MAX, so this always
        // terminates:

        while (pActiveList->Next->X < newActive->X)
            pActiveList = pActiveList->Next;

#if SORT_EDGES_INCLUDING_SLOPE
        // The activeList edge list sentinel has Dx = INT_MAX, so this always
        // terminates:

        while ((pActiveList->Next->X == newActive->X) &&
               (pActiveList->Next->Dx < newActive->Dx))
        {
            pActiveList = pActiveList->Next;
        }
#endif

        newActive->Next = pActiveList->Next;
        pActiveList->Next = newActive;

        inactive++;

    } while (inactive->Edge->StartY == iCurrentY);

    *pYNextInactive = inactive->Edge->StartY;
    *ppInactiveEdge = inactive;
}

/**************************************************************************\
*
* Function Description:
*
*   Sort the edges so that they're in ascending 'x' order.
*
*   We use a bubble-sort for this stage, because edges maintain good
*   locality and don't often switch ordering positions.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
SortActiveEdges(
    __inout_ecount(1) CEdge *list
    )
{
    BOOL swapOccurred;
    CEdge *tmp;

    // We should never be called with an empty active edge list:

    Assert(list->Next->X != INT_MAX);

    do {
        swapOccurred = FALSE;

        CEdge *previous = list;
        CEdge *current = list->Next;
        CEdge *next = current->Next;
        INT nextX = next->X;

        do {
            if (nextX < current->X)
            {
                swapOccurred = TRUE;

                previous->Next = next;
                current->Next = next->Next;
                next->Next = current;

                SWAP(tmp, next, current);
            }

            previous = current;
            current = next;
            next = next->Next;

        } while ((nextX = next->X) != INT_MAX);

    } while (swapOccurred);
}

/**************************************************************************\
*
* Function Description:
*
* For each scan-line to be filled:
*
*   1.  Remove any stale edges from the active edge list
*   2.  Insert into the active edge list any edges new to this scan-line
*   3.  Advance the DDAs of every active edge
*   4.  If any active edges are out of order, re-sort the active edge list
*   5.  Now that the active edges are ready for this scan, call the filler
*       to traverse the edges and output the spans appropriately
*   6.  Lather, rinse, and repeat
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

void
CAliasedFiller::RasterizeEdges(
    __inout_ecount(1) CEdge *activeList,
    __inout_xcount(array terminated by an edge with StartY != iCurrentY)
        CInactiveEdge *inactiveArray,
    INT iCurrentY,
    INT yBottom,
    MilFillMode::Enum fillMode
    )
{
    INT yNextInactive;

    InsertNewEdges(activeList, iCurrentY, &inactiveArray, &yNextInactive);

    ASSERTACTIVELIST(activeList, iCurrentY);

    FillEdges(fillMode, activeList, iCurrentY);

    while (++iCurrentY < yBottom)
    {
        AdvanceDDAAndUpdateActiveEdgeList(iCurrentY, activeList);

        if (iCurrentY == yNextInactive)
        {
            InsertNewEdges(activeList, iCurrentY, &inactiveArray, &yNextInactive);
        }

        ASSERTACTIVELIST(activeList, iCurrentY);

        // Do the appropriate alternate or winding, supersampled or
        // non-supersampled fill:

        FillEdges(fillMode, activeList, iCurrentY);
    }
}


/**************************************************************************\
*
* Function Description:
*  
*   Generate complemented output for the case where there are no input
*   edges.
*
* Created:
*
*   04/30/2006 Danwo
*
\**************************************************************************/
HRESULT
CAntialiasedFiller::RasterizeNoEdges()
{
    Assert(CreateComplementGeometry());
    
    for (int y = m_rcComplementBounds.top; y < m_rcComplementBounds.bottom; ++y)
    {
        GenerateOutput(y << c_nShift);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
* For each scan-line to be filled:
*
*   1.  Remove any stale edges from the active edge list
*   2.  Insert into the active edge list any edges new to this scan-line
*   3.  Advance the DDAs of every active edge
*   4.  If any active edges are out of order, re-sort the active edge list
*   5.  Now that the active edges are ready for this scan, call the filler
*       to traverse the edges and output the spans appropriately
*   6.  Lather, rinse, and repeat
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/
HRESULT
CAntialiasedFiller::RasterizeEdges(
    __inout_ecount(1) CEdge *pEdgeActiveList,
    __inout_xcount(array terminated by an edge with StartY >= nSubpixelYBottom)
        CInactiveEdge *pInactiveEdgeArray,
    INT nSubpixelYCurrent,
    INT nSubpixelYBottom,
    MilFillMode::Enum fillMode
    )
{
    // Disable instrumentation checks for this function
    SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_DONOTHING);

    HRESULT hr = S_OK;
    CEdge *pEdgePrevious;
    CEdge *pEdgeCurrent;
    INT nSubpixelYNextInactive;
    INT nSubpixelYNext;

    InsertNewEdges(pEdgeActiveList, nSubpixelYCurrent, &pInactiveEdgeArray, &nSubpixelYNextInactive);

    if (CreateComplementGeometry())
    {
        // Generate spans for rows in complement above start of shape.
        int yFirst = nSubpixelYCurrent >> c_nShift;
        for (int y = m_rcComplementBounds.top; y < yFirst; ++y)
        {
            GenerateOutput(y << c_nShift);
        }
    }

    while (nSubpixelYCurrent < nSubpixelYBottom)
    {
        ASSERTACTIVELIST(pEdgeActiveList, nSubpixelYCurrent);

        //
        // Detect two vertical edges for fast path rasterization
        //

        pEdgePrevious = pEdgeActiveList;
        pEdgeCurrent = pEdgeActiveList->Next;

        // It is important that we check pEdgeCurrent->EndY != INT_MIN before using pEdgeCurrent->Next,
        // so, the ordering of this check has been carefully selected.

        if ((nSubpixelYCurrent & c_nShiftMask) == 0                         // scanline aligned
            && nSubpixelYNextInactive >= nSubpixelYCurrent + c_nShiftSize   // next inactive after next scanline
            && pEdgeCurrent->EndY >= nSubpixelYCurrent + c_nShiftSize       // endy after next scanline
            && pEdgeCurrent->Dx == 0                                        // left edge is vertical
            && pEdgeCurrent->ErrorUp == 0
            && pEdgeCurrent->Next->EndY >= nSubpixelYCurrent + c_nShiftSize // endy after next scanline
            && pEdgeCurrent->Next->Dx == 0                                  // right edge is vertical
            && pEdgeCurrent->Next->ErrorUp == 0
            && pEdgeCurrent->Next->Next->EndY == INT_MIN                    // only two edges
            )
        {
            // Edges are paired, so we can assert we have another one
            Assert(pEdgeCurrent->Next->EndY != INT_MIN);

            // Compute end of our vertical fill area
            nSubpixelYNext = min(pEdgeCurrent->EndY, min(pEdgeCurrent->Next->EndY, nSubpixelYNextInactive));

            // Clip to nSubpixelYBottom
            nSubpixelYNext = min(nSubpixelYNext, nSubpixelYBottom);

            // Snap to scanline boundary
            nSubpixelYNext &= ~c_nShiftMask;

            // Compute coverage and display
            if (pEdgeCurrent->X == pEdgeCurrent->Next->X)
            {
                // It's empty, so just advance nSubpixelYCurrent;
                nSubpixelYCurrent = nSubpixelYNext;
            }
            else
            {
                // Compute the coverage
                for (int i = 0; i < c_nShiftSize; i++)
                {
                    IFC(m_coverageBuffer.AddInterval(pEdgeCurrent->X, pEdgeCurrent->Next->X));
                }

                // Output the scans
                while (nSubpixelYCurrent < nSubpixelYNext)
                {
                    GenerateOutput(nSubpixelYCurrent);
                    nSubpixelYCurrent += c_nShiftSize;
                }
                m_coverageBuffer.Reset();
            }

            Assert(nSubpixelYCurrent == nSubpixelYNext);

            // Remove stale edges.
            while (pEdgeCurrent->EndY != INT_MIN)
            {
                if (pEdgeCurrent->EndY <= nSubpixelYCurrent)
                {
                    // Unlink and advance

                    pEdgeCurrent = pEdgeCurrent->Next;
                    pEdgePrevious->Next = pEdgeCurrent;
                }
                else
                {
                    // Advance

                    pEdgePrevious = pEdgeCurrent;
                    pEdgeCurrent = pEdgeCurrent->Next;
                }
            }
        }
        else
        {
            //
            // Not two vertical edges, so fall back to the general case.
            //

            IFC(FillEdges(fillMode, pEdgeActiveList, nSubpixelYCurrent));

            // Advance nSubpixelYCurrent
            nSubpixelYCurrent += 1;

            // Advance DDA and update edge list
            AdvanceDDAAndUpdateActiveEdgeList(nSubpixelYCurrent, pEdgeActiveList);
        }

        //
        // Update edge list
        //

        if (nSubpixelYCurrent == nSubpixelYNextInactive)
        {
            InsertNewEdges(
                pEdgeActiveList,
                nSubpixelYCurrent,
                &pInactiveEdgeArray,
                &nSubpixelYNextInactive
                );
        }
    }

    //
    // Output the last scanline that has partial coverage
    //

    if ((nSubpixelYCurrent & c_nShiftMask) != 0)
    {
        GenerateOutput(nSubpixelYCurrent);
    }

    if (CreateComplementGeometry())
    {
        // Generate spans for scanlines in complement below start of shape.
        m_coverageBuffer.Reset();
        // +c_nShiftMask makes sure we advance to next full Y not generated.
        int y = (nSubpixelYCurrent + c_nShiftMask) >> c_nShift;
        while (y < m_rcComplementBounds.bottom)
        {
            GenerateOutput(y << c_nShift);
            ++y;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:  RasterizePath
//
//  Synopsis:  Fill (or sometimes stroke) that path.
//

HRESULT
RasterizePath(
    __in_ecount(cPoints)   const MilPoint2F *rgPoints,      // Points of the path to stroke/fill
    __in_ecount(cPoints)   const BYTE *rgTypes,            // Types array of the path
    __in_range(>=,2) const UINT cPoints,             // Number of points in the path
    __in_ecount(1) const CBaseMatrix *pMatPointsToDevice,
    MilFillMode::Enum fillMode,
    MilAntiAliasMode::Enum antiAliasMode,
    __inout_ecount(1) CSpanSink *pSpanSink,                // The sink for the spans produced by the
                                                           // rasterizer. For AA, this sink must
                                                           // include an operation to apply the AA
                                                           // coverage.
    __in_ecount(1) CSpanClipper *pClipper,                 // Clipper.
    __in_ecount(1) const MilPointAndSizeL *prcBounds,               // Bounding rectangle of the path points.
    float rComplementFactor,
    __in_ecount_opt(1) const CMILSurfaceRect *prcComplementBounds
    )
{
    HRESULT hr = S_OK;
    CInactiveEdge inactiveArrayStack[INACTIVE_LIST_NUMBER];
    CInactiveEdge *inactiveArray;
    CInactiveEdge *inactiveArrayAllocation = NULL;
    CEdge headEdge;
    CEdge tailEdge;
    CEdge *activeList;
    CEdgeStore edgeStore;
    CInitializeEdgesContext edgeContext;

    Assert(rComplementFactor < 0 || antiAliasMode == MilAntiAliasMode::EightByEight);
    Assert(rComplementFactor < 0 || prcComplementBounds);
    
    Assert(pMatPointsToDevice);

    edgeContext.ClipRect = NULL;

    tailEdge.X = INT_MAX;       // Terminator to active list
#if SORT_EDGES_INCLUDING_SLOPE
    tailEdge.Dx = INT_MAX;      // Terminator to active list
#endif
    tailEdge.StartY = INT_MAX;  // Terminator to inactive list

    tailEdge.EndY = INT_MIN;
    headEdge.X = INT_MIN;       // Beginning of active list
    edgeContext.MaxY = INT_MIN;

    headEdge.Next = &tailEdge;
    activeList = &headEdge;
    edgeContext.Store = &edgeStore;

    edgeContext.AntiAliasMode = antiAliasMode;

    //////////////////////////////////////////////////////////////////////////

    CMILSurfaceRect rc;
    pClipper->GetClipBounds(&rc);
    pClipper->SetOutputSpan(pSpanSink);

    MilPointAndSizeL rcTemp;
    MilPointAndSizeL rcMilPointAndSizeL = {rc.left, rc.top, rc.Width(), rc.Height()};

    INT yClipBottom = rc.bottom;
    UINT totalCount = 0;
    
    // check to see if we're fully clipped.
    // If the path contains 0 or 1 points, we can ignore it.
    if ((cPoints > 1) && IntersectRect(&rcTemp, &rcMilPointAndSizeL, prcBounds))
    {
        //   Need input path validation
        //  This check is a band-aid. Generally speaking, RasterizePath assumes (and asserts) that
        //  the input path is valid (and so this check should be an assertion).
        //
        //  The advantage of this is that other internal code which generates paths (e.g. widening)
        //  can use RasterizePath without needing full consistency checking.
        //
        //  But what we are missing, is path-validation code at the MILRender entry point level.


        // Scale the clip bounds rectangle by 16 to account for our
        // scaling to 28.4 coordinates:

        RECT clipBounds;
        clipBounds.left = rc.left * 16;
        clipBounds.top = rc.top * 16;
        clipBounds.right = rc.right * 16;
        clipBounds.bottom = rc.bottom * 16;

        edgeContext.ClipRect = &clipBounds;

        // The clipper should call the sink's OutputSpan

        {
            //////////////////////////////////////////////////////////////////////////

            // Convert all our points to 28.4 fixed point:

            CMILMatrix matrix(*pMatPointsToDevice);

            // Given matrix transforms points to device space in half-pixel-center
            // notation. We need integer-pixel-center notation here, so we
            // adjust the matrix to shift all the coordinates by 1/2 of pixel.
            matrix.SetDx(matrix.GetDx() - 0.5f);
            matrix.SetDy(matrix.GetDy() - 0.5f);

            AppendScaleToMatrix(&matrix, TOREAL(16), TOREAL(16));

            // Enumerate the path and construct the edge table:

            MIL_THR(FixedPointPathEnumerate(
                        rgPoints,
                        rgTypes,
                        cPoints,
                        &matrix,
                        edgeContext.ClipRect,
                        &edgeContext
                        ));

            if (FAILED(hr))
            {
                if (hr == WGXERR_VALUEOVERFLOW)
                {
                    // Draw nothing on value overflow and return
                    hr = S_OK;
                }
                goto Cleanup;
            }
        }
        totalCount = edgeStore.StartEnumeration();        
    }

    if (totalCount == 0)
    {
        // Path empty or totally clipped.  We're almost done.
        // May need to take care of complement geometry.
        if (rComplementFactor >= 0)
        {
            // Complement factor only support in AA rendering.
            Assert(antiAliasMode != MilAntiAliasMode::None);
            
            CAntialiasedFiller filler(pClipper, antiAliasMode);
            filler.SetComplementFactor(
                rComplementFactor,
                prcComplementBounds
                );

            pSpanSink->SetAntialiasedFiller(&filler);

            IFC(filler.RasterizeNoEdges());
        }

        hr = S_OK;
        goto Cleanup;
    }

    // At this point, there has to be at least two edges.  If there's only
    // one, it means that we didn't do the trivial rejection properly.

    Assert(totalCount >= 2);

    inactiveArray = &inactiveArrayStack[0];
    if (totalCount > (INACTIVE_LIST_NUMBER - 2))
    {
        UINT tempCount = 0;
        IFC (UIntAdd(totalCount, 2, &tempCount));
        IFC(HrMalloc(
            Mt(MAARasterizerEdge),
            sizeof(CInactiveEdge),
            tempCount,
            (void **)&inactiveArrayAllocation
            ));

        inactiveArray = inactiveArrayAllocation;
    }

    // Initialize and sort the inactive array:

    INT iCurrentY = InitializeInactiveArray(
        &edgeStore,
        inactiveArray,
        totalCount,
        &tailEdge
        );

    INT yBottom = edgeContext.MaxY;

    Assert(yBottom > 0);

    // Skip the head sentinel on the inactive array:

    inactiveArray++;

    if (antiAliasMode != MilAntiAliasMode::None)
    {
        CAntialiasedFiller filler(pClipper, antiAliasMode);
        if (rComplementFactor >= 0)
        {
            filler.SetComplementFactor(
                rComplementFactor,
                prcComplementBounds
                );
        }

        pSpanSink->SetAntialiasedFiller(&filler);

        // 'yClipBottom' is in 28.4 format, and has to be converted
        // to the 30.2 (or 29.3) format we use for antialiasing:

        yBottom = min(yBottom, yClipBottom << c_nShift);

        // 'totalCount' should have been zero if all the edges were
        // clipped out (RasterizeEdges assumes there's at least one edge
        // to be drawn):

        Assert(yBottom > iCurrentY);

        IFC(filler.RasterizeEdges(
            activeList,
            inactiveArray,
            iCurrentY,
            yBottom,
            fillMode
            ));
    }
    else
    {
        CAliasedFiller filler(pClipper);
        Assert(!(rComplementFactor >= 0));

        yBottom = min(yBottom, yClipBottom);

        // 'totalCount' should have been zero if all the edges were
        // clipped out (RasterizeEdges assumes there's at least one edge
        // to be drawn):

        Assert(yBottom > iCurrentY);

        filler.RasterizeEdges(
            activeList,
            inactiveArray,
            iCurrentY,
            yBottom,
            fillMode
            );
    }

Cleanup:
    // Free any objects and get outta here:

    if (inactiveArrayAllocation != NULL)
    {
        GpFree(inactiveArrayAllocation);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:  IsPPAAMode
//
//  Synopsis:  Returns TRUE if the given MilAntiAliasMode::Enum is a per-primitive
//             antialiasing (PPAA) mode.
//

BOOL IsPPAAMode(
    MilAntiAliasMode::Enum aam
    )
{
    switch (aam)
    {
    case MilAntiAliasMode::None:
        return FALSE;

    case MilAntiAliasMode::EightByEight:
        return TRUE;

    default:
        AssertMsg(FALSE, "Unrecognized antialias mode");
        return FALSE;
    }
}







