/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dataobj.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the OS Object performance data objects

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
#include "dataobj.h"

// dummy variable for field sizing.
static OBJECTS_COUNTER_DATA ocd;

//
//  Constant structure initializations 
//      defined in dataobj.h
//
OBJECTS_DATA_DEFINITION ObjectsDataDefinition = {

    {   sizeof(OBJECTS_DATA_DEFINITION) + sizeof(OBJECTS_COUNTER_DATA),
        sizeof(OBJECTS_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        OBJECT_OBJECT_TITLE_INDEX,
        0,
        261,
        0,
        PERF_DETAIL_NOVICE,
        (sizeof(OBJECTS_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        -1,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        248,
        0,
        249,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Processes),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Processes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        250,
        0,
        251,
        0,
        -1,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Threads),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Threads
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        252,
        0,
        253,
        0,
        -1,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Events),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Events
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        254,
        0,
        255,
        0,
        -1,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Semaphores),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Semaphores
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        256,
        0,
        257,
        0,
        0,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Mutexes),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Mutexes
    },
    {   sizeof(PERF_COUNTER_DEFINITION),
        258,
        0,
        259,
        0,
        -1,
        PERF_DETAIL_EXPERT,
        PERF_COUNTER_RAWCOUNT,
        sizeof(ocd.Sections),
        (DWORD)(ULONG_PTR)&((POBJECTS_COUNTER_DATA)0)->Sections
    }
};

