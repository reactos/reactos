/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfnet.c

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
#include "perfnet.h"
#include "netsvcmc.h"

// bit field definitions for collect function flags

#define POS_COLLECT_SERVER_DATA         ((DWORD)0x00000001)
#define POS_COLLECT_SERVER_QUEUE_DATA   ((DWORD)0x00000002)
#define POS_COLLECT_REDIR_DATA          ((DWORD)0x00000004)
#define POS_COLLECT_BROWSER_DATA        ((DWORD)0x00000008)

#define POS_COLLECT_GLOBAL_DATA         ((DWORD)0x0000000F)
#define POS_COLLECT_FOREIGN_DATA        ((DWORD)0)
#define POS_COLLECT_COSTLY_DATA         ((DWORD)0)

// global variables to this DLL

HANDLE  ThisDLLHandle = NULL;
HANDLE  hEventLog     = NULL;
HANDLE  hLibHeap      = NULL;

// variables local to this module

static POS_FUNCTION_INFO    posDataFuncInfo[] = {
    {SERVER_OBJECT_TITLE_INDEX,         POS_COLLECT_SERVER_DATA,    0, CollectServerObjectData},
    {SERVER_QUEUE_OBJECT_TITLE_INDEX,   POS_COLLECT_SERVER_QUEUE_DATA,     0, CollectServerQueueObjectData},
    {REDIRECTOR_OBJECT_TITLE_INDEX,     POS_COLLECT_REDIR_DATA,     0, CollectRedirObjectData},
    {BROWSER_OBJECT_TITLE_INDEX,        POS_COLLECT_BROWSER_DATA,   0, CollectBrowserObjectData}
};

#define POS_NUM_FUNCS   (sizeof(posDataFuncInfo) / sizeof(posDataFuncInfo[1]))

static  bInitOk  = FALSE;
static  DWORD   dwOpenCount = 0;

static  BOOL    bReportedNotOpen = FALSE;

PM_OPEN_PROC    OpenNetSvcsObject;
PM_COLLECT_PROC CollecNetSvcsObjectData;
PM_CLOSE_PROC   CloseNetSvcsObject;

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
    WCHAR   wszTempBuffer[512];
    LONG    lStatus;
    DWORD   dwBufferSize;

    // create heap for this library
    if (hLibHeap == NULL) hLibHeap = HeapCreate (0, 1, 0);

    assert (hLibHeap != NULL);

    if (hLibHeap == NULL) {
        return FALSE;
    }
    // open handle to the event log
    if (hEventLog == NULL) hEventLog = MonOpenEventLog((LPWSTR)L"PerfNet");
    assert (hEventLog != NULL);

    return bReturn;
}

static
BOOL
DllProcessDetach (
    IN  HANDLE DllHandle
)
{
    if (dwOpenCount != 0) {
        // make sure the object has been closed before the
        // library is deleted.
        // setting dwOpenCount to 1 insures that all
        // the objects will be closed on this call
        if (dwOpenCount > 1) dwOpenCount = 1;
        CloseNetSvcsObject();
        dwOpenCount = 0;
    }

    if (hLibHeap != NULL) {
        HeapDestroy (hLibHeap); 
        hLibHeap = NULL;
    }

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
OpenNetSvcsObject (
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
    DWORD   dwErrorCount = 0;

    if (dwOpenCount == 0) {

        status = OpenServerObject (lpDeviceNames);
        // if this didn't open, it's not fatal, just no 
        // server stats will be returned
        if (status != ERROR_SUCCESS) {
            dwErrorCount++;
            status = ERROR_SUCCESS;
        }

        status = OpenServerQueueObject (lpDeviceNames);
        // if this didn't open, it's not fatal, just no 
        // server queue stats will be returned
        if (status != ERROR_SUCCESS) {
            dwErrorCount++;
            status = ERROR_SUCCESS;
        }

        status = OpenRedirObject (lpDeviceNames);
        // if this didn't open, it's not fatal, just no 
        // Redir stats will be returned
        if (status != ERROR_SUCCESS) {
            dwErrorCount++;
            status = ERROR_SUCCESS;
        }

        status = OpenBrowserObject (lpDeviceNames);
        // if this didn't open, it's not fatal, just no 
        // Browser stats will be returned
        if (status != ERROR_SUCCESS) {
            dwErrorCount++;
            status = ERROR_SUCCESS;
        }

        if (dwErrorCount < POS_NUM_FUNCS) {
            // then at least one object opened OK so continue
            bInitOk = TRUE;
            dwOpenCount++;
        } else {
            // none of the objects opened, so give up.
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFNET_UNABLE_OPEN,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
        }
    } else {
        // already opened so bump the refcount
        dwOpenCount++;
    }

    return  status;
}

DWORD APIENTRY
CollectNetSvcsObjectData (
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the processor object

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
    LONG    lReturn = ERROR_SUCCESS;

    NTSTATUS    Status;

    // build bit mask of functions to call

    DWORD       dwQueryType;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;

    DWORD       dwNumObjectsFromFunction;
    DWORD       dwOrigBuffSize;
    DWORD       dwByteSize;

    DWORD       dwReturnedBufferSize;

    if (!bInitOk) {
        if (!bReportedNotOpen) {
            bReportedNotOpen = ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFNET_NOT_OPEN,
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

    // collect data 
    *lpNumObjectTypes = 0;
    dwOrigBuffSize = dwByteSize = *lpcbTotalBytes;
    *lpcbTotalBytes = 0;

    for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
        if (posDataFuncInfo[FunctionIndex].dwCollectFunctionBit &
            FunctionCallMask) {
            dwNumObjectsFromFunction = 0;
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
    }

    // *lppData is updated by each function
    // *lpcbTotalBytes is updated after each successful function
    // *lpNumObjects is updated after each successful function

COLLECT_BAIL_OUT:
    
    return lReturn;
}

DWORD APIENTRY
CloseNetSvcsObject (
)
/*++

Routine Description:

    This routine closes the open objects for the net services counters.

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (dwOpenCount > 0) {
        dwOpenCount--;
    }
    if (dwOpenCount == 0) {
        // close stuff here
        CloseServerQueueObject();
        CloseServerObject();
        CloseRedirObject();
        CloseBrowserObject();
    }
    return  ERROR_SUCCESS;
}
