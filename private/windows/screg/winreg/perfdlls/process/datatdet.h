/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATATDET.h

Abstract:

    Header file for the Windows NT Thread Detail Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATA_THREAD_DETAIL_H_
#define _DATA_THREAD_DETAIL_H_

//
//  thread detail performance definition structure
//
typedef struct _THREAD_DETAILS_DATA_DEFINITION {
    PERF_OBJECT_TYPE        ThreadDetailsObjectType;
    PERF_COUNTER_DEFINITION UserPc;
} THREAD_DETAILS_DATA_DEFINITION, *PTHREAD_DETAILS_DATA_DEFINITION;

typedef struct _THREAD_DETAILS_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   Reserved;
    LONGLONG		    UserPc;
} THREAD_DETAILS_COUNTER_DATA, * PTHREAD_DETAILS_COUNTER_DATA;

extern THREAD_DETAILS_DATA_DEFINITION ThreadDetailsDataDefinition;

#endif // _DATA_THREAD_DETAIL_H_

