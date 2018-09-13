/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAEXPR.h

Abstract:

    Header file for the Windows NT Extended Process Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATA_EX_PROCESS_H_
#define _DATA_EX_PROCESS_H_

//
//  extended process performance definition structure
//

typedef struct _EXPROCESS_DATA_DEFINITION {
    PERF_OBJECT_TYPE        ExProcessObjectType;
    PERF_COUNTER_DEFINITION ProcessId;
    PERF_COUNTER_DEFINITION ImageReservedBytes;
    PERF_COUNTER_DEFINITION ImageFreeBytes;
    PERF_COUNTER_DEFINITION ReservedBytes;
    PERF_COUNTER_DEFINITION FreeBytes;
    PERF_COUNTER_DEFINITION CommitNoAccess;
    PERF_COUNTER_DEFINITION CommitReadOnly;
    PERF_COUNTER_DEFINITION CommitReadWrite;
    PERF_COUNTER_DEFINITION CommitWriteCopy;
    PERF_COUNTER_DEFINITION CommitExecute;
    PERF_COUNTER_DEFINITION CommitExecuteRead;
    PERF_COUNTER_DEFINITION CommitExecuteWrite;
    PERF_COUNTER_DEFINITION CommitExecuteWriteCopy;
    PERF_COUNTER_DEFINITION ReservedNoAccess;
    PERF_COUNTER_DEFINITION ReservedReadOnly;
    PERF_COUNTER_DEFINITION ReservedReadWrite;
    PERF_COUNTER_DEFINITION ReservedWriteCopy;
    PERF_COUNTER_DEFINITION ReservedExecute;
    PERF_COUNTER_DEFINITION ReservedExecuteRead;
    PERF_COUNTER_DEFINITION ReservedExecuteWrite;
    PERF_COUNTER_DEFINITION ReservedExecuteWriteCopy;
    PERF_COUNTER_DEFINITION UnassignedNoAccess;
    PERF_COUNTER_DEFINITION UnassignedReadOnly;
    PERF_COUNTER_DEFINITION UnassignedReadWrite;
    PERF_COUNTER_DEFINITION UnassignedWriteCopy;
    PERF_COUNTER_DEFINITION UnassignedExecute;
    PERF_COUNTER_DEFINITION UnassignedExecuteRead;
    PERF_COUNTER_DEFINITION UnassignedExecuteWrite;
    PERF_COUNTER_DEFINITION UnassignedExecuteWriteCopy;
    PERF_COUNTER_DEFINITION ImageTotalNoAccess;
    PERF_COUNTER_DEFINITION ImageTotalReadOnly;
    PERF_COUNTER_DEFINITION ImageTotalReadWrite;
    PERF_COUNTER_DEFINITION ImageTotalWriteCopy;
    PERF_COUNTER_DEFINITION ImageTotalExecute;
    PERF_COUNTER_DEFINITION ImageTotalExecuteRead;
    PERF_COUNTER_DEFINITION ImageTotalExecuteWrite;
    PERF_COUNTER_DEFINITION ImageTotalExecuteWriteCopy;
} EXPROCESS_DATA_DEFINITION, * PEXPROCESS_DATA_DEFINITION;

typedef struct _EXPROCESS_COUNTER_DATA {
    PERF_COUNTER_BLOCK       CounterBlock;
    DWORD                    Reserved;  // for alignment
    LONGLONG                 ProcessId;
    LONGLONG                 ImageReservedBytes;
    LONGLONG                 ImageFreeBytes;
    LONGLONG                 ReservedBytes;
    LONGLONG                 FreeBytes;
    LONGLONG                 CommitNoAccess;
    LONGLONG                 CommitReadOnly;
    LONGLONG                 CommitReadWrite;
    LONGLONG                 CommitWriteCopy;
    LONGLONG                 CommitExecute;
    LONGLONG                 CommitExecuteRead;
    LONGLONG                 CommitExecuteWrite;
    LONGLONG                 CommitExecuteWriteCopy;
    LONGLONG                 ReservedNoAccess;
    LONGLONG                 ReservedReadOnly;
    LONGLONG                 ReservedReadWrite;
    LONGLONG                 ReservedWriteCopy;
    LONGLONG                 ReservedExecute;
    LONGLONG                 ReservedExecuteRead;
    LONGLONG                 ReservedExecuteWrite;
    LONGLONG                 ReservedExecuteWriteCopy;
    LONGLONG                 UnassignedNoAccess;
    LONGLONG                 UnassignedReadOnly;
    LONGLONG                 UnassignedReadWrite;
    LONGLONG                 UnassignedWriteCopy;
    LONGLONG                 UnassignedExecute;
    LONGLONG                 UnassignedExecuteRead;
    LONGLONG                 UnassignedExecuteWrite;
    LONGLONG                 UnassignedExecuteWriteCopy;
    LONGLONG                 ImageTotalNoAccess;
    LONGLONG                 ImageTotalReadOnly;
    LONGLONG                 ImageTotalReadWrite;
    LONGLONG                 ImageTotalWriteCopy;
    LONGLONG                 ImageTotalExecute;
    LONGLONG                 ImageTotalExecuteRead;
    LONGLONG                 ImageTotalExecuteWrite;
    LONGLONG                 ImageTotalExecuteWriteCopy;
} EXPROCESS_COUNTER_DATA, *PEXPROCESS_COUNTER_DATA;

extern EXPROCESS_DATA_DEFINITION ExProcessDataDefinition;

#endif // _DATA_EX_PROCESS_H_

