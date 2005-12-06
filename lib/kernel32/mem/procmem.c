/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/procmem.c
 * PURPOSE:
 * PROGRAMMER:           Boudewijn Dekker
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
ReadProcessMemory (
        HANDLE        hProcess,
        LPCVOID        lpBaseAddress,
        LPVOID        lpBuffer,
        DWORD        nSize,
        LPDWORD        lpNumberOfBytesRead
        )
{

        NTSTATUS Status;

        Status = NtReadVirtualMemory( hProcess, (PVOID)lpBaseAddress,lpBuffer, nSize,
                (PULONG)lpNumberOfBytesRead
                );

        if (!NT_SUCCESS(Status))
             {
                SetLastErrorByStatus (Status);
                return FALSE;
             }
        return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
WriteProcessMemory (
        HANDLE hProcess,
        LPVOID lpBaseAddress,
        LPCVOID lpBuffer,
        SIZE_T nSize,
        SIZE_T *lpNumberOfBytesWritten
        )
{
        NTSTATUS Status, ProtectStatus = STATUS_SUCCESS;
        MEMORY_BASIC_INFORMATION MemInfo;
        ULONG Length;
        BOOLEAN UnProtect;

        if (lpNumberOfBytesWritten)
        {
            *lpNumberOfBytesWritten = 0;
        }

        while (nSize)
        {
            Status = NtQueryVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          MemoryBasicInformation,
                                          &MemInfo,
                                          sizeof(MEMORY_BASIC_INFORMATION),
                                          NULL);

            if (!NT_SUCCESS(Status))
            {
                SetLastErrorByStatus(Status);
                return FALSE;
            }
            Length = MemInfo.RegionSize - ((ULONG_PTR)lpBaseAddress - (ULONG_PTR)MemInfo.BaseAddress);
            if (Length > nSize)
            {
                Length = nSize;
            }
            UnProtect = MemInfo.Protect & (PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY) ? FALSE : TRUE;
            if (UnProtect)
            {
                MemInfo.BaseAddress = lpBaseAddress;
                MemInfo.RegionSize = Length;
                if (MemInfo.Protect & (PAGE_EXECUTE|PAGE_EXECUTE_READ))
                {
                    MemInfo.Protect &= ~(PAGE_EXECUTE|PAGE_EXECUTE_READ);
                    MemInfo.Protect |= PAGE_EXECUTE_READWRITE;
                }
                else
                {
                    MemInfo.Protect &= ~(PAGE_READONLY|PAGE_NOACCESS);
                    MemInfo.Protect |= PAGE_READWRITE;
                }

                ProtectStatus = NtProtectVirtualMemory(hProcess,
                                                       &MemInfo.BaseAddress,
                                                       &MemInfo.RegionSize,
                                                       MemInfo.Protect,
                                                       &MemInfo.Protect);
                if (!NT_SUCCESS(ProtectStatus))
                {
                    SetLastErrorByStatus(ProtectStatus);
                    return FALSE;
                }
                Length = MemInfo.RegionSize - ((ULONG_PTR)lpBaseAddress - (ULONG_PTR)MemInfo.BaseAddress);
                if (Length > nSize)
                {
                    Length = nSize;
                }
            }

            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          Length,
                                          &Length);
            if (UnProtect)
            {
                ProtectStatus = NtProtectVirtualMemory(hProcess,
                                                       &MemInfo.BaseAddress,
                                                       &MemInfo.RegionSize,
                                                       MemInfo.Protect,
                                                       &MemInfo.Protect);
            }
            if (!NT_SUCCESS(Status))
                 {
                SetLastErrorByStatus (Status);
                return FALSE;
                 }
            if (UnProtect && !NT_SUCCESS(ProtectStatus))
            {
                SetLastErrorByStatus (ProtectStatus);
                return FALSE;
            }
            lpBaseAddress = (LPVOID)((ULONG_PTR)lpBaseAddress + Length);
            lpBuffer = (LPCVOID)((ULONG_PTR)lpBuffer + Length);
            nSize -= Length;
            if (lpNumberOfBytesWritten)
            {
                *lpNumberOfBytesWritten += Length;
            }
        }
        return TRUE;
}

/* EOF */
