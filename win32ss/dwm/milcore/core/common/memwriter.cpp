// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    memwriter.cpp

Abstract:

   CMilDataStreamWriter.  This class manages a list of allocations which contain 
   a linear set of discrete allocations called 'items'.  An item is just continugous
   block of memory prepended by the byte count of the item.  It is important for the
   item to be contiguous so that when each item is read back, a simple pointer can be
   returned to the caller.

   Whenever an allocation occurs, this class allocates extra memory to avoid individual 
   allocations for every item.  It then uses that memory to service future memory 
   requests.  When the allocation is full, including the 'extra' memory, it performs 
   another allocation (the allocations are referred to as a 'blocks').  Blocks are linked 
   together using SLISTs.  A previous implementation didn't use SLISTs and instead used 
   ReallocHeap to dynamically grow an array.  ReallocHeap proved to be prohibitively expensive, 
   and was replaced with AllocHeap by linking blocks together.  
   

Environment:

    User mode only.

--*/

#include "precomp.hpp"

MtDefine(CMilDataStreamWriter, MILRender, "CMilDataStreamWriter");

#define MEMSTREAM_ENLARGE_LIMIT  0x10000

/*++

Routine Description:

    CMilDataStreamWriter::CMilDataStreamWriter

--*/

CMilDataStreamWriter::CMilDataStreamWriter()
{
    Initialize();
}

/*++

Routine Description:

    CMilDataStreamWriter::~CMilDataStreamWriter

--*/

CMilDataStreamWriter::~CMilDataStreamWriter()
{
    FreeResources();
}

VOID CMilDataStreamWriter::Reset()
{
    FreeResources();    
    Initialize();
}

VOID CMilDataStreamWriter::Initialize()
{
    InitializeListHead(&m_dataList);    

    m_pCurrentBlock = NULL;
    
    m_cbTotalAllocations = 0;
    m_cbTotalWritten = 0;

    m_pItemPos = NULL;
    m_nItemSize = 0;

}

VOID CMilDataStreamWriter:: FreeResources()
{
    while (!IsListEmpty(&m_dataList))
    {
        DataStreamBlock *pFree = static_cast<DataStreamBlock *>(RemoveHeadList(&m_dataList));
        
        FreeHeap (pFree);        
    }
    
    FreeHeap(m_pCurrentBlock);    
}


/*++

Routine Description:

    CMilDataStreamWriter::BeginItem

--*/

HRESULT CMilDataStreamWriter::BeginItem()
{
    HRESULT hr = S_OK;

    //
    // Validate writer state
    //

    if (IsWithinItem()
        || GetRemainingByteCount() < sizeof(UINT))
    {
        IFC(E_UNEXPECTED);
    }

    //
    // Remember where the item started, so that one
    // can fix the size and add the item id.
    //

    m_pItemPos = m_pCurrentBlock->data + m_pCurrentBlock->cbWritten;

    //
    // Initialize the item size with 0
    //
    
    RtlCopyMemory(m_pItemPos, &m_nItemSize, sizeof(UINT));

    m_nItemSize = sizeof(UINT);
    IFC(IncreaseWrittenByteCount(sizeof(UINT)));

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::AddItemData

--*/

HRESULT CMilDataStreamWriter::AddItemData(
    __in_bcount(cbData) LPCVOID pcvData,
    UINT cbData
    )
{
    HRESULT hr = S_OK;

    //
    // Validate writer state.
    //

    if (!IsWithinItem()                        // ensure the item was begun correctly
        || GetRemainingByteCount() < cbData)   // make sure we have enough storage
    {
        IFC(E_UNEXPECTED);
    }

    //
    // Add the data to the current item and fix the size of the item.
    //

    if (cbData > 0)
    {
        RtlCopyMemory(m_pCurrentBlock->data + m_pCurrentBlock->cbWritten, pcvData, cbData);

        // Update the number of bytes written
        IFC(IncreaseWrittenByteCount(cbData));

        // Update the current item size.
        // The check at the beginning of this method ensures this operation won't overflow
        m_nItemSize += cbData;
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::EndItem

--*/

HRESULT CMilDataStreamWriter::EndItem()
{
    HRESULT hr = S_OK;

    //
    // Validate the writer state.
    //

    if (!IsWithinItem())
    {
        IFC(E_UNEXPECTED);
    }

    // #Future Consideration:  Consider returning bytes used due to rounding to the caller.
    //
    // If EnsureSize is called once for multiple items, this rounding could result in less
    // memory available than the caller assumes, if the caller doesn't take the additional bytes needed
    // for rounding into account.  Since the data must be aligned to prevent exceptions on 64-bit 
    // platforms, this padding is required.
    //
    // Returning the amount of bytes used for alignment in an output-parameter would force the caller 
    // to be aware of this caveat.    
    //    

    //
    // Align the size of the batch record.
    //

    // Calculate the padding needed
    UINT nPreviousItemSize = m_nItemSize;    
    IFC(RoundUpToAlignDWORD(&m_nItemSize));
    UINT cbRounded = m_nItemSize - nPreviousItemSize;    

    // Add the alignment padding to the amount written
    IFC(IncreaseWrittenByteCount(cbRounded));

    //
    // Save the size of the item, when we are ready.
    //

    RtlCopyMemory(m_pItemPos, &m_nItemSize, sizeof(UINT));

    //
    // Mark the object state as ready for a new item.
    //

    m_pItemPos = NULL;
    m_nItemSize = 0;
    
Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::BeginAddEndItem
    This function adds just one item. It also calls BeginItem and
    EndItem as needed.

--*/

HRESULT
CMilDataStreamWriter::BeginAddEndItem(
    __in_bcount(cbData) LPCVOID pcvData,
    UINT cbData
    )
{
    HRESULT hr = S_OK;
    IFC(BeginItem());
    IFC(AddItemData(pcvData, cbData));

Cleanup:
    MIL_THR_SECONDARY(EndItem());
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::AddBlockData

--*/

HRESULT 
CMilDataStreamWriter::AddBlockData(
    __in_bcount(cbData) LPCVOID pcvData,
    UINT cbData
    )
{
    HRESULT hr = S_OK;

    if (IsWithinItem())
    {
        IFC(E_UNEXPECTED);
    }

    if (cbData > 0)
    {
        IFC(EnsureSize(cbData));

        RtlCopyMemory(m_pCurrentBlock->data + m_pCurrentBlock->cbWritten, pcvData, cbData);

        IFC(IncreaseWrittenByteCount(cbData));        
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::Initialize

--*/

HRESULT CMilDataStreamWriter::Initialize(UINT cbInitSize)
{
    HRESULT hr = S_OK;

    if (cbInitSize > 0)
    {
        UINT cbRoundedInitSize = cbInitSize;        
        
        IFC(RoundUpToAlignDWORD(&cbRoundedInitSize));
        
        IFC(AllocateNewBlock(cbRoundedInitSize));
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::EnsureItem

--*/

HRESULT CMilDataStreamWriter::EnsureItem(UINT cbItemSize)
{
    HRESULT hr = S_OK;

    UINT cbPrefixedItemSize = 0;

    IFC(UIntAdd(cbItemSize, sizeof(UINT), &cbPrefixedItemSize));

    IFC(EnsureSize(cbPrefixedItemSize));

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::EnsureSize

--*/

HRESULT CMilDataStreamWriter::EnsureSize(UINT cbMemNeeded)
{
    HRESULT hr = S_OK;

    UINT cbRoundedMemNeeded = cbMemNeeded;

    UINT cbRemainingBytes = 0;

    IFC(RoundUpToAlignDWORD(&cbRoundedMemNeeded));    

    if (m_pCurrentBlock)
    {
        cbRemainingBytes = GetRemainingByteCount();
    }

    //
    // Allocate a new block if the number of bytes remaining is less than what's needed.
    //
    
    if (m_pCurrentBlock == NULL ||
        cbRemainingBytes < cbRoundedMemNeeded)
    {
        //
        // Increase the allocation size, so that we do not alloc a whole lot.
        //

        UINT cbNewBlockSize;

        // #Future Consideration:  Consider avoiding 'wasted' memory
        //
        // If cbRemainingBytes != 0, then that memory will not be utilized.  We can't
        // have an item span over 2 blocks because a simple pointer is returned
        // when the item is read.  We found that the complexity added by working around
        // this tradeoff didn't warrant the potential gains. 
        //
        // Specifically, testing with 4 Perf BVTs (HourPicker, MSNBaml,
        // MockLayout, and AvPhoto) and 3 scalability scenarios (5K Paths, 50K Paths, &
        // 100K Paths) has shown that the relative amount of memory wasted
        // is minimal (Max. 3.36%, Min. 0.03%, Avg: 1.01%, Median: 0.66%).  Furthermore, this
        // wasted allocation is a transient allocation that doesn't contribute to steady working set.
        //
        // We did partially implement a workaround which added an extra level of indirection to
        // items (i.e., an item whose only content was a pointer to another item).  The complexity of
        // that solution didn't feel warranted for a maximum 3% improvement in transient allocations.
        // If that tradeoff changes however, other potential solutions include returning a 'smart'
        // pointer which can iterate over items that span multiple blocks, or copying items which
        // do span multiple blocks into a single contiguous block when it is read.

        // #Future Consideration:  Consider tuning MEMSTREAM_ENLARGE_LIMIT
        //
        // It is important for this growth pattern to be linear.  This is because when
        // it's exponential, it often ends up wasting half of the memory it allocated. The 
        // remaining question is, 'linear on what?'.  If the allocation size is too small
        // the heap will become excessively fragmented and we will spend too much time allocating.
        // If it's too large, we will waste too much memory.  For now we're not going to change
        // the precedent of exponential up to 64K and then linear on 64K since this seems
        // to balance these tradeoffs.  But if we find this usually isn't optimal, we should
        // consider changing it.

        UINT cbIncrement =
            m_cbTotalAllocations < MEMSTREAM_ENLARGE_LIMIT    
            ? m_cbTotalAllocations                      // Initially the growth is exponential...
            : MEMSTREAM_ENLARGE_LIMIT;                  // ...after reaching the cap make it linear.

        //
        // Release unused empty blocks that are too small
        //
        if (m_pCurrentBlock &&
            m_pCurrentBlock->cbWritten == 0)
        {
            // A batch was ensured previously, but never written to.  To avoid having
            // to loop thru empty blocks in CMilDataBlockReader, we release empty blocks 
            // that are too small. 

            FreeHeap(m_pCurrentBlock);
            m_pCurrentBlock = NULL;            
        }

        //
        // Allocate the new block
        //

        IFC(UIntAdd(cbRoundedMemNeeded, cbIncrement, &cbNewBlockSize));  

        IFC(AllocateNewBlock(cbNewBlockSize));
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::AllocateNewBlock

--*/

HRESULT CMilDataStreamWriter::AllocateNewBlock(UINT cbSize)
{
    HRESULT hr = S_OK;

    DataStreamBlock *pNewBlock = NULL;

#if DBG
    UINT cbRoundedSize = cbSize;

    IFC(RoundUpToAlignDWORD(&cbRoundedSize));

    AssertMsg(cbRoundedSize == cbSize, "CMilDataStreamWriter::GrowToSize: expected DWORD aligned size");

    // Guard that we're not allocating a new block when the previous block
    // still had enough room 
    Assert(!m_pCurrentBlock ||
           (cbSize > (m_pCurrentBlock->cbAllocated - m_pCurrentBlock->cbWritten)));

    // Guard that new block's aren't allocated in the middle of an item.  
    Assert(!IsWithinItem());

    // Guard that empty blocks aren't created.  This is ensured by EnsureSize because 
    // of the 'cbRemainingBytes < cbRoundedMemNeeded' check, and is explicitly checked 
    // for during Initialize.
    Assert(cbSize > 0);
#endif

    //
    // Reallocate the buffer only if necessary:
    //

    // Calculate the size of the allocation.
    UINT cbBlockAllocation;
    IFC(UIntAdd(
        cbSize,         
        sizeof(DataStreamBlock) - sizeof(pNewBlock->data), // Subtract the size of the inline data
        &cbBlockAllocation
        ));

    // Allocate & initialize the new block
    pNewBlock = reinterpret_cast<DataStreamBlock*>(AllocHeap(cbBlockAllocation));
    IFCOOM(pNewBlock);
    pNewBlock->cbAllocated = cbSize;
    pNewBlock->cbWritten = 0;

    // Track the total amount of memory allocated
    IFC(UIntAdd(m_cbTotalAllocations, cbBlockAllocation, &m_cbTotalAllocations));    

    // Add the old block to the list if one exists.  
    // The only time it won't exist is when we allocate the first block.
    if (m_pCurrentBlock)
    {
        InsertTailList(&m_dataList, m_pCurrentBlock);
    }

    // Handoff block pointer to m_pCurrentBlock;
    m_pCurrentBlock = pNewBlock;     
    pNewBlock = NULL;

Cleanup:

    FreeHeap(pNewBlock);
    
    RRETURN(hr);
}

/*++

Routine Description:

    CMilDataStreamWriter::IncreaseWrittenByteCount

--*/


HRESULT 
CMilDataStreamWriter::IncreaseWrittenByteCount(UINT cbBytes)
{
    HRESULT hr = S_OK;

    // Update both the current block & total amount written
    IFC(UIntAdd(m_pCurrentBlock->cbWritten, cbBytes, &m_pCurrentBlock->cbWritten));
    IFC(UIntAdd(m_cbTotalWritten, cbBytes, &m_cbTotalWritten));           

Cleanup:
    RRETURN(hr);
}



