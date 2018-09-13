/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datasrvq.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Physical Disk Server Queue data objects

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
#include "datasrvq.h"

// dummy variable for field sizing.
static SRVQ_COUNTER_DATA   sqcd;

SRVQ_DATA_DEFINITION SrvQDataDefinition = {
    {
        sizeof(SRVQ_DATA_DEFINITION) + sizeof(SRVQ_COUNTER_DATA),
        sizeof(SRVQ_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        SERVER_QUEUE_OBJECT_TITLE_INDEX,
        0,
        1301,
        0,
        PERF_DETAIL_ADVANCED,
        (sizeof(SRVQ_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
          sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1302,
        0,
        1303,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.QueueLength),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->QueueLength
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1304,
        0,
        1305,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.ActiveThreads),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->ActiveThreads
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1306,
        0,
        1307,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.AvailableThreads),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->AvailableThreads
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1308,
        0,
        1309,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.AvailableWorkItems),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->AvailableWorkItems
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1310,
        0,
        1311,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.BorrowedWorkItems),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->BorrowedWorkItems
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1312,
        0,
        1313,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.WorkItemShortages),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->WorkItemShortages
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1314,
        0,
        1315,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(sqcd.CurrentClients),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->CurrentClients
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        264,
        0,
        1317,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.BytesReceived),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->BytesReceived
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        506,
        0,
        1319,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.BytesSent),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->BytesSent
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1320,
        0,
        1321,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.TotalBytesTransfered),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->TotalBytesTransfered
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        288,
        0,
        1323,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.ReadOperations),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->ReadOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1324,
        0,
        1325,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.BytesRead),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->BytesRead
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        298,
        0,
        1327,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.WriteOperations),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->WriteOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1328,
        0,
        1329,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.BytesWritten),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->BytesWritten
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        508,
        0,
        1331,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.TotalBytes),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->TotalBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1332,
        0,
        1333,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(sqcd.TotalOperations),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->TotalOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        404,
        0,
        405,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(sqcd.TotalContextBlocksQueued),
        (DWORD)(ULONG_PTR)&((SRVQ_COUNTER_DATA *)0)->TotalContextBlocksQueued
    }
};

