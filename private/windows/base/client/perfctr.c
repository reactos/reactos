/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    perfctr.c

Abstract:

    This module contains the Win32 Performance Counter APIs

Author:

    Russ Blake (russbl)  29-May-1992

Revision History:

--*/

#include "basedll.h"


BOOL
WINAPI
QueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount
    )

/*++

    QueryPerformanceCounter -   provides access to a high-resolution
                                counter; frequency of this counter
                                is supplied by QueryPerformanceFrequency

        Inputs:

            lpPerformanceCount  -   a pointer to variable which
                                    will receive the counter

        Outputs:

            lpPerformanceCount  -   the current value of the counter,
                                    or 0 if it is not available

        Returns:

            TRUE if the performance counter is supported by the
            hardware, or FALSE if the performance counter is not
            supported by the hardware.


                                                                                            will receive the count

--*/

{
    LARGE_INTEGER PerfFreq;
    NTSTATUS Status;

    Status = NtQueryPerformanceCounter(lpPerformanceCount, &PerfFreq);

    if (!NT_SUCCESS(Status)) {
        // Call failed, report error
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    if (PerfFreq.LowPart == 0 && PerfFreq.HighPart == 0 ) {
        // Counter not supported
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
QueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency
    )

/*++

    QueryPerformanceFrequency -   provides the frequency of the high-
                                  resolution counter returned by
                                  QueryPerformanceCounter

        Inputs:


            lpFrequency         -   a pointer to variable which
                                    will receive the frequency

        Outputs:

            lpPerformanceCount  -   the frequency of the counter,
                                    or 0 if it is not available

        Returns:

            TRUE if the performance counter is supported by the
            hardware, or FALSE if the performance counter is not
            supported by the hardware.

--*/
{
    LARGE_INTEGER PerfCount;
    NTSTATUS Status;

    Status = NtQueryPerformanceCounter(&PerfCount, lpFrequency);

    if (!NT_SUCCESS(Status)) {
        // Call failed, report error
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    if (lpFrequency->LowPart == 0 && lpFrequency->HighPart == 0 ) {
        // Counter not supported
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    return TRUE;
}
