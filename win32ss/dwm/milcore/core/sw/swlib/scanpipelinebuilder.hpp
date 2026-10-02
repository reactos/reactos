// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Class CScanPipelineRendering::Builder2 - helper class to build a
//      pipeline of scan operations.
//
//      Scan pipeline is an array of items, each of which defines some scan
//      operation that are used to be executed sequentially.
//
//      With CScanPipelineRendering::Builder2, pipeline is build with following
//      schema:
//      1) Create an instance of this class.
//      2) Define operations by calling AddOperation() several times.
//      3) Call Finalize()
//
//      Each operation can manipulate with up to three data buffers, referred to
//      as src1, src2 and dest. Names are just hints; actually each buffer can
//      be either source or destination, or even serve as input and output at
//      the same operation. Each buffer can be either internal (intermediate) or
//      external (final destination or original source).
//
//      AddOperation() needs arguments to point to partucular buffers. It
//      manipulates not with buffer pointers but with virtual buffers
//      identifiers - VBIDs, that are nothing but INTs.
//
//      Some of VBIDs are preallocated:
//        VBID_NULL used when particular operation does not use this buffer;
//        VBID_DEST corresponds to external final target buffer
//        VBID_AUX means external auxiliary buffer (either original source
//                 for format convertion or alpha buffer for clear type
//                 text rendering).
//
//      VBIDs for intermediate buffers should be allocated by GetBuffer() call.
//
//      The amount of intermediate VBIDs is not limited with amount of real
//      intermediate buffers. Allocate as much VBIDs as you need. The only rule
//      is that if you've then you'll get same real buffer. Builder provides
//      optimization that forces different VBIDs to share same real buffers
//      whenever it is safe.
//
//      VBIDs are associated with real buffers on Finalize() call that
//      implements this optimization.
//
//      The Redirect() method provides additional flexibility for complicated
//      builder's users. At any moment prior to Finalize() you may replace all
//      the mentions of some VBID with another one. Typically, it is useful when
//      user suddenly discovers that final format conversion is not required and
//      just redirects intermediate VBID to VBID_DEST, thus avoiding copy
//      operation.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

enum VBID {
    VBID_NULL = 0,
    VBID_DEST = 1,
    VBID_AUX  = 2,
    VBID_FIRST_INTERNAL = 3,
    VBID_LAST_INTERNAL  = 16,
    VBID_MAX = VBID_LAST_INTERNAL + 1
};

bool IsNothing(VBID vbid) 
{ 
    return vbid == VBID_NULL; 
}

bool IsExternal(VBID vbid) 
{ 
    return vbid == VBID_DEST || vbid == VBID_AUX;
}

bool IsInternal(VBID vbid) 
{
    return vbid >= VBID_FIRST_INTERNAL && vbid <= VBID_LAST_INTERNAL;
}

//+-----------------------------------------------------------------------------
//
//  Structure:
//      PipelineItemProxy
//
//  Synopsis:
//      internal entity to keep information for single PipelineItem.
//
//------------------------------------------------------------------------------
struct PipelineItemProxy
{
    ScanOpFunc m_pfnScanOp;
    OpSpecificData *m_posd;
    VBID m_vbids[3];
    // m_vbids[0] corresponds to PipelineItem.m_Params.m_pvSrc1
    // m_vbids[1] corresponds to PipelineItem.m_Params.m_pvSrc2
    // m_vbids[2] corresponds to PipelineItem.m_Params.m_pvDest
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CScanPipelineRendering::Builder2
//
//  Synopsis:
//      see comments in file header
//
//------------------------------------------------------------------------------
class CScanPipelineRendering::Builder2
{
public:

    CScanPipelineRendering::Builder2(
        __in_ecount(1) CScanPipelineRendering *pSP,
        __in_ecount(1) CSPIntermediateBuffers *pIntermediateBuffers
        );

    void AddOperation(
        ScanOpFunc pScanOp,
        OpSpecificData *posd,
        VBID vbidSrc1,
        VBID vbidSrc2,
        VBID vbidDest
        );

    UINT GetCount() const 
    { 
        return m_proxyCount; 
    }

    void Redirect(
        VBID vbidFrom,
        VBID vbidTo
        );

    HRESULT Finalize();

    VBID GetBuffer()
    {
        Assert(m_nextVBID < VBID_MAX);
        return (VBID)m_nextVBID++;
    }

private:
    CScanPipelineRendering *m_pSP;
    CSPIntermediateBuffers *m_pIntermediateBuffers;
    INT m_nextVBID;

    static const int PROXY_SIZE = 16;
    PipelineItemProxy m_proxy[PROXY_SIZE];
    UINT m_proxyCount;

    VOID* m_pAssocTable[VBID_MAX];
    UINT m_pAllocTable[VBID_MAX];

    bool m_fIntermediateBufferFree[NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS];

private:
    HRESULT MakeupAssociationTable();
    HRESULT AllocIntermediateBuffer(VBID vbid);
    VOID FreeIntermediateBuffer(VBID vbid);
    HRESULT AddBufferReference(VOID** ppvPointer, VBID vbid);
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::CScanPipelineRendering::Builder2
//
//  Synopsis:
//      Prepare for building.
//
//------------------------------------------------------------------------------

CScanPipelineRendering::Builder2::Builder2(
    __in_ecount(1) CScanPipelineRendering *pSP,
    __in_ecount(1) CSPIntermediateBuffers *pIntermediateBuffers
    )
{
    m_pSP = pSP;
    m_pIntermediateBuffers = pIntermediateBuffers;
    m_nextVBID = VBID_AUX+1;
    m_proxyCount = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::AddOperation
//
//  Synopsis:
//      Add an operation into pipeline.
//
//------------------------------------------------------------------------------
void
CScanPipelineRendering::Builder2::AddOperation(
    ScanOpFunc pScanOp,
    OpSpecificData *posd,
    VBID vbidSrc1,
    VBID vbidSrc2,
    VBID vbidDest
    )
{
    Assert(pScanOp != NULL);
    Assert(m_proxyCount < PROXY_SIZE);
#pragma prefast (suppress : 37001 37002 37003) // Checked in the Assert above
    PipelineItemProxy &proxy = m_proxy[m_proxyCount++];

    proxy.m_pfnScanOp = pScanOp;
    proxy.m_posd = posd;
    proxy.m_vbids[0] = vbidSrc1;
    proxy.m_vbids[1] = vbidSrc2;
    proxy.m_vbids[2] = vbidDest;
}

void CScanPipelineRendering::Builder2::Redirect(
    VBID vbidFrom,
    VBID vbidTo
    )
{
    for (UINT i = 0; i < m_proxyCount; i++)
    {
        PipelineItemProxy &proxy = m_proxy[i];
        for (int j = 0; j < 3; j++)
        {
            if (proxy.m_vbids[j] == vbidFrom)
            {
                proxy.m_vbids[j] = vbidTo;
            }
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::Finalize
//
//  Synopsis:
//      Compose the pipeline.
//
//------------------------------------------------------------------------------

HRESULT
CScanPipelineRendering::Builder2::Finalize()
{
    HRESULT hr = S_OK;

    IFC(MakeupAssociationTable());

    // Allocate space in the pipeline
    PipelineItem *pPI;
    IFC(m_pSP->m_rgPipeline.AddMultiple(m_proxyCount, &pPI));

    // fill the pipeline from proxy,
    // gather external buffer references along the way
    for (UINT i = 0; i < m_proxyCount; i++)
    {
        PipelineItem &item = pPI[i];
        PipelineItemProxy &proxy = m_proxy[i];

        VBID vbid_Src1 = proxy.m_vbids[0];
        VBID vbid_Src2 = proxy.m_vbids[1];
        VBID vbid_Dest = proxy.m_vbids[2];

        item.m_pfnScanOp = proxy.m_pfnScanOp;
        item.m_Params.m_posd = proxy.m_posd;

        item.m_Params.m_pvSrc1 = m_pAssocTable[vbid_Src1];
        item.m_Params.m_pvSrc2 = m_pAssocTable[vbid_Src2];
        item.m_Params.m_pvDest = m_pAssocTable[vbid_Dest];

        if (IsExternal(vbid_Src1))
        {
            IFC( AddBufferReference((void**)&item.m_Params.m_pvSrc1, vbid_Src1));
        }

        if (IsExternal(vbid_Src2))
        {
            IFC( AddBufferReference((void**)&item.m_Params.m_pvSrc2, vbid_Src2));
        }

        if (IsExternal(vbid_Dest))
        {
            IFC( AddBufferReference((void**)&item.m_Params.m_pvDest, vbid_Dest));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::MakeupAssociationTable
//
//  Synopsis:
//      Make up the corresponcdence between VBIDs and real intermediate buffers.
//
//------------------------------------------------------------------------------
HRESULT
CScanPipelineRendering::Builder2::MakeupAssociationTable()
{
    HRESULT hr = S_OK;
    //
    // Initialize the tables
    //

    INT lastUsed[VBID_MAX];
    for (INT i = 0; i < VBID_MAX; i++)
    {
        lastUsed[i] = -1;
        m_pAssocTable[i] = NULL;
    }

    for (UINT i = 0; i < NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS; i++)
    {
        m_fIntermediateBufferFree[i] = true;
    }


    //
    // Pass 1: gather usage information
    //         for each VBID, detect the index of operation
    //         where it was used last time

    for (UINT i = 0; i < m_proxyCount; i++)
    {
        const PipelineItemProxy &proxy = m_proxy[i];
        
        __pfx_assert(proxy.m_vbids[0] < VBID_MAX, "VBID out of range");
        __pfx_assert(proxy.m_vbids[1] < VBID_MAX, "VBID out of range");
        __pfx_assert(proxy.m_vbids[2] < VBID_MAX, "VBID out of range");

        __analysis_assume(proxy.m_vbids[0] < VBID_MAX);
        __analysis_assume(proxy.m_vbids[1] < VBID_MAX);
        __analysis_assume(proxy.m_vbids[2] < VBID_MAX);

        lastUsed[proxy.m_vbids[0]] = i;
        lastUsed[proxy.m_vbids[1]] = i;
        lastUsed[proxy.m_vbids[2]] = i;
    }


    // Pass 2: makeup association table
    for (UINT i = 0; i < m_proxyCount; i++)
    {
        const PipelineItemProxy &proxy = m_proxy[i];

        for (int j = 0; j < 3; j++)
        {
            VBID vbid = proxy.m_vbids[j];
            IFC(AllocIntermediateBuffer(vbid));
        }

        for (int j = 0; j < 3; j++)
        {
            VBID vbid = proxy.m_vbids[j];
            if (lastUsed[vbid] == static_cast<INT>(i)) FreeIntermediateBuffer(vbid);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::AllocIntermediateBuffer
//
//  Synopsis:
//      Associate given VBID with intermediate buffer. Do it only if it not yet
//      done, and if vbid refers to intermediate buffer. Find the free
//      intermediate buffer using the table m_fIntermediateBufferFree[] that
//      reflects idle state of each intermediate buffer for the moment of
//      executing certain scan operation.
//
//------------------------------------------------------------------------------
HRESULT
CScanPipelineRendering::Builder2::AllocIntermediateBuffer(VBID vbid)
{
    HRESULT hr = S_OK;

    if (!IsInternal(vbid))
        goto Cleanup; // preallocated or null

    if (m_pAssocTable[vbid])
        goto Cleanup; // already allocated

    // Do allocate
    UINT i = 0;
    for (; i < NUM_SCAN_PIPELINE_INTERMEDIATE_BUFFERS; i++)
    {
        if (m_fIntermediateBufferFree[i])
        {
        #if DBG_ANALYSIS
            UINT cbDbgAnalysisBufferSize;
        #endif
            VOID *pBuffer;
            m_pIntermediateBuffers->GetBuffer(
                i,
                &pBuffer
                DBG_ANALYSIS_COMMA_PARAM(&cbDbgAnalysisBufferSize)
                );
            m_fIntermediateBufferFree[i] = false;
            m_pAssocTable[vbid] = pBuffer;

            // remember intermediate buffer index for easier freeing
            m_pAllocTable[vbid] = i;
            goto Cleanup;
        }
    }

    // This point should never be reached, because
    // there should be at least one free buffer.
    // If not so then we need review builder usage or
    // increase g_cScanPipelineIntermediateBuffers constant.

    RIP("No free intermediate buffers");
    IFC(WGXERR_INTERNALERROR);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::FreeIntermediateBuffer
//
//  Synopsis:
//      Undo the allocation made by AllocIntermediateBuffer(). Intermediate
//      buffer associated with given VBID is freeed, however m_pAssocTable
//      continues holding the association.
//
//------------------------------------------------------------------------------
VOID
CScanPipelineRendering::Builder2::FreeIntermediateBuffer(VBID vbid)
{
    if (vbid <= VBID_AUX) return;

    UINT intermediateBufferIndex = m_pAllocTable[vbid];
    m_fIntermediateBufferFree[intermediateBufferIndex] = true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CScanPipelineRendering::Builder2::AddBufferReference
//
//  Synopsis:
//      Add the reference to external buffer to the pipeline, so that
//      CScanPipelineRendering::UpdatePipelinePointers can set it before use.
//
//------------------------------------------------------------------------------
HRESULT
CScanPipelineRendering::Builder2::AddBufferReference(VOID** ppvPointer, VBID vbid)
{
    INT_PTR ofsPointer = m_pSP->ConvertPipelinePointerToOffset((const VOID**)ppvPointer);
    if (vbid == VBID_DEST)
    {
        return m_pSP->m_rgofsDestPointers.Add(ofsPointer);
    }
    else
    {
        Assert(vbid == VBID_AUX);
        return m_pSP->m_rgofsSrcPointers.Add(ofsPointer);
    }
}



