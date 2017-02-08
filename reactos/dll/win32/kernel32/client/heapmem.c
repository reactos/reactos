/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/heapmem.c
 * PURPOSE:         Heap Memory APIs (wrappers for RtlHeap*)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

RTL_HANDLE_TABLE BaseHeapHandleTable;
HANDLE BaseHeap;
ULONG_PTR SystemRangeStart;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
BaseDllInitializeMemoryManager(VOID)
{
    BaseHeap = RtlGetProcessHeap();
    RtlInitializeHandleTable(0xFFFF,
                             sizeof(BASE_HEAP_HANDLE_ENTRY),
                             &BaseHeapHandleTable);
    NtQuerySystemInformation(SystemRangeStartInformation,
                             &SystemRangeStart,
                             sizeof(SystemRangeStart),
                             NULL);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
HeapCreate(DWORD flOptions,
           SIZE_T dwInitialSize,
           SIZE_T dwMaximumSize)
{
    HANDLE hRet;
    ULONG Flags;

    /* Remove non-Win32 flags and tag this allocation */
    Flags = (flOptions & (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE)) |
            HEAP_CLASS_1;

    /* Check if heap is growable and ensure max size is correct */
    if (dwMaximumSize == 0)
        Flags |= HEAP_GROWABLE;
    else if (dwMaximumSize < BaseStaticServerData->SysInfo.PageSize &&
            dwInitialSize > dwMaximumSize)
    {
        /* Max size is non-zero but less than page size which can't be correct.
           Fix it up by bumping it to the initial size whatever it is. */
        dwMaximumSize = dwInitialSize;
    }

    /* Call RTL Heap */
    hRet = RtlCreateHeap(Flags,
                         NULL,
                         dwMaximumSize,
                         dwInitialSize,
                         NULL,
                         NULL);

    /* Set the last error if we failed, and return the pointer */
    if (!hRet) SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return hRet;
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapDestroy(HANDLE hHeap)
{
    /* Return TRUE if the heap was destroyed */
   if (!RtlDestroyHeap(hHeap)) return TRUE;

    /* Otherwise, we got the handle back, so fail */
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetProcessHeap(VOID)
{
    /* Call the RTL API */
    return RtlGetProcessHeap();
}

/*
 * @implemented
 */
DWORD
WINAPI
GetProcessHeaps(DWORD NumberOfHeaps,
                PHANDLE ProcessHeaps)
{
    /* Call the RTL API */
    return RtlGetProcessHeaps(NumberOfHeaps, ProcessHeaps);
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapLock(HANDLE hHeap)
{
    /* Call the RTL API */
    return RtlLockHeap(hHeap);
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapUnlock(HANDLE hHeap)
{
    /* Call the RTL API */
    return RtlUnlockHeap(hHeap);
}

/*
 * @implemented
 */
SIZE_T
WINAPI
HeapCompact(HANDLE hHeap, DWORD dwFlags)
{
    /* Call the RTL API */
    return RtlCompactHeap(hHeap, dwFlags);
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapValidate(HANDLE hHeap,
             DWORD dwFlags,
             LPCVOID lpMem)
{
    /* Call the RTL API */
    return RtlValidateHeap(hHeap, dwFlags, (PVOID)lpMem);
}

/*
 * @implemented
 */
DWORD
WINAPI
HeapCreateTagsW(HANDLE hHeap,
                DWORD dwFlags,
                PWSTR lpTagName,
                PWSTR lpTagSubName)
{
    /* Call the RTL API */
    return RtlCreateTagHeap(hHeap,
                            dwFlags,
                            lpTagName,
                            lpTagSubName);
}

/*
 * @implemented
 */
DWORD
WINAPI
HeapExtend(HANDLE hHeap,
           DWORD dwFlags,
           PVOID BaseAddress,
           DWORD dwBytes)
{
    NTSTATUS Status;

    /* Call the RTL API. Gone in Vista, so commented out. */
    Status = STATUS_NOT_IMPLEMENTED; //RtlExtendHeap(hHeap, dwFlags, BaseAddress, dwBytes);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
PWSTR
WINAPI
HeapQueryTagW(HANDLE hHeap,
              DWORD dwFlags,
              WORD wTagIndex,
              BOOL bResetCounters,
              PVOID lpTagInfo)
{
    /* Call the RTL API */
    return RtlQueryTagHeap(hHeap,
                           dwFlags,
                           wTagIndex,
                           (BOOLEAN)bResetCounters,
                           lpTagInfo);
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapSummary(HANDLE hHeap,
            DWORD dwFlags,
            PVOID Summary)
{
    NTSTATUS Status;
    RTL_HEAP_USAGE Usage;

    /* Fill in the length information */
    Usage.Length = sizeof(Usage);

    /* Call RTL. Gone in Vista, so commented out */
    Status = STATUS_NOT_IMPLEMENTED; //RtlUsageHeap(hHeap, dwFlags, &Usage);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* FIXME: Summary == Usage?! */
    RtlCopyMemory(Summary, &Usage, sizeof(Usage));
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapUsage(HANDLE hHeap,
          DWORD dwFlags,
          DWORD Unknown,
          DWORD Unknown2,
          IN PVOID Usage)
{
    NTSTATUS Status;

    /* Call RTL. Gone in Vista, so commented out */
    Status = STATUS_NOT_IMPLEMENTED; //RtlUsageHeap(hHeap, dwFlags, &Usage);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }
    else if (Status == STATUS_MORE_ENTRIES)
    {
        /* There are still more entries to parse */
        return TRUE;
    }

    /* Otherwise, we're completely done, so we return FALSE, but NO_ERROR */
    SetLastError(NO_ERROR);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapWalk(HANDLE	hHeap,
         LPPROCESS_HEAP_ENTRY lpEntry)
{
    NTSTATUS Status;

    DPRINT1("Warning, HeapWalk is calling RtlWalkHeap with Win32 parameters\n");

    Status = RtlWalkHeap(hHeap, lpEntry);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapQueryInformation(HANDLE HeapHandle,
                     HEAP_INFORMATION_CLASS HeapInformationClass,
                     PVOID HeapInformation OPTIONAL,
                     SIZE_T HeapInformationLength OPTIONAL,
                     PSIZE_T ReturnLength OPTIONAL)
{
    NTSTATUS Status;

    Status = RtlQueryHeapInformation(HeapHandle,
                                     HeapInformationClass,
                                     HeapInformation,
                                     HeapInformationLength,
                                     ReturnLength);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
HeapSetInformation(HANDLE HeapHandle,
                   HEAP_INFORMATION_CLASS HeapInformationClass,
                   PVOID HeapInformation OPTIONAL,
                   SIZE_T HeapInformationLength OPTIONAL)
{
    NTSTATUS Status;

    Status = RtlSetHeapInformation(HeapHandle,
                                   HeapInformationClass,
                                   HeapInformation,
                                   HeapInformationLength);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
HGLOBAL
NTAPI
GlobalAlloc(UINT uFlags,
            SIZE_T dwBytes)
{
    ULONG Flags = 0;
    PVOID Ptr = NULL;
    HANDLE hMemory;
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    BASE_TRACE_ALLOC(dwBytes, uFlags);
    ASSERT(BaseHeap);

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
        Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes ? dwBytes : 1);
        if (!Ptr) SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        BASE_TRACE_ALLOC2(Ptr);
        return Ptr;
    }

    /* This is heap based, so lock it in first */
    RtlLockHeap(BaseHeap);

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
    }
    else
    {
        /* Get the object and make sure we have size */
        hMemory = &HandleEntry->Object;
        if (dwBytes)
        {
            /* Allocate the actual memory for it */
            Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes);
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
                RtlSetUserValueHeap(BaseHeap, HEAP_NO_SERIALIZE, Ptr, hMemory);
            }
        }
    }

    /* Cleanup! First unlock the heap */
    RtlUnlockHeap(BaseHeap);

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
    return RtlCompactHeap(BaseHeap, 0);
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
    RtlLockHeap(BaseHeap);
    _SEH2_TRY
    {
        /* Check if this is a simple RTL Heap Managed block */
        if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
        {
            /* Then we'll query RTL Heap */
            RtlGetUserInfoHeap(BaseHeap, Flags, hMem, &Handle, &Flags);
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

                /* Now check if it's discardable */
                if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSABLE)
                {
                    /* Set the Win32 Flag */
                    uFlags |= GMEM_DISCARDABLE;
                }

                /* Check if it's DDE Shared */
                if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_DDESHARE)
                {
                    /* Set the Win32 Flag */
                    uFlags |= GMEM_DDESHARE;
                }

                /* Now check if it's discarded */
                if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSE)
                {
                   /* Set the Win32 Flag */
                   uFlags |= GMEM_DISCARDED;
               }
            }
        }

        /* Check if by now, we still haven't gotten any useful flags */
        if (uFlags == GMEM_INVALID_HANDLE) SetLastError(ERROR_INVALID_HANDLE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set the exception code */
        BaseSetLastNTError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* All done! Unlock heap and return Win32 Flags */
    RtlUnlockHeap(BaseHeap);
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
        if (RtlFreeHeap(BaseHeap, 0, hMem))
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
    RtlLockHeap(BaseHeap);
    _SEH2_TRY
    {
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
            /* Free it with the RTL Heap Manager */
            if (RtlFreeHeap(BaseHeap, HEAP_NO_SERIALIZE, Ptr))
            {
                /* Everything worked */
                hMem = NULL;
            }
            else
            {
                /* This wasn't a real heap handle */
                SetLastError(ERROR_INVALID_HANDLE);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set the exception code */
        BaseSetLastNTError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* We're done, so unlock the heap and return the handle */
    RtlUnlockHeap(BaseHeap);
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
    RtlLockHeap(BaseHeap);
    _SEH2_TRY
    {
        /* Query RTL Heap */
        if (!RtlGetUserInfoHeap(BaseHeap,
                                HEAP_NO_SERIALIZE,
                                (PVOID)pMem,
                                &Handle,
                                &Flags))
        {
            /* RTL Heap Manager does not know about this heap */
            SetLastError(ERROR_INVALID_HANDLE);
        }
        else
        {
            /*
             * Check if RTL Heap didn't find a handle for us or said that
             * this heap isn't movable.
             */
            BASE_TRACE_PTR(Handle, pMem);
            if (!(Handle) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
            {
                /* We're actually handle-based, so the pointer is a handle */
                Handle = (HANDLE)pMem;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set the exception code */
        BaseSetLastNTError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* All done, unlock the heap and return the handle */
    RtlUnlockHeap(BaseHeap);
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
        /* Make sure it's not a kernel or invalid address */
        if ((hMem >= (HGLOBAL)SystemRangeStart) || (IsBadReadPtr(hMem, 1)))
        {
            /* Signal an error */
            SetLastError(ERROR_INVALID_HANDLE);
            return NULL;
        }

        /* It's all good */
        return hMem;
    }

    /* Otherwise, lock the heap */
    RtlLockHeap(BaseHeap);
    _SEH2_TRY
    {
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
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        Ptr = NULL;
    }
    _SEH2_END;

    /* All done. Unlock the heap and return the pointer */
    RtlUnlockHeap(BaseHeap);
    return Ptr;
}

HGLOBAL
NTAPI
GlobalReAlloc(HGLOBAL hMem,
              SIZE_T dwBytes,
              UINT uFlags)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    HANDLE Handle;
    LPVOID Ptr;
    ULONG Flags = 0;

    /* Throw out invalid flags */
    if (uFlags & ~(GMEM_VALID_FLAGS | GMEM_MODIFY))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Throw out invalid combo */
    if ((uFlags & GMEM_DISCARDABLE) && !(uFlags & GMEM_MODIFY))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Convert ZEROINIT */
    if (uFlags & GMEM_ZEROINIT) Flags |= HEAP_ZERO_MEMORY;

    /* If this wasn't a movable heap, then we MUST re-alloc in place */
    if (!(uFlags & GMEM_MOVEABLE)) Flags |= HEAP_REALLOC_IN_PLACE_ONLY;

    /* Lock the heap and disable built-in locking in the RTL Heap funcitons */
    RtlLockHeap(BaseHeap);
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
                HandleEntry->Flags &= ~BASE_HEAP_ENTRY_FLAG_REUSABLE;
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
                        if (RtlFreeHeap(BaseHeap, Flags, Ptr))
                        {
                            /* Free the handle */
                            HandleEntry->Object = NULL;
                            HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSE;

                            /* Get the object pointer */
                            hMem = &HandleEntry->Object;
                        }
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
                    Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes);
                    BASE_TRACE_ALLOC2(Ptr);
                    if (Ptr)
                    {
                        /* Allocation succeeded, so save our entry */
                        RtlSetUserValueHeap(BaseHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            hMem);
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

                    /* Do the re-allocation. No need to save the entry again */
                    Ptr = RtlReAllocateHeap(BaseHeap, Flags, Ptr, dwBytes);
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
            if (RtlGetUserInfoHeap(BaseHeap,
                                   HEAP_NO_SERIALIZE,
                                   hMem,
                                   &Handle,
                                   NULL))
            {
                /*
                 * Check if the handle matches the pointer or the moveable flag
                 * isn't there, which is what we expect since it currenly isn't.
                 */
                if ((Handle == hMem) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
                {
                    /* Allocate a handle for it */
                    HandleEntry = BaseHeapAllocEntry();
                    if (!HandleEntry)
                    {
                        /* No entry could be allocated */
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        RtlUnlockHeap(BaseHeap);
                        return NULL;
                    }

                    /* Calculate the size of the current heap */
                    dwBytes = RtlSizeHeap(BaseHeap, HEAP_NO_SERIALIZE, hMem);

                    /* Set the movable flag */
                    Flags |= HEAP_SETTABLE_USER_VALUE | BASE_HEAP_FLAG_MOVABLE;

                    /* Now allocate the actual heap for it */
                    HandleEntry->Object = RtlAllocateHeap(BaseHeap,
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
                        /* Otherwise, copy the new heap and free the old one */
                        RtlMoveMemory(HandleEntry->Object, hMem, dwBytes);
                        RtlFreeHeap(BaseHeap, HEAP_NO_SERIALIZE, hMem);

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
                        RtlSetUserValueHeap(BaseHeap,
                                            HEAP_NO_SERIALIZE,
                                            HandleEntry->Object,
                                            hMem);
                    }
                }
            }
        }
    }
    else
    {
        /* Otherwise, this is a simple RTL Managed Heap, so just call it */
        hMem = RtlReAllocateHeap(BaseHeap,
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
    RtlUnlockHeap(BaseHeap);
    return hMem;
}

/*
 * @implemented
 */
SIZE_T
NTAPI
GlobalSize(HGLOBAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    PVOID Handle = NULL;
    ULONG Flags = 0;
    SIZE_T dwSize = MAXULONG_PTR;

    /* Lock the heap */
    RtlLockHeap(BaseHeap);
    _SEH2_TRY
    {
        /* Check if this is a simple RTL Heap Managed block */
        if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
        {
            /* Then we'll query RTL Heap */
            if (RtlGetUserInfoHeap(BaseHeap, Flags, hMem, &Handle, &Flags))
            {
                BASE_TRACE_PTR(Handle, hMem);
                /*
                 * Check if RTL Heap didn't give us a handle or said that this
                 * heap isn't movable.
                 */
                if (!(Handle) || !(Flags & BASE_HEAP_FLAG_MOVABLE))
                {
                    /* We're not a handle heap, so use the generic call */
                    dwSize = RtlSizeHeap(BaseHeap, HEAP_NO_SERIALIZE, hMem);
                }
                else
                {
                    /* We're a handle heap so get the internal handle */
                    hMem = Handle;
                }
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
                dwSize = RtlSizeHeap(BaseHeap,
                                     HEAP_NO_SERIALIZE,
                                     HandleEntry->Object);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set failure for later */
        dwSize = MAXULONG_PTR;
    }
    _SEH2_END;

    /* Check if by now, we still haven't gotten any useful size */
    if (dwSize == MAXULONG_PTR)
    {
        /* Fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_HANDLE);
        dwSize = 0;
    }

    /* All done! Unlock heap and return the size */
    RtlUnlockHeap(BaseHeap);
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
    RtlLockHeap(BaseHeap);

    /* Get the handle entry */
    HandleEntry = BaseHeapGetEntry(hMem);
    BASE_TRACE_HANDLE(HandleEntry, hMem);

    _SEH2_TRY
    {
        /* Make sure it's valid */
        if (!BaseHeapValidateEntry(HandleEntry))
        {
            /* It's not, fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_INVALID_HANDLE);
            RetVal = FALSE;
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
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
    }
    _SEH2_END;

    /* All done. Unlock the heap and return the pointer */
    RtlUnlockHeap(BaseHeap);
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

    if (lpBuffer->dwLength != sizeof(*lpBuffer))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Query performance information */
    NtQuerySystemInformation(SystemPerformanceInformation,
                             &PerformanceInfo,
                             sizeof(PerformanceInfo),
                             NULL);

    /* Calculate memory load */
    lpBuffer->dwMemoryLoad = ((DWORD)(BaseStaticServerData->SysInfo.NumberOfPhysicalPages -
                                      PerformanceInfo.AvailablePages) * 100) /
                                      BaseStaticServerData->SysInfo.NumberOfPhysicalPages;

    /* Save physical memory */
    PhysicalMemory = BaseStaticServerData->SysInfo.NumberOfPhysicalPages *
                     BaseStaticServerData->SysInfo.PageSize;
    lpBuffer->ullTotalPhys = PhysicalMemory;

    /* Now save available physical memory */
    PhysicalMemory = PerformanceInfo.AvailablePages *
                     BaseStaticServerData->SysInfo.PageSize;
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
    lpBuffer->ullTotalPageFile *= BaseStaticServerData->SysInfo.PageSize;

    /* Calculate how many pages are left */
    PageFile = PerformanceInfo.CommitLimit - PerformanceInfo.CommittedPages;

    /* Save the total */
    lpBuffer->ullAvailPageFile = min(PageFile,
                                     QuotaLimits.PagefileLimit -
                                     VmCounters.PagefileUsage);
    lpBuffer->ullAvailPageFile *= BaseStaticServerData->SysInfo.PageSize;

    /* Now calculate the total virtual space */
    lpBuffer->ullTotalVirtual = (BaseStaticServerData->SysInfo.MaximumUserModeAddress -
                                 BaseStaticServerData->SysInfo.MinimumUserModeAddress) + 1;

    /* And finally the avilable virtual space */
    lpBuffer->ullAvailVirtual = lpBuffer->ullTotalVirtual - VmCounters.VirtualSize;
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
        lpBuffer->dwTotalPhys = (SIZE_T)min(lpBufferEx.ullTotalPhys, MAXULONG_PTR);
        lpBuffer->dwAvailPhys = (SIZE_T)min(lpBufferEx.ullAvailPhys, MAXULONG_PTR);
        lpBuffer->dwTotalPageFile = (SIZE_T)min(lpBufferEx.ullTotalPageFile, MAXULONG_PTR);
        lpBuffer->dwAvailPageFile = (SIZE_T)min(lpBufferEx.ullAvailPageFile, MAXULONG_PTR);
        lpBuffer->dwTotalVirtual = (SIZE_T)min(lpBufferEx.ullTotalVirtual, MAXULONG_PTR);
        lpBuffer->dwAvailVirtual = (SIZE_T)min(lpBufferEx.ullAvailVirtual, MAXULONG_PTR);
    }
}

/*
 * @implemented
 */
HLOCAL
NTAPI
LocalAlloc(UINT uFlags,
           SIZE_T dwBytes)
{
    ULONG Flags = 0;
    PVOID Ptr = NULL;
    HANDLE hMemory;
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    BASE_TRACE_ALLOC(dwBytes, uFlags);
    ASSERT(BaseHeap);

    /* Make sure the flags are valid */
    if (uFlags & ~LMEM_VALID_FLAGS)
    {
        /* They aren't, fail */
        BASE_TRACE_FAILURE();
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Convert ZEROINIT */
    if (uFlags & LMEM_ZEROINIT) Flags |= HEAP_ZERO_MEMORY;

    /* Check if we're not movable, which means pointer-based heap */
    if (!(uFlags & LMEM_MOVEABLE))
    {
        /* Allocate heap for it */
        Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes);
        BASE_TRACE_ALLOC2(Ptr);
        return Ptr;
    }

    /* This is heap based, so lock it in first */
    RtlLockHeap(BaseHeap);

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
        Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes);
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
            RtlSetUserValueHeap(BaseHeap, HEAP_NO_SERIALIZE, Ptr, hMemory);
        }
    }

Quickie:
    /* Cleanup! First unlock the heap */
    RtlUnlockHeap(BaseHeap);

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
LocalCompact(UINT dwMinFree)
{
    /* Call the RTL Heap Manager */
    return RtlCompactHeap(BaseHeap, 0);
}

/*
 * @implemented
 */
UINT
NTAPI
LocalFlags(HLOCAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    HANDLE Handle = NULL;
    ULONG Flags = 0;
    UINT uFlags = LMEM_INVALID_HANDLE;

    /* Start by locking the heap */
    RtlLockHeap(BaseHeap);

    /* Check if this is a simple RTL Heap Managed block */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
        /* Then we'll query RTL Heap */
        RtlGetUserInfoHeap(BaseHeap, Flags, hMem, &Handle, &Flags);
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
            uFlags = HandleEntry->LockCount & LMEM_LOCKCOUNT;

            /* Now check if it's discardable */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSABLE)
            {
                /* Set the Win32 Flag */
                uFlags |= LMEM_DISCARDABLE;
            }

            /* Now check if it's discarded */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSE)
               /* Set the Win32 Flag */
               uFlags |= LMEM_DISCARDED;
        }
    }

    /* Check if by now, we still haven't gotten any useful flags */
    if (uFlags == LMEM_INVALID_HANDLE) SetLastError(ERROR_INVALID_HANDLE);

    /* All done! Unlock heap and return Win32 Flags */
    RtlUnlockHeap(BaseHeap);
    return uFlags;
}

/*
 * @implemented
 */
HLOCAL
NTAPI
LocalFree(HLOCAL hMem)
{
    /* This is identical to a Global Free */
    return GlobalFree(hMem);
}

/*
 * @implemented
 */
HLOCAL
NTAPI
LocalHandle(LPCVOID pMem)
{
    /* This is identical to a Global Handle */
    return GlobalHandle(pMem);
}

/*
 * @implemented
 */
LPVOID
NTAPI
LocalLock(HLOCAL hMem)
{
    /* This is the same as a GlobalLock, assuming these never change */
    C_ASSERT(LMEM_LOCKCOUNT == GMEM_LOCKCOUNT);
    return GlobalLock(hMem);
}

HLOCAL
NTAPI
LocalReAlloc(HLOCAL hMem,
             SIZE_T dwBytes,
             UINT uFlags)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    LPVOID Ptr;
    ULONG Flags = 0;

    /* Convert ZEROINIT */
    if (uFlags & LMEM_ZEROINIT) Flags |= HEAP_ZERO_MEMORY;

    /* If this wasn't a movable heap, then we MUST re-alloc in place */
    if (!(uFlags & LMEM_MOVEABLE)) Flags |= HEAP_REALLOC_IN_PLACE_ONLY;

    /* Lock the heap and disable built-in locking in the RTL Heap funcitons */
    RtlLockHeap(BaseHeap);
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
        else if (uFlags & LMEM_MODIFY)
        {
            /* User is changing flags... check if the memory was discardable */
            if (uFlags & LMEM_DISCARDABLE)
            {
                /* Then set the flag */
                HandleEntry->Flags |= BASE_HEAP_ENTRY_FLAG_REUSABLE;
            }
            else
            {
                /* Otherwise, remove the flag */
                HandleEntry->Flags &= ~BASE_HEAP_ENTRY_FLAG_REUSABLE;
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
                    if ((uFlags & LMEM_MOVEABLE) && !(HandleEntry->LockCount))
                    {
                        /* Free the current heap */
                        RtlFreeHeap(BaseHeap, Flags, Ptr);

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
                    Ptr = RtlAllocateHeap(BaseHeap, Flags, dwBytes);
                    BASE_TRACE_ALLOC2(Ptr);
                    if (Ptr)
                    {
                        /* Allocation succeeded, so save our entry */
                        RtlSetUserValueHeap(BaseHeap,
                                            HEAP_NO_SERIALIZE,
                                            Ptr,
                                            hMem);
                    }
                }
                else
                {
                    /*
                     * If it's not movable or currently locked, we MUST allocate
                     * in-place!
                     */
                    if (!(uFlags & LMEM_MOVEABLE) && (HandleEntry->LockCount))
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
                    Ptr = RtlReAllocateHeap(BaseHeap, Flags, Ptr, dwBytes);
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
    else if (!(uFlags & LMEM_MODIFY))
    {
        /* Otherwise, this is a simple RTL Managed Heap, so just call it */
        hMem = RtlReAllocateHeap(BaseHeap,
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
    RtlUnlockHeap(BaseHeap);
    return hMem;
}

/*
 * @implemented
 */
SIZE_T
WINAPI
LocalShrink(HLOCAL hMem,
            UINT cbNewSize)
{
    /* Call RTL */
    return RtlCompactHeap(BaseHeap, 0);
}

/*
 * @implemented
 */
SIZE_T
NTAPI
LocalSize(HLOCAL hMem)
{
    /* This is the same as a Global Size */
    return GlobalSize(hMem);
}

/*
 * @implemented
 */
BOOL
NTAPI
LocalUnlock(HLOCAL hMem)
{
    PBASE_HEAP_HANDLE_ENTRY HandleEntry;
    BOOL RetVal = TRUE;

    /* Check if this was a simple allocated heap entry */
    if (!((ULONG_PTR)hMem & BASE_HEAP_IS_HANDLE_ENTRY))
    {
       /* Fail, because LocalUnlock is not supported on LMEM_FIXED allocations */
       SetLastError(ERROR_NOT_LOCKED);
       return FALSE;
    }

    /* Otherwise, lock the heap */
    RtlLockHeap(BaseHeap);

    /* Get the handle entry */
    HandleEntry = BaseHeapGetEntry(hMem);
    BASE_TRACE_HANDLE(HandleEntry, hMem);
    _SEH2_TRY
    {
        /* Make sure it's valid */
        if (!BaseHeapValidateEntry(HandleEntry))
        {
            /* It's not, fail */
            BASE_TRACE_FAILURE();
            SetLastError(ERROR_INVALID_HANDLE);
            RetVal = FALSE;
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
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        RetVal = FALSE;
    }
    _SEH2_END;

    /* All done. Unlock the heap and return the pointer */
    RtlUnlockHeap(BaseHeap);
    return RetVal;
}

/* EOF */
