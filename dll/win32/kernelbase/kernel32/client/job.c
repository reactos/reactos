/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/job.c
 * PURPOSE:         Job functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *                  Created 9/23/2004
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#include <winspool.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
IsProcessInJob(IN HANDLE ProcessHandle,
               IN HANDLE JobHandle,
               OUT PBOOL Result)
{
    NTSTATUS Status;

    Status = NtIsProcessInJob(ProcessHandle, JobHandle);
    if (NT_SUCCESS(Status))
    {
        *Result = (Status == STATUS_PROCESS_IN_JOB);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}
