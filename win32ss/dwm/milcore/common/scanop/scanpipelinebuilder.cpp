// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Builder for CScanPipeline
//
//------------------------------------------------------------------------------
#include "precomp.hpp"


//+-------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::Builder
//
//  Synopsis:  Sets the builder state ready to start building the pipeline.
//
//  Notes:     Caller must call ResetPipeline() first.
//
//             The buffers in ppvIntermediateBuffers must each be at least
//             16 * maxCount bytes in size, where maxCount is the largest
//             iCount that will be passed to Run().
//

ScanPipelineBuilder::ScanPipelineBuilder(
    __in_ecount(1) CScanPipeline *pSP,
    __inout_ecount(1) CSPIntermediateBuffers *pIntermediateBuffers,
    BuilderMode eBuilderMode
    )
{
    // First initialize simple members. Later operations in this function
    // (e.g. GetFreeBuffer, IsPipelineEmpty) rely on this.

    m_pSP = pSP;
    m_pIntermediateBuffers = pIntermediateBuffers;
    m_uiIntermediateBuffers = 0;

    for (int i=0; i<NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS; i++)
    {
        m_rgfIntermediateBufferFree[i] = true;
    }

    // Now we can do the more complicated initialization.

    Assert(IsPipelineEmpty());     // Caller must call ResetPipeline before
                                   // creating the Builder.

    switch (eBuilderMode)
    {
    case BM_FORMATCONVERSION:
        // We just use BL_MAIN, converting from the source to
        // the destination.

        m_rglocSubpipeData[SP_MAIN] = BL_SRCBUFFER;
        m_rglocSubpipeData[SP_BLENDSOURCE] = BL_INVALID;
        m_rguiDestBufferIndex[SP_MAIN] = 0;
        m_rguiDestBufferIndex[SP_BLENDSOURCE] = 0;
        break;

    case BM_RENDERING:
        // SP_BLENDSOURCE generates the brush data, and applies alpha
        // masks/PPAA to it.
        //
        // For regular SourceOver, SP_MAIN converts the destination data to
        // the blend format; a blend operation combines SP_MAIN and
        // SP_BLENDSOURCE, and finally SP_MAIN converts the result back to
        // the destination format.
        //
        // For SourceCopy, SP_MAIN is not used. SP_BLENDSOURCE converts the
        // result to the destination format.

        m_rglocSubpipeData[SP_MAIN] = BL_DESTBUFFER;
        m_rglocSubpipeData[SP_BLENDSOURCE] = GetFreeIntermediateBuffer();
        m_rguiDestBufferIndex[SP_MAIN] = 0;
        m_rguiDestBufferIndex[SP_BLENDSOURCE] = ++m_uiIntermediateBuffers;

        break;
    }

    C_ASSERT(BM_NUM == 2);
    C_ASSERT(SP_NUM == 2);  // This initialization code will need to be
                            // updated if SP_NUM or BM_NUM changes.
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddOp_Unary
//
//  Synopsis:  Add a unary operation, in the given sub-pipeline
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::AddOp_Unary(
    ScanOpFunc pScanOp,
    __in_ecount_opt(1) OpSpecificData *posd,
    Subpipe eSubpipe
    )
{
    HRESULT hr = S_OK;

    // Check that we're not calling this when we shouldn't

    Assert(m_rglocSubpipeData[eSubpipe] != BL_INVALID);

    if (!pScanOp)
    {
        IFC( WGXERR_UNSUPPORTED_OPERATION );
    }

    PipelineItem *pPI;
    IFC(AddOperation(pScanOp, posd, m_rguiDestBufferIndex[eSubpipe], &pPI));

    pPI->m_Params.m_pvSrc1 = NULL;  // Not used for unary operations
    pPI->m_Params.m_pvSrc2 = NULL;

    IFC( AddBufferReference(&(pPI->m_Params.m_pvDest), m_rglocSubpipeData[eSubpipe]) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::End
//
//  Synopsis:  End the pipeline, performing final tasks to complete it.
//
//------------------------------------------------------------------------------
HRESULT
ScanPipelineBuilder::End()
{
    HRESULT hr = S_OK;

    // Check that we have at least one operation
    Assert(!IsPipelineEmpty());

    // The last intermediate buffer we allocated, is a proxy for the final
    // "destination buffer". Convert the buffer references.

    IFC( ConvertDestBufferReferences() );

    // This must have added at least 1 dest-buffer reference.

    Assert(m_pSP->m_rgofsDestPointers.GetCount() > 0);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddOp_Binary_Inplace
//
//  Synopsis:  Add a binary operation operating in-place, in the given
//             sub-pipeline. "In-place" means that m_pvSrc1 == m_pvDest.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::AddOp_Binary_Inplace(
    ScanOpFunc pScanOp,
    __in_ecount_opt(1) OpSpecificData *posd,
    Subpipe eSubpipe
    )
{
    HRESULT hr = S_OK;

    // Check that we're not calling this when we shouldn't

    Assert(m_rglocSubpipeData[eSubpipe] != BL_INVALID);

    if (!pScanOp)
    {
        IFC( WGXERR_UNSUPPORTED_OPERATION );
    }

    PipelineItem *pPI;
    IFC(AddOperation(pScanOp, posd, m_rguiDestBufferIndex[eSubpipe], &pPI));

    pPI->m_Params.m_pvSrc2 = NULL;  // Not used for binary operations

    IFC( AddBufferReference(&(pPI->m_Params.m_pvSrc1), m_rglocSubpipeData[eSubpipe]) );
    IFC( AddBufferReference(&(pPI->m_Params.m_pvDest), m_rglocSubpipeData[eSubpipe]) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddOp_Binary
//
//  Synopsis:  Add a binary operation, in the given sub-pipeline
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::AddOp_Binary(
    ScanOpFunc pScanOp,
    __in_ecount_opt(1) OpSpecificData *posd,
    Subpipe eSubpipe
    )
{
    HRESULT hr = S_OK;

    // Check that we're not calling this when we shouldn't

    Assert(m_rglocSubpipeData[eSubpipe] != BL_INVALID);

    if (!pScanOp)
    {
        IFC( WGXERR_UNSUPPORTED_OPERATION );
    }

    PipelineItem *pPI;
    IFC(AddOperation(pScanOp, posd, m_rguiDestBufferIndex[eSubpipe], &pPI));

    pPI->m_Params.m_pvSrc2 = NULL;  // Not used for binary operations

    IFC( AddBufferReference(&(pPI->m_Params.m_pvSrc1), m_rglocSubpipeData[eSubpipe]) );

    PingPongBuffer(eSubpipe);
    pPI->m_uiDestBuffer = m_rguiDestBufferIndex[eSubpipe];
    IFC( AddBufferReference(&(pPI->m_Params.m_pvDest), m_rglocSubpipeData[eSubpipe]) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddOp_PTernary
//
//  Synopsis:  Add a pseudo-ternary operation: BlendSource op Main -> Main
//             See scanoperation.h for definition of "pseudo-ternary".
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::AddOp_PTernary(
    ScanOpFunc pScanOp,
    __in_ecount_opt(1) OpSpecificData *posd,
    __out_ecount(1) bool *pfNeedWriteRMW
        // If true, caller must ensure that the last operation in the pipeline
        // is a WriteRMW operation. (Either by adding one explicitly, or if
        // this blend is the last operation in the pipeline.)
        //
        // Without this, we would output garbage wherever a blend pixel is
        // fully transparent.
    )
{
    HRESULT hr = S_OK;
    bool fNeedWriteRMW = true;

    // Check that we're not calling this when we shouldn't

    Assert(m_rglocSubpipeData[SP_MAIN] != BL_INVALID);
    Assert(m_rglocSubpipeData[SP_BLENDSOURCE] != BL_INVALID);

    if (!pScanOp)
    {
        IFC( WGXERR_UNSUPPORTED_OPERATION );
    }

    PipelineItem *pPI;
    IFC(AddOperation(pScanOp, posd, m_rguiDestBufferIndex[SP_MAIN], &pPI));

    IFC( AddBufferReference(&(pPI->m_Params.m_pvSrc1), m_rglocSubpipeData[SP_BLENDSOURCE]) );
    IFC( AddBufferReference(&(pPI->m_Params.m_pvSrc2), m_rglocSubpipeData[SP_MAIN]) );

    if (IsIntermediateBuffer(m_rglocSubpipeData[SP_MAIN]))
    {
        // If "DestIn" is an intermediate buffer, then "DestOut" can be the same
        // buffer and no WriteRMW is needed.
        //
        // This is true even if this is the last operation in the pipeline (i.e.
        // "DestOut" will end up pointing to the final destination).

        ReuseBuffer(SP_MAIN);

        fNeedWriteRMW = false;
    }
    else
    {
        // "DestOut" must be a different buffer from "DestIn".

        PingPongBuffer(SP_MAIN);

        // ... and so a WriteRMW will be needed.

        fNeedWriteRMW = true;
    }

    pPI->m_uiDestBuffer = m_rguiDestBufferIndex[SP_MAIN];
    IFC( AddBufferReference(&(pPI->m_Params.m_pvDest), m_rglocSubpipeData[SP_MAIN]) );

    *pfNeedWriteRMW = fNeedWriteRMW;

    // 
    //  Consider setting a member variable here, and checking in End() that
    //  the last operation is a WriteRMW (or a blend, which is also a WriteRMW).
    //  Problem: Right now it's not easy to look up information about an operation
    //  once it has been added.

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::Append_Convert_Interchange
//
//  Synopsis:  Appends zero or more operations, to convert between two
//             interchange formats, in the given sub-pipeline.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::Append_Convert_Interchange(
    MilPixelFormat::Enum fmtDest,
    MilPixelFormat::Enum fmtSrc,
    Subpipe eSubpipe
    )
{
    HRESULT hr = S_OK;

    Assert(IsInterchangeFormat(fmtDest));
    Assert(IsInterchangeFormat(fmtSrc));

    if (fmtSrc != fmtDest)
    {
        switch (fmtSrc)
        {
        case MilPixelFormat::BGRA32bpp:
            switch (fmtDest)
            {
            case MilPixelFormat::RGBA64bpp:
                IFC( AddOp_Binary(Convert_32bppARGB_64bppARGB, NULL, eSubpipe) );
                break;

            case MilPixelFormat::RGBA128bppFloat:
                IFC( AddOp_Binary(GammaConvert_32bppARGB_128bppABGR, NULL, eSubpipe) );
                break;

            default:
                Assert(FALSE);
                IFC( WGXERR_INTERNALERROR );
                break;
            }
        break;

        case MilPixelFormat::RGBA64bpp:
            switch (fmtDest)
            {
            case MilPixelFormat::BGRA32bpp:
                IFC( AddOp_Binary(Convert_64bppARGB_32bppARGB, NULL, eSubpipe) );
                break;

            case MilPixelFormat::RGBA128bppFloat:

                //
                // convert colors without gamma correction because both source
                // and destination are linear.
                //

                IFC( AddOp_Binary(GammaConvert_64bppARGB_128bppABGR, NULL, eSubpipe) );
                break;

            default:
                Assert(FALSE);
                IFC( WGXERR_INTERNALERROR );
                break;
            }
        break;

        case MilPixelFormat::RGBA128bppFloat:
            switch (fmtDest)
            {
            case MilPixelFormat::BGRA32bpp:
                IFC( AddOp_Binary(GammaConvert_128bppABGR_32bppARGB, NULL, eSubpipe) );
                break;

            case MilPixelFormat::RGBA64bpp:

                //
                // convert colors without gamma correction because both source
                // and destination are linear.
                //

                IFC( AddOp_Binary(GammaConvert_128bppABGR_64bppARGB, NULL, eSubpipe) );

                break;

            default:
                Assert(FALSE);
                IFC( WGXERR_INTERNALERROR );
                break;
            }
        break;

        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::Append_Convert_BGRA32bpp_Grayscale
//
//  Synopsis:  Convert 32bpp (P)ARGB to the same, but in grayscale, so all the
//             channels have uniform intensity non-halftoned formats, in the
//             given sub-pipeline.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::Append_Convert_BGRA32bpp_Grayscale(
    Subpipe eSubpipe
    )
{
    RRETURN(AddOp_Binary(Convert_32bppARGB_Grayscale, NULL, eSubpipe));
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::Append_Convert_NonHalftoned
//
//  Synopsis:  Append zero or more operations to convert between two
//             non-halftoned formats, in the given sub-pipeline.
//
//  Notes:     Caller must set m_fDither16bpp appropriately. (This setting
//             affects all 16bpp dithering operations in the pipeline.)
//
//             If converting from 32bppARGB or 32bppPARGB, and the input data is
//             known to be opaque - you can use 32bppRGB instead, which will
//             produce a faster conversion in some cases. Likewise for
//             128bppBGR.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::Append_Convert_NonHalftoned(
    MilPixelFormat::Enum fmtDest,
    MilPixelFormat::Enum fmtSrc,
    BOOL fSrcOpaque,
    Subpipe eSubpipe
    )
{
    HRESULT hr = S_OK;

    Assert(!IsIndexedPixelFormat(fmtDest));
    Assert(!IsIndexedPixelFormat(fmtSrc));

    if (fmtSrc == fmtDest)
    {
        // For non-indexed formats, this is a NOP.
    }
    else
    {
        MilPixelFormat::Enum fmtDestInterchange = GetNearestInterchangeFormat(fmtDest);
        MilPixelFormat::Enum fmtSrcInterchange = GetNearestInterchangeFormat(fmtSrc);

        BOOL fDestHasAlpha = HasAlphaChannel(fmtDest);

        //
        // Convert from source format to nearest interchange format, if necessary
        //

        if (fmtSrc == fmtSrcInterchange)
        {
            // NOP.
        }
        else if (fSrcOpaque && IsPremultipliedFormOf(fmtSrc, fmtSrcInterchange))
        {
            // If the source data is opaque, conversion between a premultiplied
            // format and its equivalent non-premultiplied format, is a NOP.
        }
        else if (!fDestHasAlpha && (fmtSrc == MilPixelFormat::BGR32bpp))
        {
            // NOP. If the destination has no alpha channel, then the alpha
            // channel will not be used during conversion. So 32bppRGB data
            // can be treated like 32bppARGB.

            // For 128bppBGR we could do the same, if we're sure that treating a
            // random bit pattern as a floating-point number:
            //
            // * won't throw exceptions
            // * won't cause perf problems
            //
            // We think this is the case for x86 - so this could be improved somewhat.
        }
        else
        {
            IFC( AddOp_Binary(GetOp_ConvertFormat_ToInterchange(fmtSrc), NULL, eSubpipe) );
        }

        //
        // Convert between interchange formats, if necessary
        //

        IFC( Append_Convert_Interchange(fmtDestInterchange, fmtSrcInterchange, eSubpipe) );

        //
        // Convert from interchange format to destination format, if necessary
        //

        if (fmtDestInterchange == fmtDest)
        {
            // NOP.
        }
        else if (fSrcOpaque && IsPremultipliedFormOf(fmtDest, fmtDestInterchange))
        {
            // If the source data is opaque, conversion between a premultiplied
            // format and its equivalent non-premultiplied format, is a NOP.
        }
        else if (IsNoAlphaFormOf(fmtDest, fmtDestInterchange))
        {
            // NOP. For 32bppRGB or 128bppBGR, we can write garbage to the
            // "unused" channel. So, no conversion is necessary.
        }
        else
        {
            IFC( AddOp_Binary(GetOp_ConvertFormat_InterchangeToNonHalftoned(fmtDest), NULL, eSubpipe) );
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::Append_Convert_NonHalftoned_ReportNOP
//
//  Synopsis:  Same as Append_Convert_NonHalftoned, but also reports whether
//             this evaluated to a NOP.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::Append_Convert_NonHalftoned_ReportNOP(
    MilPixelFormat::Enum fmtDest,
    MilPixelFormat::Enum fmtSrc,
    BOOL fSrcOpaque,
    Subpipe eSubpipe,
    __out_ecount(1) bool *fIsNop
    )
{
    HRESULT hr = S_OK;

    UINT uStart = GetOpCount();

    IFC(Append_Convert_NonHalftoned(fmtDest, fmtSrc, fSrcOpaque, eSubpipe));

    UINT uEnd = GetOpCount();

    *fIsNop = (uEnd == uStart);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::GetFreeIntermediateBuffer
//
//  Synopsis:  Choose a new intermediate buffer from the free list.
//
//------------------------------------------------------------------------------

BufferLocation ScanPipelineBuilder::GetFreeIntermediateBuffer()
{
    UINT idxFree;
    for (idxFree=0; idxFree<NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS; idxFree++)
    {
        if (m_rgfIntermediateBufferFree[idxFree]) break;
    }

    // There should be at least one free buffer, because we have 1 more buffer than
    // we need for each subpipeline.

    AssertMsg(idxFree < NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS, "No free intermediate buffers");

    m_rgfIntermediateBufferFree[idxFree] = false;

    return static_cast<BufferLocation>(BL_INTERMEDIATEBUFFER_FIRST + idxFree);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::ReleaseBuffer
//
//  Synopsis:  Release a buffer (or do nothing if it is not an intermediate
//             buffer).
//
//------------------------------------------------------------------------------

VOID ScanPipelineBuilder::ReleaseBuffer(BufferLocation bloc)
{
    // Release the current buffer

    if (IsIntermediateBuffer(bloc))
    {
        // BufferLocation is an enum
#pragma prefast(push)
#pragma prefast (disable : 37001 37002 37003 "IsIntermediateBuffer checks to ensure bloc > BL_INTERMEDIATEBUFFER_FIRST")
        m_rgfIntermediateBufferFree[bloc - BL_INTERMEDIATEBUFFER_FIRST] = true;
#pragma prefast(pop)
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::PingPongBuffer
//
//  Synopsis:  Allocate a new buffer, and free the current one, for the given
//             subpipe.
//
//------------------------------------------------------------------------------

VOID ScanPipelineBuilder::PingPongBuffer(Subpipe sp)
{
    Assert(sp >= 0 && sp < SP_NUM);

    BufferLocation blocNew = GetFreeIntermediateBuffer();
    BufferLocation blocCurrent = m_rglocSubpipeData[sp];

    Assert(blocNew != blocCurrent);

    ReleaseBuffer(blocCurrent);

    m_rglocSubpipeData[sp] = blocNew;
    m_rguiDestBufferIndex[sp] = ++m_uiIntermediateBuffers;
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::ReuseBuffer
//
//  Synopsis:  Reuse the current buffer as an output, for the given subpipe.
//
//  Notes:     This is important because of how we fix up the "last buffer"
//             used (End() calls ConvertDestBufferReferences()).
//
//------------------------------------------------------------------------------

VOID ScanPipelineBuilder::ReuseBuffer(Subpipe sp)
{
    Assert(sp >= 0 && sp < SP_NUM);

    // Although we're reusing the buffer, we need to mark it as "a different
    // buffer" from the POV of ConvertDestBufferReferences().

    m_rguiDestBufferIndex[sp] = ++m_uiIntermediateBuffers;
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::EnforcePipelineDone
//
//  Synopsis:  Mark the pipeline as complete, so that any further attempt
//             to add to it, will cause an assertion.
//
//------------------------------------------------------------------------------

#if DBG
VOID ScanPipelineBuilder::EnforcePipelineDone()
{
    for (int sp = 0; sp < SP_NUM; sp++)
    {
        m_rglocSubpipeData[sp] = BL_INVALID;
    }
}
#endif

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::ConvertDestBufferReferences
//
//  Synopsis:  Convert each reference to the last intermediate buffer allocated,
//             into a reference to the "destination" buffer.
//
//------------------------------------------------------------------------------

HRESULT ScanPipelineBuilder::ConvertDestBufferReferences()
{
    HRESULT hr = S_OK;

    if (!m_uiIntermediateBuffers)
    {
        // We didn't use any intermediate buffers, so do nothing.
    }
    else
    {
        UINT uiLastIntermediateBuffer = m_uiIntermediateBuffers;

        // The last intermediate buffer we allocated, should instead be the
        // final output buffer. Record each reference to this final
        // intermediate buffer.

        INT cItems = m_pSP->m_rgPipeline.GetCount();
        PipelineItem *pPI = &(m_pSP->m_rgPipeline[0]);

        while (cItems--)
        {
            if (pPI->m_uiDestBuffer == uiLastIntermediateBuffer)
            {
                IFC( AddBufferReference(&(pPI->m_Params.m_pvDest), BL_DESTBUFFER) );
            }

            pPI++;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddBufferReference
//
//  Synopsis:  Add a reference to the given buffer
//
//------------------------------------------------------------------------------

HRESULT ScanPipelineBuilder::AddBufferReference(
    __deref_out_xcount_opt(size of intermediate buffer) VOID **ppvPointer,
    BufferLocation bloc
    )
{
    // If this is a writable pointer (i.e. m_pvDest), then we must not be
    // referring to the "original source" (which is read-only).

    Assert(bloc != BL_SRCBUFFER);

    return AddBufferReference(
        const_cast<const VOID **>(ppvPointer),
        bloc
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddBufferReference
//
//  Synopsis:  Add a reference to the given read-only buffer
//
//------------------------------------------------------------------------------

HRESULT ScanPipelineBuilder::AddBufferReference(
    __deref_out_xcount_opt(size of intermediate buffer) const VOID **ppvPointer,
    BufferLocation bloc
    )
{
    HRESULT hr=S_OK;

    // For intermediate buffers, we just need to set the pointer.

    if (IsIntermediateBuffer(bloc))
    {
    #if DBG_ANALYSIS
        UINT cbDbgAnalysisBufferSize;
    #endif

        // bloc is an enum of type BufferLocation.  IsIntermediateBuffer checks to make sure
        // that bloc is between the values BL_INTERMEDIATEBUFFER_FIRST and BL_INTERMEDIATEBUFFER_LAST
        // and as such doing bloc - BL_INTERMEDIATEBUFFER_FIRST should always be positive
        m_pIntermediateBuffers->GetBuffer(
            bloc - BL_INTERMEDIATEBUFFER_FIRST,
            OUT ppvPointer
            DBG_ANALYSIS_COMMA_PARAM(&cbDbgAnalysisBufferSize)
            );
    }
    else
    {
        // Otherwise, we register the pointer so that
        // CScanPipeline::UpdatePipelinePointers can set it before use.

        *ppvPointer = NULL;   // NULL the pointer to avoid hiding bugs. We
                              // expect UpdatePipelinePointers to overwrite
                              // this before it is actually used.

        INT_PTR ofsPointer = m_pSP->ConvertPipelinePointerToOffset(ppvPointer);

        if (bloc == BL_DESTBUFFER)
        {
            hr = m_pSP->m_rgofsDestPointers.Add(ofsPointer);
        }
        else if (bloc == BL_SRCBUFFER)
        {
            hr = m_pSP->m_rgofsSrcPointers.Add(ofsPointer);
        }
        else
        {
            AssertMsg(FALSE, "Unrecognized BufferLocation");
            hr = WGXERR_INTERNALERROR;
        }
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ScanPipelineBuilder::AddOperation
//
//  Synopsis:  Adds an operation to the pipeline. Caller must fill in the data
//             except for pScanOp and posd.
//
//------------------------------------------------------------------------------

HRESULT
ScanPipelineBuilder::AddOperation(
    ScanOpFunc pScanOp,
    __in_ecount_opt(1) OpSpecificData *posd,
    UINT uiDestBuffer,
    __deref_out_ecount(1) PipelineItem **ppPI
    )
{
    HRESULT hr;

    Assert(pScanOp != NULL);
    Assert(ppPI);

    IFC(m_pSP->m_rgPipeline.AddMultiple(1, ppPI));
    Assert(*ppPI);

    (*ppPI)->m_pfnScanOp = pScanOp;
    (*ppPI)->m_Params.m_posd = posd;
    (*ppPI)->m_uiDestBuffer = uiDestBuffer;

Cleanup:
    RRETURN(hr);
}






