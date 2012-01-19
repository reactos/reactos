/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/logon.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapi?n
 * NOTES:           This file contains exported functions relevant to
 *                  userinit, winlogon, lsass and friends in vista.
 */
#include <winsta.h>

VOID
WINSTAAPI WinStationDisconnect(PVOID A,
                               PVOID B,
                               PVOID C)
{
    UNIMPLEMENTED;
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

VOID
WINSTAAPI WinStationGetLoggedOnCount(PVOID A,
                                     PVOID B)
{
    UNIMPLEMENTED;
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

VOID
WINSTAAPI WinStationReset(PVOID A,
                          PVOID B,
                          PVOID C)
{
    UNIMPLEMENTED;
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
