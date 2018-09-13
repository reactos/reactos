/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    datapage.c

Abstract:
       
    a file containing the constant data structures used by the Performance
    Monitor data for the Page file performance data objects

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
#include "datapage.h"

// dummy variable for field sizing.
static PAGEFILE_COUNTER_DATA    pcd;

//
//  Constant structure initializations 
//      defined in datapage.h
//

PAGEFILE_DATA_DEFINITION  PagefileDataDefinition = {
    {   sizeof (PAGEFILE_DATA_DEFINITION) +  sizeof(PAGEFILE_COUNTER_DATA),
        sizeof (PAGEFILE_DATA_DEFINITION),
        sizeof (PERF_OBJECT_TYPE),
        PAGEFILE_OBJECT_TITLE_INDEX,
        0,
        701,
        0,
        PERF_DETAIL_ADVANCED,
        (sizeof(PAGEFILE_DATA_DEFINITION) - sizeof (PERF_OBJECT_TYPE))/
        sizeof(PERF_COUNTER_DEFINITION),
        0,
        0,
        UNICODE_CODE_PAGE,
        {0L,0L},
        {0L,0L}        
    },
    {   sizeof (PERF_COUNTER_DEFINITION),
        702,
        0,
        703,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof (pcd.PercentInUse),
        (DWORD)(ULONG_PTR)&((PPAGEFILE_COUNTER_DATA)0)->PercentInUse
    },
    {   sizeof (PERF_COUNTER_DEFINITION),
        702,
        0,
        703,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof (pcd.PercentInUseBase),
        (DWORD)(ULONG_PTR)&((PPAGEFILE_COUNTER_DATA)0)->PercentInUseBase
    },
    {   sizeof (PERF_COUNTER_DEFINITION),
        704,
        0,
        705,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof (pcd.PeakUsage),
        (DWORD)(ULONG_PTR)&((PPAGEFILE_COUNTER_DATA)0)->PeakUsage
    },
    {   sizeof (PERF_COUNTER_DEFINITION),
        704,
        0,
        705,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof (pcd.PeakUsageBase),
        (DWORD)(ULONG_PTR)&((PPAGEFILE_COUNTER_DATA)0)->PeakUsageBase
    }
};

