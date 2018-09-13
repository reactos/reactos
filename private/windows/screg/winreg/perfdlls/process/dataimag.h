/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATAIMAG.h

Abstract:

    Header file for the Windows NT Image Details Performance counters.

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
//  image details disk performance definition structure
//

typedef struct _IMAGE_DATA_DEFINITION {
    PERF_OBJECT_TYPE        ImageObjectType;
    PERF_COUNTER_DEFINITION ImageAddrNoAccess;
    PERF_COUNTER_DEFINITION ImageAddrReadOnly;
    PERF_COUNTER_DEFINITION ImageAddrReadWrite;
    PERF_COUNTER_DEFINITION ImageAddrWriteCopy;
    PERF_COUNTER_DEFINITION ImageAddrExecute;
    PERF_COUNTER_DEFINITION ImageAddrExecuteReadOnly;
    PERF_COUNTER_DEFINITION ImageAddrExecuteReadWrite;
    PERF_COUNTER_DEFINITION ImageAddrExecuteWriteCopy;
} IMAGE_DATA_DEFINITION, *PIMAGE_DATA_DEFINITION;


typedef struct _IMAGE_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    LONGLONG                 ImageAddrNoAccess;
    LONGLONG                 ImageAddrReadOnly;
    LONGLONG                 ImageAddrReadWrite;
    LONGLONG                 ImageAddrWriteCopy;
    LONGLONG                 ImageAddrExecute;
    LONGLONG                 ImageAddrExecuteReadOnly;
    LONGLONG                 ImageAddrExecuteReadWrite;
    LONGLONG                 ImageAddrExecuteWriteCopy;
} IMAGE_COUNTER_DATA, * PIMAGE_COUNTER_DATA;

extern IMAGE_DATA_DEFINITION  ImageDataDefinition;

#endif // _DATAPHYS_H_

