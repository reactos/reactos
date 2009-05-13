/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/global.c
 * PURPOSE:         Global Memory APIs (sits on top of Heap*)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

extern SYSTEM_BASIC_INFORMATION BaseCachedSysInfo;
RTL_HANDLE_TABLE BaseHeapHandleTable;

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
HGLOBAL
NTAPI
GlobalAlloc(UINT uFlags,
            DWORD dwBytes)
{
    ULONG Flags = 0;
    PVOID Ptr = NULL;
    HANDLE hMemory;
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    BASE_TRACE_ALLOC(dwBytes, uFlags);
    ASSERT(hProcessHeap);

    /* Make sure the flags are valid */
    if (uFlags & ~GMEM_VALID_FLAGS)
    {
        /* They aren't, fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Convert ZEROINIT */
    if (uFlags & GMEM_ZEROINIT) Flags |= HEAP_ZERO_MEMORY;

    /* Check if we're not movable, which means pointer-based heap */
    if (!(uFlags & GMEM_MOVEABLE))
    {
        /* Check if this is DDESHARE (deprecated) */
        if (uFlags & GMEM_DDESHARE) Flags |= BASE_HEAP_ENTRY_FLAG_DDESHARE;

        /* Allocate heap for it */
        Ptr = RtlAllocateHeap(hProcessHeap, Flags, dwBytes);
        BASE_TRACE_ALLOC2(Ptr);
        return Ptr;
    }

    /* This is heap based, so lock it in first */
    RtlLockHeap(hProcessHeap);

    /*
     * Disable locking, enable custom flags, and write the
     * movable flag (deprecated)
     */
    Flags |= HEAP_NO_SERIALIZE |
             HEAP_SETTABLE_USER_VALUE |
             BASE_HEAP_FLAG_MOVABLE;

    /* Allocate the handle */
    HandleEntry = BaseHeapAllocEntry();
    if (!HandleEntry)
    {
        /* Fail */
        hMemory = NULL;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        BASE_TRACE_FAILURE();
        goto Quickie;
    }

    /* Get the object and make sure we have size */
    hMemory = &HandleEntry->Object;
    if (dwBytes)
    {
        /* Allocate the actual memory for it */
        Ptr = RtlAllocateHeap(hProcessHeap, Flags, dwBytes);
        BASE_TRACE_PTR(HandleEntry, Ptr);
        if (!Ptr)
        {
            /* We failed, manually set the allocate flag and free the handle */
            HandleEntry->Flags = RTL_HANDLE_VALID;
            BaseHeapFreeEntry(HandleEntry);

            /* For the cleanup case */
            HandleEntry = NULL;
        }
        else
        {
            /* All worked well, save our heap entry */
            RtlSetUserValueHeap(hProcessHeap, HEAP_NO_SERIALIZE, Ptr, hMemory);
            RtlSetUserFlagsHeap(hProcessHeap, HEAP_NO_SERIALIZE, Ptr, Flags);
        }
    }

Quickie:
    /* Cleanup! First unlock the heap */
    RtlUnlockHeap(hProcessHeap);

    /* Check if a handle was allocated */
    if (HandleEntry)
    {
        /* Set the pointer and allocated flag */
        HandleEntry->Object = Ptr;
        HandleEntry->Flags = RTL_HANDLE_VALID;
        if (!Ptr)
        {
            /* We don't have a valid pointer, but so reuse this handle */
            HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSE;
        }

        /* Check if the handle is discardable */
        if (uFlags & GMEM_DISCARDABLE)
        {
            /* Save it in the handle entry */
            HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSABLE;
        }

        /* Check if the handle is moveable */
        if (uFlags & GMEM_MOVEABLE)
        {
            /* Save it in the handle entry */
            HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_MOVABLE;
        }

        /* Check if the handle is DDE Shared */
        if (uFlags & GMEM_DDESHARE)
        {
            /* Save it in the handle entry */
            HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_DDESHARE;
        }

        /* Set the pointer */
        Ptr = hMemory;
    }

    /* Return the pointer */
    return Ptr;
}

/*
 * @implemented
 */
SIZE_T
NTAPI
GlobalCompact(DWORD dwMinFree)
{
    /* Call the RTL Heap Manager */
    return RtlCompactHeap(hProcessHeap, 0);
}

/*
 * @implemented
 */
VOID
NTAPI
GlobalFix(HGLOBAL hMem)
{
    /* Lock the memory if it the handle is valid */
    if (INVALID_HANDLE_VALUE != hMem) GlobalLock(hMem);
}

/*
 * @implemented
 */
UINT
NTAPI
GlobalFlags(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    HANDLE Handle = NULL;
    ULONG Flags = 0;
    UINT uFlags = GMEM_INVALID_HANDLE;

    /* Start by locking the heap */
    RtlLockHeap(hProcessHeap);

    /* Check if this is a simple RTL Heap Managed block */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Then we'll query RTL Heap */
        RtlGetUserInfoHeap(hProcessHeap, Flags, hMem, &Handle, &Flags);
        BASE_TRACE_PTR(Handle, hMem);

        /*
         * Check if RTL Heap didn't find a handle associated with us or
         * said that this heap isn't movable, which means something we're
         * really not a handle-based heap.
         */
        if (!(Handle) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
        {
            /* Then set the flags to 0 */
            uFlags = 0;
        }
        else
        {
            /* Otherwise we're handle-based, so get the internal handle */
            hMem = Handle;
        }
    }

    /* Check if the handle is actually an entry in our table */
    if ((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY)
    {
        /* Then get the entry */
        HandleEntry = BaseHeapGetEntry(hMem);
        BASE_TRACE_HANDLE(HandleEntry, hMem);

        /* Make sure it's a valid handle */
        if (BaseHeapValidateEntry(HandleEntry))
        {
            /* Get the lock count first */
            uFlags = HandleEntry->LockCount & GMEM_LOCKCOUNT;

            /* Now check if it's discarded */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSABLE)
            {
                /* Set the Win32 Flag */
                uFlags |= GMEM_DISCARDED;
            }

            /* Check if it's movable */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_MOVABLE)
            {
                /* Set the Win32 Flag */
                uFlags |= GMEM_MOVEABLE;
            }

            /* Check if it's DDE Shared */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_DDESHARE)
            {
                /* Set the Win32 Flag */
                uFlags |= GMEM_DDESHARE;
            }
        }
    }

    /* Check if by now, we still haven't gotten any useful flags */
    if (uFlags == GMEM_INVALID_HANDLE) SetLastError(ERROR_INVALID_HANDLE);

    /* All done! Unlock heap and return Win32 Flags */
    RtlUnlockHeap(hProcessHeap);
    return uFlags;
}

/*
 * @implemented
 */
HGLOBAL
NTAPI
GlobalFree(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    LPVOID Ptr;
    BASE_TRACE_DEALLOC(hMem);

    /* Check if this was a simple allocated heap entry */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Free it with the RTL Heap Manager */
        if (RtlFreeHeap(hProcessHeap, 0, hMem))
        {
            /* Return NULL since there's no handle */
            return NULL;
        }
        else
        {
            /* Otherwise fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_INVALID_HANDLE);
            return hMem;
        }
    }

    /* It's a handle probably, so lock the heap */
    RtlLockHeap(hProcessHeap);

    /* Make sure that this is an entry in our handle database */
    if ((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY)
    {
        /* Get the entry */
        HandleEntry = BaseHeapGetEntry(hMem);
        BASE_TRACE_HANDLE(HandleEntry, hMem);

        /* Make sure the handle is valid */
        if (!BaseHeapValidateEntry(HandleEntry))
        {
            /* It's not, fail */
            SetLastError(ERROR_INVALID_HANDLE);
            Ptr = NULL;
        }
        else
        {
            /* It's valid, so get the pointer */
            Ptr = HandleEntry->Object;

            /* Free this handle */
            BaseHeapFreeEntry(HandleEntry);

            /* If the pointer is 0, then we don't have a handle either */
            if (!Ptr) hMem = NULL;
        }
    }
    else
    {
        /* Otherwise, reuse the handle as a pointer */
        BASE_TRACE_FAILURE();
        Ptr = hMem;
    }

    /* Check if we got here with a valid heap pointer */
    if (Ptr)
    {
        /* Free it */
        RtlFreeHeap(hProcessHeap, HEAP_NO_SERIALIZE, Ptr);
        hMem = NULL;
    }

    /* We're done, so unlock the heap and return the handle */
    RtlUnlockHeap(hProcessHeap);
    return hMem;
}

/*
 * @implemented
 */
HGLOBAL
NTAPI
GlobalHandle(LPCVOID pMem)
{
    HANDLE Handle = NULL;
    ULONG Flags;

    /* Lock the heap */
    RtlLockHeap(hProcessHeap);

    /* Query RTL Heap */
    RtlGetUserInfoHeap(hProcessHeap,
                       HEAP_NO_SERIALIZE,
                       (PVOID)pMem,
                       &Handle,
                       &Flags);
    BASE_TRACE_PTR(Handle, pMem);

    /*
     * Check if RTL Heap didn't find a handle for us or said that
     * this heap isn't movable.
     */
    if (!(Handle) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
    {
        /* We're actually handle-based, so the pointer is a handle */
        Handle = (HANDLE)pMem;
    }

    /* All done, unlock the heap and return the handle */
    RtlUnlockHeap(hProcessHeap);
    return Handle;
}

/*
 * @implemented
 */
LPVOID
NTAPI
GlobalLock(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    LPVOID Ptr;

    /* Check if this was a simple allocated heap entry */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Then simply return the pointer */
        return hMem;
    }

    /* Otherwise, lock the heap */
    RtlLockHeap(hProcessHeap);

    /* Get the handle entry */
    HandleEntry = BaseHeapGetEntry(hMem);
    BASE_TRACE_HANDLE(HandleEntry, hMem);

    /* Make sure it's valid */
    if (!BaseHeapValidateEntry(HandleEntry))
    {
        /* It's not, fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_HANDLE);
        Ptr = NULL;
    }
    else
    {
        /* Otherwise, get the pointer */
        Ptr = HandleEntry->Object;
        if (Ptr)
        {
            /* Increase the lock count, unless we've went too far */
            if (HandleEntry->LockCount++ == GMEM_LOCKCOUNT)
            {
                /* In which case we simply unlock once */
                HandleEntry->LockCount--;
            }
        }
        else
        {
            /* The handle is still there but the memory was already freed */
            SetLastError(ERROR_DISCARDED);
        }
    }

    /* All done. Unlock the heap and return the pointer */
    RtlUnlockHeap(hProcessHeap);
    return Ptr;
}

HGLOBAL
NTAPI
GlobalReAlloc(HGLOBAL hMem,
              DWORD dwBytes,
              UINT uFlags)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    HANDLE Handle;
    LPVOID Ptr;
    ULONG Flags = 0;

    /* Convert ZEROINIT */
    if (uFlags & GMEM_ZEROINIT) Flags |= HEAP_ZERO_MEMORY;

    /* If this wasn't a movable heap, then we MUST re-alloc in place */
    if (!(uFlags & GMEM_MOVEABLE)) Flags |= HEAP_REALLOC_IN_PLACE_ONLY;

    /* Lock the heap and disable built-in locking in the RTL Heap funcitons */
    RtlLockHeap(hProcessHeap);
    Flags |= HEAP_NO_SERIALIZE;

    /* Check if this is a simple handle-based block */
    if (((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Get the entry */
        HandleEntry = BaseHeapGetEntry(hMem);
        BASE_TRACE_HANDLE(HandleEntry, hMem);

        /* Make sure the handle is valid */
        if (!BaseHeapValidateEntry(HandleEntry))
        {
            /* Fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_INVALID_HANDLE);
            hMem = NULL;
        }
        else if (uFlags & GMEM_MODIFY)
        {
            /* User is changing flags... check if the memory was discardable */
            if (uFlags & GMEM_DISCARDABLE)
            {
                /* Then set the flag */
                HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSABLE;
            }
            else
            {
                /* Otherwise, remove the flag */
                HandleEntry->Flags &= BASE_HEAP_ENTRY_FLAG_REUSABLE;
            }
        }
        else
        {
            /* Otherwise, get the object and check if we have no size */
            Ptr = HandleEntry->Object;
            if (!dwBytes)
            {
                /* Clear the handle and check for a pointer */
                hMem = NULL;
                if (Ptr)
                {
                    /* Make sure the handle isn't locked */
                    if ((uFlags & GMEM_MOVEABLE) && !(HandleEntry->LockCount))
                    {
                        /* Free the current heap */
                        RtlFreeHeap(hProcessHeap, Flags, Ptr);

                        /* Free the handle */
                        HandleEntry->Object = NULL;
                        HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSE;

                        /* Get the object pointer */
                        hMem = &HandleEntry->Object;
                    }
                }
                else
                {
                    /* Otherwise just return the object pointer */
                    hMem = &HandleEntry->Object;
                }
            }
            else
            {
                /* Otherwise, we're allocating, so set the new flags needed */
                Flags |= HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVABLE;
                if (!Ptr)
                {
                    /* We don't have a base, so allocate one */
                    Ptr = RtlAllocateHeap(hProcessHeap, Flags, dwBytes);
                    BASE_TRACE_ALLOC2(Ptr);
                    if (Ptr)
                    {
                        /* Allocation succeeded, so save our entry */
                        RtlSetUserValueHeap(hProcessHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            hMem);
                        RtlSetUserFlagsHeap(hProcessHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            Flags);
                    }
                }
                else
                {
                    /*
                     * If it's not movable or currently locked, we MUST allocate
                     * in-place!
                     */
                    if (!(uFlags & GMEM_MOVEABLE) && (HandleEntry->LockCount))
                    {
                        /* Set the flag */
                        Flags |= HEAP_REALLOC_IN_PLACE_ONLY;
                    }
                    else
                    {
                        /* Otherwise clear the flag if we set it previously */
                        Flags &= ~HEAP_REALLOC_IN_PLACE_ONLY;
                    }

                    /* And do the re-allocation */
                    Ptr = RtlReAllocateHeap(hProcessHeap, Flags, Ptr, dwBytes);

                    if (Ptr)
                    {
                        /* Allocation succeeded, so save our entry */
                        RtlSetUserValueHeap(hProcessHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            hMem);
                        RtlSetUserFlagsHeap(hProcessHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            Flags);
                    }

                }

                /* Make sure we have a pointer by now */
                if (Ptr)
                {
                    /* Write it in the handle entry and mark it in use */
                    HandleEntry->Object = Ptr;
                    HandleEntry->Flags &= ~BASE_HEAP_ENTRY_FLAG_REUSE;
                }
                else
                {
                    /* Otherwise we failed */
                    hMem = NULL;
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }
    }
    else if (uFlags & GMEM_MODIFY)
    {
        /* This is not a handle-based heap and the caller wants it to be one */
        if (uFlags & GMEM_MOVEABLE)
        {
            /* Get information on its current state */
            Handle = hMem;
            DPRINT1("h h %lx %lx\n", Handle, hMem);
            RtlGetUserInfoHeap(hProcessHeap,
                               HEAP_NO_SERIALIZE,
                               hMem,
                               &Handle,
                               &Flags);
            DPRINT1("h h %lx %lx\n", Handle, hMem);

            /*
             * Check if the handle matches the pointer or if the moveable flag
             * isn't there, which is what we expect since it currenly isn't.
             */
            if (Handle == hMem || !(Flags & BASE_HEAP_FLAG_MOVABLE))
            {
                /* Allocate a handle for it */
                HandleEntry = BaseHeapAllocEntry();

                /* Calculate the size of the current heap */
                dwBytes = RtlSizeHeap(hProcessHeap, HEAP_NO_SERIALIZE, hMem);

                /* Set the movable flag */
                Flags |= HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVABLE;

                /* Now allocate the actual heap for it */
                HandleEntry->Object = RtlAllocateHeap(hProcessHeap,
                                                      Flags,
                                                      dwBytes);
                BASE_TRACE_PTR(HandleEntry->Object, HandleEntry);
                if (!HandleEntry->Object)
                {
                    /*
                     * We failed, manually set the allocate flag and
                     * free the handle
                     */
                    HandleEntry->Flags = RTL_HANDLE_VALID;
                    BaseHeapFreeEntry(HandleEntry);

                    /* For the cleanup case */
                    BASE_TRACE_FAILURE();
                    HandleEntry = NULL;
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    /* Otherwise, copy the current heap and free the old one */
                    RtlMoveMemory(HandleEntry->Object, hMem, dwBytes);
                    RtlFreeHeap(hProcessHeap, HEAP_NO_SERIALIZE, hMem);

                    /* Select the heap pointer */
                    hMem = (HANDLE)&HandleEntry->Object;

                    /* Initialize the count and default flags */
                    HandleEntry->LockCount = 0;
                    HandleEntry->Flags = RTL_HANDLE_VALID |
                                         BASE_HEAP_ENTRY_FLAG_MOVABLE;

                    /* Check if it's also discardable */
                    if (uFlags & GMEM_DISCARDABLE)
                    {
                        /* Set the internal flag */
                        HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSABLE;
                    }

                    /* Check if it's also DDE Shared */
                    if (uFlags & GMEM_DDESHARE)
                    {
                        /* Set the internal flag */
                        HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_DDESHARE;
                    }

                    /* Allocation succeeded, so save our entry */
                    RtlSetUserValueHeap(hProcessHeap,
                                        HEAP_NO_SERIALIZE,
                                        HandleEntry->Object,
                                        hMem);
                    RtlSetUserFlagsHeap(hProcessHeap,
                                        HEAP_NO_SERIALIZE,
                                        HandleEntry->Object,
                                        Flags);
                }
            }
        }
    }
    else
    {
        /* Otherwise, this is a simple RTL Managed Heap, so just call it */
        hMem = RtlReAllocateHeap(hProcessHeap,
                                 Flags | HEAP_NO_SERIALIZE,
                                 hMem,
                                 dwBytes);
        if (!hMem)
        {
            /* Fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    /* All done, unlock the heap and return the pointer */
    RtlUnlockHeap(hProcessHeap);
    return hMem;
}

/*
 * @implemented
 */
DWORD
NTAPI
GlobalSize(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    PVOID Handle = NULL;
    ULONG Flags = 0;
    SIZE_T dwSize = MAXULONG_PTR;

    /* Lock the heap */
    RtlLockHeap(hProcessHeap);

    /* Check if this is a simple RTL Heap Managed block */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Then we'll query RTL Heap */
        RtlGetUserInfoHeap(hProcessHeap, Flags, hMem, &Handle, &Flags);
        BASE_TRACE_PTR(Handle, hMem);

        /*
         * Check if RTL Heap didn't give us a handle or said that this heap
         * isn't movable.
         */
        if (!(Handle) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
        {
            /* This implies we're not a handle heap, so use the generic call */
            dwSize = RtlSizeHeap(hProcessHeap, HEAP_NO_SERIALIZE, hMem);
        }
        else
        {
            /* Otherwise we're a handle heap, so get the internal handle */
            hMem = Handle;
        }
    }

    /* Make sure that this is an entry in our handle database */
    if ((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY)
    {
        /* Get the entry */
        HandleEntry = BaseHeapGetEntry(hMem);
        BASE_TRACE_HANDLE(HandleEntry, hMem);

        /* Make sure the handle is valid */
        if (!BaseHeapValidateEntry(HandleEntry))
        {
            /* Fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_INVALID_HANDLE);
        }
        else if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSE)
        {
            /* We've reused this block, but we've saved the size for you */
            dwSize = HandleEntry->OldSize;
        }
        else
        {
            /* Otherwise, query RTL about it */
            dwSize = RtlSizeHeap(hProcessHeap,
                                 HEAP_NO_SERIALIZE,
                                 HandleEntry->Object);
        }
    }

    /* Check if by now, we still haven't gotten any useful size */
    if (dwSize == MAXULONG_PTR)
    {
        /* Fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_HANDLE);
        dwSize = 0;
    }

    /* All done! Unlock heap and return the size */
    RtlUnlockHeap(hProcessHeap);
    return dwSize;
}

/*
 * @implemented
 */
VOID
NTAPI
GlobalUnfix(HGLOBAL hMem)
{
    /* If the handle is valid, unlock it */
    if (hMem != INVALID_HANDLE_VALUE) GlobalUnlock(hMem);
}

/*
 * @implemented
 */
BOOL
NTAPI
GlobalUnlock(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    BOOL RetVal = TRUE;

    /* Check if this was a simple allocated heap entry */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY)) return RetVal;

    /* Otherwise, lock the heap */
    RtlLockHeap(hProcessHeap);

    /* Get the handle entry */
    HandleEntry = BaseHeapGetEntry(hMem);
    BASE_TRACE_HANDLE(HandleEntry, hMem);

    /* Make sure it's valid */
    if (!BaseHeapValidateEntry(HandleEntry))
    {
        /* It's not, fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else
    {
        /* Otherwise, decrement lock count, unless we're already at 0*/
        if (!HandleEntry->LockCount--)
        {
            /* In which case we simply lock it back and fail */
            HandleEntry->LockCount++;
            SetLastError(ERROR_NOT_LOCKED);
            RetVal = FALSE;
        }
        else if (!HandleEntry->LockCount)
        {
            /* Nothing to unlock */
            SetLastError(NO_ERROR);
            RetVal = FALSE;
        }
    }

    /* All done. Unlock the heap and return the pointer */
    RtlUnlockHeap(hProcessHeap);
    return RetVal;
}

/*
 * @implemented
 */
BOOL
NTAPI
GlobalUnWire(HGLOBAL hMem)
{
    /* This is simply an unlock */
    return GlobalUnlock(hMem);
}

/*
 * @implemented
 */
LPVOID
NTAPI
GlobalWire(HGLOBAL hMem)
{
    /* This is just a lock */
    return GlobalLock(hMem);
}

/*
 * @implemented
 */
BOOL
NTAPI
GlobalMemoryStatusEx(LPMEMORYSTATUSEX lpBuffer)
{
    SYSTEM_PERFORMANCE_INFORMATION PerformanceInfo;
    VM_COUNTERS VmCounters;
    QUOTA_LIMITS QuotaLimits;
    ULONGLONG PageFile, PhysicalMemory;

    /* Query performance information */
    NtQuerySystemInformation(SystemPerformanceInformation,
                             &PerformanceInfo,
                             sizeof(PerformanceInfo),
                             NULL);

    /* Calculate memory load */
    lpBuffer->dwMemoryLoad = ((DWORD)(BaseCachedSysInfo.NumberOfPhysicalPages -
                                      PerformanceInfo.AvailablePages) * 100) /
                                      BaseCachedSysInfo.NumberOfPhysicalPages;

    /* Save physical memory */
    PhysicalMemory = BaseCachedSysInfo.NumberOfPhysicalPages *
                     BaseCachedSysInfo.PageSize;
    lpBuffer->ullTotalPhys = PhysicalMemory;

    /* Now save available physical memory */
    PhysicalMemory = PerformanceInfo.AvailablePages *
                     BaseCachedSysInfo.PageSize;
    lpBuffer->ullAvailPhys = PhysicalMemory;

    /* Query VM and Quota Limits */
    NtQueryInformationProcess(NtCurrentProcess(),
                              ProcessQuotaLimits,
                              &QuotaLimits,
                              sizeof(QUOTA_LIMITS),
                              NULL);
    NtQueryInformationProcess(NtCurrentProcess(),
                              ProcessVmCounters,
                              &VmCounters,
                              sizeof(VM_COUNTERS),
                              NULL);

    /* Save the commit limit */
    lpBuffer->ullTotalPageFile = min(QuotaLimits.PagefileLimit,
                                     PerformanceInfo.CommitLimit);

    /* Calculate how many pages are left */
    PageFile = PerformanceInfo.CommitLimit - PerformanceInfo.CommittedPages;

    /* Save the total */
    lpBuffer->ullAvailPageFile = min(PageFile,
                                     QuotaLimits.PagefileLimit -
                                     VmCounters.PagefileUsage);
    lpBuffer->ullAvailPageFile *= BaseCachedSysInfo.PageSize;

    /* Now calculate the total virtual space */
    lpBuffer->ullTotalVirtual = (BaseCachedSysInfo.MaximumUserModeAddress -
                                 BaseCachedSysInfo.MinimumUserModeAddress) + 1;

    /* And finally the avilable virtual space */
    lpBuffer->ullAvailVirtual = lpBuffer->ullTotalVirtual -
                                VmCounters.VirtualSize;
    lpBuffer->ullAvailExtendedVirtual = 0;
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer)
{
    MEMORYSTATUSEX lpBufferEx;

    /* Call the extended function */
    lpBufferEx.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&lpBufferEx))
    {
        /* Reset the right size and fill out the information */
        lpBuffer->dwLength = sizeof(MEMORYSTATUS);
        lpBuffer->dwMemoryLoad = lpBufferEx.dwMemoryLoad;
        lpBuffer->dwTotalPhys = (SIZE_T)lpBufferEx.ullTotalPhys;
        lpBuffer->dwAvailPhys = (SIZE_T)lpBufferEx.ullAvailPhys;
        lpBuffer->dwTotalPageFile = (SIZE_T)lpBufferEx.ullTotalPageFile;
        lpBuffer->dwAvailPageFile = (SIZE_T)lpBufferEx.ullAvailPageFile;
        lpBuffer->dwTotalVirtual = (SIZE_T)lpBufferEx.ullTotalVirtual;
        lpBuffer->dwAvailVirtual = (SIZE_T)lpBufferEx.ullAvailVirtual;
    }
}

/* EOF */
