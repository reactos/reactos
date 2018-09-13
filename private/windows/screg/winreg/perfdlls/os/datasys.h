/*++ 

Copyright (c) 1995-6 Microsoft Corporation

Module Name:

      DATASYS.h

Abstract:

    Header file for the Windows NT Operating System Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATASYS_H_
#define _DATASYS_H_

//
//  System data object
//

typedef struct _SYSTEM_DATA_DEFINITION {
    PERF_OBJECT_TYPE		    SystemObjectType;
    PERF_COUNTER_DEFINITION     cdReadOperations;
    PERF_COUNTER_DEFINITION     cdWriteOperations;
    PERF_COUNTER_DEFINITION     cdOtherIOOperations;
    PERF_COUNTER_DEFINITION     cdReadBytes;
    PERF_COUNTER_DEFINITION     cdWriteBytes;
    PERF_COUNTER_DEFINITION     cdOtherIOBytes;
    PERF_COUNTER_DEFINITION     cdContextSwitches;
    PERF_COUNTER_DEFINITION     cdSystemCalls;
    PERF_COUNTER_DEFINITION     cdTotalReadWrites;
    PERF_COUNTER_DEFINITION     cdSystemElapsedTime;
    PERF_COUNTER_DEFINITION     cdProcessorQueueLength;
    PERF_COUNTER_DEFINITION     cdProcessCount;
    PERF_COUNTER_DEFINITION     cdThreadCount;
    PERF_COUNTER_DEFINITION     cdAlignmentFixups;
    PERF_COUNTER_DEFINITION     cdExceptionDispatches;
    PERF_COUNTER_DEFINITION     cdFloatingPointEmulations;
    PERF_COUNTER_DEFINITION     cdRegistryQuotaUsed;
    PERF_COUNTER_DEFINITION     cdRegistryQuotaAllowed;
} SYSTEM_DATA_DEFINITION, * PSYSTEM_DATA_DEFINITION;

typedef struct _SYSTEM_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                       ReadOperations;
    DWORD                       WriteOperations; 
    DWORD                       OtherIOOperations;
    LONGLONG                    ReadBytes;
    LONGLONG                    WriteBytes;
    LONGLONG                    OtherIOBytes;
    DWORD                       ContextSwitches;
    DWORD                       SystemCalls;
    DWORD                       TotalReadWrites;
    LONGLONG                    SystemElapsedTime;
    DWORD                       ProcessorQueueLength;
    DWORD                       ProcessCount;
    DWORD                       ThreadCount;
    DWORD                       AlignmentFixups;
    DWORD                       ExceptionDispatches;
    DWORD                       FloatingPointEmulations;
    DWORD                       RegistryQuotaUsed;
    DWORD                       RegistryQuotaAllowed;
} SYSTEM_COUNTER_DATA, * PSYSTEM_COUNTER_DATA;

extern SYSTEM_DATA_DEFINITION SystemDataDefinition;

#endif // _DATASYS_H_

