/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATACPU.h

Abstract:

    Header file for the Windows NT Processor Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATACPU_H_
#define _DATACPU_H_

//
//  Processor data object
//

typedef struct _PROCESSOR_DATA_DEFINITION {
    PERF_OBJECT_TYPE		ProcessorObjectType;
    PERF_COUNTER_DEFINITION	cdProcessorTime;
    PERF_COUNTER_DEFINITION	cdUserTime;
    PERF_COUNTER_DEFINITION	cdKernelTime;
    PERF_COUNTER_DEFINITION	cdInterrupts;
    PERF_COUNTER_DEFINITION	cdDpcTime;
    PERF_COUNTER_DEFINITION	cdInterruptTime;
    PERF_COUNTER_DEFINITION cdDpcCountRate;
    PERF_COUNTER_DEFINITION cdDpcRate;
    PERF_COUNTER_DEFINITION cdDpcBypassRate;
    PERF_COUNTER_DEFINITION cdApcBypassRate;
} PROCESSOR_DATA_DEFINITION, *PPROCESSOR_DATA_DEFINITION;

typedef struct _PROCESSOR_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    LONGLONG                ProcessorTime;
    LONGLONG                UserTime;
    LONGLONG                KernelTime;
    DWORD                   Interrupts;
    LONGLONG                DpcTime;
    LONGLONG                InterruptTime;
    DWORD                   DpcCountRate;
    DWORD                   DpcRate;
    DWORD                   DpcBypassRate;
    DWORD                   ApcBypassRate;
} PROCESSOR_COUNTER_DATA, *PPROCESSOR_COUNTER_DATA;

extern PROCESSOR_DATA_DEFINITION ProcessorDataDefinition;

#endif // _DATACPU_H_

