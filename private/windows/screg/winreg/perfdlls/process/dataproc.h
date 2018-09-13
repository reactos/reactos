/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAPROC.h

Abstract:

    Header file for the Windows NT Processor Process counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/
#ifndef _DATAPROC_H_
#define _DATAPROC_H_

#ifdef	_DATAPROC_PRIVATE_WS_
#undef	_DATAPROC_PRIVATE_WS_
#endif

//
//  Process data object definitions.
//
//
//  This is the counter structure presently returned by NT.  The
//  Performance Monitor MUST *NOT* USE THESE STRUCTURES!
//

typedef struct _PROCESS_DATA_DEFINITION {
    PERF_OBJECT_TYPE		    ProcessObjectType;
    PERF_COUNTER_DEFINITION	    cdProcessorTime;
    PERF_COUNTER_DEFINITION	    cdUserTime;
    PERF_COUNTER_DEFINITION	    cdKernelTime;
    PERF_COUNTER_DEFINITION	    cdPeakVirtualSize;
    PERF_COUNTER_DEFINITION	    cdVirtualSize;
    PERF_COUNTER_DEFINITION	    cdPageFaults;
    PERF_COUNTER_DEFINITION	    cdPeakWorkingSet;
    PERF_COUNTER_DEFINITION	    cdTotalWorkingSet;
#ifdef _DATAPROC_PRIVATE_WS_
	PERF_COUNTER_DEFINITION		cdPrivateWorkingSet;
	PERF_COUNTER_DEFINITION		cdSharedWorkingSet;
#endif
	PERF_COUNTER_DEFINITION	    cdPeakPageFile;
    PERF_COUNTER_DEFINITION	    cdPageFile;
    PERF_COUNTER_DEFINITION	    cdPrivatePages;
    PERF_COUNTER_DEFINITION     cdThreadCount;
    PERF_COUNTER_DEFINITION     cdBasePriority;
    PERF_COUNTER_DEFINITION     cdElapsedTime;
    PERF_COUNTER_DEFINITION     cdProcessId;
    PERF_COUNTER_DEFINITION     cdCreatorProcessId;
    PERF_COUNTER_DEFINITION     cdPagedPool;
    PERF_COUNTER_DEFINITION     cdNonPagedPool;
    PERF_COUNTER_DEFINITION     cdHandleCount;
    PERF_COUNTER_DEFINITION     cdReadOperationCount;
    PERF_COUNTER_DEFINITION     cdWriteOperationCount;
    PERF_COUNTER_DEFINITION     cdDataOperationCount;
    PERF_COUNTER_DEFINITION     cdOtherOperationCount;
    PERF_COUNTER_DEFINITION     cdReadTransferCount;
    PERF_COUNTER_DEFINITION     cdWriteTransferCount;
    PERF_COUNTER_DEFINITION     cdDataTransferCount;
    PERF_COUNTER_DEFINITION     cdOtherTransferCount;
} PROCESS_DATA_DEFINITION, * PPROCESS_DATA_DEFINITION;

typedef struct _PROCESS_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                  	    PageFaults;
    LONGLONG                    ProcessorTime;
    LONGLONG                    UserTime;
    LONGLONG                    KernelTime;
    LONGLONG              	    PeakVirtualSize;
    LONGLONG                    VirtualSize;
    DWORD                  	    PeakWorkingSet;
    DWORD                  	    TotalWorkingSet;
#ifdef _DATAPROC_PRIVATE_WS_
	DWORD						PrivateWorkingSet;
	DWORD						SharedWorkingSet;
#endif
    LONGLONG                    PeakPageFile;
    LONGLONG                    PageFile;
    LONGLONG                    PrivatePages;
    DWORD                       ThreadCount;
    DWORD                       BasePriority;
    LONGLONG                    ElapsedTime;
    DWORD                       ProcessId;
    DWORD                       CreatorProcessId;
    DWORD                       PagedPool;
    DWORD                       NonPagedPool;
    DWORD                       HandleCount;
    DWORD                       Reserved;   // for alignment
    LONGLONG                    ReadOperationCount;
    LONGLONG                    WriteOperationCount;
    LONGLONG                    DataOperationCount;
    LONGLONG                    OtherOperationCount;
    LONGLONG                    ReadTransferCount;
    LONGLONG                    WriteTransferCount;
    LONGLONG                    DataTransferCount;
    LONGLONG                    OtherTransferCount;
} PROCESS_COUNTER_DATA, * PPROCESS_COUNTER_DATA;

extern PROCESS_DATA_DEFINITION ProcessDataDefinition;

#endif // _DATAPROC_H_
