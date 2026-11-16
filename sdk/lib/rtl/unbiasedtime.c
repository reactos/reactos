/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        sdk/lib/rtl/unbiasedtime.c
 * PURPOSE:     RtlQueryUnbiasedInterruptTime from shared interrupt time
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * QueryUnbiasedInterruptTime (realtimeapiset.h) returns interrupt time that
 * excludes sleep/hibernation, in 100ns units, and fails on a null pointer:
 * https://learn.microsoft.com/en-us/windows/win32/api/realtimeapiset/nf-realtimeapiset-queryunbiasedinterrupttime
 * KUSER_SHARED_DATA InterruptTime / InterruptTimeBias are in
 * sdk/include/ndk/ketypes.h. KiReadSystemTime is in sdk/include/ndk/kefuncs.h.
 * dll/win32/kernelbase/wine/kernelbase.spec forwards QueryUnbiasedInterruptTime
 * to ntdll.RtlQueryUnbiasedInterruptTime.
 */

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlQueryUnbiasedInterruptTime(_Out_ ULONGLONG *UnbiasedTime)
{
    LARGE_INTEGER InterruptTime;
    ULONGLONG Bias;

    if (UnbiasedTime == NULL)
    {
        return FALSE;
    }

    InterruptTime = KiReadSystemTime(&SharedUserData->InterruptTime);
    Bias = SharedUserData->InterruptTimeBias;
    *UnbiasedTime = (ULONGLONG)InterruptTime.QuadPart - Bias;
    return TRUE;
}
