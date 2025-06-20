/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Win32 Base API
 * FILE:                 dll/win32/kernel32/client/perfcnt.c
 * PURPOSE:              Performance Counter
 * PROGRAMMER:           Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
QueryPerformanceCounter(OUT PLARGE_INTEGER lpPerformanceCount)
{
    LARGE_INTEGER Frequency;
    NTSTATUS Status;

    Status = NtQueryPerformanceCounter(lpPerformanceCount, &Frequency);
    if (Frequency.QuadPart == 0) Status = STATUS_NOT_IMPLEMENTED;

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
QueryPerformanceFrequency(OUT PLARGE_INTEGER lpFrequency)
{
    LARGE_INTEGER Count;
    NTSTATUS Status;

    Status = NtQueryPerformanceCounter(&Count, lpFrequency);
    if (lpFrequency->QuadPart == 0) Status = STATUS_NOT_IMPLEMENTED;

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
