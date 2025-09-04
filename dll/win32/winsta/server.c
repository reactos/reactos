/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/server.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapión
 *
 */

#include "winsta.h"
#include "reactos/ts/winsta.h"

VOID
WINSTAAPI WinStationOpenServerA(PVOID A)
{
    UNIMPLEMENTED;
}

HANDLE
WINSTAAPI WinStationOpenServerW(_In_opt_ PCWSTR ServerName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationCloseServer(_In_ HANDLE ServerHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI ServerGetInternetConnectorStatus(PVOID A,
                                           PVOID B,
                                           PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI ServerSetInternetConnectorStatus(PVOID A,
                                           PVOID B,
                                           PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI ServerQueryInetConnectorInformationA(PVOID A,
                                               PVOID B,
                                               PVOID C,
                                               PVOID D)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI ServerQueryInetConnectorInformationW(PVOID A,
                                               PVOID B,
                                               PVOID C,
                                               PVOID D)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationServerPing(_In_opt_ HANDLE ServerHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationGetTermSrvCountersValue(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG Count,
    _Inout_ PTS_COUNTER Counters) // set counter IDs before calling
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI _WinStationShadowTarget(PVOID A,
                                  PVOID B,
                                  PVOID C,
                                  PVOID D,
                                  PVOID E,
                                  PVOID F,
                                  PVOID G,
                                  PVOID H,
                                  PVOID I,
                                  PVOID J)
{
    UNIMPLEMENTED;
}
/* EOF */
