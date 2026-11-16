/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/win32/kernelbase/realtime_virt.c
 * PURPOSE:     KernelBase QueryInterruptTime and VirtualAlloc2 for Firefox
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson
 *
 * Guest serial output/ntdll-apps/serial-App-20261116T055814Z.log shows
 * LDR missing export KernelBase.dll!QueryInterruptTime and
 * kernelbase.dll!VirtualAlloc2 before the firefox.exe user AV.
 * QueryInterruptTime: https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryinterrupttime
 * Interrupt time in SharedUserData: sdk/include/xdk/ketypes.h KUSER_SHARED_DATA.InterruptTime
 * and dll/win32/kernel32/client/time.c KiReadSystemTime pattern.
 * VirtualAlloc2: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc2
 * Extended parameters are ignored; allocation uses VirtualAllocEx.
 */

#include <windows.h>
#include <ndk/kefuncs.h>

VOID
WINAPI
QueryInterruptTime(PULONGLONG InterruptTime)
{
    LARGE_INTEGER Time;

    Time = KiReadSystemTime(&SharedUserData->InterruptTime);
    *InterruptTime = Time.QuadPart;
}

PVOID
WINAPI
VirtualAlloc2(HANDLE Process,
              PVOID BaseAddress,
              SIZE_T Size,
              ULONG AllocationType,
              ULONG PageProtection,
              MEM_EXTENDED_PARAMETER *ExtendedParameters,
              ULONG ParameterCount)
{
    UNREFERENCED_PARAMETER(ExtendedParameters);
    UNREFERENCED_PARAMETER(ParameterCount);

    if (!Process)
    {
        Process = GetCurrentProcess();
    }

    return VirtualAllocEx(Process,
                          BaseAddress,
                          Size,
                          AllocationType,
                          PageProtection);
}
