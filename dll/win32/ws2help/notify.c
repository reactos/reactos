/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2help/notify.c
 * PURPOSE:     WinSock 2 DLL header
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

/* DATA **********************************************************************/

#define HANH        (HANDLE)'2SOR'

PSECURITY_DESCRIPTOR pSDPipe = NULL;

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
WahCloseNotificationHandleHelper(IN HANDLE HelperHandle)
{
    DWORD ErrorCode;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate handle */
    if (HelperHandle != HANH) return ERROR_INVALID_PARAMETER;

    /* return */
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WahCreateNotificationHandle(IN HANDLE HelperHandle,
                            OUT PHANDLE NotificationHelperHandle)
{
    UNREFERENCED_PARAMETER(HelperHandle);
    UNREFERENCED_PARAMETER(NotificationHelperHandle);
    return 0;
}

DWORD
WINAPI
WahOpenNotificationHandleHelper(OUT PHANDLE HelperHandle)
{
    DWORD ErrorCode;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Validate handle */
    if (!HelperHandle) return ERROR_INVALID_PARAMETER;

    /* Return a bogus handle ("ROS2") */
    *HelperHandle = HANH;
    return ERROR_SUCCESS;
}

INT
WINAPI
WahNotifyAllProcesses(IN HANDLE NotificationHelperHandle)
{
    UNREFERENCED_PARAMETER(NotificationHelperHandle);
    return 0;
}

INT
WINAPI
WahWaitForNotification(IN HANDLE NotificationHelperHandle,
                       IN HANDLE lpNotificationHandle,
                       IN LPWSAOVERLAPPED lpOverlapped,
                       IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    UNREFERENCED_PARAMETER(NotificationHelperHandle);
    UNREFERENCED_PARAMETER(lpNotificationHandle);
    UNREFERENCED_PARAMETER(lpOverlapped);
    UNREFERENCED_PARAMETER(lpCompletionRoutine);
    return 0;
}

/* EOF */
