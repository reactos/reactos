/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/utils_shared.c
 * PURPOSE:         Utility functions shared with kernel32_vista
 * PROGRAMMER:      Thomas Faber
*/

/* INCLUDES *******************************************************************/

#include <k32.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

/*
* @implemented
*/
DWORD
WINAPI
BaseSetLastNTError(IN NTSTATUS Status)
{
    DWORD dwErrCode;

    /* Convert from NT to Win32, then set */
    dwErrCode = RtlNtStatusToDosError(Status);
    SetLastError(dwErrCode);
    return dwErrCode;
}
