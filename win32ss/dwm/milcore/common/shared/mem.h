// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
*

*
* Module Name:
*
*   Memory management functions
*
* Abstract:
*
*   Wrapper functions for memory management.
*   This file is C-includable.
*
\**************************************************************************/

#pragma once

#define GpMemset memset
#define GpMemcpy memcpy
#define GpMemmove memmove
#define GpMemcmp memcmp

// macro to simply memory clearing
#define ZEROMEM(a) RtlZeroMemory(&(a), sizeof(a)) 

MtExtern(MILRawMemory);

//--------------------------------------------------------------------------
// Building for native DLL
//--------------------------------------------------------------------------

void *GpMalloc(PERFMETERTAG mt, size_t size);
void GpFree(void *memblock);

#define IS_POWER_OF_2(X)    (((X)&((X)-1)) == 0)

#define __sal_max(a,b)  (a>b?a:b)
#define __sal_min(a,b)  (a<b?a:b)

//+----------------------------------------------------------------------------
//
//  Function:  IsAlignedTo/IsPtrAligned
//
//  Synopsis:  Check given value to see if it aligned according to given
//             alignment; greater alignment is okay
//
//  Notes:     IsPtrAligned will use the pointer type's alignement in the
//             absence of an explicit alignment argument.
//

#define IS_ALIGNED_TO(value, alignment) \
    (((value & (alignment-1)) & (alignment-1)) == 0)

#define C_ASSERT_IS_ALIGNED_TO(value, alignment) \
    C_ASSERT(alignment > 0);                     \
    C_ASSERT(IS_POWER_OF_2(alignment));          \
    C_ASSERT(IS_ALIGNED_TO(value, alignment))

template <class T>
MIL_FORCEINLINE
bool IsAlignedTo(
    T value,
    __in_range(1,(SIZE_T_MAX+1)/2) size_t alignment
    )
{
    // We might be able to get away with asserting alignment is greater than 1,
    // but that would mean no one could call this for an array of BYTEs.
    Assert(alignment > 0);
    Assert(IS_POWER_OF_2(alignment));
    return IS_ALIGNED_TO(value, alignment);
}

template <class T>
MIL_FORCEINLINE
bool IsPtrAligned(
    T *value,
    __in_range(2,(SIZE_T_MAX+1)/2) size_t alignment = __alignof(T)
    )
{
    // If IsPtrAligned is called with a void *, but no alignment the default
    // alignment will be 1, but that isn't useful; so, assert that the
    // alignment given is greater than 1.
    Assert(alignment > 1);
    return IsAlignedTo(reinterpret_cast<uintptr_t>(value), alignment);
}

//+----------------------------------------------------------------------------
//
//  Function:  IncrAlignTo
//
//  Synopsis:  Increase the given value from its expect alignment to the
//             greater given one; if an adjustment is needed, value is
//             numerically increased
//

template <class T>
MIL_FORCEINLINE
__range(__sal_max(value,toAlignment),value+toAlignment-1) T
IncrAlignTo(
    T value,
    __in_range(fromDbgAlignment,(SIZE_T_MAX+1)/2) size_t toAlignment,
    __in_range(1,(SIZE_T_MAX+1)/2)                size_t fromDbgAlignment = 1
    )
{
    Assert(IS_POWER_OF_2(toAlignment));
    Assert(IS_POWER_OF_2(fromDbgAlignment));
    Assert(IsAlignedTo(value, fromDbgAlignment));
    Assert(toAlignment >= fromDbgAlignment);
    return ((value + toAlignment-1) & ~(toAlignment-1));
}

VOID FreeHeap(
    VOID *pv
    );

PVOID ReallocHeap(
    IN PVOID BaseAddress,
    IN SIZE_T Size
    );

PVOID AllocHeap(
    IN SIZE_T Size
    );

PVOID AllocHeapClear(
    IN SIZE_T Size
    );

HRESULT
EnsureBufferSize(
    PERFMETERTAG mt,
    UINT uRequestedBufferSize,
    __inout_ecount(1) UINT *uCurrentBufferSize,    
    __inout_ecount(1) void **ppBuffer
    );




