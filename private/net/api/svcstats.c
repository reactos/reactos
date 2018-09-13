/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SvcStats.c

Abstract:

    Contains Net statistics routines for net DLL:

        NetStatisticsGet

Author:

    Richard L Firth (rfirth) 12-05-1991

Revision History:

    21-Jan-1992 rfirth
        Call wrapper routines, not rpc client-side stubs

    12-05-1991 rfirth
        Created

--*/

#include <windows.h>
#include <lmcons.h>
#include <lmstats.h>
#include <lmsname.h>
#include <tstring.h>
#include <netstats.h>   // private statistics routines in server and wksta services



NET_API_STATUS
NET_API_FUNCTION
NetStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  LPTSTR  Service,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Returns statistics to the caller from the specified service. Only SERVER
    and WORKSTATION are currently supported.

Arguments:

    ServerName  - where to run this API
    Service     - name of service to get stats from
    Level       - of information required. MBZ
    Options     - various flags. Currently, only bit 0 (clear) is supported
    Buffer      - pointer to pointer to returned buffer

Return Value:

    NET_API_STATUS
        Success - NERR_Success

        Failure - ERROR_INVALID_LEVEL
                    Level not 0

                  ERROR_INVALID_PARAMETER
                    Unsupported options requested

                  ERROR_NOT_SUPPORTED
                    Service is not SERVER or WORKSTATION

                  ERROR_ACCESS_DENIED
                    Caller doesn't have necessary access rights for request

--*/

{
    //
    // set the caller's buffer pointer to known value. This will kill the
    // calling app if it gave us a bad pointer and didn't use try...except
    //

    *Buffer = NULL;

    //
    // leave other parameter validation to specific stats function
    //

    if (!STRICMP(Service, SERVICE_WORKSTATION)) {
        return NetWkstaStatisticsGet(ServerName, Level, Options, Buffer);
    } else if (!STRICMP(Service, SERVICE_SERVER)) {
        return NetServerStatisticsGet(ServerName, Level, Options, Buffer);
    } else {
        return ERROR_NOT_SUPPORTED;
    }
}
