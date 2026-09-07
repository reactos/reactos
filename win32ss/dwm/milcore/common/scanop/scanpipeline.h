// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      A pipeline of scan operations.
//
//      This class composes scan operations to form the back-end rasterizer
//      pipeline. It includes brush color generation, modification such as
//      alpha-masking, and alpha-blending to the destination.
//

// Number of intermediate buffers needed. Count them:
//   1. Brush colors
//   2. Destination pixels (may need to read them in to convert them before the
//     blend).
//   3. One extra so that we can ping-pong between buffers.
//
//  (4. When DrawGlyphs is integrated: ClearType mask?)

#define NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS 3

// We represent the pipeline with an array of PipelineItem structures.

struct PipelineItem
{
    ScanOpFunc m_pfnScanOp;   // The operation function
    ScanOpParams m_Params;    // Parameters to this operation
    UINT m_uiDestBuffer;      // Used only during pipeline construction, in
                              // CScanPipeline::Builder::End. Identifies the
                              // destination buffer.
};

//+-----------------------------------------------------------------------------
//
//  Class:     CSPIntermediateBuffers
//
//  Synopsis:  Manages intermediate buffers to be used by CScanPipeline.
//

class CSPIntermediateBuffers
{
public:
    CSPIntermediateBuffers()
    {
        m_rgpvBuffers[0] = NULL;
    }

    ~CSPIntermediateBuffers()
    {
        FreeBuffers();
    }

    HRESULT AllocateBuffers(
        PERFMETERTAG mt,
        UINT uiMaxWidth
        )
    {
        HRESULT hr = S_OK;

        Assert(NULL == m_rgpvBuffers[0]);

        IFC( HrMalloc(
            mt,
            sizeof(MilColorF) * NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS,
                // We know these numbers are low, so this multiplication can't
                // overflow.
            uiMaxWidth,
            &(m_rgpvBuffers[0])) );

    #if DBG_ANALYSIS
        {
        // prefast doesn't know that this is ensured by HrMalloc
            HRESULT tempHR = MultiplyUINT(
                sizeof(MilColorF) * NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS,
                uiMaxWidth,
                OUT m_uDbgAnalysisMaxAllowableWidth
                );
            Assert(SUCCEEDED(tempHR));
        }
    #endif

        for (UINT i=1; i<NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS; i++)
        {
            m_rgpvBuffers[i] = static_cast<MilColorF *>(m_rgpvBuffers[i-1]) + uiMaxWidth;
        }

    Cleanup:
        RRETURN(hr);
    }

    VOID FreeBuffers()
    {
        GpFree(m_rgpvBuffers[0]);
        m_rgpvBuffers[0] = NULL;
    }

    VOID GetBuffer(
        __in_range(0, NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS-1) UINT uBufferIndex,
        __deref_out_bcount(*pcbDbgAnalysisBufferSize) void **ppIntermediateBuffer
        DBG_ANALYSIS_COMMA_PARAM(__out_ecount(1) UINT *pcbDbgAnalysisBufferSize)
        )
    {
        GetBuffer(
            uBufferIndex,
            const_cast<const void **>(ppIntermediateBuffer)
            DBG_ANALYSIS_COMMA_PARAM(pcbDbgAnalysisBufferSize)
            );
    }

    VOID GetBuffer(
        __in_range(0, NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS-1) UINT uBufferIndex,
        __deref_out_bcount(*pcbDbgAnalysisBufferSize) const void **ppIntermediateBuffer
        DBG_ANALYSIS_COMMA_PARAM(__out_ecount(1) UINT *pcbDbgAnalysisBufferSize)
        )
    {
        *ppIntermediateBuffer = m_rgpvBuffers[uBufferIndex];
    #if DBG_ANALYSIS
        *pcbDbgAnalysisBufferSize = m_cbDbgAnalysisIndividualBufferSize;
    #endif
    }

#if DBG_ANALYSIS
    UINT DbgAanalysisGetMaxAllowableWidth() const
    {
        return m_uDbgAnalysisMaxAllowableWidth;
    }
#endif

private:

    // These buffers are in a single allocation. m_rgpvBuffers[0]
    // points to the start of the memory.
    void *m_rgpvBuffers[NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS];
#if DBG_ANALYSIS
    UINT m_cbDbgAnalysisIndividualBufferSize;
    UINT m_uDbgAnalysisMaxAllowableWidth;
#endif
};

//+-----------------------------------------------------------------------------
//
//  Class:     CScanPipeline
//
//  Synopsis:  A set of scan operations that, once set up, can be run on a set
//             of spans. This class is used:
//
//             * for image format conversion
//
//------------------------------------------------------------------------------

class CScanPipeline
{

public:

    CScanPipeline();
    virtual ~CScanPipeline();

    VOID Run(
        __inout_bcount(cbDbgAnalysisDestSize) VOID *pvDest,
        __in_bcount_opt(cbDbgAnalysisSrcSize) const VOID *pvSrc,   // Only needed for format conversion
        UINT iCount,
        INT iX,
        INT iY
        DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisDestSize)
        DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisSrcSize)
        );

    // Release expensive resources. See CSpanSink::ReleaseExpensiveResources().
    // *Must* be called between calls to Initialize*.
    virtual VOID ReleaseExpensiveResources();

protected:
    friend class ScanPipelineBuilder;

#if DBG
    virtual VOID AssertNoExpensiveResources();
#else
    virtual VOID AssertNoExpensiveResources() {}
#endif // DBG

    virtual VOID ResetPipeline()
    {
        m_rgPipeline.Reset();
        m_rgofsDestPointers.Reset();
        m_rgofsSrcPointers.Reset();

        AssertNoExpensiveResources();
    }

    INT_PTR ConvertPipelinePointerToOffset(__in_ecount(1) const VOID **ppvPointer);
    __out_ecount(1) VOID **ConvertOffsetToPipelinePointer(INT_PTR ofsPointer);

    VOID UpdatePipelinePointers(
        __inout_bcount(cbDbgAnalysisDestBufferSize) VOID *pvDest,
        __in_bcount_opt(cbDbgAnalysisSrcBufferSize) const VOID *pvSrc
        DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisDestBufferSize)
        DBG_ANALYSIS_COMMA_PARAM(UINT cbDbgAnalysisSrcBufferSize)
        );

    // We use an initial allocation that's big enough for most normal pipelines.

    DynArrayIA<PipelineItem, 10> m_rgPipeline;
    PipelineParams m_PipelineParams;

    // See UpdatePipelinePointers for an explanation of these two arrays.

    DynArrayIA<INT_PTR, 3> m_rgofsDestPointers;
    DynArrayIA<INT_PTR, 2> m_rgofsSrcPointers;
};




