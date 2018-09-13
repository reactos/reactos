/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datasys.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Operating System performance data objects

    This file contains a set of constant data structures which are
    currently defined for the Signal Generator Perf DLL.

Created:

    Bob Watson  20-Oct-1996

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
#include "datasys.h"

// dummy variable for field sizing.
static SYSTEM_COUNTER_DATA scd;

//
//  Constant structure initializations 
//      defined in datasys.h
//

SYSTEM_DATA_DEFINITION SystemDataDefinition = {
    {   sizeof(SYSTEM_DATA_DEFINITION) + sizeof(SYSTEM_COUNTER_DATA),
        sizeof(SYSTEM_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        SYSTEM_OBJECT_TITLE_INDEX,
        0,
        3,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(SYSTEM_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
         sizeof(PERF_COUNTER_DEFINITION),
        8,       // Default: TOTAL_PROCESSOR_TIME
        -1,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {10000000L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        10,
        0,
        11,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(scd.ReadOperations),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ReadOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        12,
        0,
        13,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(scd.WriteOperations),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->WriteOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        14,
        0,
        15,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(scd.OtherIOOperations),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->OtherIOOperations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        16,
        0,
        17,
        0,
        -4,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_BULK_COUNT,
        sizeof(scd.ReadBytes),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ReadBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        18,
        0,
        19,
        0,
        -4,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_BULK_COUNT,
        sizeof(scd.WriteBytes),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->WriteBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        20,
        0,
        21,
        0,
        -3,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_BULK_COUNT,
        sizeof(scd.OtherIOBytes),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->OtherIOBytes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        146,
        0,
        147,
        0,
        -2,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(scd.ContextSwitches),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ContextSwitches
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        150,
        0,
        151,
        0,
        -1,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(scd.SystemCalls),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->SystemCalls
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        406,
        0,
        407,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_COUNTER,
        sizeof(scd.TotalReadWrites),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->TotalReadWrites
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        674,
        0,
        675,
        0,
        -5,
        PERF_DETAIL_NOVICE,
        PERF_ELAPSED_TIME,
        sizeof(scd.SystemElapsedTime),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->SystemElapsedTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        44,
        0,
        45,
        0,
        1,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof(scd.ProcessorQueueLength),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ProcessorQueueLength
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        248,
        0,
        249,
        0,
        1,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof(scd.ProcessCount),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ProcessCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        250,
        0,
        251,
        0,
        1,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof(scd.ThreadCount),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ThreadCount
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        686,
        0,
        687,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof(scd.AlignmentFixups),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->AlignmentFixups
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        688,
        0,
        689,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof(scd.ExceptionDispatches),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->ExceptionDispatches
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        690,
        0,
        691,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_COUNTER,
        sizeof(scd.FloatingPointEmulations),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->FloatingPointEmulations
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1350,
        0,
        1351,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(scd.RegistryQuotaUsed),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->RegistryQuotaUsed
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        1350,
        0,
        1351,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(scd.RegistryQuotaAllowed),
        (DWORD)(ULONG_PTR)&((PSYSTEM_COUNTER_DATA)0)->RegistryQuotaAllowed
    }
};

