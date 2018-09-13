/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datasrv.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Server Performance data objects

Created:

    Bob Watson  22-Oct-1996

Revision History:

    None.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "datasrv.h"

// dummy variable for field sizing.
static SRV_COUNTER_DATA   scd;

//
//  Constant structure initializations 
//      defined in datasrv.h
//

SRV_DATA_DEFINITION SrvDataDefinition = {
    {   sizeof(SRV_DATA_DEFINITION) + sizeof(SRV_COUNTER_DATA),
        sizeof(SRV_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        SERVER_OBJECT_TITLE_INDEX,
        0,
        331,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(SRV_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        -1,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        388,
        0,
        395,
        0,
        -4,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_BULK_COUNT,
        sizeof (scd.TotalBytes),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->TotalBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        264,
        0,
        333,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof (scd.TotalBytesReceived),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->TotalBytesReceived
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        276,
        0,
        335,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof (scd.TotalBytesSent),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->TotalBytesSent
    },               
    {   sizeof(PERF_COUNTER_DEFINITION),
        340,
        0,
        341,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.SessionsTimedOut),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->SessionsTimedOut
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        342,
        0,
        343,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.SessionsErroredOut),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->SessionsErroredOut
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        344,
        0,
        345,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.SessionsLoggedOff),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->SessionsLoggedOff
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        346,
        0,
        347,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.SessionsForcedLogOff),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->SessionsForcedLogOff
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        348,
        0,
        349,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.LogonErrors),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->LogonErrors
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        350,
        0,
        351,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.AccessPermissionErrors),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->AccessPermissionErrors
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        352,
        0,
        353,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.GrantedAccessErrors),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->GrantedAccessErrors
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        354,
        0,
        355,
        0,
        0,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.SystemErrors),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->SystemErrors
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        356,
        0,
        357,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof (scd.BlockingSmbsRejected),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->BlockingSmbsRejected
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        358,
        0,
        359,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof (scd.WorkItemShortages),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->WorkItemShortages
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        360,
        0,
        361,
        0,
        -3,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.TotalFilesOpened),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->TotalFilesOpened
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        362,
        0,
        363,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.CurrentOpenFiles),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->CurrentOpenFiles
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        314,
        0,
        365,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.CurrentSessions),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->CurrentSessions
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        366,
        0,
        367,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.CurrentOpenSearches),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->CurrentOpenSearches
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        58,
        0,
        369,
        0,
        -4,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.CurrentNonPagedPoolUsage),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->CurrentNonPagedPoolUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        370,
        0,
        371,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof (scd.NonPagedPoolFailures),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->NonPagedPoolFailures
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        372,
        0,
        373,
        0,
        -4,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.PeakNonPagedPoolUsage),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->PeakNonPagedPoolUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        56,
        0,
        375,
        0,
        -4,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.CurrentPagedPoolUsage),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->CurrentPagedPoolUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        376,
        0,
        377,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.PagedPoolFailures),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->PagedPoolFailures
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        378,
        0,
        379,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.PeakPagedPoolUsage),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->PeakPagedPoolUsage
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        404,
        0,
        405,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (scd.ContextBlockQueueRate),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->ContextBlockQueueRate
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        692,
        0,
        693,
        0,
        1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof (scd.NetLogon),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->NetLogon
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1260,
        0,
        1261,
        0,
        1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof (scd.NetLogonTotal),
        (DWORD)(ULONG_PTR)&((PSRV_COUNTER_DATA)0)->NetLogonTotal
    }
};

