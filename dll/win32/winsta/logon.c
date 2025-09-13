/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/logon.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapi?n
 * NOTES:           This file contains exported functions relevant to
 *                  userinit, winlogon, lsass and friends in vista.
 */


#include "reactos/ts/winsta.h"
#include "winsta.h"

BOOLEAN
WINSTAAPI WinStationDisconnect(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationFreeUserCredentials(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationFreeUserCertificates(PVOID A)
{
    UNIMPLEMENTED;
}

BOOL
WINSTAAPI WinStationIsSessionPermitted()
{
    UNIMPLEMENTED;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

VOID
WINSTAAPI WinStationNegotiateSession(PVOID A,
                                     PVOID B,
                                     PVOID C,
                                     PVOID D,
                                     PVOID E,
                                     PVOID F)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationRedirectErrorMessage(PVOID A,
                                         PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationReportUIResult(PVOID A,
                                   PVOID B,
                                   PVOID C)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationGetLoggedOnCount(_Out_ PULONG LoggedOnUserCount,
    _Out_ PULONG LoggedOnDeviceCount)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationUserLoginAccessCheck(PVOID A,
                                         PVOID B,
                                         PVOID C,
                                         PVOID D)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationWaitForConnect()
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationReset(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI _WinStationNotifyLogoff()
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationNotifyLogon(PVOID A,
                                 PVOID B,
                                 PVOID C,
                                 PVOID D,
                                 PVOID E,
                                 PVOID F,
                                 PVOID G,
                                 PVOID H)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationCanLogonProceed(PVOID A,
                                    PVOID B,
                                    PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationOpenSessionDirectory(PVOID A,
                                          PVOID B)
{
    UNIMPLEMENTED;
}
