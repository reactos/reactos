/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

      DATAJOB.h

Abstract:

    Header file for the Windows NT Processor Job Object counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/
#ifndef _DATAJOB_H_
#define _DATAJOB_H_

// don't include the "TOTAL" counters since we are reporting only rates
// (for now) the total rates are redundant.

#ifndef _DATAJOB_INCLUDE_TOTAL_COUNTERS
#define _DATAJOB_INCLUDE_TOTAL_COUNTERS
#endif


//
//  Process data object definitions.
//
//
//  This is the counter structure presently returned by NT.  The
//  Performance Monitor MUST *NOT* USE THESE STRUCTURES!
//

typedef struct _JOB_DATA_DEFINITION {
    PERF_OBJECT_TYPE		    JobObjectType;
    PERF_COUNTER_DEFINITION	    cdCurrentProcessorTime;
    PERF_COUNTER_DEFINITION	    cdCurrentUserTime;
    PERF_COUNTER_DEFINITION	    cdCurrentKernelTime;
#ifdef _DATAJOB_INCLUDE_TOTAL_COUNTERS
    PERF_COUNTER_DEFINITION	    cdTotalProcessorTime;
    PERF_COUNTER_DEFINITION	    cdTotalUserTime;
    PERF_COUNTER_DEFINITION	    cdTotalKernelTime;
    PERF_COUNTER_DEFINITION	    cdCurrentProcessorUsage;
    PERF_COUNTER_DEFINITION	    cdCurrentUserUsage;
    PERF_COUNTER_DEFINITION	    cdCurrentKernelUsage;
#endif
    PERF_COUNTER_DEFINITION	    cdPageFaults;
	PERF_COUNTER_DEFINITION		cdTotalProcessCount;
	PERF_COUNTER_DEFINITION		cdCurrentProcessCount;
	PERF_COUNTER_DEFINITION		cdTerminatedProcessCount;
} JOB_DATA_DEFINITION, * PJOB_DATA_DEFINITION;

typedef struct _JOB_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    LONGLONG                    CurrentProcessorTime;
    LONGLONG                    CurrentUserTime;
    LONGLONG                    CurrentKernelTime;
#ifdef _DATAJOB_INCLUDE_TOTAL_COUNTERS
    LONGLONG                    TotalProcessorTime;
    LONGLONG                    TotalUserTime;
    LONGLONG                    TotalKernelTime;
    LONGLONG                    CurrentProcessorUsage;
    LONGLONG                    CurrentUserUsage;
    LONGLONG                    CurrentKernelUsage;
#endif //_DATAJOB_INCLUDE_TOTAL_COUNTERS
    DWORD                  	    PageFaults;
    DWORD                  	    TotalProcessCount;
    DWORD                  	    ActiveProcessCount;
    DWORD                  	    TerminatedProcessCount;
} JOB_COUNTER_DATA, * PJOB_COUNTER_DATA;

extern JOB_DATA_DEFINITION JobDataDefinition;

typedef struct _JOB_DETAILS_DATA_DEFINITION {
    PERF_OBJECT_TYPE		    JobDetailsObjectType;
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
} JOB_DETAILS_DATA_DEFINITION, * PJOB_DETAILS_DATA_DEFINITION;

typedef struct _JOB_DETAILS_COUNTER_DATA {
    PERF_COUNTER_BLOCK          CounterBlock;
    DWORD                  	    PageFaults;
    LONGLONG                    ProcessorTime;
    LONGLONG                    UserTime;
    LONGLONG                    KernelTime;
    LONGLONG                    PeakVirtualSize;
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
    LONGLONG                    ProcessId;
    LONGLONG                    CreatorProcessId;
    DWORD                       PagedPool;
    DWORD                       NonPagedPool;
    DWORD                       HandleCount;
    DWORD                       Reserved;       // for alignment
    LONGLONG                    ReadOperationCount;
    LONGLONG                    WriteOperationCount;
    LONGLONG                    DataOperationCount;
    LONGLONG                    OtherOperationCount;
    LONGLONG                    ReadTransferCount;
    LONGLONG                    WriteTransferCount;
    LONGLONG                    DataTransferCount;
    LONGLONG                    OtherTransferCount;
} JOB_DETAILS_COUNTER_DATA, *PJOB_DETAILS_COUNTER_DATA;

extern JOB_DETAILS_DATA_DEFINITION	JobDetailsDataDefinition;

#endif // _DATAJOB_H_

