/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATATHRD.h

Abstract:

    Header file for the Windows NT Thread Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATATHRD_H_
#define _DATATHRD_H_

//
//  This is the counter structure presently returned by NT.  The
//  Performance Monitor MUST *NOT* USE THESE STRUCTURES!
//

typedef struct _THREAD_DATA_DEFINITION {
    PERF_OBJECT_TYPE		ThreadObjectType;
    PERF_COUNTER_DEFINITION	ProcessorTime;
    PERF_COUNTER_DEFINITION	UserTime;
    PERF_COUNTER_DEFINITION	KernelTime;
    PERF_COUNTER_DEFINITION	ContextSwitches;
    PERF_COUNTER_DEFINITION ThreadElapsedTime;
    PERF_COUNTER_DEFINITION ThreadPriority;
    PERF_COUNTER_DEFINITION ThreadBasePriority;
    PERF_COUNTER_DEFINITION ThreadStartAddr;
    PERF_COUNTER_DEFINITION ThreadState;
    PERF_COUNTER_DEFINITION WaitReason;
    PERF_COUNTER_DEFINITION ProcessId;
    PERF_COUNTER_DEFINITION ThreadId;
} THREAD_DATA_DEFINITION;


typedef struct _THREAD_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    LONGLONG        	    ProcessorTime;
    LONGLONG        	    UserTime;
    LONGLONG        	    KernelTime;
    DWORD                  	ContextSwitches;
    LONGLONG                ThreadElapsedTime;
    DWORD                   ThreadPriority;
    DWORD                   ThreadBasePriority;
    LPVOID                  ThreadStartAddr;
    DWORD                   ThreadState;
    DWORD                   WaitReason;
    DWORD                   ProcessId;
    DWORD                   ThreadId;
} THREAD_COUNTER_DATA, * PTHREAD_COUNTER_DATA;

extern  THREAD_DATA_DEFINITION ThreadDataDefinition;

#endif // _DATATHRD_H_


