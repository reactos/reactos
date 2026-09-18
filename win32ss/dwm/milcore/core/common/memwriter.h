// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

// 
// Abstract:
// 
//    CMilDataStreamWriter.  This class manages a list of allocations which contain 
//    a linear set of discrete allocations called 'items'.  An item is just continugous
//    block of memory prepended by the byte count of the item.  It is important for the
//    item to be contiguous so that when each item is read back, a simple pointer can be
//    returned to the caller.
// 
//    Whenever an allocation occurs, this class allocates extra memory to avoid individual 
//    allocations for every item.  It then uses that memory to service future memory 
//    requests.  When the allocation is full, including the 'extra' memory, it performs 
//    another allocation (the allocations are referred to as a 'blocks').  Blocks are linked 
//    together using SLISTs.  A previous implementation didn't use SLISTs and instead used 
//    ReallocHeap to dynamically grow an array.  ReallocHeap proved to be prohibitively expensive, 
//    and was replaced with AllocHeap by linking blocks together.  
// 
//-----------------------------------------------------------------------------------------------

#pragma once

MtExtern(CMilDataStreamWriter);

// Header for an allocation 'block' used to link blocks together & track their size.
struct DataStreamBlock : LIST_ENTRY
{    
    UINT cbAllocated; // Number of bytes allocated, including the 4 bytes for the data field.
    UINT cbWritten;   // Number of bytes written.
    BYTE data[4];     // First DWORD of the allocation.  This exists within the DataStreamBlock
                      // structure to make the data following the header easier (and thus, 
                      // less error-prone) to access.
};

//
// This class manages writing items to a provided buffer. It manages memory
// allocation and an exponential growth algorithm.
//

class CMilDataStreamWriter
{
public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilDataStreamWriter));

    //
    // NOTE: the destructor is explicitly non-virtual - this class intentionally
    // has no virtual methods. Use caution inheriting from this class.
    // See CMilCommandBatch for an example of how this class is intended to
    // be used.
    //

    CMilDataStreamWriter();
    ~CMilDataStreamWriter();


    VOID Reset();

public:

    //
    // The following methods build up a data item incrementally, the only
    // restriction is that some maximum of the item size is known before hand
    // (this is done so that we can reduce the reallocation cost).
    //

    HRESULT BeginItem();

    HRESULT AddItemData(
        __in_bcount(cbData) LPCVOID pcvData,
        UINT cbData
        );
    
    HRESULT EndItem();

    HRESULT BeginAddEndItem(
        __in_bcount(cbData) LPCVOID pcvData,
        UINT cbData
    );

    HRESULT AddBlockData(
        __in_bcount(cbData) LPCVOID pcvData,
        UINT cbData
        );
    
    //
    // General methods.
    //

    UINT GetTotalWrittenByteCount() const
    {
        return m_cbTotalWritten;
    }

    LIST_ENTRY *FlushData()
    {
        //
        // Ends this buffer by moving the last block to the end of the allocation list.
        //
        
        // Should not be called until all item's are ended.        
        Assert (!IsWithinItem());

        if (NULL != m_pCurrentBlock)
        {
            InsertTailList(&m_dataList, m_pCurrentBlock);        
            m_pCurrentBlock = NULL;
        }

        return &m_dataList;
    }

    HRESULT Initialize(UINT cbInitialSize);

    //
    // Ensure that we can write an item of size cbSize to the datablock. This
    // includes expanding the size to include a header.
    //

    HRESULT EnsureItem(UINT cbSize);

    // NOTE - this method should be moved back to "protected" once GlyphCache
    // is rewritten to no longer make use of fake DUCE channels when circumventing
    // regular DUCE channels.
    HRESULT EnsureSize(UINT cbSize);

protected:

    VOID Initialize();
    VOID FreeResources();
        

    bool IsWithinItem () const
    {     
        // Either of these conditions is enough to determine if we're currently
        // between BeginItem & EndItem calls, but we check both of them to help 
        // prevent bugs from getting us into a bad state (i.e., we're just being
        // extra cautious).
        return m_pItemPos != NULL ||
               m_nItemSize != 0;
    }

    UINT GetRemainingByteCount() const
    { 
        return m_pCurrentBlock->cbAllocated - m_pCurrentBlock->cbWritten;
    }

    HRESULT IncreaseWrittenByteCount(UINT cbBytes);
    
    //
    // ensure that there are at least cbSize bytes avaliable in the datablock.
    // EnsureSize uses a 2x growth algorithm. GrowToSize is used to actually
    // grow the buffer and is used by EnsureSize.
    //

    HRESULT AllocateNewBlock(UINT cbSize);

    LIST_ENTRY m_dataList;              // Head of the allocation list.
    DataStreamBlock *m_pCurrentBlock;   // Currently active allocation.

    UINT m_cbTotalAllocations;          // Total number of bytes allocated.  Used by
                                        // memory growth alogorithm.
                                        
    UINT m_cbTotalWritten;              // Total number of bytes written.  Used
                                        // when copying the list of allocations into
                                        // a contiguous array.

    BYTE *m_pItemPos;                   // Beginning of the current item.  Size is written
                                        // here during EndItem();
                                        
    UINT m_nItemSize;                   // Keeps track of number of bytes written to this item.
};



