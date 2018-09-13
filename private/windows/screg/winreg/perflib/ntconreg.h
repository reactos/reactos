/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1996   Microsoft Corporation

Module Name:

    ntconreg.h

Abstract:

    Header file for the NT Configuration Registry

    This file contains definitions which provide the interface to
    the Performance Configuration Registry.

Author:

    Russ Blake  11/15/91

Revision History:

    04/20/91    -   russbl      -   Converted to lib in Registry
                                      from stand-alone .dll form.
    11/04/92    -   a-robw      -  added pagefile counters


--*/
//
#include <winperf.h>    // for fn prototype declarations
#include <ntddnfs.h>
#include <srvfsctl.h>
#include <assert.h>
//
//  Until USER supports Unicode, we have to work in ASCII:
//

#define DEFAULT_NT_CODE_PAGE 437
#define UNICODE_CODE_PAGE      0

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

//    (assumes dword is 4 bytes long and pointer is a dword in size)
#define ALIGN_ON_DWORD(x) ((VOID *)( ((ULONG_PTR) x & 0x00000003) ? ( ((ULONG_PTR) x & (INT_PTR)-4 ) + 4 ) : ( (ULONG_PTR) x ) ))

#define QWORD_MULTIPLE(x) (((x+sizeof(LONGLONG)-1)/sizeof(LONGLONG))*sizeof(LONGLONG))

//    (assumes quadword is 8 bytes long and pointer is a dword in size)
#define ALIGN_ON_QWORD(x) ((VOID *)( ((ULONG_PTR) x & 0x00000007) ? ( ((ULONG_PTR) x & (INT_PTR)-8) + 8 ) : ( (ULONG_PTR) x ) ))

//
//  Definitions for internal use by the Performance Configuration Registry
//

#define NUM_VALUES 2
#define MAX_INSTANCE_NAME 32
#define DEFAULT_LARGE_BUFFER 8*1024
#define INCREMENT_BUFFER_SIZE 4*1024
#define MAX_PROCESS_NAME_LENGTH 256*sizeof(WCHAR)
#define MAX_THREAD_NAME_LENGTH 10*sizeof(WCHAR)
#define MAX_KEY_NAME_LENGTH 256*sizeof(WCHAR)
#define MAX_VALUE_NAME_LENGTH 256*sizeof(WCHAR)
#define MAX_VALUE_DATA_LENGTH 256*sizeof(WCHAR)

//
//  Definition of handle table for extensible objects
//
typedef PM_OPEN_PROC    *OPENPROC;
typedef PM_COLLECT_PROC *COLLECTPROC;
typedef PM_QUERY_PROC   *QUERYPROC;
typedef PM_CLOSE_PROC   *CLOSEPROC;

#define EXT_OBJ_INFO_NAME_LENGTH    32

typedef struct _DllValidationData {
    FILETIME    CreationDate;
    LONGLONG    FileSize;
} DllValidationData, *pDllValidationData;

typedef struct _PerfDataSectionHeader {
    DWORD       dwEntriesInUse;
    DWORD       dwMaxEntries;
    DWORD       dwMissingEntries;
    DWORD       dwInitSignature;
    BYTE        reserved[112];
} PerfDataSectionHeader, *pPerfDataSectionHeader;

#define PDSH_INIT_SIG   ((DWORD)0x01234567)

#define PDSR_SERVICE_NAME_LEN   32
typedef struct _PerfDataSectionRecord {
    WCHAR       szServiceName[PDSR_SERVICE_NAME_LEN];
    LONGLONG    llElapsedTime;
    DWORD       dwCollectCount; // number of times Collect successfully called
    DWORD       dwOpenCount;    // number of Loads & opens
    DWORD       dwCloseCount;   // number of Unloads & closes
    DWORD       dwLockoutCount; // count of lock timeouts
    DWORD       dwErrorCount;   // count of errors (other than timeouts)
    DWORD       dwLastBufferSize; // size of the last buffer returned
    DWORD       dwMaxBufferSize; // size of MAX buffer returned
    DWORD       dwMaxBufferRejected; // size of largest buffer returned as too small
    BYTE        Reserved[24];     // reserved to make structure 128 bytes
} PerfDataSectionRecord, *pPerfDataSectionRecord;

typedef struct _ExtObject {
        struct _ExtObject *pNext;   // pointer to next item in list
        HANDLE      hMutex;         // sync mutex for this function
        OPENPROC    OpenProc;       // address of the open routine
        LPSTR       szOpenProcName; // open procedure name
        LPWSTR      szLinkageString; // param for open proc
        DWORD       dwOpenTimeout;  // wait time in MS for open proc
        COLLECTPROC CollectProc;    // address of the collect routine
        QUERYPROC   QueryProc;      // address of query proc
        LPSTR       szCollectProcName;  // collect procedure name
        DWORD       dwCollectTimeout;   // wait time in MS for collect proc
        CLOSEPROC   CloseProc;     // address of the close routine
        LPSTR       szCloseProcName;    // close procedure name
        HANDLE      hLibrary ;     // handle returned by LoadLibraryW
        LPWSTR      szLibraryName;  // full path of library
        HKEY        hPerfKey;       // handle to performance sub key fo this service
        DWORD       dwNumObjects;  // number of supported objects
        DWORD       dwObjList[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];    // address of array of supported objects
        DWORD       dwFlags;        // flags
        LPWSTR      szServiceName;  // service name
        LONGLONG    llLastUsedTime; // FILETIME of last access
        DllValidationData   LibData; // validation data
        FILETIME    ftLastGoodDllFileDate; // creation date of last successfully accessed DLL 
        DWORD       dwValidationLevel; // collect function validation/test level
// Performance statistics
        pPerfDataSectionRecord      pPerfSectionEntry;  // pointer to entry in global section
        LONGLONG    llElapsedTime;  // time spent in call
        DWORD       dwCollectCount; // number of times Collect successfully called
        DWORD       dwOpenCount;    // number of Loads & opens
        DWORD       dwCloseCount;   // number of Unloads & closes
        DWORD       dwLockoutCount; // count of lock timeouts
        DWORD       dwErrorCount;   // count of errors (other than timeouts)
        DWORD       dwLastBufferSize; // size of the last buffer returned
        DWORD       dwMaxBufferSize; // size of MAX buffer returned
        DWORD       dwMaxBufferRejected; // size of largest buffer returned as too small
} ExtObject, *pExtObject;

// ext object flags

// use query proc
#define PERF_EO_QUERY_FUNC          ((DWORD)0x00000001) 
// true when DLL ret. error
#define PERF_EO_BAD_DLL             ((DWORD)0x00000002) 
// true if lib should not be trimmed
#define PERF_EO_KEEP_RESIDENT       ((DWORD)0x00000004) 
// true when in query list
#define PERF_EO_OBJ_IN_QUERY        ((DWORD)0x80000000) 
// set if alignment error has been posted to event log
#define PERF_EO_ALIGN_ERR_POSTED    ((DWORD)0x00000008) 
// set of the "Disable Performance Counters" value is set
#define PERF_EO_DISABLED            ((DWORD)0x00000010) 
// set when the DLL is deemed trustworthy
#define PERF_EO_TRUSTED             ((DWORD)0x00000020)
// set when the DLL has been replaced with a new file
#define PERF_EO_NEW_FILE            ((DWORD)0x00000040)

typedef struct _EXT_OBJ_ITEM {
    DWORD       dwObjId;
    DWORD       dwFlags;
} EXT_OBJ_LIST, *PEXT_OBJ_LIST;

#define PERF_EOL_ITEM_FOUND ((DWORD)0x00000001)

// convert mS to relative time
#define MakeTimeOutValue(ms) ((LONGLONG)((LONG)(ms) * -10000L))

typedef struct _COLLECT_THREAD_DATA {
    DWORD   dwQueryType;
    LPWSTR  lpValueName;
    LPBYTE  lpData;
    LPDWORD lpcbData;
    LPVOID  *lppDataDefinition;
    pExtObject  pCurrentExtObject;
    LONG    lReturnValue;
    DWORD   dwActionFlags;
} COLLECT_THREAD_DATA, * PCOLLECT_THREAD_DATA;

#define CTD_AF_NO_ACTION        ((DWORD)0x00000000)
#define CTD_AF_CLOSE_THREAD     ((DWORD)0x00000001)
#define CTD_AF_OPEN_THREAD      ((DWORD)0x00000002)
//
//  Definitions of Data Provider functions
//
static LONG QueryExtensibleData ( COLLECT_THREAD_DATA * );

//static LONG QueryExtensibleData (DWORD, LPWSTR, LPBYTE, LPDWORD, LPVOID *);

extern DWORD    NumberOfOpens;
extern DWORD    ComputerNameLength;
extern LPWSTR   pComputerName;
extern UCHAR    *pProcessBuffer;
extern HANDLE   hGlobalDataMutex;
extern HANDLE   hExtObjListIsNotInUse;
extern DWORD    dwExtObjListRefCount;
extern DWORD    NumExtensibleObjects;
extern LONG     lPerflibConfigFlags;

//  Misc. configuration flags used by lPerflibConfigFlags
//
//      PLCF_NO_ALIGN_ERRORS        if set inhibit alignment error messages
//      PLCF_NO_DISABLE_DLLS        if set, auto disable of bad perf DLL's is inhibited
//      PLCF_NO_DLL_TESTING         disable all DLL testing for ALL dll's (overrides lExtCounterTestLevel)
//      PLCF_ENABLE_TIMEOUT_DISABLE if set then disable when timeout errors occur (unless PLCF_NO_DISABLE_DLLS is set)
//      PLCF_ENABLE_PERF_SECTION    enable the perflib performance data memory section
//
#define PLCF_NO_ALIGN_ERRORS        ((DWORD)0x00000001)
#define PLCF_NO_DISABLE_DLLS        ((DWORD)0x00000002)
#define PLCF_NO_DLL_TESTING         ((DWORD)0x00000004)
#define PLCF_ENABLE_TIMEOUT_DISABLE ((DWORD)0x00000008)  
#define PLCF_ENABLE_PERF_SECTION    ((DWORD)0x00000010)

VOID
OpenExtensibleObjects(
    );

NTSTATUS
PerfGetNames (
   DWORD    QueryType,
   PUNICODE_STRING lpValueName,
   LPBYTE   lpData,
   LPDWORD  lpcbData,
   LPDWORD  lpcbLen,
   LPWSTR   lpLangId
);

DWORD
PerfOpenKey ();

//
//  Memory Probe macro (not implemented)
//
#define HEAP_PROBE()    ;

#define ALLOCMEM(size)     RtlAllocateHeap (RtlProcessHeap(), HEAP_ZERO_MEMORY, size)
#define REALLOCMEM(pointer, newsize) \
                                    RtlReAllocateHeap (RtlProcessHeap(), 0, pointer, newsize)
#define FREEMEM(pointer)   RtlFreeHeap (RtlProcessHeap(), 0, pointer)


#define CLOSE_WAIT_TIME     5000L   // wait time for query mutex (in ms)
#define QUERY_WAIT_TIME     2000L    // wait time for query mutex (in ms)
#define OPEN_PROC_WAIT_TIME 10000L  // default wait time for open proc to finish (in ms)

__inline
DWORD
RegisterExtObjListAccess ()
{
    LONG    Status;
    LARGE_INTEGER   liWaitTime;

    if (hGlobalDataMutex != NULL) {
        liWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);
        // wait for access to the list of ext objects
        Status = NtWaitForSingleObject (
            hGlobalDataMutex,
            FALSE,
            &liWaitTime);
        if (Status != WAIT_TIMEOUT) {
            if (hExtObjListIsNotInUse != NULL) {
                // indicate that we are going to use the list
                InterlockedIncrement ((LONG *)&dwExtObjListRefCount);
                if (dwExtObjListRefCount > 0) {
                    ResetEvent (hExtObjListIsNotInUse); // indicate list is busy
                } else {
                    SetEvent (hExtObjListIsNotInUse); // indicate list is not busy
                }
                Status = ERROR_SUCCESS;
            } else {
                Status = ERROR_NOT_READY;
            }
            ReleaseMutex (hGlobalDataMutex);
        }  // else return status;
    } else {
        Status = ERROR_LOCK_FAILED;
    }
    return Status;
}


__inline
DWORD
DeRegisterExtObjListAccess ()
{
    LONG    Status;
    LARGE_INTEGER   liWaitTime;

    if (hGlobalDataMutex != NULL) {
        liWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);
        // wait for access to the list of ext objects
        Status = NtWaitForSingleObject (
            hGlobalDataMutex,
            FALSE,
            &liWaitTime);
        if (Status != WAIT_TIMEOUT) {
            if (hExtObjListIsNotInUse != NULL) {
                assert (dwExtObjListRefCount > 0);
                // indicate that we are going to use the list
                InterlockedDecrement ((LONG *)&dwExtObjListRefCount);
                if (dwExtObjListRefCount > 0) {
                    ResetEvent (hExtObjListIsNotInUse); // indicate list is busy
                } else {
                    SetEvent (hExtObjListIsNotInUse); // indicate list is not busy
                }
                Status = ERROR_SUCCESS;
            } else {
                Status = ERROR_NOT_READY;
            }
            ReleaseMutex (hGlobalDataMutex);
        }  // else return status;
    } else {
        Status = ERROR_LOCK_FAILED;
    }
    return Status;
}

__inline
LONGLONG
GetTimeAsLongLong ()
/*++
    Returns time performance timer converted to ms.

-*/
{
    LARGE_INTEGER liCount, liFreq;
    LONGLONG        llReturn;

    if (NtQueryPerformanceCounter (&liCount, &liFreq) == STATUS_SUCCESS) {
        llReturn = liCount.QuadPart * 1000 / liFreq.QuadPart;
    } else {
        llReturn = 0;
    }
    return llReturn;
}



