/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/query.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapi?n
 * NOTE:            Get, query and enum functions.
 */

#include <winsta.h>

VOID
WINSTAAPI
WinStationQueryLogonCredentialsW(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationQueryInformationA(PVOID A,
                                      PVOID B,
                                      PVOID C,
                                      PVOID D,
                                      PVOID E,
                                      PVOID F)
{
    UNIMPLEMENTED;
}

/*
http://msdn2.microsoft.com/En-US/library/aa383827.aspx
*/
BOOLEAN
WINSTAAPI
WinStationQueryInformationW(HANDLE hServer,
                                    ULONG LogonId,
                                    WINSTATIONINFOCLASS WinStationInformationClass,
                                    PVOID pWinStationInformation,
                                    ULONG WinStationInformationLength,
                                    PULONG pReturnLength)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationQueryAllowConcurrentConnections()
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationQueryEnforcementCore(PVOID A,
                                         PVOID B,
                                         PVOID C,
                                         PVOID D,
                                         PVOID E,
                                         PVOID F)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationQueryLicense(PVOID A,
                                 PVOID B,
                                 PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationQueryUpdateRequired(PVOID A,
                                        PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerateLicenses(PVOID A,
                                      PVOID B,
                                      PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerateProcesses(PVOID A,
                                       PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerateA(PVOID A,
                               PVOID B,
                               PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerateW(PVOID A,
                               PVOID B,
                               PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerate_IndexedA(PVOID A,
                                       PVOID B,
                                       PVOID C,
                                       PVOID D,
                                       PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationEnumerate_IndexedW(PVOID A,
                                       PVOID B,
                                       PVOID C,
                                       PVOID D,
                                       PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationRequestSessionsList(PVOID A,
                                        PVOID B,
                                        PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetAllProcesses(PVOID A,
                                    PVOID B,
                                    PVOID C,
                                    PVOID D)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetLanAdapterNameA(PVOID A,
                                       PVOID B,
                                       PVOID C,
                                       PVOID D,
                                       PVOID E,
                                       PVOID F)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetLanAdapterNameW(PVOID A,
                                       PVOID B,
                                       PVOID C,
                                       PVOID D,
                                       PVOID E,
                                       PVOID F)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetConnectionProperty(PVOID A,
                                          PVOID B,
                                          PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetInitialApplication(PVOID A,
                                          PVOID B,
                                          PVOID C,
                                          PVOID D,
                                          PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetProcessSid(PVOID A,
                                  PVOID B,
                                  PVOID C,
                                  PVOID D,
                                  PVOID E,
                                  PVOID F)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI
WinStationGetUserCertificates(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI
WinStationGetUserCredentials(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationGetUserProfile(PVOID A,
                                   PVOID B,
                                   PVOID C,
                                   PVOID D)
{
    UNIMPLEMENTED;
}


VOID
WINSTAAPI _WinStationGetApplicationInfo(PVOID A,
                                        PVOID B,
                                        PVOID C,
                                        PVOID D)
{
    UNIMPLEMENTED;
}

