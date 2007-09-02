/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/mem/procmem.c
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
BOOL
NTAPI
ReadProcessMemory(IN HANDLE hProcess,
                  IN LPCVOID lpBaseAddress,
                  IN LPVOID lpBuffer,
                  IN DWORD nSize,
                  OUT LPDWORD lpNumberOfBytesRead)
{
    NTSTATUS Status;

    /* Do the read */
    Status = NtReadVirtualMemory(hProcess,
                                 (PVOID)lpBaseAddress,
                                 lpBuffer,
                                 nSize,
                                 lpNumberOfBytesRead);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        SetLastErrorByStatus (Status);
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
WriteProcessMemory(IN HANDLE hProcess,
                   IN LPVOID lpBaseAddress,
                   IN LPCVOID lpBuffer,
                   IN SIZE_T nSize,
                   OUT SIZE_T *lpNumberOfBytesWritten)
{
    NTSTATUS Status;
    ULONG OldValue;
    SIZE_T RegionSize;
    PVOID Base;
    BOOLEAN UnProtect;

    /* Set parameters for protect call */
    RegionSize = nSize;
    Base = lpBaseAddress;

    /* Check the current status */
    Status = NtProtectVirtualMemory(hProcess,
                                    &Base,
                                    &RegionSize,
                                    PAGE_EXECUTE_READWRITE,
                                    &OldValue);
    if (NT_SUCCESS(Status))
    {
        /* Check if we are unprotecting */
        UnProtect = OldValue & (PAGE_READWRITE |
                                PAGE_WRITECOPY |
                                PAGE_EXECUTE_READWRITE |
                                PAGE_EXECUTE_WRITECOPY) ? FALSE : TRUE;
        if (UnProtect)
        {
            /* Set the new protection */
            Status = NtProtectVirtualMemory(hProcess,
                                            &Base,
                                            &RegionSize,
                                            OldValue,
                                            &OldValue);

            /* Write the memory */
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          nSize,
                                          lpNumberOfBytesWritten);
            if (!NT_SUCCESS(Status))
            {
                /* We failed */
                SetLastErrorByStatus(Status);
                return FALSE;
            }

            /* Flush the ITLB */
            NtFlushInstructionCache(hProcess, lpBaseAddress, nSize);
            return TRUE;
        }
        else
        {
            /* Check if we were read only */
            if ((OldValue & PAGE_NOACCESS) || (OldValue & PAGE_READONLY))
            {
                /* Restore protection and fail */
                NtProtectVirtualMemory(hProcess,
                                       &Base,
                                       &RegionSize,
                                       OldValue,
                                       &OldValue);
                SetLastErrorByStatus(STATUS_ACCESS_VIOLATION);
                return FALSE;
            }

            /* Otherwise, do the write */
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          nSize,
                                          lpNumberOfBytesWritten);

            /* And restore the protection */
            NtProtectVirtualMemory(hProcess,
                                   &Base,
                                   &RegionSize,
                                   OldValue,
                                   &OldValue);
            if (!NT_SUCCESS(Status))
            {
                /* We failed */
                SetLastErrorByStatus(STATUS_ACCESS_VIOLATION);
                return FALSE;
            }

            /* Flush the ITLB */
            NtFlushInstructionCache(hProcess, lpBaseAddress, nSize);
            return TRUE;
        }
    }
    else
    {
        /* We failed */
        SetLastErrorByStatus(Status);
        return FALSE;
    }
}

/* EOF */
