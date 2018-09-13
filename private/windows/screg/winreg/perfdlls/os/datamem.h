/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

      DATAMEM.h

Abstract:

    Header file for the Windows NT Memory Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATAMEM_H_
#define _DATAMEM_H_

//
//  Memory Performance Counter
//

typedef struct _MEMORY_DATA_DEFINITION {
    PERF_OBJECT_TYPE		    MemoryObjectType;
    PERF_COUNTER_DEFINITION	    cdPageFaults;
    PERF_COUNTER_DEFINITION     cdAvailablePages;
    PERF_COUNTER_DEFINITION	    cdCommittedPages;
    PERF_COUNTER_DEFINITION     cdCommitList;
    PERF_COUNTER_DEFINITION	    cdWriteCopies;
    PERF_COUNTER_DEFINITION	    cdTransitionFaults;
    PERF_COUNTER_DEFINITION     cdCacheFaults;
    PERF_COUNTER_DEFINITION	    cdDemandZeroFaults;
    PERF_COUNTER_DEFINITION     cdPages;
    PERF_COUNTER_DEFINITION	    cdPagesInput;
    PERF_COUNTER_DEFINITION     cdPageReads;
    PERF_COUNTER_DEFINITION	    cdDirtyPages;
    PERF_COUNTER_DEFINITION     cdPagedPool;
    PERF_COUNTER_DEFINITION	    cdNonPagedPool;
    PERF_COUNTER_DEFINITION	    cdDirtyWrites;
    PERF_COUNTER_DEFINITION	    cdPagedPoolAllocs;
    PERF_COUNTER_DEFINITION	    cdNonPagedPoolAllocs;
    PERF_COUNTER_DEFINITION     cdFreeSystemPtes;
    PERF_COUNTER_DEFINITION     cdCacheBytes;
    PERF_COUNTER_DEFINITION     cdPeakCacheBytes;
    PERF_COUNTER_DEFINITION     cdResidentPagedPoolBytes;
    PERF_COUNTER_DEFINITION     cdTotalSysCodeBytes;
    PERF_COUNTER_DEFINITION     cdResidentSysCodeBytes;
    PERF_COUNTER_DEFINITION     cdTotalSsysDriverBytes;
    PERF_COUNTER_DEFINITION     cdResidentSysDriverBytes;
    PERF_COUNTER_DEFINITION     cdResidentSysCacheBytes;
    PERF_COUNTER_DEFINITION     cdCommitBytesInUse;
    PERF_COUNTER_DEFINITION     cdCommitBytesLimit;
    PERF_COUNTER_DEFINITION     cdAvailableKBytes;
    PERF_COUNTER_DEFINITION     cdAvailableMBytes;
//    PERF_COUNTER_DEFINITION	    cdSystemVlmCommitCharge;
//    PERF_COUNTER_DEFINITION	    cdSystemVlmPeakCommitCharge;
//    PERF_COUNTER_DEFINITION	    cdSystemVlmSharedCommitCharge;
} MEMORY_DATA_DEFINITION, * PMEMORY_DATA_DEFINITION;

typedef struct _MEMORY_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                       PageFaults;
    LONGLONG                    AvailablePages;
    LONGLONG                    CommittedPages;
    LONGLONG                    CommitList;
    DWORD                       WriteCopies;
    DWORD                       TransitionFaults;
    DWORD                       CacheFaults;
    DWORD                       DemandZeroFaults;
    DWORD                       Pages;
    DWORD                       PagesInput;
    DWORD                       PageReads;
    DWORD                       DirtyPages;
    LONGLONG                    PagedPool;
    LONGLONG                    NonPagedPool;
    DWORD                       DirtyWrites;
    DWORD                       PagedPoolAllocs;
    DWORD                       NonPagedPoolAllocs;
    DWORD                       FreeSystemPtes;
    LONGLONG                    CacheBytes;
    LONGLONG                    PeakCacheBytes;
    LONGLONG                    ResidentPagedPoolBytes;
    LONGLONG                    TotalSysCodeBytes;
    LONGLONG                    ResidentSysCodeBytes;
    LONGLONG                    TotalSysDriverBytes;
    LONGLONG                    ResidentSysDriverBytes;
    LONGLONG                    ResidentSysCacheBytes;
    DWORD                       CommitBytesInUse;
    DWORD                       CommitBytesLimit;
    LONGLONG                    AvailableKBytes;
    LONGLONG                    AvailableMBytes;
//    LONGLONG                    SystemVlmCommitCharge;
//    LONGLONG                    SystemVlmPeakCommitCharge;
//    LONGLONG                    SystemVlmSharedCommitCharge;
} MEMORY_COUNTER_DATA, *PMEMORY_COUNTER_DATA;

extern MEMORY_DATA_DEFINITION MemoryDataDefinition;

#endif //_DATAMEM_H_


