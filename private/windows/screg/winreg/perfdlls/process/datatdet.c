/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datatdet.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Thread Detail Performance data objects

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
#include "datatdet.h"

// dummy variable for field sizing.
static THREAD_DETAILS_COUNTER_DATA  tdcd;

//
//  Constant structure initializations 
//      defined in datatdet.h
//

THREAD_DETAILS_DATA_DEFINITION ThreadDetailsDataDefinition =
{
    {
        0,
        sizeof (THREAD_DETAILS_DATA_DEFINITION),
        sizeof (PERF_OBJECT_TYPE),
        THREAD_DETAILS_OBJECT_TITLE_INDEX,
        0,
        (THREAD_DETAILS_OBJECT_TITLE_INDEX+1),
        0,
        PERF_DETAIL_WIZARD,
        (sizeof(THREAD_DETAILS_DATA_DEFINITION) - sizeof(PERF_OBJECT_TYPE)) /
            sizeof (PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L, 0L},
        {0L, 0L}
    },
    {
        sizeof (PERF_COUNTER_DEFINITION),
        708,
        0,
        709,
        0,
        0,
        PERF_DETAIL_WIZARD,
        PERF_COUNTER_LARGE_RAWCOUNT_HEX,
        sizeof (tdcd.UserPc),
        (DWORD)(ULONG_PTR)&((PTHREAD_DETAILS_COUNTER_DATA)0)->UserPc
    }
};

