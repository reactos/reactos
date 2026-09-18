// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//      A pipeline of scan operations.
//

#include "precomp.hpp"

MtDefine(MFormatConversionScanlineBuffers, MILRawMemory, "MFormatConversionScanlineBuffers");

//
// CScanPipeline
//

CScanPipeline::CScanPipeline()
{
}

CScanPipeline::~CScanPipeline()
{
    ReleaseExpensiveResources();
}

//+-----------------------------------------------------------------------------
//
//  Member:     CScanPipeline::UpdatePipelinePointers
//
//  Synopsis:   The "original source" and "ultimate destination" pointers may be
//              different for each call to Run(). (In contrast, the
//              "intermediate buffer" pointers do not change.)
//
//              AddBufferReference remembers references to the "original source"
//              or "ultimate destination", in two arrays, so that this function
//              can update them.
//
//              When rendering text in clear type mode, "original source"
//              pointers are used as "auxiliary destination" ones that contain
//              vector alpha values.
//

VOID CScanPipeline::UpdatePipelinePointers(
    __inout_bcount(cbDbgAnalysisDestBufferSize) VOID *pvDest,
    __in_bcount_opt(cbDbgAnalysisSrcBufferSize) const VOID *pvSrcIn    // Only needed for format conversion
    DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisDestBufferSize)
    DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisSrcBufferSize)
    )
{
    UINT uiCount;

    // Update pointers to the "ultimate destination" buffer

    uiCount = m_rgofsDestPointers.GetCount();
    Assert(uiCount);

    INT_PTR *pofsDest = &(m_rgofsDestPointers.First());

    while (uiCount--)
    {
        *(ConvertOffsetToPipelinePointer(*pofsDest)) = pvDest;
        pofsDest++;
    }

    // Update pointers to the "original source" buffer

    if (pvSrcIn)
    {
        // Cast away const - we assume here that the builder code only uses source
        // references for m_pvSrc1 or m_pvSrc2.

        VOID *pvSrc = const_cast<VOID *>(pvSrcIn);

        uiCount = m_rgofsSrcPointers.GetCount();
        INT_PTR *pofsSrc = &(m_rgofsSrcPointers.First());

        while (uiCount--)
        {
            *(ConvertOffsetToPipelinePointer(*pofsSrc)) = pvSrc;
            pofsSrc++;
        }
    }
    else
    {
        // We can have a NULL pvSrc (for rendering), but if so, the pipeline
        // must have no references to it.

        Assert(m_rgofsSrcPointers.GetCount() == 0);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:    CScanPipeline::Run
//
//  Synopsis:
//
//      Output pixels to the given destination.
//
//  Arguments:
//
//      pvDest:   The destination buffer
//      pvSrc:    The source buffer (unused for some pipelines)
//      iCount:   The number of pixels to output
//      iX/iY:    The device-space position of the first pixel being output.
//                Used for brush color generation and dithering.
//
// Return Value:
//
//   NONE
//
//------------------------------------------------------------------------------

VOID
CScanPipeline::Run(
    __inout_bcount(cbDbgAnalysisDestBufferSize) VOID *pvDest,
    __in_bcount_opt(cbDbgAnalysisSrcBufferSize) const VOID *pvSrc,   // Only needed for format conversion
    UINT uiCount,
    INT iX,
    INT iY
    DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisDestBufferSize)
    DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisSrcBufferSize)
    )
{
    if (!uiCount)
    {
        return;
    }

    m_PipelineParams.m_iX = iX;
    m_PipelineParams.m_iY = iY;
    m_PipelineParams.m_uiCount = uiCount;

    UpdatePipelinePointers(
        pvDest,
        pvSrc
        DBG_ANALYSIS_COMMA_PARAM(cbDbgAnalysisDestBufferSize)
        DBG_ANALYSIS_COMMA_PARAM(cbDbgAnalysisSrcBufferSize)
        );

    INT cItems = m_rgPipeline.GetCount();
    Assert(cItems);

    const PipelineItem *pPI = &(m_rgPipeline[0]);

    while (cItems--)
    {
        Assert(pPI->m_pfnScanOp);

        pPI->m_pfnScanOp(&m_PipelineParams, &(pPI->m_Params));
        pPI++;
    }
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:    CScanPipeline::AssertNoExpensiveResources
//
//  Synopsis:  Checks that all "expensive resources" have been released using
//             ReleaseExpensiveResources.
//
//------------------------------------------------------------------------------

VOID
CScanPipeline::AssertNoExpensiveResources()
{
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:    CScanPipeline::ReleaseExpensiveResources
//
//  Synopsis:  Release potentially-expensive resources.
//
//  Notes:     AssertNoExpensiveResources needs to be updated if this function
//             is changed.
//
//------------------------------------------------------------------------------

VOID
CScanPipeline::ReleaseExpensiveResources()
{
    // AssertNoExpensiveResources needs to be kept in sync with this function.
    AssertNoExpensiveResources();
}

//+-------------------------------------------------------------------------
//
//  Member:    CScanPipeline::ConvertPipelinePointerToOffset
//
//  Synopsis:  Converts a pointer to memory in the pipeline, into an offset
//             (which can be stored safely so that the pipeline can be
//             updated later).
//
//--------------------------------------------------------------------------

INT_PTR CScanPipeline::ConvertPipelinePointerToOffset(
    __in_ecount(1) const VOID **ppvPointer   // Points to a void* in the pipeline array
    )
{
    // Check that ppvPointer points into the pipeline array. Technically, this
    // pointer arithmetic is not safe - but it's acceptable for an assertion.

    Assert(((const VOID **) &(m_rgPipeline.First()) <= ppvPointer));
    Assert((ppvPointer < (const VOID **) (&(m_rgPipeline.Last()) + 1)));

    INT_PTR ofsPointer =
        reinterpret_cast<INT_PTR>(ppvPointer) -
        reinterpret_cast<INT_PTR>(&(m_rgPipeline.First()));

    return ofsPointer;
}

//+-----------------------------------------------------------------------------
//
//  Member:    CScanPipeline::ConvertOffsetToPipelinePointer
//
//  Synopsis:  Converts an offset from the start of the pipeline array, to a
//             pointer.
//
//------------------------------------------------------------------------------

__out_ecount(1) VOID **CScanPipeline::ConvertOffsetToPipelinePointer(
    INT_PTR ofsPointer
    )
{
    // Check that the offset is within the pipeline array.

    VOID **ppvPointer =
        reinterpret_cast<VOID **>(
            reinterpret_cast<INT_PTR>(&(m_rgPipeline.First()))
            + ofsPointer);

    // Check that ppvPointer points into the pipeline array. Technically,
    // this pointer arithmetic is not safe - but it's acceptable for an
    // assertion.

    Assert(   ((VOID **) &(m_rgPipeline.First()) <= ppvPointer)
           && (ppvPointer <= (VOID **) (&(m_rgPipeline.Last()) + 1)));

    return ppvPointer;
}



