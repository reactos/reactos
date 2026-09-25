// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CBufferDispenser implementation
//

#include "precomp.hpp"


MtDefine(CBufferDispenser_overflow_count, MILRender,
         "Memory allocations sent to heap when no space in buffer was available");

//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::CBufferDispenser
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

CBufferDispenser::CBufferDispenser(
    __inout_bcount(size) void *pvBuffer,
    __in_range(1,SIZE_T_MAX) size_t size
    )
    : m_ptrBuffer(IncrAlignTo(reinterpret_cast<uintptr_t>(pvBuffer),
                              kMinBufferAllocationAlignment))
{
    Assert(pvBuffer);
    Assert(size > 0);

    uintptr_t ptrBuffer = reinterpret_cast<uintptr_t>(pvBuffer);

    Assert((m_ptrBuffer - ptrBuffer) < size);

    Assert((reinterpret_cast<uintptr_t>(this) & AllocatedFromHeap) == 0);

    m_ptrNextAvailable = m_ptrBuffer;
    m_cbSpaceLeft = size - (m_ptrBuffer - ptrBuffer);
    m_cAllocations = 0;
#if DBG
    m_cDbgHeapAllocations = 0;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::Allocate
//
//  Synopsis:  Allocate requested memory from stack buffer is possible;
//             otherwise, request the memory from the heap
//

__allocator __bcount(size) void *
CBufferDispenser::Allocate(
    __in_range(1,SIZE_T_MAX)       size_t size,
    __in_range(1,(SIZE_T_MAX+1)/2) size_t alignment,
    PERFMETERTAG mt
    )
{
    Assert(size > 0);
    Assert(alignment > 0);
    // alignment should be a power of two
    Assert(IS_POWER_OF_2(alignment));
    // size should be a multiple of alignment
    Assert(IsAlignedTo(size, alignment));

    const size_t sizeRequested = size;

    //
    // Fix up size and alignment if needed
    //

    if (alignment > kMinBufferAllocationAlignment)
    {
        size += (alignment - kMinBufferAllocationAlignment);
        // Maximum add is (SIZE_T_MAX+1)/2, use SSIZE_T_MAX+1 to avoid C4307: '+' : integral constant overflow
        C_ASSERT(SIZE_T(SSIZE_T_MAX)+1 + kOverheadPerBufferAllocation <= SIZE_T_MAX);
    }
    else if (alignment < kMinBufferAllocationAlignment)
    {
        // Increase alignment to buffer dispenser's minimum
        size = IncrAlignTo(size, kMinBufferAllocationAlignment, alignment);
        alignment = kMinBufferAllocationAlignment;
        // Maximum add is kMinBufferAllocationAlignment-1
        C_ASSERT(kMinBufferAllocationAlignment-1 + kOverheadPerBufferAllocation <= SIZE_T_MAX);
    }

    // Add space for pointer storage
    C_ASSERT(kOverheadPerBufferAllocation > 0);
    size += kOverheadPerBufferAllocation;

    Assert(IsAlignedTo(size, kMinBufferAllocationAlignment));

    //
    // Attempt to allocate from buffer first, but fall back to heap if there
    // isn't enough space.
    //

    uintptr_t ptrRet = NULL;

    // If size <= sizeRequested we must have had overflow.  Size added will
    // never be greater than SIZE_T_MAX, per two C_ASSERTs above; so we only
    // need this one overflow check.
    if (size > sizeRequested)
    {
        if (this && m_cbSpaceLeft >= size)
        {
            ptrRet = AllocateFromBuffer(size, alignment, mt);
        }
        else
        {
            // Increment allocation overflow count whenever an allocation from the
            // buffer fails.
            MtAdd(Mt(CBufferDispenser_overflow_count), 1, 0);

            ptrRet = AllocateFromHeap(size, alignment, mt);
        }
    }

    return reinterpret_cast<void *>(ptrRet);
}

//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::AllocateFromBuffer
//
//  Synopsis:  Allocate aligned memory from the buffer
//

__allocator __bcount(size) uintptr_t
CBufferDispenser::AllocateFromBuffer(
    __in_range(alignment,SIZE_T_MAX) size_t size,
    __in_range(1,(SIZE_T_MAX+1)/2)   size_t alignment,
    PERFMETERTAG mt
    )
{
    uintptr_t ptrRet;

    // Size should already include space for a header and any alignment
    // corrections.
    Assert(IsAlignedTo(size, kMinBufferAllocationAlignment));

    Assert(this);

    Assert(IsAlignedTo(m_ptrNextAvailable, kMinBufferAllocationAlignment));

    // Compute aligned return address leaving space for the header
    ptrRet = IncrAlignTo(m_ptrNextAvailable + kExtraSpacePreAllocation,
                         alignment,
                         kMinBufferAllocationAlignment);

    //
    // Store header data just before return pointer
    //

    BufferAllocationHeader *pHeader =
        reinterpret_cast<BufferAllocationHeader *>
            (ptrRet - kExtraSpacePreAllocation);
    Assert(IsPtrAligned(pHeader));

    #if defined(PERFMETER)
        pHeader->mt = mt;
        pHeader->cbAllocated = size-kOverheadPerBufferAllocation;
        MtAdd(mt, +1, static_cast<LONG>(pHeader->cbAllocated));
    #endif
    pHeader->pDispenser = this;

    // ptrHeapAllocation and pDispenser share the same space in the header.
    // The AllocatedFromHeap flag shouldn't be set for this allocation.
    Assert((pHeader->ptrHeapAllocation & AllocatedFromHeap) == 0);

    //
    // Adjust dispenser members
    //

    m_ptrNextAvailable += size;
    m_cbSpaceLeft -= size;
    m_cAllocations++;

    Assert(IsAlignedTo(m_ptrNextAvailable, kMinBufferAllocationAlignment));

    return ptrRet;
}

//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::AllocateFromHeap
//
//  Synopsis:  Allocate aligned memory from process heap
//

__allocator __bcount(size) uintptr_t
CBufferDispenser::AllocateFromHeap(
    __in_range(alignment,SIZE_T_MAX) size_t size,
    __in_range(1,(SIZE_T_MAX+1)/2)   size_t alignment,
    PERFMETERTAG mt
    )
{
    uintptr_t ptrAlloc, ptrRet = NULL;

    // Size should already include space for a pointer and any alignment
    // corrections to get to a minimum of buffer alignment.
    Assert(IsAlignedTo(size, kMinBufferAllocationAlignment));

    // Forward allocation request to process heap
    ptrAlloc = WPFAllocType(uintptr_t,
        ProcessHeap,
        mt,
        size);

    if (ptrAlloc)
    {
        Assert((ptrAlloc & AllocatedFromHeap) == 0);

        // Compute aligned return address leaving space for header.
        ptrRet = IncrAlignTo(ptrAlloc + kExtraSpacePreAllocation,
                             alignment,
                             kMinBufferAllocationAlignment);

        // Store header data just before return pointer
        BufferAllocationHeader *pHeader =
            reinterpret_cast<BufferAllocationHeader *>
                (ptrRet - kExtraSpacePreAllocation);
        Assert(IsPtrAligned(pHeader));

        #if defined(PERFMETER)
            // WPFAlloc will handle meter additions itself, but we remove
            // subtract the buffer alllocation overhead to keep the meter
            // values consistent.
            pHeader->mt = mt;
            MtAdd(pHeader->mt, 0, -kOverheadPerBufferAllocation);
        #endif
        #if DBG
            pHeader->pDbgDispenser = this;
            if (this)
            {
                m_cDbgHeapAllocations++;
            }
        #elif defined(PERFMETER)
            // Just to give it a value
            pHeader->cbAllocated = size-kOverheadPerBufferAllocation;
        #endif
        // Store ptrAlloc and allocated from heap flag
        pHeader->ptrHeapAllocation = (ptrAlloc | AllocatedFromHeap);
    }

    return ptrRet;
}

//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::FreeInternal
//
//  Synopsis:  Free memory previously allocated by Allocate
//

void
CBufferDispenser::FreeInternal(
    void *pv
    )
{
    Assert(pv != NULL);

    // Get address of header relative to pointer to free
    BufferAllocationHeader *pHeader =
        reinterpret_cast<BufferAllocationHeader *>
            (reinterpret_cast<uintptr_t>(pv) - kExtraSpacePreAllocation);

    // Check header to see if the allocation comes from the heap or buffer
    if (pHeader->ptrHeapAllocation & AllocatedFromHeap)
    {
        FreeFromHeap(pHeader);
    }
    else
    {
        pHeader->pDispenser->FreeFromBuffer(pHeader);
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::FreeFromBuffer
//
//  Synopsis:  Free memory previously allocated by AllocateFromBuffer
//

void
CBufferDispenser::FreeFromBuffer(
    const BufferAllocationHeader *pHeader
    )
{
    Assert(pHeader != NULL);

    // Check that pHeader is in a valid buffer range
    Assert(reinterpret_cast<uintptr_t>(pHeader) >= m_ptrBuffer);
    Assert(reinterpret_cast<uintptr_t>(pHeader) < (m_ptrNextAvailable
                                                   - kOverheadPerBufferAllocation
                                                   - kMinBufferAllocationAlignment));

    MtAdd(pHeader->mt, -1, static_cast<LONG>(-1*pHeader->cbAllocated));

    Assert(m_cAllocations > 0);
    m_cAllocations--;

    if (m_cAllocations == 0)
    {
        //
        // Reset available space to full buffer now that all allocations
        // have been freed.
        //

        m_cbSpaceLeft += (m_ptrNextAvailable - m_ptrBuffer);
        m_ptrNextAvailable = m_ptrBuffer;
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CBufferDispenser::FreeFromHeap
//
//  Synopsis:  Free memory allocated from process heap by AllocateFromHeap
//

void
CBufferDispenser::FreeFromHeap(
    const BufferAllocationHeader *pHeader
    )
{
    uintptr_t ptr = pHeader->ptrHeapAllocation;

    Assert(ptr & AllocatedFromHeap);

    // Remove flag from pointer value.  Note that subtraction is used so that
    // in invalid address coming in is still invalid when sent to the heap.
    ptr = ptr - AllocatedFromHeap;

    Assert(ptr);

#if DBG
    //
    // In debug builds a pointer to the dispenser is stored at the beginning
    // of the allocated block.
    //

    CBufferDispenser *pDispenser = pHeader->pDbgDispenser;

    // Make sure the allocation was made through a real dispenser before
    // accessing members.
    if (pDispenser)
    {
        // Check that allocation is not within buffer range
        Assert((ptr < pDispenser->m_ptrBuffer - kOverheadPerBufferAllocation) ||
               (ptr > pDispenser->m_ptrNextAvailable+pDispenser->m_cbSpaceLeft)
              );

        Assert(pDispenser->m_cDbgHeapAllocations > 0);

        pDispenser->m_cDbgHeapAllocations--;
    }
#endif

    // Undo the earlier overhead subtraction during allocation
    MtAdd(pHeader->mt, 0, +kOverheadPerBufferAllocation);

    // Free the memory
    WPFFree(ProcessHeap, reinterpret_cast<void *>(ptr));

    return;
}



