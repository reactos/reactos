/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/heap.c
 * PURPOSE:         Heap Memory APIs (wrappers for RtlHeap*)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
HeapCreate(DWORD flOptions,
           DWORD dwInitialSize,
           DWORD dwMaximumSize)
{
    HANDLE hRet;
    ULONG Flags;

    /* Remove non-Win32 flags and tag this allocation */
    Flags = (flOptions & (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE)) |
            HEAP_CLASS_1;

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
        SetLastErrorByStatus(Status);
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
        SetLastErrorByStatus(Status);
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
        SetLastErrorByStatus(Status);
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
STDCALL
HeapWalk(HANDLE	hHeap,
         LPPROCESS_HEAP_ENTRY lpEntry)
{
    NTSTATUS Status;

    Status = RtlWalkHeap(hHeap, lpEntry);

    if (Status)
        SetLastError(RtlNtStatusToDosError(Status));

    return !Status;
}

/* EOF */
