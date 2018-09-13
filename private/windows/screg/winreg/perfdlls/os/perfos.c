/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfos.c

Abstract:


Author:

    Bob Watson (a-robw) Aug 95

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <assert.h>
#include <perfutil.h>
#include "perfos.h"
#include "perfosmc.h"

// bit field definitions for collect function flags
#define POS_GET_SYS_PERF_INFO       ((DWORD)0x00010000)

#define POS_COLLECT_CACHE_DATA      ((DWORD)0x00010001)
#define POS_COLLECT_CPU_DATA        ((DWORD)0x00000002)
#define POS_COLLECT_MEMORY_DATA     ((DWORD)0x00010004)
#define POS_COLLECT_OBJECTS_DATA    ((DWORD)0x00000008)
#define POS_COLLECT_PAGEFILE_DATA   ((DWORD)0x00000010)
#define POS_COLLECT_SYSTEM_DATA     ((DWORD)0x00010020)

#define POS_COLLECT_FUNCTION_MASK   ((DWORD)0x0000003F)

#define POS_COLLECT_GLOBAL_DATA     ((DWORD)0x0001003F)
#define POS_COLLECT_FOREIGN_DATA    ((DWORD)0)
#define POS_COLLECT_COSTLY_DATA     ((DWORD)0)

// global variables to this DLL

HANDLE  ThisDLLHandle = NULL;
HANDLE  hEventLog     = NULL;
HANDLE  hLibHeap      = NULL;

SYSTEM_BASIC_INFORMATION BasicInfo;
SYSTEM_PERFORMANCE_INFORMATION  SysPerfInfo;

PM_OPEN_PROC    OpenOSObject;
PM_COLLECT_PROC CollectOSObjectData;
PM_CLOSE_PROC   CloseOSObject;

LPWSTR  wszTotal = NULL;

// variables local to this module

static POS_FUNCTION_INFO    posDataFuncInfo[] = {
    {CACHE_OBJECT_TITLE_INDEX,      POS_COLLECT_CACHE_DATA,     0, CollectCacheObjectData},
    {PROCESSOR_OBJECT_TITLE_INDEX,  POS_COLLECT_CPU_DATA,       0, CollectProcessorObjectData},
    {MEMORY_OBJECT_TITLE_INDEX,     POS_COLLECT_MEMORY_DATA,    0, CollectMemoryObjectData},
    {OBJECT_OBJECT_TITLE_INDEX,     POS_COLLECT_OBJECTS_DATA,   0, CollectObjectsObjectData},
    {PAGEFILE_OBJECT_TITLE_INDEX,   POS_COLLECT_PAGEFILE_DATA,  0, CollectPageFileObjectData},
    {SYSTEM_OBJECT_TITLE_INDEX,     POS_COLLECT_SYSTEM_DATA,    0, CollectSystemObjectData}
};

#define POS_NUM_FUNCS   (sizeof(posDataFuncInfo) / sizeof(posDataFuncInfo[1]))

static  bInitOk  = FALSE;
static  bReportedNotOpen = FALSE;

static
BOOL
DllProcessAttach (
    IN  HANDLE DllHandle
)
/*++

Description:

    perform any initialization function that apply to all object
    modules
   
--*/
{
    BOOL    bReturn = TRUE;
    LONG    status;
    WCHAR   wszTempBuffer[512];
    LONG    lStatus;
    DWORD   dwBufferSize;

    if (hLibHeap == NULL) {
        hLibHeap = HeapCreate (0, 1, 0);
    }

    assert (hLibHeap != NULL);

    if (hLibHeap == NULL) {
        return FALSE;
    }

    // open handle to the event log
    if (hEventLog == NULL) {
        hEventLog = MonOpenEventLog((LPWSTR)L"PerfOS");
        //
        //  collect basic and static processor data
        //

        status = NtQuerySystemInformation(
                     SystemBasicInformation,
                     &BasicInfo,
                     sizeof(SYSTEM_BASIC_INFORMATION),
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            BasicInfo.PageSize = 0;
            status = (LONG)RtlNtStatusToDosError(status);
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFOS_UNABLE_QUERY_BASIC_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);

            bReturn = FALSE;
        }
    }

    lStatus = GetPerflibKeyValue (
        szTotalValue,
        REG_SZ,
        sizeof(wszTempBuffer),
        (LPVOID)&wszTempBuffer[0],
        DEFAULT_TOTAL_STRING_LEN,
        (LPVOID)&szDefaultTotalString[0]);

    if (lStatus == ERROR_SUCCESS) {
        // then a string was returned in the temp buffer
        dwBufferSize = lstrlenW (wszTempBuffer) + 1;
        dwBufferSize *= sizeof (WCHAR);
        wszTotal = ALLOCMEM (hLibHeap, HEAP_ZERO_MEMORY, dwBufferSize);
        if (wszTotal == NULL) {
            // unable to allocate buffer so use static buffer
            wszTotal = (LPWSTR)&szDefaultTotalString[0];
        } else {
            memcpy (wszTotal, wszTempBuffer, dwBufferSize);
        }
    } else {
        // unable to get string from registry so just use static buffer
        wszTotal = (LPWSTR)&szDefaultTotalString[0];
    }

    return bReturn;
}

static
BOOL
DllProcessDetach (
    IN  HANDLE DllHandle
)
{
    if ((dwCpuOpenCount + dwPageOpenCount + dwObjOpenCount) != 0) {
        // close the objects now sinc this is the last chance
        // as the DLL is in the process of being unloaded
        // if any of the open counters are > 1, then set them to 1 
        // to insure the object is closed on this call
        if (dwCpuOpenCount > 1) dwCpuOpenCount = 1;
        if (dwPageOpenCount > 1) dwPageOpenCount = 1;
        if (dwObjOpenCount > 1) dwObjOpenCount = 1;

        CloseOSObject();
    }

    assert ((dwCpuOpenCount + dwPageOpenCount + dwObjOpenCount) == 0);

    if ((wszTotal != NULL) && (wszTotal != &szDefaultTotalString[0])) {
        FREEMEM (hLibHeap, 0, wszTotal);
        wszTotal = NULL;
    }

    if (HeapDestroy (hLibHeap)) hLibHeap = NULL;

    if (hEventLog != NULL) {
        MonCloseEventLog ();
	hEventLog = NULL;	 
    }
    return TRUE;
}

BOOL
__stdcall
DllInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
)
{
    ReservedAndUnused;

    // this will prevent the DLL from getting
    // the DLL_THREAD_* messages
    DisableThreadLibraryCalls (DLLHandle);

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            return DllProcessAttach (DLLHandle);

        case DLL_PROCESS_DETACH:
            return DllProcessDetach (DLLHandle);

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            return TRUE;
    }
}

DWORD APIENTRY
OpenOSObject (
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
    DWORD   status;

    // cache object does not need to be opened

    // open Processor Object
    status = OpenProcessorObject (lpDeviceNames);

    // memory object does not need to be opened

    // open Objects object
    if (status == ERROR_SUCCESS) {
        status = OpenObjectsObject (lpDeviceNames);
        // open Pagefile object
        if (status == ERROR_SUCCESS) {
            status = OpenPageFileObject (lpDeviceNames);
            if (status != ERROR_SUCCESS) {
               // processor & Objects opened & page file did not
               // close the open objects
               CloseProcessorObject ();
               CloseObjectsObject();
            }
         } else {
            // processor Opend and Objects did not
            // close the open objects
            CloseProcessorObject();
         }
    } else {
        // nothing opened
    }

    // System Object does not need to be opened

    if (status == ERROR_SUCCESS) {
        bInitOk = TRUE;
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFOS_UNABLE_OPEN,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&status);
    }

    return  status;
}

static
DWORD APIENTRY
ReadOSObjectData (
    IN      DWORD   FunctionCallMask,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the OS object

Arguments:

   IN       DWORD FunctionCallMask
            bit mask of functions to call

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            data structure. In the case of an item list, Global or Costly
            query, this will be a collection of one or more perf data objects.
            In the case of a PERF_QUERY_OBJECTS request, this will be an array
            of DWORDs listing the object ID's of the perf data objects
            supported by this DLL.

         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the number of objects listed in the array of DWORDs referenced
            by the pObjList argument
            
         OUT: the number of objects returned by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    NTSTATUS    Status;
    DWORD       lReturn = ERROR_SUCCESS;

    DWORD       FunctionIndex;

    DWORD       dwNumObjectsFromFunction;
    DWORD       dwOrigBuffSize;
    DWORD       dwByteSize;

    DWORD       dwReturnedBufferSize;

    // collect data 
    if (FunctionCallMask & POS_GET_SYS_PERF_INFO) {
        Status = NtQuerySystemInformation(
            SystemPerformanceInformation,
            &SysPerfInfo,
            sizeof(SysPerfInfo),
            &dwReturnedBufferSize
            );

        if (!NT_SUCCESS(Status)) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFOS_UNABLE_QUERY_SYS_PERF_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&Status);
            memset (&SysPerfInfo, 0, sizeof(SysPerfInfo));
        }
    } else {
        memset (&SysPerfInfo, 0, sizeof(SysPerfInfo));
    }

    *lpNumObjectTypes = 0;
    dwOrigBuffSize = dwByteSize = *lpcbTotalBytes;
    *lpcbTotalBytes = 0;

    // remove query bits
    FunctionCallMask &= POS_COLLECT_FUNCTION_MASK;

    for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
        if (posDataFuncInfo[FunctionIndex].dwCollectFunctionBit &
            FunctionCallMask) {
            dwNumObjectsFromFunction = 0;

            // check for QUADWORD alignment of data buffer
            assert (((DWORD)(*lppData) & 0x00000007) == 0);

            lReturn = (*posDataFuncInfo[FunctionIndex].pCollectFunction) (
                lppData,
                &dwByteSize,
                &dwNumObjectsFromFunction);

            if (lReturn == ERROR_SUCCESS) {
                *lpNumObjectTypes += dwNumObjectsFromFunction;
                *lpcbTotalBytes += dwByteSize;
                dwOrigBuffSize -= dwByteSize;
                dwByteSize = dwOrigBuffSize;
            } else {
                break;
            }
        }
        // *lppData is updated by each function
        // *lpcbTotalBytes is updated after each successful function
        // *lpNumObjects is updated after each successful function
    }

    return lReturn;
}   

DWORD APIENTRY
QueryOSObjectData (
    IN      LPDWORD pObjList,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

Arguments:

   IN       LPDWORD *pObjList
            pointer to an array of Performance Objects that are
            to be returned to the caller. Each object is referenced by its
            DWORD value. If the first element in the array is one of the
            following then only the first item is read and the following
            data is returned:

                PERF_QUERY_OBJECTS   an array of object id's supported
                                by this function is returned in the data

                PERF_QUERY_GLOBAL    all perf objects supported by this
                                function are returned (Except COSTLY objects)

                PERF_QUERY_COSTLY    all COSTLY perf objects supported
                                by this function are returned

                Foreign objects are not supported by this API

   IN OUT   LPVOID   *lppData
         IN: pointer to the address of the buffer to receive the completed
            data structure. In the case of an item list, Global or Costly
            query, this will be a collection of one or more perf data objects.
            In the case of a PERF_QUERY_OBJECTS request, this will be an array
            of DWORDs listing the object ID's of the perf data objects
            supported by this DLL.

         OUT: points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

   IN OUT   LPDWORD  lpcbTotalBytes
         IN: the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
         OUT: the number of bytes added by this routine is writted to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the number of objects listed in the array of DWORDs referenced
            by the pObjList argument
            
         OUT: the number of objects returned by this routine is writted to the
            DWORD pointed to by this argument

   Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    LONG        lReturn = ERROR_SUCCESS;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;
    LPDWORD     pdwRetBuffer;

    DWORD       ObjectIndex;
    
    if (!bInitOk) {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFOS_NOT_OPEN,
            NULL,
            0,
            0,
            NULL,
            NULL);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
        goto QUERY_BAIL_OUT;
    }

    // evaluate the object list

    if (*lpNumObjectTypes == 1) {
        // then see if it's a predefined request value
        if (pObjList[0] == PERF_QUERY_GLOBAL) {
            FunctionCallMask = POS_COLLECT_GLOBAL_DATA;
        } else if (pObjList[0] == PERF_QUERY_COSTLY) {
            FunctionCallMask = POS_COLLECT_COSTLY_DATA;
        } else if (pObjList[0] == PERF_QUERY_OBJECTS) {
            if (*lpcbTotalBytes < (POS_NUM_FUNCS * sizeof(DWORD))) {
                lReturn = ERROR_MORE_DATA;
            } else {
                pdwRetBuffer = (LPDWORD)*lppData;
                for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
                    pdwRetBuffer[FunctionIndex] =
                        posDataFuncInfo[FunctionIndex].dwObjectId;
                }
                *lppData = &pdwRetBuffer[FunctionIndex];
                *lpcbTotalBytes = (POS_NUM_FUNCS * sizeof(DWORD));
                *lpNumObjectTypes = FunctionIndex;
                lReturn = ERROR_SUCCESS;
                goto QUERY_BAIL_OUT;
            }
        }
    }

    if (FunctionCallMask == 0) {
        // it's not a predfined value so run through the list
        // read the object list and build the call mask
        ObjectIndex = 0;
        while (ObjectIndex < *lpNumObjectTypes) {
            // search for this object in the list of object id's 
            // supported by this DLL
            for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
                if (pObjList[ObjectIndex] ==
                    posDataFuncInfo[FunctionIndex].dwObjectId) {
                    FunctionCallMask |=
                        posDataFuncInfo[FunctionIndex].dwCollectFunctionBit;
                    break; // out of inner loop
                }
            }
            ObjectIndex++;
        }
    }

    if (FunctionCallMask != 0) {
        lReturn = ReadOSObjectData (FunctionCallMask,
                                lppData,    
                                lpcbTotalBytes,
                                lpNumObjectTypes);
    } else {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
    }

QUERY_BAIL_OUT:
    return  lReturn;
}

DWORD APIENTRY
CollectOSObjectData (
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

Arguments:

   IN       LPWSTR   lpValueName
            pointer to a wide character string passed by registry.

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
    LONG    lReturn = ERROR_SUCCESS;

    // build bit mask of functions to call

    DWORD       dwQueryType;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;

    if (!bInitOk) {
        if (!bReportedNotOpen) {
            bReportedNotOpen = ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFOS_NOT_OPEN,
                NULL,
                0,
                0,
                NULL,
                NULL);
        }
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
        goto COLLECT_BAIL_OUT;
    }

    dwQueryType = GetQueryType (lpValueName);

    switch (dwQueryType) {
        case QUERY_ITEMS:
            for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
                if (IsNumberInUnicodeList (
                    posDataFuncInfo[FunctionIndex].dwObjectId, lpValueName)) {
                    FunctionCallMask |=
                        posDataFuncInfo[FunctionIndex].dwCollectFunctionBit;
                }
            }
            break;

        case QUERY_GLOBAL:
            FunctionCallMask = POS_COLLECT_GLOBAL_DATA;
            break;

        case QUERY_FOREIGN:
            FunctionCallMask = POS_COLLECT_FOREIGN_DATA;
            break;

        case QUERY_COSTLY:
            FunctionCallMask = POS_COLLECT_COSTLY_DATA;
            break;

        default:
            FunctionCallMask = POS_COLLECT_COSTLY_DATA;
            break;
    }

    if (FunctionCallMask != 0) {
        lReturn = ReadOSObjectData (FunctionCallMask,
                                lppData,    
                                lpcbTotalBytes,
                                lpNumObjectTypes);
    } else {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
    }

COLLECT_BAIL_OUT:
    
    return lReturn;
}

DWORD APIENTRY
CloseOSObject (
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
    DWORD   status;
    DWORD   dwReturn;

    // cache object does not need to be closeed

    // close Processor Object
    status = CloseProcessorObject ();
    assert (status == ERROR_SUCCESS);
    if (status != ERROR_SUCCESS) dwReturn = status;

    // memory object does not need to be closeed

    // close Objects object
    status = CloseObjectsObject ();
    assert (status == ERROR_SUCCESS);
    if (status != ERROR_SUCCESS) dwReturn = status;

    // close Pagefile object
    status = ClosePageFileObject ();
    assert (status == ERROR_SUCCESS);
    if (status != ERROR_SUCCESS) dwReturn = status;

    // System Object does not need to be closeed

    return  dwReturn;

}
