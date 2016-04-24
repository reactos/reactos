/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/virtmem.c
 * PURPOSE:         Handles virtual memory APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
LPVOID
NTAPI
VirtualAllocEx(IN HANDLE hProcess,
               IN LPVOID lpAddress,
               IN SIZE_T dwSize,
               IN DWORD flAllocationType,
               IN DWORD flProtect)
{
    NTSTATUS Status;

    /* Make sure the address is within the granularity of the system (64K) */
    if ((lpAddress) &&
        (lpAddress < (PVOID)BaseStaticServerData->SysInfo.AllocationGranularity))
    {
        /* Fail the call */
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Handle any possible exceptions */
    _SEH2_TRY
    {
        /* Allocate the memory */
        Status = NtAllocateVirtualMemory(hProcess,
                                         &lpAddress,
                                         0,
                                         &dwSize,
                                         flAllocationType,
                                         flProtect);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Check for status */
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Return the allocated address */
    return lpAddress;
}

/*
 * @implemented
 */
LPVOID
NTAPI
VirtualAlloc(IN LPVOID lpAddress,
             IN SIZE_T dwSize,
             IN DWORD flAllocationType,
             IN DWORD flProtect)
{
    /* Call the extended API */
    return VirtualAllocEx(GetCurrentProcess(),
                          lpAddress,
                          dwSize,
                          flAllocationType,
                          flProtect);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualFreeEx(IN HANDLE hProcess,
              IN LPVOID lpAddress,
              IN SIZE_T dwSize,
              IN DWORD dwFreeType)
{
    NTSTATUS Status;

    /* Validate size and flags */
    if (!(dwSize) || !(dwFreeType & MEM_RELEASE))
    {
        /* Free the memory */
        Status = NtFreeVirtualMemory(hProcess,
                                     &lpAddress,
                                     &dwSize,
                                     dwFreeType);
        if (!NT_SUCCESS(Status))
        {
            /* We failed */
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Return success */
        return TRUE;
    }

    /* Invalid combo */
    BaseSetLastNTError(STATUS_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualFree(IN LPVOID lpAddress,
            IN SIZE_T dwSize,
            IN DWORD dwFreeType)
{
    /* Call the extended API */
    return VirtualFreeEx(GetCurrentProcess(),
                         lpAddress,
                         dwSize,
                         dwFreeType);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualProtect(IN LPVOID lpAddress,
               IN SIZE_T dwSize,
               IN DWORD flNewProtect,
               OUT PDWORD lpflOldProtect)
{
    /* Call the extended API */
    return VirtualProtectEx(GetCurrentProcess(),
                            lpAddress,
                            dwSize,
                            flNewProtect,
                            lpflOldProtect);
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualProtectEx(IN HANDLE hProcess,
                 IN LPVOID lpAddress,
                 IN SIZE_T dwSize,
                 IN DWORD flNewProtect,
                 OUT PDWORD lpflOldProtect)
{
    NTSTATUS Status;

    /* Change the protection */
    Status = NtProtectVirtualMemory(hProcess,
                                    &lpAddress,
                                    &dwSize,
                                    flNewProtect,
                                    (PULONG)lpflOldProtect);
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
BOOL
NTAPI
VirtualLock(IN LPVOID lpAddress,
            IN SIZE_T dwSize)
{
    NTSTATUS Status;
    SIZE_T RegionSize = dwSize;
    PVOID BaseAddress = lpAddress;

    /* Lock the memory */
    Status = NtLockVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &RegionSize,
                                 MAP_PROCESS);
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
SIZE_T
NTAPI
VirtualQuery(IN LPCVOID lpAddress,
             OUT PMEMORY_BASIC_INFORMATION lpBuffer,
             IN SIZE_T dwLength)
{
    /* Call the extended API */
    return VirtualQueryEx(NtCurrentProcess(),
                          lpAddress,
                          lpBuffer,
                          dwLength);
}

/*
 * @implemented
 */
SIZE_T
NTAPI
VirtualQueryEx(IN HANDLE hProcess,
               IN LPCVOID lpAddress,
               OUT PMEMORY_BASIC_INFORMATION lpBuffer,
               IN SIZE_T dwLength)
{
    NTSTATUS Status;
    SIZE_T ResultLength;

    /* Query basic information */
    Status = NtQueryVirtualMemory(hProcess,
                                  (LPVOID)lpAddress,
                                  MemoryBasicInformation,
                                  lpBuffer,
                                  dwLength,
                                  &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Return the length returned */
    return ResultLength;
}

/*
 * @implemented
 */
BOOL
NTAPI
VirtualUnlock(IN LPVOID lpAddress,
              IN SIZE_T dwSize)
{
    NTSTATUS Status;
    SIZE_T RegionSize = dwSize;
    PVOID BaseAddress = lpAddress;

    /* Lock the memory */
    Status = NtUnlockVirtualMemory(NtCurrentProcess(),
                                   &BaseAddress,
                                   &RegionSize,
                                   MAP_PROCESS);
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
UINT
WINAPI
GetWriteWatch(IN DWORD dwFlags,
              IN PVOID lpBaseAddress,
              IN SIZE_T dwRegionSize,
              IN PVOID *lpAddresses,
              OUT PULONG_PTR lpdwCount,
              OUT PULONG lpdwGranularity)
{
    NTSTATUS Status;

    Status = NtGetWriteWatch(GetCurrentProcess(),
                             dwFlags,
                             lpBaseAddress,
                             dwRegionSize,
                             lpAddresses,
                             lpdwCount,
                             lpdwGranularity);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return -1;
    }

    return 0;
}

/*
 * @implemented
 */
UINT
WINAPI
ResetWriteWatch(IN LPVOID lpBaseAddress,
                IN SIZE_T dwRegionSize)
{
    NTSTATUS Status;

    Status = NtResetWriteWatch(NtCurrentProcess(),
                               lpBaseAddress,
                               dwRegionSize);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return -1;
    }

    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
AllocateUserPhysicalPages(IN HANDLE hProcess,
                          IN PULONG_PTR NumberOfPages,
                          OUT PULONG_PTR UserPfnArray)
{
    NTSTATUS Status;

    Status = NtAllocateUserPhysicalPages(hProcess, NumberOfPages, UserPfnArray);
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
FreeUserPhysicalPages(IN HANDLE hProcess,
                      IN PULONG_PTR NumberOfPages,
                      IN PULONG_PTR PageArray)
{
    NTSTATUS Status;

    Status = NtFreeUserPhysicalPages(hProcess, NumberOfPages, PageArray);
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
MapUserPhysicalPages(IN PVOID VirtualAddress,
                     IN ULONG_PTR NumberOfPages,
                     OUT PULONG_PTR PageArray OPTIONAL)
{
    NTSTATUS Status;

    Status = NtMapUserPhysicalPages(VirtualAddress, NumberOfPages, PageArray);
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
MapUserPhysicalPagesScatter(IN PVOID *VirtualAddresses,
                            IN ULONG_PTR NumberOfPages,
                            OUT PULONG_PTR PageArray OPTIONAL)
{
    NTSTATUS Status;

    Status = NtMapUserPhysicalPagesScatter(VirtualAddresses,
                                           NumberOfPages,
                                           PageArray);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
