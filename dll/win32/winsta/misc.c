/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/misc.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapión
 * NOTES:           Misc functions.
 *
 */

#include "winsta.h"
#include "reactos/ts/winsta.h"

VOID
WINSTAAPI LogonIdFromWinStationNameA(PVOID A,
                                     PVOID B,
                                     PVOID C)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI LogonIdFromWinStationNameW(_In_opt_ HANDLE ServerHandle,
    _In_ PCWSTR WinStationName,
    _Out_ PULONG SessionId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI RemoteAssistancePrepareSystemRestore(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _NWLogonQueryAdmin(PVOID A,
                             PVOID B,
                             PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _NWLogonSetAdmin(PVOID A,
                           PVOID B,
                           PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationNameFromLogonIdA(PVOID A,
                                     PVOID B,
                                     PVOID C)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationNameFromLogonIdW(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(WINSTATIONNAME_LENGTH + 1) PWSTR WinStationName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */


