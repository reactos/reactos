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

// As we build the pipeline, we remember which data is in which buffer (this
// can be an input/output buffer, or an intermediate buffer). This enum names
// the different locations.

enum BufferLocation {
    BL_INVALID,            // Not applicable to this pipeline, or
                           // EnforcePipelineDone has been called.
    BL_DESTBUFFER,         // The final destination buffer
    BL_SRCBUFFER,          // The original source buffer (used for format
                           // conversion)
    BL_INTERMEDIATEBUFFER_FIRST,
    BL_INTERMEDIATEBUFFER_LAST =
        BL_INTERMEDIATEBUFFER_FIRST + NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS - 1
};

inline bool IsIntermediateBuffer(BufferLocation bloc)
{
    return (bloc >= BL_INTERMEDIATEBUFFER_FIRST) && (bloc <= BL_INTERMEDIATEBUFFER_LAST);
}

// ScanPipelineBuilder: Holds the intermediate state and logic used to build the blending
//          pipeline.

class ScanPipelineBuilder
{
public:
    enum BuilderMode {
        BM_FORMATCONVERSION,
        BM_RENDERING,
        BM_NUM
    };

    enum Subpipe {
        SP_MAIN,            // The main pipeline. SrcCopy uses it exclusively.
                            // In SrcOver, this subpipeline converts destination
                            // pixels to the blend format, blends with
                            // SP_BLENDSOURCE, then converts back to the
                            // destination format.

        SP_BLENDSOURCE,     // Color data to be blended in SrcOver. This
                            // subpipeline generates brush data, applies
                            // effects, and applies PPAA coverage. The result
                            // is used by SrcOver, ReadRMW and WriteRMW ops.

        SP_NUM


        // Other possible sub-pipelines: Alpha mask, ClearType data.
    };

    ScanPipelineBuilder(
        __in_ecount(1) CScanPipeline *pSP,
        __inout_ecount(1) CSPIntermediateBuffers *pIntermediateBuffers,
        BuilderMode eBuilderMode
        );

    BOOL IsPipelineEmpty() const
    {
        return (m_pSP->m_rgPipeline.GetCount() == 0);
    }

    UINT GetOpCount() const
    {
        return m_pSP->m_rgPipeline.GetCount();
    }

    HRESULT End();

    //
    // AddOp_<type>_*
    //
    // These functions add a scan operation to the pipeline.
    //
    // <type> can be "Unary", "Binary", or "PTernary". For definitions of these
    // terms, see the description of ScanOpFunc in scanoperation.h.
    //

    HRESULT
    AddOp_Unary(
        ScanOpFunc pScanOp,
        __in_ecount_opt(1) OpSpecificData *posd,
        Subpipe eSubpipe
        );

    HRESULT
    AddOp_Binary_Inplace(
        ScanOpFunc pScanOp,
        __in_ecount_opt(1) OpSpecificData *posd,
        Subpipe eSubpipe
        );

    HRESULT
    AddOp_Binary(
        ScanOpFunc pScanOp,
        __in_ecount_opt(1) OpSpecificData *posd,
        Subpipe eSubpipe
        );

    HRESULT
    AddOp_PTernary(
        ScanOpFunc pScanOp,
        __in_ecount_opt(1) OpSpecificData *posd,
        __out_ecount(1) bool *pfNeedWriteRMW
        );


    //
    // Append_<task>_*
    //
    // These functions add zero or more operations to the pipeline, which together accomplish the given task.
    //

    HRESULT
    Append_Convert_Interchange(
        MilPixelFormat::Enum fmtDest,
        MilPixelFormat::Enum fmtSrc,
        Subpipe eSubpipe
        );

#if DBG
    VOID EnforcePipelineDone();

#else // DBG
    VOID EnforcePipelineDone()
    {
    }
#endif // DBG

    HRESULT
    Append_Convert_BGRA32bpp_Grayscale(
        Subpipe eSubpipe
        );

    HRESULT
    Append_Convert_NonHalftoned(
        MilPixelFormat::Enum fmtDest,
        MilPixelFormat::Enum fmtSrc,
        BOOL fSrcOpaque,
        Subpipe eSubpipe
        );

    HRESULT
    Append_Convert_NonHalftoned_ReportNOP(
        MilPixelFormat::Enum fmtDest,
        MilPixelFormat::Enum fmtSrc,
        BOOL fSrcOpaque,
        Subpipe eSubpipe,
        __out_ecount(1) bool *fIsNop
        );

protected:
    BufferLocation GetFreeIntermediateBuffer();
    VOID ReleaseBuffer(BufferLocation bloc);
    VOID PingPongBuffer(Subpipe sp);
    VOID ReuseBuffer(Subpipe sp);
    HRESULT ConvertDestBufferReferences();
    HRESULT AddBufferReference(
        __deref_out_xcount_opt(size of intermediate buffer) VOID **ppvPointer,
        BufferLocation bloc
        );
    HRESULT AddBufferReference(
        __deref_out_xcount_opt(size of intermediate buffer) const VOID **ppvPointer,
        BufferLocation bloc
        );
    HRESULT AddOperation(
        ScanOpFunc pScanOp,
        __in_ecount_opt(1) OpSpecificData *posd,
        UINT uiDestBuffer,
        __deref_out_ecount(1) PipelineItem **ppPI
        );

    CScanPipeline *m_pSP;

    CSPIntermediateBuffers *m_pIntermediateBuffers;  // The intermediate scan-line buffers

    bool m_rgfIntermediateBufferFree[NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS];

    UINT m_uiIntermediateBuffers;       // Counter used for
                                        // m_rguiDestBufferIndex. 0 means
                                        // invalid.

    //
    // BufferLocation variables for each sub-pipeline
    //

    BufferLocation m_rglocSubpipeData[SP_NUM];  // Which buffer/intermediate
                                                // buffer the data is currently
                                                // in.
    INT m_rguiDestBufferIndex[SP_NUM];          // An integer identifier for
                                                // that buffer, if it is an
                                                // intermediate buffer. (-1
                                                // otherwise). Used in End().

};





