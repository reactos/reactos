/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2help/apc.c
 * PURPOSE:     WinSock 2 DLL header
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

/* DATA **********************************************************************/

#define APCH        (HANDLE)'SOR '

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
WahOpenApcHelper(OUT PHANDLE ApcHelperHandle)
{
    DWORD ErrorCode;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate handle */
    if (!ApcHelperHandle) return ERROR_INVALID_PARAMETER;

    /*
     * Return a bogus handle ("ROS")
     * Historical note:(MS sends "CKM", which probably stands for "Keith Moore"
     * (KM), one of the core architects of Winsock 2.2 from Microsoft.
     */
    *ApcHelperHandle = APCH;
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WahCloseApcHelper(IN HANDLE ApcHelperHandle)
{
    DWORD ErrorCode;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate handle */
    if (ApcHelperHandle != APCH) return ERROR_INVALID_PARAMETER;

    /* return */
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WahCloseThread(IN HANDLE ApcHelperHandle,
               IN LPWSATHREADID ThreadId)
{
    DWORD ErrorCode;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate handles */
    if ((ApcHelperHandle != APCH) || (!ThreadId) || (!ThreadId->ThreadHandle))
    {
        /* Invalid helper/thread handles */
        return ERROR_INVALID_PARAMETER;
    }

    /* Close the thread handle */
    if (CloseHandle(ThreadId->ThreadHandle))
    {
        /* Clear the structure */
        ThreadId->ThreadHandle = NULL;
        ThreadId->Reserved = 0;
        return NO_ERROR;
    }

    /* return */
    return GetLastError();
}

INT
WINAPI
WahQueueUserApc(IN HANDLE ApcHelperHandle,
                IN LPWSATHREADID ThreadId,
                IN LPWSAUSERAPC ApcRoutine,
                IN PVOID ApcContext OPTIONAL)
{
    /* Validate params  */
    if ((ApcHelperHandle != APCH) ||
        (!ThreadId) ||
        (!ThreadId->ThreadHandle) ||
        (!ApcRoutine))
    {
        /* Invalid parameters */
        return ERROR_INVALID_PARAMETER;
    }

    /* Queue the APC */
    if (QueueUserAPC(ApcRoutine, ThreadId->ThreadHandle, (ULONG_PTR)ApcContext))
    {
        /* Return success */
        return ERROR_SUCCESS;
    }

    /* Fail */
    return GetLastError();
}

DWORD
WINAPI
WahOpenCurrentThread(IN HANDLE ApcHelperHandle,
                     OUT LPWSATHREADID ThreadId)
{
    HANDLE ProcessHandle, ThreadHandle;

    /* Validate params  */
    if ((ApcHelperHandle != APCH) || (!ThreadId))
    {
        /* Invalid parameters */
        return ERROR_INVALID_PARAMETER;
    }

    /* Get the process/thread handles */
    ProcessHandle = GetCurrentProcess();
    ThreadHandle = GetCurrentThread();

    /* Duplicate the handle */
    if (DuplicateHandle(ProcessHandle,
                        ThreadHandle,
                        ProcessHandle,
                        &ThreadId->ThreadHandle,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS))
    {
        /* Save the thread handle and return */
        ThreadId->Reserved = (DWORD_PTR)ThreadHandle;
        return ERROR_SUCCESS;
    }

    /* Fail */
    return GetLastError();
}

/* EOF */
