/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAPAGE.h

Abstract:

    Header file for the Windows NT Operating System Pagefile
    Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATAPAGE_H_
#define _DATAPAGE_H_

//
//  Pagefile performance object
//

typedef struct _PAGEFILE_DATA_DEFINITION {
    PERF_OBJECT_TYPE        PagefileObjectType;
    PERF_COUNTER_DEFINITION cdPercentInUse;
    PERF_COUNTER_DEFINITION cdPercentInUseBase;
    PERF_COUNTER_DEFINITION cdPeakUsage;
    PERF_COUNTER_DEFINITION cdPeakUsageBase;
} PAGEFILE_DATA_DEFINITION, * PPAGEFILE_DATA_DEFINITION;

typedef struct _PAGEFILE_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                       PercentInUse;
    DWORD                       PercentInUseBase;
    DWORD                       PeakUsage;
    DWORD                       PeakUsageBase;
} PAGEFILE_COUNTER_DATA, *PPAGEFILE_COUNTER_DATA;

extern PAGEFILE_DATA_DEFINITION  PagefileDataDefinition;

#endif //_DATAPAGE_H_


