// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:  Memory allocation routines.
//

#include "precomp.hpp"

MtDefine(MILRawMemory, Mem, "MIL Raw memory");
MtDefine(CGenericTableMapData, Mem, "CGenericTableMapData");

/**************************************************************************\
*
* Function Description:
*
*   Allocates a block of memory.
*
\**************************************************************************/

void *GpMalloc(PERFMETERTAG mt, size_t size)
{
    return WPF::Alloc(ProcessHeap, mt, size);
}

/**************************************************************************\
*
* Function Description:
*
*   Frees a block of memory.
*
* Arguments:
*
*   [IN] memblock - block to free
*
* Notes:
*
*   If memblock is NULL, does nothing.
*
*
\**************************************************************************/

void GpFree(void *memblock)
{
    WPFFree(ProcessHeap, memblock);
}

MtDefine(WMG_HEAP, Mem, "WMG Heap Memory");

/*++

Routine Description:

    Free a block of memory from the process heap.

Arguments:

    pv - memory to free.

Return Value:

    none

--*/

VOID FreeHeap(void *pv)
{
    WPFFree(ProcessHeap, pv);
}

/*++

Routine Description:

    Allocate a block of memory.

Arguments:

    cbSize    - size of the memory block.

Note:

    If you find yourself writing AllocHeap(a * b), call AllocHeapArray
    to properly check for multiplication overflow.

Return Value:

    PVOID - pointer to allocated memory. NULL if it failed.

--*/

PVOID AllocHeap(
    IN SIZE_T cbSize
    )
{
    return WPFAlloc(ProcessHeap, Mt(WMG_HEAP), cbSize);
}

/*++

Routine Description:

    Allocate a block of zero-initialized memory.

Arguments:

    cbSize    - size of the memory block.

Return Value:

    PVOID - pointer to allocated memory. NULL if it failed.

--*/

PVOID AllocHeapClear(
    IN SIZE_T cbSize
    )
{
    return WPFAllocClear(ProcessHeap, Mt(WMG_HEAP), cbSize);
}

/*++

Routine Description:

    ReAllocate a block of memory. Handles the input pointer == NULL case and
    allocates.

NOTE:

   As with all reallocations, use a temporary variable. The following pattern
   is always wrong because it will leak on allocation failure:
   p = ReallocHeap(p, size);

Arguments:

    cbSize    - size of the memory block.

Return Value:

    PVOID - pointer to allocated memory. NULL if it failed.

--*/

PVOID ReallocHeap(
    IN PVOID BaseAddress,
    IN SIZE_T cbSize
    )
{
    HRESULT hr = WPFRealloc(
        ProcessHeap,
        Mt(WMG_HEAP),
        &BaseAddress,
        cbSize
        );

    return (hr == S_OK) ? BaseAddress : NULL;
}

//+------------------------------------------------------------------------
//
//  Function:  
//      EnsureBufferSize
//
//  Synopsis:  
//      Ensures that a buffer is as large as the requested size by
//      reallocating the buffer when it is too small.
//
//-------------------------------------------------------------------------
HRESULT
EnsureBufferSize(
    PERFMETERTAG mt,
        // Meter tag to use during allocations
    UINT cbRequestedSize,
        // The needed buffer size
    __inout_ecount(1) UINT *pcbCurrentBuffer,
        // The size of the buffer
    __inout_ecount(1) void **ppBuffer
        // The buffer whose size needs to be ensured
    )
{
    HRESULT hr = S_OK;

    VOID *pNewBuffer = NULL;

    // If the requested size is greater than the current size, we need
    // to reallocate the buffer.
    if (cbRequestedSize > *pcbCurrentBuffer)
    {
        // Allocate a larger buffer of the requested size
        // Avoid allocation annotations here - caller should be annotated
        #undef HrAlloc
        IFC(WPF::HrAlloc(
            mt,
            cbRequestedSize,
            &pNewBuffer
            ));
        #define HrAlloc _Need_Valid_HrAlloc_Define_Or_Undef_

        //
        // If successful, release the old buffer & set the new buffer
        //
        
        WPFFree(ProcessHeap, *ppBuffer);

        *pcbCurrentBuffer = cbRequestedSize;        
        *ppBuffer = pNewBuffer;
        pNewBuffer = NULL; // Assign NULL to prevent releasing during Cleanup         
    }
#if DBG
    else
    {
        // Ensure that callers don't rely on the contents of ppBuffer to remain the same, in DBG builds
        ZeroMemory(*ppBuffer, *pcbCurrentBuffer);
    }
#endif

Cleanup:

    WPFFree(ProcessHeap, pNewBuffer);
    
    RRETURN(hr);
}


/*++

Routine Description:

    MIDL allocator/deallocator

--*/

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(len)
void __RPC_FAR * __RPC_API MIDL_user_allocate(_In_ size_t len)
{
    return AllocHeap(len);
}

void __RPC_API MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void __RPC_FAR * ptr)
{
    FreeHeap(ptr);
}




