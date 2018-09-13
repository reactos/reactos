/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfobj.c

Abstract:

    This file implements an Performance Object that presents
    System Object Performance Counters

Created:

    Bob Watson  22-Oct-1996

Revision History


--*/
//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include <stdio.h>
#include "perfos.h"
#include "perfosmc.h"
#include "dataobj.h"

DWORD   dwObjOpenCount = 0;        // count of "Open" threads

// variables local to this module.

static  HANDLE hEvent = NULL;
static  HANDLE hMutex = NULL;
static  HANDLE hSemaphore = NULL;
static  HANDLE hSection = NULL;


DWORD APIENTRY
OpenObjectsObject (
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will initialize the data structures used to pass
    data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PerfGen)

Return Value:

    None.

--*/

{
    DWORD   status = ERROR_SUCCESS;
	LONG_PTR	TempHandle = -1;
    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (dwObjOpenCount == 0) {
        // open Eventlog interface

        hEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
        hSemaphore = CreateSemaphore(NULL,1,256,NULL);
        hMutex = CreateMutex(NULL,FALSE,NULL);
        hSection = CreateFileMapping((HANDLE)TempHandle,NULL,PAGE_READWRITE,0,8192,NULL);
    }

    dwObjOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

    return status;
}


DWORD APIENTRY
CollectObjectsObjectData (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the system objects object

Arguments:

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is writted to the
            DWORD pointed to by this argument

    Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD  TotalLen;            //  Length of the total return block

    NTSTATUS    status;

    POBJECTS_DATA_DEFINITION    pObjectsDataDefinition;
    POBJECTS_COUNTER_DATA       pOCD;

    POBJECT_TYPE_INFORMATION ObjectInfo;
    WCHAR Buffer[ 256 ];

    //
    //  Check for sufficient space for objects data
    //

    pObjectsDataDefinition = (OBJECTS_DATA_DEFINITION *) *lppData;

    TotalLen = sizeof(OBJECTS_DATA_DEFINITION) +
                sizeof (OBJECTS_COUNTER_DATA);

    TotalLen = QWORD_MULTIPLE (TotalLen);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //

    memcpy (pObjectsDataDefinition,
        &ObjectsDataDefinition,
        sizeof(OBJECTS_DATA_DEFINITION));

    //
    //  Format and collect objects data
    //

    pOCD = (POBJECTS_COUNTER_DATA)&pObjectsDataDefinition[1];

    pOCD->CounterBlock.ByteLength = sizeof (OBJECTS_COUNTER_DATA);

    ObjectInfo = (POBJECT_TYPE_INFORMATION)Buffer;
    status = NtQueryObject( NtCurrentProcess(),
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Processes = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_PROCESS_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Processes = 0;
    }


    status = NtQueryObject( NtCurrentThread(),
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Threads = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_THREAD_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Threads = 0;
    }


    status = NtQueryObject( hEvent,
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Events = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_EVENT_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Events = 0;
    }


    status = NtQueryObject( hSemaphore,
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Semaphores = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_SEMAPHORE_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Semaphores = 0;
    }


    status = NtQueryObject( hMutex,
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Mutexes = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_MUTEX_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Mutexes = 0;
    }

    status = NtQueryObject( hSection,
                ObjectTypeInformation,
                ObjectInfo,
                sizeof( Buffer ),
                NULL
                );

    if (NT_SUCCESS(status)) {
        pOCD->Sections = ObjectInfo->TotalNumberOfObjects;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_SECTION_OBJECT_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);

        pOCD->Sections = 0;
    }

    *lppData = (LPVOID)&pOCD[1];

    // round up buffer to the nearest QUAD WORD

    *lppData = ALIGN_ON_QWORD (*lppData);

    *lpcbTotalBytes =
        pObjectsDataDefinition->ObjectsObjectType.TotalByteLength =
            (DWORD)((LPBYTE) *lppData -
            (LPBYTE) pObjectsDataDefinition);

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}

#pragma warning (disable : 4706)
DWORD APIENTRY
CloseObjectsObject (
)
/*++

Routine Description:

    This routine closes the open handles to the Signal Gen counters.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (dwObjOpenCount > 0) {
        dwObjOpenCount--;
        if (dwObjOpenCount == 0) { // when this is the last thread...
            // close stuff here
            if (hEvent != NULL) {
                CloseHandle(hEvent);
                hEvent = NULL;
            }

            if (hMutex != NULL) {
                CloseHandle(hMutex);
                hMutex = NULL;
            }

            if (hSemaphore != NULL) {
                CloseHandle(hSemaphore);
                hSemaphore = NULL;
            }
            
            if (hSection != NULL) {
                CloseHandle(hSection);
                hSection = NULL;
            }
        }
    }

    return ERROR_SUCCESS;

}
#pragma warning (default : 4706)
