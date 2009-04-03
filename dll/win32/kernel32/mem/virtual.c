/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/virtual.c
 * PURPOSE:         Handles virtual memory APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

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

    /* Allocate the memory */
    Status = NtAllocateVirtualMemory(hProcess,
                                     (PVOID *)&lpAddress,
                                     0,
                                     &dwSize,
                                     flAllocationType,
                                     flProtect);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus(Status);
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

    if (dwSize == 0 || !(dwFreeType & MEM_RELEASE))
    {
        /* Free the memory */
        Status = NtFreeVirtualMemory(hProcess,
                                     (PVOID *)&lpAddress,
                                     (PULONG)&dwSize,
                                     dwFreeType);
        if (!NT_SUCCESS(Status))
        {
            /* We failed */
            SetLastErrorByStatus(Status);
            return FALSE;
        }

        /* Return success */
        return TRUE;
    }

    SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
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
        SetLastErrorByStatus(Status);
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
    ULONG BytesLocked;
    NTSTATUS Status;

    /* Lock the memory */
    Status = NtLockVirtualMemory(NtCurrentProcess(),
                                 lpAddress,
                                 dwSize,
                                 &BytesLocked);
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
DWORD
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
DWORD
NTAPI
VirtualQueryEx(IN HANDLE hProcess,
               IN LPCVOID lpAddress,
               OUT PMEMORY_BASIC_INFORMATION lpBuffer,
               IN SIZE_T dwLength)
{
    NTSTATUS Status;
    ULONG ResultLength;

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
        SetLastErrorByStatus(Status);
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
    ULONG BytesLocked;
    NTSTATUS Status;

    /* Unlock the memory */
    Status = NtUnlockVirtualMemory(NtCurrentProcess(),
                                   lpAddress,
                                   dwSize,
                                   &BytesLocked);
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
UINT
WINAPI
GetWriteWatch(
    DWORD  dwFlags,
    PVOID  lpBaseAddress,
    SIZE_T dwRegionSize,
    PVOID *lpAddresses,
    PULONG_PTR lpdwCount,
    PULONG lpdwGranularity
    )
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
        SetLastErrorByStatus(Status);
        return -1;
    }

    return 0;
}

/*
 * @implemented
 */
UINT
WINAPI
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    )
{
    NTSTATUS Status;

    Status = NtResetWriteWatch(NtCurrentProcess(),
                               lpBaseAddress,
                               dwRegionSize);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return -1;
    }

    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR UserPfnArray
    )
{
    NTSTATUS Status;

    Status = NtAllocateUserPhysicalPages(hProcess,
                                         NumberOfPages,
                                         UserPfnArray);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    NTSTATUS Status;

    Status = NtFreeUserPhysicalPages(hProcess,
                                     NumberOfPages,
                                     PageArray);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray  OPTIONAL
    )
{
    NTSTATUS Status;

    Status = NtMapUserPhysicalPages(VirtualAddress,
                                    NumberOfPages,
                                    PageArray);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray  OPTIONAL
    )
{
    NTSTATUS Status;

    Status = NtMapUserPhysicalPagesScatter(VirtualAddresses,
                                           NumberOfPages,
                                           PageArray);

    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
