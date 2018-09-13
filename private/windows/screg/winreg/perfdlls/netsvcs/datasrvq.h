/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATASRVQ.h

Abstract:

    Header file for the Windows NT Processor Server Queue counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATASRVQ_H_
#define _DATASRVQ_H_

//
//  define for Server Queue Statistics
//

typedef struct _SRVQ_DATA_DEFINITION {
    PERF_OBJECT_TYPE        SrvQueueObjectType;
    PERF_COUNTER_DEFINITION cdQueueLength;
    PERF_COUNTER_DEFINITION cdActiveThreads;
    PERF_COUNTER_DEFINITION cdAvailableThreads;
    PERF_COUNTER_DEFINITION cdAvailableWorkItems;
    PERF_COUNTER_DEFINITION cdBorrowedWorkItems;
    PERF_COUNTER_DEFINITION cdWorkItemShortages;
    PERF_COUNTER_DEFINITION cdCurrentClients;
    PERF_COUNTER_DEFINITION cdBytesReceived;
    PERF_COUNTER_DEFINITION cdBytesSent;
    PERF_COUNTER_DEFINITION cdTotalBytesTransfered;
    PERF_COUNTER_DEFINITION cdReadOperations;
    PERF_COUNTER_DEFINITION cdBytesRead;
    PERF_COUNTER_DEFINITION cdWriteOperations;
    PERF_COUNTER_DEFINITION cdBytesWritten;
    PERF_COUNTER_DEFINITION cdTotalBytes;
    PERF_COUNTER_DEFINITION cdTotalOperations;
    PERF_COUNTER_DEFINITION cdTotalContextBlocksQueued;
} SRVQ_DATA_DEFINITION, * PSRVQ_DATA_DEFINITION;


typedef struct _SRVQ_COUNTER_DATA{
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   QueueLength;
    DWORD                   ActiveThreads;
    DWORD                   AvailableThreads;
    DWORD                   AvailableWorkItems;
    DWORD                   BorrowedWorkItems;
    DWORD                   WorkItemShortages;
    DWORD                   CurrentClients;
    LONGLONG                BytesReceived;
    LONGLONG                BytesSent;
    LONGLONG                TotalBytesTransfered;
    LONGLONG                ReadOperations;
    LONGLONG                BytesRead;
    LONGLONG                WriteOperations;
    LONGLONG                BytesWritten;
    LONGLONG                TotalBytes;
    LONGLONG                TotalOperations;
    DWORD                   TotalContextBlocksQueued;
} SRVQ_COUNTER_DATA, * PSRVQ_COUNTER_DATA;

extern SRVQ_DATA_DEFINITION SrvQDataDefinition;

#endif // _DATASRVQ_H_
