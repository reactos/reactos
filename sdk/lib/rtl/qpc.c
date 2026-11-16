/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        sdk/lib/rtl/qpc.c
 * PURPOSE:     RtlQueryPerformanceCounter / Frequency via NtQueryPerformanceCounter
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Declarations: sdk/include/wine/winternl.h RtlQueryPerformanceCounter /
 * RtlQueryPerformanceFrequency (BOOL, LARGE_INTEGER*). NtQueryPerformanceCounter:
 * https://learn.microsoft.com/en-us/windows/win32/devnotes/ntqueryperformancecounter
 * QueryPerformanceCounter:
 * https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
 * sdk/include/ndk/kefuncs.h already declares NtQueryPerformanceCounter.
 * dll/win32/kernelbase/wine/kernelbase.spec forwards QueryPerformanceCounter to
 * ntdll.RtlQueryPerformanceCounter. Firefox PE import: ntdll!RtlQueryPerformanceCounter.
 */

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlQueryPerformanceCounter(_Out_ PLARGE_INTEGER PerformanceCount)
{
    LARGE_INTEGER Frequency;
    NTSTATUS Status;

    if (PerformanceCount == NULL)
    {
        return FALSE;
    }

    Status = NtQueryPerformanceCounter(PerformanceCount, &Frequency);
    if (!NT_SUCCESS(Status) || Frequency.QuadPart == 0)
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlQueryPerformanceFrequency(_Out_ PLARGE_INTEGER Frequency)
{
    LARGE_INTEGER Count;
    NTSTATUS Status;

    if (Frequency == NULL)
    {
        return FALSE;
    }

    Status = NtQueryPerformanceCounter(&Count, Frequency);
    if (!NT_SUCCESS(Status) || Frequency->QuadPart == 0)
    {
        return FALSE;
    }

    return TRUE;
}
