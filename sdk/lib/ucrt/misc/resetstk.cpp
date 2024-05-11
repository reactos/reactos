/***
*resetstk.c - Recover from Stack overflow.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines the _resetstkoflw() function.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <malloc.h>

#define MIN_STACK_REQ_WINNT 2
 


/***
* void _resetstkoflw(void) - Recovers from Stack Overflow
*
* Purpose:
*       Sets the guard page to its position before the stack overflow.
*
* Exit:
*       Returns nonzero on success, zero on failure
*
*******************************************************************************/

extern "C" int __cdecl _resetstkoflw()
{
    LPBYTE pStack, pStackBase, pMaxGuard, pMinGuard;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO si;
    DWORD PageSize;
    DWORD RegionSize;
    DWORD flOldProtect;
    ULONG StackSizeInBytes;

    // Use _alloca() to get the current stack pointer
#pragma warning(push)
#pragma warning(disable:6255)
    // prefast(6255): This alloca is safe and we do not want a __try here
    pStack = (LPBYTE)_alloca(1);
#pragma warning(pop)

    // Find the base of the stack.

    if (VirtualQuery(pStack, &mbi, sizeof mbi) == 0) {
        return 0;
    }

    pStackBase = (LPBYTE)mbi.AllocationBase;

    GetSystemInfo(&si);
    PageSize = si.dwPageSize;
    RegionSize = 0;
    StackSizeInBytes = 0;               // Indicate just querying
    if (__acrt_SetThreadStackGuarantee(&StackSizeInBytes) && StackSizeInBytes > 0) {
        RegionSize = StackSizeInBytes;
    }

#pragma warning(push)
#pragma warning(disable:6255)
    // Silence prefast about overflow/underflow
    RegionSize = (RegionSize + PageSize - 1) & ~(PageSize - 1);
#pragma warning(pop)

    //
    // If there is a stack guarantee (RegionSize nonzero), then increase
    // our guard page size by 1 so that even a subsequent fault that occurs
    // midway (instead of at the beginning) through the first guard page
    // will have the extra page to preserve the guarantee.
    //

    if (RegionSize != 0) {
        RegionSize += PageSize;
    }

    if (RegionSize < MIN_STACK_REQ_WINNT * PageSize) {
        RegionSize = MIN_STACK_REQ_WINNT * PageSize;
    }

    //
    // Find the page(s) just below where the stack pointer currently points.
    // This is the highest potential guard page.
    //

    pMaxGuard = (LPBYTE) (((DWORD_PTR)pStack & ~(DWORD_PTR)(PageSize - 1))
                       - RegionSize);

    //
    // If the potential guard page is too close to the start of the stack
    // region, abandon the reset effort for lack of space.  Win9x has a
    // larger reserved stack requirement.
    //

    pMinGuard = pStackBase + PageSize;

    if (pMaxGuard < pMinGuard) {
        return 0;
    }

    // Set the new guard page just below the current stack page.

    if (VirtualAlloc(pMaxGuard, RegionSize, MEM_COMMIT, PAGE_READWRITE) == nullptr ||
        VirtualProtect(pMaxGuard, RegionSize, PAGE_READWRITE | PAGE_GUARD, &flOldProtect) == 0) {
        return 0;
    }

    return 1;
}
