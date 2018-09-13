/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dataproc.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Process Performance data objects

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
#include "dataproc.h"

// dummy variable for field sizing.
static PROCESS_COUNTER_DATA   pcd;

//
//  Constant structure initializations 
//      defined in dataproc.h
//

PROCESS_DATA_DEFINITION ProcessDataDefinition = {

    {   0,  // depends on number of instanced found
        sizeof(PROCESS_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        PROCESS_OBJECT_TITLE_INDEX,
        0,
        231,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(PROCESS_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {10000000L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        6,
        0,
        189,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_100NSEC_TIMER,
        sizeof(pcd.ProcessorTime),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        142,
        0,
        157,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.UserTime),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->UserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        144,
        0,
        159,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof(pcd.KernelTime),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->KernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        172,
        0,
        173,
        0,
        -6,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(pcd.PeakVirtualSize),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PeakVirtualSize
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        174,
        0,
        175,
        0,
        -6,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(pcd.VirtualSize),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->VirtualSize
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        28,
        0,
        177,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(pcd.PageFaults),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PageFaults
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        178,
        0,
        179,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.PeakWorkingSet),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PeakWorkingSet
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        180,
        0,
        181,
        0,
        -5,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.TotalWorkingSet),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->TotalWorkingSet
    },
#ifdef _DATAPROC_PRIVATE_WS_
    {   sizeof(PERF_COUNTER_DEFINITION),
        1478,
        0,
        1479,
        0,
        -5,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.PrivateWorkingSet),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PrivateWorkingSet
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1480,
        0,
        1481,
        0,
        -5,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.SharedWorkingSet),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->SharedWorkingSet
    },
#endif
    {   sizeof(PERF_COUNTER_DEFINITION),
        182,
        0,
        183,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(pcd.PeakPageFile),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PeakPageFile
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        184,
        0,
        185,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(pcd.PageFile),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PageFile
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        186,
        0,
        187,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(pcd.PrivatePages),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PrivatePages
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        680,
        0,
        681,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.ThreadCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ThreadCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        682,
        0,
        683,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.BasePriority),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->BasePriority
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        684,
        0,
        685,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_ELAPSED_TIME,
        sizeof(pcd.ElapsedTime),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ElapsedTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        784,
        0,
        785,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.ProcessId),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ProcessId
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1410,
        0,
        1411,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.CreatorProcessId),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->CreatorProcessId
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        56,
        0,
        57,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.PagedPool),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->PagedPool
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        58,
        0,
        59,
        0,
        -5,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.NonPagedPool),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->NonPagedPool
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        952,
        0,
        953,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(pcd.HandleCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->HandleCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1412,
        0,
        1413,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.ReadOperationCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ReadOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1414,
        0,
        1415,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.WriteOperationCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->WriteOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1416,
        0,
        1417,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.DataOperationCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->DataOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1418,
        0,
        1419,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.OtherOperationCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->OtherOperationCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1420,
        0,
        1421,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.ReadTransferCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->ReadTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1422,
        0,
        1423,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.WriteTransferCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->WriteTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1424,
        0,
        1425,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.DataTransferCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->DataTransferCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1426,
        0,
        1427,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(pcd.OtherTransferCount),
        (DWORD)(ULONG_PTR)&((PPROCESS_COUNTER_DATA)0)->OtherTransferCount
    }
};