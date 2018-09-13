/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datathrd.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Thread Performance data objects

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
#include "datathrd.h"

// dummy variable for field sizing.
static THREAD_COUNTER_DATA   tcd;

//
//  Constant structure initializations 
//      defined in datathrd.h
//
THREAD_DATA_DEFINITION ThreadDataDefinition = {
    {   0,
        sizeof(THREAD_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        THREAD_OBJECT_TITLE_INDEX,
        0,
        233,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(THREAD_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
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
        191,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_100NSEC_TIMER,
        sizeof (tcd.ProcessorTime),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ProcessorTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        142,
        0,
        193,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof (tcd.UserTime),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->UserTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        144,
        0,
        195,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_100NSEC_TIMER,
        sizeof (tcd.KernelTime),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->KernelTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        146,
        0,
        197,
        0,
        -2,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof (tcd.ContextSwitches),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ContextSwitches
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        684,
        0,
        699,
        0,
        -4,
        PERF_DETAIL_ADVANCED,
        PERF_ELAPSED_TIME,
        sizeof (tcd.ThreadElapsedTime),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadElapsedTime
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        694,
        0,
        695,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.ThreadPriority),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadPriority
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        682,
        0,
        697,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.ThreadBasePriority),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadBasePriority
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        706,
        0,
        707,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT_HEX,
        sizeof (tcd.ThreadStartAddr),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadStartAddr
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        46,
        0,
        47,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.ThreadState),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadState
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        336,
        0,
        337,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.WaitReason),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->WaitReason
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        784,
        0,
        785,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.ProcessId),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ProcessId
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        804,
        0,
        805,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_RAWCOUNT,
        sizeof (tcd.ThreadId),
        (DWORD)(ULONG_PTR)&((PTHREAD_COUNTER_DATA)0)->ThreadId
    }
};

