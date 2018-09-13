/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAPHYS.h

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

#ifndef _DATAPHYS_H_
#define _DATAPHYS_H_

//
//  physical disk performance definition structure
//

typedef struct _PDISK_DATA_DEFINITION {
    PERF_OBJECT_TYPE            DiskObjectType;
    PERF_COUNTER_DEFINITION     cdDiskCurrentQueueLength;
    PERF_COUNTER_DEFINITION     cdDiskTime;
    PERF_COUNTER_DEFINITION     cdDiskTimeTimeStamp;
    PERF_COUNTER_DEFINITION     cdDiskAvgQueueLength;
    PERF_COUNTER_DEFINITION     cdDiskReadTime;
    PERF_COUNTER_DEFINITION     cdDiskReadTimeTimeStamp;
    PERF_COUNTER_DEFINITION     cdDiskReadQueueLength;
    PERF_COUNTER_DEFINITION     cdDiskWriteTime;
    PERF_COUNTER_DEFINITION     cdDiskWriteTimeTimeStamp;
    PERF_COUNTER_DEFINITION     cdDiskWriteQueueLength;
    PERF_COUNTER_DEFINITION     cdDiskAvgTime;
    PERF_COUNTER_DEFINITION     cdDiskTransfersBase1;
    PERF_COUNTER_DEFINITION     cdDiskAvgReadTime;
    PERF_COUNTER_DEFINITION     cdDiskReadsBase1;
    PERF_COUNTER_DEFINITION     cdDiskAvgWriteTime;
    PERF_COUNTER_DEFINITION     cdDiskWritesBase1;
    PERF_COUNTER_DEFINITION     cdDiskTransfers;
    PERF_COUNTER_DEFINITION     cdDiskReads;
    PERF_COUNTER_DEFINITION     cdDiskWrites;
    PERF_COUNTER_DEFINITION     cdDiskBytes;
    PERF_COUNTER_DEFINITION     cdDiskReadBytes;
    PERF_COUNTER_DEFINITION     cdDiskWriteBytes;
    PERF_COUNTER_DEFINITION     cdDiskAvgBytes;
    PERF_COUNTER_DEFINITION     cdDiskTransfersBase2;
    PERF_COUNTER_DEFINITION     cdDiskAvgReadBytes;
    PERF_COUNTER_DEFINITION     cdDiskReadsBase2;
    PERF_COUNTER_DEFINITION     cdDiskAvgWriteBytes;
    PERF_COUNTER_DEFINITION     cdDiskWritesBase2;
    PERF_COUNTER_DEFINITION     cdIdleTime;
    PERF_COUNTER_DEFINITION     cdIdleTimeTimeStamp;
    PERF_COUNTER_DEFINITION     cdSplitCount;
} PDISK_DATA_DEFINITION, * PPDISK_DATA_DEFINITION;



typedef struct _PDISK_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   DiskCurrentQueueLength;
    LONGLONG                DiskTime;
    LONGLONG                DiskAvgQueueLength;
    LONGLONG                DiskReadTime;
    LONGLONG                DiskReadQueueLength;
    LONGLONG                DiskWriteTime;
    LONGLONG                DiskWriteQueueLength;
    LONGLONG                DiskAvgTime;
    LONGLONG                DiskAvgReadTime;
    DWORD                   DiskTransfersBase1;
    DWORD                   DiskReadsBase1;
    LONGLONG                DiskAvgWriteTime;
    DWORD                   DiskWritesBase1;
    DWORD                   DiskTransfers;
    DWORD                   DiskReads;
    DWORD                   DiskWrites;
    LONGLONG                DiskBytes;
    LONGLONG                DiskReadBytes;
    LONGLONG                DiskWriteBytes;
    LONGLONG                DiskAvgBytes;
    LONGLONG                DiskAvgReadBytes;
    DWORD                   DiskTransfersBase2;
    DWORD                   DiskReadsBase2;
    LONGLONG                DiskAvgWriteBytes;
    LONGLONG                IdleTime;
    LONGLONG                DiskTimeTimeStamp;
    DWORD                   DiskWritesBase2;
    DWORD                   SplitCount;
} PDISK_COUNTER_DATA, * PPDISK_COUNTER_DATA;

extern PDISK_DATA_DEFINITION PhysicalDiskDataDefinition;

#endif // _DATAPHYS_H_
