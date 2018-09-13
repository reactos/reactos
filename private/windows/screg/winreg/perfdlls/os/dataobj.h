/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAOBJ.h

Abstract:

    Header file for the Windows NT OS Objects Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATAOBJ_H_
#define _DATAOBJ_H_

//
//  Objects Performance Data Object
//

typedef struct _OBJECTS_DATA_DEFINITION {
    PERF_OBJECT_TYPE            ObjectsObjectType;
    PERF_COUNTER_DEFINITION     cdProcesses;
    PERF_COUNTER_DEFINITION     cdThreads;
    PERF_COUNTER_DEFINITION     cdEvents;
    PERF_COUNTER_DEFINITION     cdSemaphores;
    PERF_COUNTER_DEFINITION     cdMutexes;
    PERF_COUNTER_DEFINITION     cdSections;
} OBJECTS_DATA_DEFINITION, * POBJECTS_DATA_DEFINITION;

typedef struct _OBJECTS_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                       Processes;
    DWORD                       Threads;
    DWORD                       Events;
    DWORD                       Semaphores;
    DWORD                       Mutexes;
    DWORD                       Sections;
} OBJECTS_COUNTER_DATA, * POBJECTS_COUNTER_DATA;


extern OBJECTS_DATA_DEFINITION ObjectsDataDefinition;

#endif // _DATAOBJ_H_

