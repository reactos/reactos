// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:     CBufferDispenser
//
//  Synopsis:  Manages allocations from a buffer to attempt fast allocations
//
//  Notes:     If an requested allocation is too large to fit in the remaining
//             space the process heap will be used to fulfill the request.
//
//-----------------------------------------------------------------------------

class CBufferDispenser
{
private:

    //+------------------------------------------------------------------------
    //
    //  Structure:  BufferAllocationHeader
    //
    //  Synopsis:   Data block located just before actual allocations which
    //              contains information about how the blocked was allocated
    //              and how it should be freed.
    //
    //-------------------------------------------------------------------------

    struct BufferAllocationHeader {
#if defined(PERFMETER)
        PERFMETERTAG mt;        // Meter tag
        union {
            size_t cbAllocated;            // Size of buffer allocation
                                           // (including alignment adjustment)
#endif
#if DBG
            CBufferDispenser *pDbgDispenser;  // Allocating dispenser (for heap
                                              // allocations)
#endif
#if defined(PERFMETER)
        };
#endif
        union {
            // Allocations are made from either the buffer or from the heap.
            // The least significant bit of this field is used to distinquish
            // between allocations from the buffer and a heap.  It is set for
            // heap allocations (AllocatedFromHeap).
            CBufferDispenser *pDispenser;   // Allocating dispenser
            uintptr_t ptrHeapAllocation;    // Base of heap memory allocation
        };
    };

    // Allocation Flags
    enum {
        AllocatedFromHeap = 1
    };

    enum {
        kExtraSpacePreAllocation = sizeof(BufferAllocationHeader),
        kExtraSpacePostAllocation = 0
    };

public:

    enum {
        kOverheadPerBufferAllocation =
            kExtraSpacePreAllocation + kExtraSpacePostAllocation,
        kMinBufferAllocationAlignment = __alignof(BufferAllocationHeader)
    };

    //
    // Assertions for constants
    //
    C_ASSERT_IS_ALIGNED_TO(kExtraSpacePreAllocation, kMinBufferAllocationAlignment);
    C_ASSERT_IS_ALIGNED_TO(kExtraSpacePostAllocation, kMinBufferAllocationAlignment);
    C_ASSERT_IS_ALIGNED_TO((size_t)kOverheadPerBufferAllocation, kMinBufferAllocationAlignment);
    C_ASSERT(MEMORY_ALLOCATION_ALIGNMENT >= kMinBufferAllocationAlignment);
    C_ASSERT((MEMORY_ALLOCATION_ALIGNMENT & AllocatedFromHeap) == 0);

    CBufferDispenser(
        __inout_bcount(size) void *pvBuffer,
        __in_range(1,SIZE_T_MAX) size_t size
        );

    ~CBufferDispenser()
    {
        Assert(m_cAllocations == 0);
#if DBG
        Assert(m_cDbgHeapAllocations == 0);
#endif
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    Allocate
    //
    //  Synopsis:  Allocate requested memory from stack buffer is possible;
    //             otherwise, request the memory from the heap
    //
    //  Notes:     It is acceptable to call Allocate using a NULL
    //             CBufferDispenser pointer.  This allows classes to overload
    //             placement new to attempt a buffer allocation, but still use
    //             non-placement new.  In such a case Allocate will just
    //             request the allocation from the process heap, but will add
    //             its regular memory block modifications so that delete can
    //             always call Free without having to know which new was used.
    //
    //             Calling with a NULL CDispenserBuffer * may also be used to
    //             make allocations with alignments greater than the defualt
    //             MEMORY_ALLOCATION_ALIGNMENT.
    //
    //-------------------------------------------------------------------------

    __allocator __bcount(size) void *Allocate(
        __in_range(1,SIZE_T_MAX)       size_t size,
        __in_range(1,(SIZE_T_MAX+1)/2) size_t alignment,
        PERFMETERTAG mt
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    Free
    //
    //  Synopsis:  Free memory previously allocated by Allocate
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE static void Free(
        void *pv
        )
    {
        if (pv != NULL) FreeInternal(pv);
    }

private:

    __allocator __bcount(size) uintptr_t AllocateFromBuffer(
        __in_range(alignment,SIZE_T_MAX) size_t size,
        __in_range(1,(SIZE_T_MAX+1)/2)   size_t alignment,
        PERFMETERTAG mt
        );

    __allocator __bcount(size) uintptr_t AllocateFromHeap(
        __in_range(alignment,SIZE_T_MAX) size_t size,
        __in_range(1,(SIZE_T_MAX+1)/2)   size_t alignment,
        PERFMETERTAG mt
        );

    static void FreeInternal(
        void *pv
        );

    void FreeFromBuffer(
        const BufferAllocationHeader *pHeader
        );

    static void FreeFromHeap(
        const BufferAllocationHeader *pHeader
        );

private:

    uintptr_t m_ptrBuffer;           // Beginning of buffer to manage
    uintptr_t m_ptrNextAvailable;    // Next location available in buffer
    size_t m_cbSpaceLeft;            // Space from NextAvailable to end of
                                     // buffer
    UINT m_cAllocations;             // Number of allocations currently in
                                     // buffer
#if DBG
    UINT m_cDbgHeapAllocations;      // Number of allocations currently in heap
#endif

};

//
// These NEW and DELETE defines should be used to declare and define the new
// and delete operators for types that may be allocated from a
// CBufferDispenser.
//
// These new operators allow new to be called without passing a
// CBufferDispenser, which will result in an allocation request to the process
// heap.
//

#define DECLARE_BUFFERDISPENSER_NEW(type, mt)                           \
    MIL_FORCEINLINE void * operator new(                                \
        size_t cb,                                                      \
        CBufferDispenser *pBufferDispenser = NULL                       \
        )                                                               \
    {                                                                   \
        return pBufferDispenser->Allocate(cb, __alignof(type), mt);     \
    }                                                                   \
                                                                        \
    MIL_FORCEINLINE void * operator new[](                              \
        size_t cb,                                                      \
        CBufferDispenser *pBufferDispenser = NULL                       \
        )                                                               \
    {                                                                   \
        return pBufferDispenser->Allocate(cb, __alignof(type), mt);     \
    }

#define DECLARE_BUFFERDISPENSER_DELETE                  \
    MIL_FORCEINLINE void operator delete(void * pv)     \
    {                                                   \
        CBufferDispenser::Free(pv);                     \
    }                                                   \
    MIL_FORCEINLINE void operator delete[](void * pv)   \
    {                                                   \
        CBufferDispenser::Free(pv);                     \
    }                                                   \
    MIL_FORCEINLINE void operator delete(               \
        void* pv,                                       \
        CBufferDispenser*                               \
        )                                               \
    {                                                   \
        CBufferDispenser::Free(pv);                     \
    }                                                   \
    MIL_FORCEINLINE void operator delete[](             \
        void* pv,                                       \
        CBufferDispenser*                               \
        )                                               \
   {                                                    \
        CBufferDispenser::Free(pv);                     \
   }


//+----------------------------------------------------------------------------
//
//  Class:     CDispensableBuffer<size_t>
//
//  Synopsis:  Acts as a CBufferDispenser with a built-in buffer of the given
//             size
//
//  Notes:     It must be on the stack (as a local) or a member.  Once an
//             instance of this class goes out of scope, all allocations made
//             from the buffer will be lost.
//
//-----------------------------------------------------------------------------

template <size_t BufferSize, unsigned int ExpectedAllocationCount>
class CDispensableBuffer : public CBufferDispenser
{
public:

    CDispensableBuffer()
    : CBufferDispenser(m_rgBuffer, sizeof(m_rgBuffer))
    {
    }

private:

    // Generate a compile time error (via private protection) when trying to
    // dynamically allocate one of these.  They should always be on the stack
    // or be a member.
    MIL_FORCEINLINE void *operator new(size_t size) { return NULL; }

    BYTE m_rgBuffer[BufferSize +
                    ExpectedAllocationCount*kOverheadPerBufferAllocation +
                    kMinBufferAllocationAlignment];

};


//
// Determines the maximum space needed to allocate a type and always be able
// to align it within that space.
//

#define MAX_SPACE_FOR_TYPE(type)    (sizeof(type)+__alignof(type))


