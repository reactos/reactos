/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/local.c
 * PURPOSE:         Local Memory APIs (sits on top of Heap*)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "debug.h"

/* TYPES *********************************************************************/

extern SYSTEM_BASIC_INFORMATION BaseCachedSysInfo;

/* FUNCTIONS ***************************************************************/

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
    ASSERT(hProcessHeap);

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
    return RtlCompactHeap(hProcessHeap, 0);
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
            uFlags = HandleEntry->LockCount & LMEM_LOCKCOUNT;

            /* Now check if it's discarded */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_REUSABLE)
            {
                /* Set the Win32 Flag */
                uFlags |= LMEM_DISCARDED;
            }

            /* Check if it's movable */
            if (HandleEntry->Flags & BASE_HEAP_ENTRY_FLAG_MOVABLE)
            {
                /* Set the Win32 Flag */
                uFlags |= LMEM_DISCARDABLE;
            }
        }
    }

    /* Check if by now, we still haven't gotten any useful flags */
    if (uFlags == LMEM_INVALID_HANDLE) SetLastError(ERROR_INVALID_HANDLE);

    /* All done! Unlock heap and return Win32 Flags */
    RtlUnlockHeap(hProcessHeap);
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
    ASSERT(LMEM_LOCKCOUNT == GMEM_LOCKCOUNT);
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
                    if ((uFlags & LMEM_MOVEABLE) && !(HandleEntry->LockCount))
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
                    Ptr = RtlReAllocateHeap(hProcessHeap, Flags, Ptr, dwBytes);
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
SIZE_T
STDCALL
LocalShrink(HLOCAL hMem,
            UINT cbNewSize)
{
    /* Call RTL */
    return RtlCompactHeap(hProcessHeap, 0);
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
    /* This is the same as a Global Unlock */
    return GlobalUnlock(hMem);
}

/* EOF */
