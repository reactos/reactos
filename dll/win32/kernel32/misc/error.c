/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/error.c
 * PURPOSE:         Error functions
 * PROGRAMMER:      Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>


DWORD g_dwLastErrorToBreakOn;

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
VOID
WINAPI
SetLastError(
    IN DWORD dwErrCode)
{
    if (g_dwLastErrorToBreakOn)
    {
        /* If we have error to break on and if current matches, break */
        if (g_dwLastErrorToBreakOn == dwErrCode)
        {
            DbgBreakPoint();
        }
    }

    /* Set last error */
    NtCurrentTeb()->LastErrorValue = dwErrCode;
}

/*
 * @implemented
 */
VOID
WINAPI
BaseSetLastNTError(
    IN NTSTATUS Status)
{
    SetLastError(RtlNtStatusToDosError(Status));
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLastError()
{
    return NtCurrentTeb()->LastErrorValue;
}

/* EOF */
