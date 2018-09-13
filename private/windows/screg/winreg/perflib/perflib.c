/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perflib.c

Abstract:

    This file implements the Configuration Registry
    for the purposes of the Performance Monitor.


    This file contains the code which implements the Performance part
    of the Configuration Registry.

Author:

    Russ Blake  11/15/91

Revision History:

    04/20/91    -   russbl      -   Converted to lib in Registry
                                      from stand-alone .dll form.
    11/04/92    -   a-robw      -  added pagefile and image counter routines

    11/01/96    -   bobw        -  revamped to support dynamic loading and
                                    unloading of performance modules

--*/
#define UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <winperf.h>
#include <rpc.h>
#include "regrpc.h"
#include "ntconreg.h"
#include "perfsec.h"
#include "prflbmsg.h"   // event log messages
#include "utils.h"

//
//  static constant definitions
//
//      constants used by guard page testing
//
#define GUARD_PAGE_SIZE 1024
#define GUARD_PAGE_CHAR 0xA5
#define GUARD_PAGE_DWORD 0xA5A5A5A5
//
//  performance gathering thead priority
//
#define DEFAULT_THREAD_PRIORITY     THREAD_BASE_PRIORITY_LOWRT
//
//  constants
//
const   WCHAR DLLValue[] = L"Library";
const   CHAR OpenValue[] = "Open";
const   CHAR CloseValue[] = "Close";
const   CHAR CollectValue[] = "Collect";
const   CHAR QueryValue[] = "Query";
const   WCHAR ObjListValue[] = L"Object List";
const   WCHAR LinkageKey[] = L"\\Linkage";
const   WCHAR ExportValue[] = L"Export";
const   WCHAR PerflibKey[] = L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const   WCHAR HKLMPerflibKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const   WCHAR CounterValue[] = L"Counter";
const   WCHAR HelpValue[] = L"Help";
const   WCHAR PerfSubKey[] = L"\\Performance";
const   WCHAR ExtPath[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services";
const   WCHAR OpenTimeout[] = L"Open Timeout";
const   WCHAR CollectTimeout[] = L"Collect Timeout";
const   WCHAR EventLogLevel[] = L"EventLogLevel";
const   WCHAR ExtCounterTestLevel[] = L"ExtCounterTestLevel";
const   WCHAR OpenProcedureWaitTime[] = L"OpenProcedureWaitTime";
const   WCHAR TotalInstanceName[] = L"TotalInstanceName";
const   WCHAR LibraryUnloadTime[] = L"Library Unload Time";
const   WCHAR KeepResident[] = L"Keep Library Resident";
const   WCHAR NULL_STRING[] = L"\0";    // pointer to null string
const   WCHAR UseCollectionThread[] = L"UseCollectionThread";
const   WCHAR cszLibraryValidationData[] = L"Library Validation Code";
const   WCHAR cszSuccessfulFileData[] = L"Successful File Date";
const   WCHAR cszPerflibFlags[] = L"Configuration Flags";

//
//  external variables
//      defined in perfname.c
//
extern   WCHAR    DefaultLangId[];
extern   WCHAR    NativeLangId[4];

//
//  Data collection thread variables
//
#define COLLECTION_WAIT_TIME        10000L  // 10 seconds to get all the data 
static  HANDLE   hCollectThread = NULL;
#define COLLECT_THREAD_PROCESS_EVENT    0
#define COLLECT_THREAD_EXIT_EVENT       1
#define COLLECT_THREAD_LOOP_EVENT_COUNT 2

#define COLLECT_THREAD_DONE_EVENT       2
#define COLLECT_THREAD_EVENT_COUNT      3
static  HANDLE  hCollectEvents[COLLECT_THREAD_EVENT_COUNT];
static  BOOL    bThreadHung = FALSE;

static  DWORD CollectThreadFunction (LPVOID dwArg);

#define COLL_FLAG_USE_SEPARATE_THREAD   1
static  DWORD   dwCollectionFlags = 0;

//
//      Global variable Definitions
//
// event log handle for perflib generated errors
//
HANDLE  hEventLog = NULL;

//
//  used to count concurrent opens.
//
DWORD NumberOfOpens = 0;

//
//  Synchronization objects for Multi-threaded access
//
HANDLE   hGlobalDataMutex = NULL; // sync for ctr object list
static DWORD    dwExtCtrOpenProcWaitMs = OPEN_PROC_WAIT_TIME;

//
//  computer name cache buffers. Initialized in predefh.c
//

DWORD ComputerNameLength;
LPWSTR pComputerName;

//  The next pointer is used to point to an array of addresses of
//  Open/Collect/Close routines found by searching the Configuration Registry.

//                  object list head
static  pExtObject  ExtensibleObjects = NULL;
//
//                  count of active list users (threads)
DWORD       dwExtObjListRefCount = 0;
//
//                  event to indicate the object list is not in use
HANDLE      hExtObjListIsNotInUse = NULL;
//
//                  Number of Extensible Objects found during the "open" call
DWORD       NumExtensibleObjects = 0;
//
//  see if the perflib data is restricted to ADMIN's ONLY or just anyone
//
static  LONG    lCheckProfileSystemRight = CPSR_NOT_DEFINED;

//
//  flag to see if the ProfileSystemPerformance priv should be set.
//      if it is attempted and the caller does not have permission to use this priv.
//      it won't be set. This is only attempted once.
//
static  BOOL    bEnableProfileSystemPerfPriv = FALSE;

//
//  timeout value (in mS) for timing threads & libraries
//
DWORD   dwThreadAndLibraryTimeout = PERFLIB_TIMING_THREAD_TIMEOUT;

//      global key for access to HKLM\Software\....\Perflib
//
HKEY    ghKeyPerflib = NULL;


//
//  flag to determine the "noisiness" of the event logging
//  this value is read from the system registry when the extensible
//  objects are loaded and used for the subsequent calls.
//
//
//    Levels:  LOG_UNDEFINED = registry log level not read yet
//             LOG_NONE = No event log messages ever
//             LOG_USER = User event log messages (e.g. errors)
//             LOG_DEBUG = Minimum Debugging      (warnings & errors)
//             LOG_VERBOSE = Maximum Debugging    (informational, success,
//                              error and warning messages
//
#define  LOG_UNDEFINED  ((LONG)-1)
#define  LOG_NONE       0
#define  LOG_USER       1
#define  LOG_DEBUG      2
#define  LOG_VERBOSE    3

LONG    lEventLogLevel = LOG_UNDEFINED;
//
//  define configurable extensible counter buffer testing
//
//  Test Level      Event that will prevent data buffer
//                  from being returne in PerfDataBlock
//
//  EXT_TEST_NOMEMALLOC Collect Fn. writes directly to calling fn's buffer
//
//      all the following test levels have the collect fn. write to a
//      buffer allocated separately from the calling fn's buffer
//
//  EXT_TEST_NONE   Collect Fn. Returns bad status or generates exception
//  EXT_TEST_BASIC  Collect Fn. has buffer overflow or violates guard page
//  EXT_TEST_ALL    Collect Fn. object or instance lengths are not conistent
//
//
#define     EXT_TEST_UNDEFINED  0
#define     EXT_TEST_ALL        1
#define     EXT_TEST_BASIC      2
#define     EXT_TEST_NONE       3
#define     EXT_TEST_NOMEMALLOC 4

LONG    lExtCounterTestLevel = EXT_TEST_UNDEFINED;

//
//  Misc. configuration flags
//
//      PLCF_NO_ALIGN_ERRORS        if set inhibit alignment error messages
//      PLCF_NO_DISABLE_DLLS        if set, auto disable of bad perf DLL's is inhibited
//      PLCF_NO_DLL_TESTING         disable all DLL testing for ALL dll's (overrides lExtCounterTestLevel)
//      PLCF_ENABLE_TIMEOUT_DISABLE if set then disable when timeout errors occur (unless PLCF_NO_DISABLE_DLLS is set)
//      PLCF_ENABLE_PERF_SECTION    enable the perflib performance data memory section
//

#define     PLCF_DEFAULT    PLCF_ENABLE_PERF_SECTION
LONG    lPerflibConfigFlags = PLCF_DEFAULT;

// default trusted file list
// all files presume to start with "perf"

static LONGLONG    llTrustedNamePrefix = 0x0066007200650050;   // "Perf"

static DWORD       dwTrustedFileNames[] = {
    0x0053004F,         // "OS"   for PerfOS.dll
    0x0065004E,         // "Ne"   for PerfNet.dll
    0x00720050,         // "Pr"   for PerfProc.dll
    0x00690044          // "Di"   for PerfDisk.dll
};

static CONST DWORD dwTrustedFileNameCount = sizeof(dwTrustedFileNames) / sizeof (dwTrustedFileNames[0]);

// there must be at least 8 chars in the name to be checked as trusted by default 
// trusted file names are at least 8 chars in length
static CONST DWORD dwMinTrustedFileNameLen = 6; 

// performance data block entries
WCHAR   szPerflibSectionFile[MAX_PATH];
WCHAR   szPerflibSectionName[MAX_PATH];
HANDLE  hPerflibSectionFile = NULL;
HANDLE  hPerflibSectionMap = NULL;
LPVOID  lpPerflibSectionAddr = NULL;

#define     dwPerflibSectionMaxEntries  127L
const DWORD dwPerflibSectionSize = (sizeof(PerfDataSectionHeader) + (sizeof(PerfDataSectionRecord) * dwPerflibSectionMaxEntries));
// forward function references

LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    );

DWORD
CloseExtObjectLibrary (
    pExtObject  pObj,
    BOOL        bCloseNow
);


BOOL
ServiceIsTrustedByDefault (
    LPCWSTR     szServiceName
)
{
    BOOL        bReturn = FALSE;
    PLONGLONG   pPrefixToTest;
    PDWORD      pNameToTest;
    DWORD       dwIdx;

    if (szServiceName != NULL) {
        // check for min size
        dwIdx = 0;
        while ((dwIdx < dwMinTrustedFileNameLen) && (szServiceName[dwIdx] > 0)) dwIdx++;

        if (dwIdx == dwMinTrustedFileNameLen) {
            // test first 4 bytes to see if they match
            pPrefixToTest = (LONGLONG *)szServiceName;
            if (*pPrefixToTest == llTrustedNamePrefix) {
                // then see if the rest is in this list
                pNameToTest = (DWORD *)(++pPrefixToTest);   // go to next 2 characters
                for (dwIdx = 0; dwIdx < dwTrustedFileNameCount; dwIdx++) {
                    if (*pNameToTest == dwTrustedFileNames[dwIdx]) {
                        // match found
                        bReturn = TRUE;
                        break;
                    } else {
                        // no match so continue
                    }
                }
            } else {
                // no match so return false
            }
        } else {
            // the name to be checked is too short so it mustn't be 
            // a trusted one.
        }
    } else {
        // no string so return false
    }
    return bReturn;
}
#if 0 // collection thread functions are not supported
DWORD
OpenCollectionThread (
)
{
    BOOL    bError = FALSE;
    DWORD   dwThreadID;

    assert (hCollectThread == NULL);

    // if it's already created, then just return
    if (hCollectThread != NULL) return ERROR_SUCCESS;

    bThreadHung = FALSE;
    hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] == NULL;
    assert (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] != NULL);

    hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] == NULL) | bError;
    assert (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] != NULL);

    hCollectEvents[COLLECT_THREAD_DONE_EVENT] = CreateEvent (
        NULL,  // default security
        FALSE, // auto reset
        FALSE, // non-signaled
        NULL); // no name
    bError = (hCollectEvents[COLLECT_THREAD_DONE_EVENT] == NULL) | bError;
    assert (hCollectEvents[COLLECT_THREAD_DONE_EVENT] != NULL);
   
    if (!bError) {
        // create data collection thread
        hCollectThread = CreateThread (
            NULL,   // default security
            0,      // default stack size
            (LPTHREAD_START_ROUTINE)CollectThreadFunction,
            NULL,   // no argument
            0,      // no flags
            &dwThreadID);  // we don't need the ID so it's in an automatic variable

        if (hCollectThread == NULL) {
            bError = TRUE;
        }

        assert (hCollectThread != NULL);
    }

    if (bError) {
        if (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);
            hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;
        }
        if (hCollectEvents[COLLECT_THREAD_EXIT_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
            hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;
        }
        if (hCollectEvents[COLLECT_THREAD_DONE_EVENT] != NULL) {
            CloseHandle (hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL);
            hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
        }

        if (hCollectThread != NULL) {
            CloseHandle (hCollectThread);
            hCollectThread = NULL;
        }

        return (GetLastError());
    } else {
        return ERROR_SUCCESS;
    }
}


DWORD
CloseCollectionThread (
)
{
    if (hCollectThread != NULL) {
        // close the data collection thread
        if (bThreadHung) {
            // then kill it the hard way 
            // this might cause problems, but it's better than
            // a thread leak
            TerminateThread (hCollectThread, ERROR_TIMEOUT);
        } else {
            // then ask it to leave
            SetEvent (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
        }
        // wait for thread to leave
        WaitForSingleObject (hCollectThread, COLLECTION_WAIT_TIME);

        // close the handles and clear the variables
        CloseHandle (hCollectThread);
        hCollectThread = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);
        hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_EXIT_EVENT]);
        hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;

        CloseHandle (hCollectEvents[COLLECT_THREAD_DONE_EVENT]);
        hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
    } else {
        // nothing was opened 
    }
    return ERROR_SUCCESS;
}
#endif

DWORD
PerfOpenKey (
)
{

    BOOL    bBusy = FALSE;
    LARGE_INTEGER       liPerfDataWaitTime;

    NTSTATUS status;
    DWORD   dwFnStatus = ERROR_SUCCESS;

    DWORD   dwType, dwSize, dwValue;

    if (hGlobalDataMutex == NULL) {
        hGlobalDataMutex = CreateMutex (
            NULL,
            TRUE,                   // and acquire the mutex
            NULL);                  // no name

        if (hGlobalDataMutex == NULL) {
            dwFnStatus = GetLastError();
            KdPrint (("\nPERFLIB: Perf Data Mutex Not Initialized"));
            goto OPD_Error_Exit_NoSemaphore;
        }
    } else {

        liPerfDataWaitTime.QuadPart = MakeTimeOutValue(dwThreadAndLibraryTimeout);

        status = NtWaitForSingleObject (
            hGlobalDataMutex, // Mutex
            FALSE,          // not alertable
            &liPerfDataWaitTime);   // wait time

        if (status == STATUS_TIMEOUT) {
            // unable to contine, return error;
            dwFnStatus = (DWORD)RtlNtStatusToDosError(status);
            goto OPD_Error_Exit_NoSemaphore;
        }
    }

    // if here, then the data semaphore has been acquired by this thread

    if (!NumberOfOpens++) {

        if (ghKeyPerflib == NULL) {
            dwFnStatus = (DWORD)RegOpenKeyExW (
                HKEY_LOCAL_MACHINE,
                HKLMPerflibKey,
                0L,
                KEY_READ,
                &ghKeyPerflib);
        }

        assert (ghKeyPerflib != NULL);
        dwSize = sizeof(dwValue);
        dwValue = dwType = 0;
        dwFnStatus = PrivateRegQueryValueExW (
            ghKeyPerflib,
            DisablePerformanceCounters,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);

        if ((dwFnStatus == ERROR_SUCCESS) &&
            (dwType == REG_DWORD) &&
            (dwValue == 1)) {
            // then DON'T Load any libraries and unload any that have been
            // loaded
            NumberOfOpens--;    // since it didn't open.
            dwFnStatus = ERROR_SERVICE_DISABLED;
        } else {
            ComputerNameLength = 0;
            GetComputerNameW(pComputerName, &ComputerNameLength);
            ComputerNameLength++;  // account for the NULL terminator

            if ( !(pComputerName = ALLOCMEM(ComputerNameLength *
                                                   sizeof(WCHAR))) ||
                 !GetComputerNameW(pComputerName, &ComputerNameLength) ) {
                //
                // Signal failure to data collection routine
                //

                ComputerNameLength = 0;
            } else {
                pComputerName[ComputerNameLength] = UNICODE_NULL;
                ComputerNameLength = (ComputerNameLength+1) * sizeof(WCHAR);
            }

            // create event and indicate the list is busy
            hExtObjListIsNotInUse = CreateEvent (NULL, TRUE, FALSE, NULL);

            // read collection thread flag
            dwType = 0;
            dwSize = sizeof(DWORD);
            dwFnStatus = PrivateRegQueryValueExW (ghKeyPerflib,
                            cszPerflibFlags,
                            NULL,
                            &dwType,
                            (LPBYTE)&lPerflibConfigFlags,
                            &dwSize);

            if ((dwFnStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                // then keep it
            } else {
                // apply default value
                lPerflibConfigFlags = PLCF_DEFAULT;
            }

            // create global section for perf data on perflibs
            if ((hPerflibSectionFile == NULL) && (lPerflibConfigFlags & PLCF_ENABLE_PERF_SECTION)) {
                WCHAR   szTmpFileName[MAX_PATH];
                pPerfDataSectionHeader  pHead;
                WCHAR   szPID[32];

                // create section name
                lstrcpyW (szPerflibSectionName, (LPCWSTR)L"Perflib_Perfdata_");
                _ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
                lstrcatW (szPerflibSectionName, szPID);

                // create filename
                lstrcpyW (szTmpFileName, (LPCWSTR)L"%windir%\\system32\\");
                lstrcatW (szTmpFileName, szPerflibSectionName);
                lstrcatW (szTmpFileName, (LPCWSTR)L".dat");
                ExpandEnvironmentStrings (szTmpFileName, szPerflibSectionFile, MAX_PATH);

                hPerflibSectionFile = CreateFile (szPerflibSectionFile,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_TEMPORARY,
                    NULL);

                if (hPerflibSectionFile != INVALID_HANDLE_VALUE) {
                    // create file mapping object
                    hPerflibSectionMap = CreateFileMapping (
                        hPerflibSectionFile,
                        NULL,
                        PAGE_READWRITE,
                        0, dwPerflibSectionSize,
                        szPerflibSectionName);

                    if (hPerflibSectionMap != NULL) {
                        // map view of file
                        lpPerflibSectionAddr = MapViewOfFile (
                            hPerflibSectionMap,
                            FILE_MAP_WRITE,
                            0,0, dwPerflibSectionSize);
                        if (lpPerflibSectionAddr != NULL) {
                            // init section if not already
                            pHead = (pPerfDataSectionHeader)lpPerflibSectionAddr;
                            if (pHead->dwInitSignature != PDSH_INIT_SIG) {
                                // then init
                                // clear file to 0
                                memset (pHead, 0, dwPerflibSectionSize);
                                pHead->dwEntriesInUse = 0;
                                pHead->dwMaxEntries = dwPerflibSectionMaxEntries;
                                pHead->dwMissingEntries = 0;
                                pHead->dwInitSignature = PDSH_INIT_SIG;
                            } else {
                                // already initialized so leave it
                            }
                        } else { 
                            // unable to map file so close 
                            DbgPrint ("PERFLIB: Unable to map file for sharing perf data\n");
                            CloseHandle (hPerflibSectionMap);
                            hPerflibSectionMap = NULL;
                            CloseHandle (hPerflibSectionFile);
                            hPerflibSectionFile = NULL;
                        }
                    } else {
                        // unable to create file mapping so close file
                        DbgPrint ("PERFLIB: Unable to create file mapping object for sharing perf data\n");
                        CloseHandle (hPerflibSectionFile);
                        hPerflibSectionFile = NULL;
                    }
                } else {
                    // unable to open file so no perf stats available
                    DbgPrint ("PERFLIB: Unable to create file for sharing perf data\n");
                    hPerflibSectionFile = NULL;
                }
            }

            // find and open perf counters
            OpenExtensibleObjects();

            dwExtObjListRefCount = 0;
            SetEvent (hExtObjListIsNotInUse); // indicate the list is not busy

            // read collection thread flag
            dwType = 0;
            dwSize = sizeof(DWORD);
            dwFnStatus = PrivateRegQueryValueExW (ghKeyPerflib,
                            UseCollectionThread,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwCollectionFlags,
                            &dwSize);
            if ((dwFnStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                // validate the answer
                switch (dwCollectionFlags) {
                    case 0:
                        // this is a valid value
                        break;

                    case COLL_FLAG_USE_SEPARATE_THREAD:
                        // this feature is not supported so skip through
                    default:
                        // this is for invalid values
                        dwCollectionFlags = 0;
                        //
                        //  BUGBUG: Make single threaded collection the
                        //  default until multi-threaded bugs are worked out
                        //
                        // dwCollectionFlags = COLL_FLAG_USE_SEPARATE_THREAD;
                        break;
                }
            }

            if (dwFnStatus != ERROR_SUCCESS) {
                dwCollectionFlags = 0;
                //
                //  BUGBUG: Make single threaded collection the
                //  default until multi-threaded bugs are worked out
                //
                // dwCollectionFlags = COLL_FLAG_USE_SEPARATE_THREAD;
            }

            if (dwCollectionFlags == COLL_FLAG_USE_SEPARATE_THREAD) {
                // create data collection thread
                // a seperate thread is required for COM/OLE compatibity as some 
                // client threads may be COM initialized incorrectly for the
                // extensible counter DLL's that may be called
//                status = OpenCollectionThread ();
            } else {
                hCollectEvents[COLLECT_THREAD_PROCESS_EVENT] = NULL;
                hCollectEvents[COLLECT_THREAD_EXIT_EVENT] = NULL;
                hCollectEvents[COLLECT_THREAD_DONE_EVENT] = NULL;
                hCollectThread = NULL;
            }
            dwFnStatus = ERROR_SUCCESS;
        }
    }
//    KdPrint (("\nPERFLIB: [Open]  Pid: %d, Number Of PerflibHandles: %d", 
//            GetCurrentProcessId(), NumberOfOpens));

    if (hGlobalDataMutex != NULL) ReleaseMutex (hGlobalDataMutex);

OPD_Error_Exit_NoSemaphore:
    return dwFnStatus;
}


LONG
PerfRegQueryValue (
    IN HKEY hKey,
    IN PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE  lpData,
    OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

    PerfRegQueryValue -   Get data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be omitted (NULL)

            lpData          -   pointer to a buffer to receive the
                                performance data

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the output buffer;
                                on output, will receive the number
                                of bytes actually returned

            lpcbLen         -   Return the number of bytes to transmit to
                                the client (used by RPC) (optional).

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/
{
    DWORD  dwQueryType;         //  type of request
    DWORD  TotalLen;            //  Length of the total return block
    DWORD  Win32Error;          //  Failure code
    LONG   lFnStatus = ERROR_SUCCESS;   // Win32 status to return to caller
    LPVOID pDataDefinition;     //  Pointer to next object definition
    UNICODE_STRING  usLocalValue = {0,0, NULL};

    PERF_DATA_BLOCK *pPerfDataBlock = (PERF_DATA_BLOCK *)lpData;

    LARGE_INTEGER   liQueryWaitTime ;
    THREAD_BASIC_INFORMATION    tbiData;

    LONG   lOldPriority, lNewPriority;

    NTSTATUS status = STATUS_SUCCESS;

    BOOL    bCheckCostlyCalls = FALSE;

    LPWSTR  lpLangId = NULL;

    DBG_UNREFERENCED_PARAMETER(lpReserved);

    HEAP_PROBE();

    if ((ULONG_PTR)lpData & (ULONG)0x00000007) {
        KdPrint (("\nPERFLIB: Caller passed in a data buffer that is not 8-byte aligned."));
    }

    // make a local copy of the value string if the arg references
    // the static buffer since it can be overwritten by
    // some of the RegistryEventSource call made by this routine

    if (lpValueName != NULL) {
        if (lpValueName == &NtCurrentTeb( )->StaticUnicodeString) {
            if (RtlCreateUnicodeString (
                &usLocalValue, lpValueName->Buffer)) {
                lFnStatus = ERROR_SUCCESS;
            } else {
                // unable to create string
                lFnStatus = ERROR_INVALID_PARAMETER;
            }
        } else {
            // copy the arg to the local structure
            memcpy (&usLocalValue, lpValueName, sizeof(UNICODE_STRING));
        }
    } else {
        lFnStatus = ERROR_INVALID_PARAMETER;
        goto PRQV_ErrorExit1;
    }

    if (lFnStatus != ERROR_SUCCESS) {
        goto PRQV_ErrorExit1;
    }

    if (hGlobalDataMutex == NULL) {
        // if a Mutex was not allocated then the key needs to be opened.
        // without synchronization, it's too easy for threads to get
        // tangled up
        lFnStatus = PerfOpenKey ();
    }

    if (lFnStatus == ERROR_SUCCESS) {
        if (!TestClientForAccess ()) {
            if (lEventLogLevel >= LOG_USER) {

                LPTSTR  szMessageArray[2];
                TCHAR   szUserName[128];
                TCHAR   szModuleName[MAX_PATH];
                DWORD   dwUserNameLength;

                dwUserNameLength = sizeof(szUserName)/sizeof(TCHAR);
                GetUserName (szUserName, &dwUserNameLength);
                GetModuleFileName (NULL, szModuleName,
                    sizeof(szModuleName)/sizeof(TCHAR));

                szMessageArray[0] = szUserName;
                szMessageArray[1] = szModuleName;

                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,        // error type
                    0,                          // category (not used)
                    (DWORD)PERFLIB_ACCESS_DENIED, // event,
                    NULL,                       // SID (not used),
                    2,                          // number of strings
                    0,                          // sizeof raw data
                    szMessageArray,             // message text array
                    NULL);                      // raw data
            }
            lFnStatus = ERROR_ACCESS_DENIED;
        }
    }

    if (lFnStatus == ERROR_SUCCESS) {
        status = NtQueryInformationThread (
            NtCurrentThread(),
            ThreadBasicInformation,
            &tbiData,
            sizeof(tbiData),
            NULL);
    } else {
        // goto the exit point
        goto PRQV_ErrorExit1;
    }

    if (NT_SUCCESS(status)) {
        lOldPriority = tbiData.Priority;
    } else {
        KdPrint (("\nPERFLIB: Unable to read current thread priority: 0x%8.8x", status));
        lOldPriority = -1;
    }

    lNewPriority = DEFAULT_THREAD_PRIORITY; // perfmon's favorite priority

    //
    //  Only RAISE the priority here. Don't lower it if it's high
    //

    if ((lOldPriority > 0) && (lOldPriority < lNewPriority)) {

        status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadPriority,
                    &lNewPriority,
                    sizeof(lNewPriority)
                    );
        if (!NT_SUCCESS(status)) {
            KdPrint (("\nPERFLIB: Set Thread Priority failed: 0x%8.8x", status));
            lOldPriority = -1;
        }

    } else {
        lOldPriority = -1;  // to save resetting at the end
    }

    //
    // Set the length parameter to zero so that in case of an error,
    // nothing will be transmitted back to the client and the client won't
    // attempt to unmarshall anything.
    //

    if( ARGUMENT_PRESENT( lpcbLen )) {
        *lpcbLen = 0;
    }

    // if here, then assume the caller has the necessary access

    /*
        determine query type, can be one of the following
            Global
                get all objects
            List
                get objects in list (usLocalValue)

            Foreign Computer
                call extensible Counter Routine only

            Costly
                costly object items

            Counter
                get counter names for the specified language Id

            Help
                get help names for the specified language Id

    */
    dwQueryType = GetQueryType (usLocalValue.Buffer);

    if (dwQueryType == QUERY_COUNTER || dwQueryType == QUERY_HELP ||
        dwQueryType == QUERY_ADDCOUNTER || dwQueryType == QUERY_ADDHELP ) {

        liQueryWaitTime.QuadPart = MakeTimeOutValue(QUERY_WAIT_TIME);

        status = NtWaitForSingleObject (
            hGlobalDataMutex, // semaphore
            FALSE,          // not alertable
            &liQueryWaitTime);          // wait 'til timeout

        if (status == STATUS_TIMEOUT) {
            lFnStatus = ERROR_BUSY;
        } else {
            if (hKey == HKEY_PERFORMANCE_DATA) {
                lpLangId = NULL;
            } else if (hKey == HKEY_PERFORMANCE_TEXT) {
                lpLangId = DefaultLangId;
            } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
                lpLangId = NativeLangId;

                if (*lpLangId == L'\0') {
                    // build the native language id
                    LANGID   iLanguage;
                    WCHAR    NativeLanguage;
                    WCHAR    nDigit;

                    iLanguage = GetUserDefaultLangID();
                    NativeLanguage = (WCHAR)MAKELANGID (iLanguage & 0x0ff, LANG_NEUTRAL);

                    nDigit =  (WCHAR)(NativeLanguage / 256);
                    NativeLangId[0] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');

                    NativeLanguage %= 256;
                    nDigit = (WCHAR)(NativeLanguage / 16);
                    NativeLangId[1] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');

                    nDigit = (WCHAR)(NativeLanguage % 16);
                    NativeLangId[2] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');
                    NativeLangId[3] = L'\0';
                }
            }

            status = PerfGetNames (
                dwQueryType,
                &usLocalValue,
                lpData,
                lpcbData,
                lpcbLen,
                lpLangId);

            if (!NT_SUCCESS(status)) {
                // convert error to win32 for return
                lFnStatus = (LONG)RtlNtStatusToDosError(status);
            }
            
            if (ARGUMENT_PRESENT (lpType)) { 
                // test for optional value
                *lpType = REG_MULTI_SZ;
            }

            ReleaseMutex (hGlobalDataMutex);
        }
    } else {
	    // define info block for data collection
	    COLLECT_THREAD_DATA CollectThreadData = {0, NULL, NULL, NULL, NULL, NULL, 0, 0};

        //
        //  Format Return Buffer: start with basic data block
        //

        TotalLen = sizeof(PERF_DATA_BLOCK) +
                   ((CNLEN+sizeof(UNICODE_NULL))*sizeof(WCHAR));

        if ( *lpcbData < TotalLen ) {
            Win32Error = ERROR_MORE_DATA;
        } else {

            // foreign data provider will return the perf data header

            if (dwQueryType == QUERY_FOREIGN) {

                // reset the values to avoid confusion

                // *lpcbData = 0;  // 0 bytes  (removed to enable foreign computers)
                pDataDefinition = (LPVOID)lpData;
                memset (lpData, 0, sizeof (PERF_DATA_BLOCK)); // clear out header

            } else {

                MonBuildPerfDataBlock(pPerfDataBlock,
                                    (PVOID *) &pDataDefinition,
                                    0,
                                    PROCESSOR_OBJECT_TITLE_INDEX);
            }

            CollectThreadData.dwQueryType = dwQueryType;
            CollectThreadData.lpValueName = usLocalValue.Buffer,
            CollectThreadData.lpData = lpData;
            CollectThreadData.lpcbData = lpcbData;
            CollectThreadData.lppDataDefinition = &pDataDefinition;
            CollectThreadData.pCurrentExtObject = NULL;
            CollectThreadData.lReturnValue = ERROR_SUCCESS;
            CollectThreadData.dwActionFlags = CTD_AF_NO_ACTION;

            if (hCollectThread == NULL) {
                // then call the function directly and hope for the best
                Win32Error = QueryExtensibleData (
                    &CollectThreadData);
            } else {
                // collect the data in a separate thread
                // load the args
                // set event to get things going
                SetEvent (hCollectEvents[COLLECT_THREAD_PROCESS_EVENT]);

                // now wait for the thread to return
                Win32Error = WaitForSingleObject (
                    hCollectEvents[COLLECT_THREAD_DONE_EVENT],
                    COLLECTION_WAIT_TIME);

                if (Win32Error == WAIT_TIMEOUT) {
                    bThreadHung = TRUE;
                    // log error

                    if (lEventLogLevel >= LOG_USER) {
                        LPSTR   szMessageArray[2];
                        WORD    wStringIndex;
                        // load data for eventlog message
                        wStringIndex = 0;
                        if (CollectThreadData.pCurrentExtObject != NULL) {
                            szMessageArray[wStringIndex++] = 
                                CollectThreadData.pCurrentExtObject->szCollectProcName;
                        } else {
                            szMessageArray[wStringIndex++] = "Unknown";
                        }

                        ReportEventA (hEventLog,
                            EVENTLOG_ERROR_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)PERFLIB_COLLECTION_HUNG,              // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            0,                          // sizeof raw data
                            szMessageArray,             // message text array
                            NULL);                      // raw data
                        
                    }

                    DisablePerfLibrary (CollectThreadData.pCurrentExtObject);

                    KdPrint (("\nPERFLIB: Collection thread is hung in %s", 
                        CollectThreadData.pCurrentExtObject->szCollectProcName != NULL ?
                        CollectThreadData.pCurrentExtObject->szCollectProcName : "Unknown"));
                    // and then wait forever for the thread to return
                    // this is done to prevent the function from returning
                    // while the collection thread is using the buffer
                    // passed in by the calling function and causing 
                    // all kind of havoc should the buffer be changed and/or
                    // deleted and then have the thread continue for some reason

                    Win32Error = WaitForSingleObject (
                        hCollectEvents[COLLECT_THREAD_DONE_EVENT],
                        INFINITE);

                } 
                bThreadHung = FALSE;    // in case it was true, but came out
                // here the thread has returned so continue on
                Win32Error = CollectThreadData.lReturnValue;
            }
#if 0
            if (CollectThreadData.dwActionFlags != CTD_AF_NO_ACTION) {
                if (CollectThreadData.dwActionFlags == CTD_AF_OPEN_THREAD) {
                    OpenCollectionThread();
                } else if (CollectThreadData.dwActionFlags == CTD_AF_CLOSE_THREAD) {
                    CloseCollectionThread();
                } else {
                    assert (CollectThreadData.dwActionFlags != 0);
                }
            }
#endif
        }

        // if an error was encountered, return it

        if (Win32Error != ERROR_SUCCESS) {
            lFnStatus = Win32Error;
        } else {
            //
            //  Final housekeeping for data return: note data size
            //

            TotalLen = (DWORD) ((PCHAR) pDataDefinition - (PCHAR) lpData);
            *lpcbData = TotalLen;

            pPerfDataBlock->TotalByteLength = TotalLen;

            lFnStatus = ERROR_SUCCESS;
        }

        if (ARGUMENT_PRESENT (lpcbLen)) { // test for optional parameter
            *lpcbLen = TotalLen;
        }

        if (ARGUMENT_PRESENT (lpType)) { // test for optional value
            *lpType = REG_BINARY;
        }
    }

PRQV_ErrorExit1:
    // reset thread to original priority
    if (lOldPriority > 0) {
        NtSetInformationThread(
            NtCurrentThread(),
            ThreadPriority,
            &lOldPriority,
            sizeof(lOldPriority)
            );
    }

    if (usLocalValue.Buffer != NULL) {
        // restore the value string if it was from the local static buffer
        // then free the local buffer
        if (lpValueName == &NtCurrentTeb( )->StaticUnicodeString) {
            memcpy (lpValueName->Buffer, usLocalValue.Buffer, usLocalValue.MaximumLength);
            RtlFreeUnicodeString (&usLocalValue);
        }
    }

    HEAP_PROBE();
    return lFnStatus;
}


LONG
PerfRegCloseKey
  (
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes all performance handles when the usage count drops to 0.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS status;
    LARGE_INTEGER   liQueryWaitTime ;

    HANDLE  hObjMutex;

    LONG    lReturn = ERROR_SUCCESS;

    pExtObject  pThisExtObj, pNextExtObj;
    //
    // Set the handle to NULL so that RPC knows that it has been closed.
    //

    if ((*phKey != HKEY_PERFORMANCE_DATA) &&
        (*phKey != HKEY_PERFORMANCE_TEXT) &&
        (*phKey != HKEY_PERFORMANCE_NLSTEXT)) {
        *phKey = NULL;
        return ERROR_SUCCESS;
    }

    *phKey = NULL;

    if (NumberOfOpens == 0) {
//        KdPrint (("\nPERFLIB: [Close] Pid: %d, Number Of PerflibHandles: %d",
//            GetCurrentProcessId(), NumberOfOpens));
        return ERROR_SUCCESS;
    }

    // wait for ext obj list to be "un"-busy

    liQueryWaitTime.QuadPart = MakeTimeOutValue (CLOSE_WAIT_TIME);
    status = NtWaitForSingleObject (
        hExtObjListIsNotInUse,
        FALSE,
        &liQueryWaitTime);

    if (status != WAIT_TIMEOUT) {
        // then the list is inactive so continue
        if (hGlobalDataMutex != NULL) {   // if a mutex was allocated, then use it

            // if here, then assume a mutex is ready

            liQueryWaitTime.QuadPart = MakeTimeOutValue(CLOSE_WAIT_TIME);

            status = NtWaitForSingleObject (
                hGlobalDataMutex, // semaphore
                FALSE,          // not alertable
                &liQueryWaitTime);          // wait forever

            if (status != WAIT_TIMEOUT) {
                // now we have a lock on the global data, so continue
                NumberOfOpens--;
                if (!NumberOfOpens) {

                    // walk down list of known objects and close and delete each one
                    pNextExtObj = ExtensibleObjects;
                    while (pNextExtObj != NULL) {
                        // close and destroy each entry in the list
                        pThisExtObj = pNextExtObj;
                        hObjMutex = pThisExtObj->hMutex;
                        status = NtWaitForSingleObject (
                            hObjMutex,
                            FALSE,
                            &liQueryWaitTime);

                        if (status != WAIT_TIMEOUT) {
                            InterlockedIncrement((LONG *)&pThisExtObj->dwLockoutCount);
                            status = CloseExtObjectLibrary(pThisExtObj, TRUE);

                            // close the handle to the perf subkey
                            NtClose (pThisExtObj->hPerfKey);

                            ReleaseMutex (hObjMutex);   // release
                            CloseHandle (hObjMutex);    // and free
                            pNextExtObj = pThisExtObj->pNext;

                            // toss the memory for this object
                            FREEMEM (pThisExtObj);
                        } else {
                            // this shouldn't happen since we've locked the
                            // list of objects
                            KdPrint (("\nPERFLIB: Unable to lock object %ws for closing",
                                pThisExtObj->szServiceName));
                            pNextExtObj = pThisExtObj->pNext;
                        }
                    }

                    // close the global objects
                    FREEMEM(pComputerName);
                    ComputerNameLength = 0;
                    pComputerName = NULL;

                    ExtensibleObjects = NULL;
                    NumExtensibleObjects = 0;

                    // close the timer thread
                    DestroyPerflibFunctionTimer ();

                    if (hEventLog != NULL) {
                        DeregisterEventSource (hEventLog);
                        hEventLog = NULL;
                    } // else the event log has already been closed

                    ReleaseMutex (hGlobalDataMutex);
                    CloseHandle (hGlobalDataMutex);
                    hGlobalDataMutex = NULL;

                    // release event handle
                    CloseHandle (hExtObjListIsNotInUse);
                    hExtObjListIsNotInUse = NULL;

//                    CloseCollectionThread();

                    if (ghKeyPerflib != NULL) {
                        RegCloseKey(ghKeyPerflib);
                        ghKeyPerflib = NULL;
                    }

                    if (lpPerflibSectionAddr != NULL) {
                        UnmapViewOfFile (lpPerflibSectionAddr);
                        lpPerflibSectionAddr = NULL;
                        CloseHandle (hPerflibSectionMap);
                        hPerflibSectionMap = NULL;
                        CloseHandle (hPerflibSectionFile);
                        hPerflibSectionFile = NULL;
                    }
                } else {
                    // this isn't the last open call so return success
                    ReleaseMutex (hGlobalDataMutex);
                }
            } else {
                // unable to lock the global data mutex in a timely fashion
                // so return
                lReturn = ERROR_BUSY;
            }
        } else {
            // if there's no mutex then something's fishy. It probably hasn't
            // been opened, yet.
            lReturn = ERROR_NOT_READY;
        }
    } else {
        // the object list is still in use so return and let the
        // caller try again later
        lReturn = WAIT_TIMEOUT;
    }

//    KdPrint (("\nPERFLIB: [Close] Pid: %d, Number Of PerflibHandles: %d",
//        GetCurrentProcessId(), NumberOfOpens));

    return lReturn;

}


LONG
PerfRegSetValue (
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN LPBYTE  lpData,
    IN DWORD cbData
    )
/*++

    PerfRegSetValue -   Set data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be REG_MULTI_SZ

            lpData          -   pointer to a buffer containing the
                                performance name

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the input buffer;

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/

{
    DWORD  dwQueryType;         //  type of request
    LPWSTR  lpLangId = NULL;
    NTSTATUS status;
    UNICODE_STRING String;

    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(Reserved);

    dwQueryType = GetQueryType (lpValueName);

    // convert the query to set commands
    if ((dwQueryType == QUERY_COUNTER) ||
        (dwQueryType == QUERY_ADDCOUNTER)) {
        dwQueryType = QUERY_ADDCOUNTER;
    } else if ((dwQueryType == QUERY_HELP) ||
              (dwQueryType == QUERY_ADDHELP)) {
        dwQueryType = QUERY_ADDHELP;
    } else {
        status = ERROR_BADKEY;
        goto Error_exit;
    }

    if (hKey == HKEY_PERFORMANCE_TEXT) {
        lpLangId = DefaultLangId;
    } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
        lpLangId = NativeLangId;

        if (*lpLangId == L'\0') {
            // build the native language id
            LANGID   iLanguage;
            WCHAR    NativeLanguage;
            WCHAR    nDigit;

            iLanguage = GetUserDefaultLangID();
            NativeLanguage = (WCHAR) MAKELANGID (iLanguage & 0x0ff, LANG_NEUTRAL);

            nDigit =  (WCHAR)(NativeLanguage / 256);
            NativeLangId[0] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');

            NativeLanguage %= 256;
            nDigit = (WCHAR)(NativeLanguage / 16);
            NativeLangId[1] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');

            nDigit = (WCHAR)(NativeLanguage % 16);
            NativeLangId[2] = (WCHAR)(nDigit <= 9 ? nDigit + L'0' : nDigit + L'7');
            NativeLangId[3] = L'\0';
        }
    } else {
        status = ERROR_BADKEY;
        goto Error_exit;
    }

    RtlInitUnicodeString(&String, lpValueName);

    status = PerfGetNames (
        dwQueryType,
        &String,
        lpData,
        &cbData,
        NULL,
        lpLangId);

    if (!NT_SUCCESS(status)) {
        status = (error_status_t)RtlNtStatusToDosError(status);
    }

Error_exit:
    return (status);
}


LONG
PerfRegEnumKey (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT PUNICODE_STRING lpClass OPTIONAL,
    OUT PFILETIME lpftLastWriteTime OPTIONAL
    )

/*++

Routine Description:

    Enumerates keys under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumKeyEx.  Returns that there are no such keys.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    if ( 0 ) {
        DBG_UNREFERENCED_PARAMETER(hKey);
        DBG_UNREFERENCED_PARAMETER(dwIndex);
        DBG_UNREFERENCED_PARAMETER(lpReserved);
    }

    lpName->Length = 0;

    if (ARGUMENT_PRESENT (lpClass)) {
        lpClass->Length = 0;
    }

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }

    return ERROR_NO_MORE_ITEMS;
}


LONG
PerfRegQueryInfoKey (
    IN HKEY hKey,
    OUT PUNICODE_STRING lpClass,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    This returns information concerning the predefined handle
    HKEY_PERFORMANCE_DATA

Arguments:

    Same as RegQueryInfoKey.

Return Value:

    Returns ERROR_SUCCESS (0) for success.

--*/

{
    DWORD TempLength=0;
    DWORD MaxValueLen=0;
    UNICODE_STRING Null;
    SECURITY_DESCRIPTOR     SecurityDescriptor;
    HKEY                    hPerflibKey;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    NTSTATUS                PerfStatus = ERROR_SUCCESS;
    UNICODE_STRING          PerflibSubKeyString;
    BOOL                    bGetSACL = TRUE;

    if ( 0 ) {
        DBG_UNREFERENCED_PARAMETER(lpReserved);
    }

    if (lpClass->MaximumLength >= sizeof(UNICODE_NULL)) {
        lpClass->Length = 0;
        *lpClass->Buffer = UNICODE_NULL;
    }
    *lpcSubKeys = 0;
    *lpcbMaxSubKeyLen = 0;
    *lpcbMaxClassLen = 0;
    *lpcValues = NUM_VALUES;
    *lpcbMaxValueNameLen = VALUE_NAME_LENGTH;
    *lpcbMaxValueLen = 0;

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }
    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        //
        // We have to go enumerate the values to determine the answer for
        // the MaxValueLen parameter.
        //
        Null.Buffer = NULL;
        Null.Length = 0;
        Null.MaximumLength = 0;
        PerfStatus = PerfEnumTextValue(hKey,
                          0,
                          &Null,
                          NULL,
                          NULL,
                          NULL,
                          &MaxValueLen,
                          NULL);
        if (PerfStatus == ERROR_SUCCESS) {
            PerfStatus = PerfEnumTextValue(hKey,
                            1,
                            &Null,
                            NULL,
                            NULL,
                            NULL,
                            &TempLength,
                            NULL);
        }

        if (PerfStatus == ERROR_SUCCESS) {
            if (TempLength > MaxValueLen) {
                MaxValueLen = TempLength;
            }
            *lpcbMaxValueLen = MaxValueLen;
        } else {
            // unable to successfully enum text values for this
            // key so return 0's and the error code
            *lpcValues = 0;
            *lpcbMaxValueNameLen = 0;
        }
    }

    if (PerfStatus == ERROR_SUCCESS) {
        // continune if all is OK
        // now get the size of SecurityDescriptor for Perflib key

        RtlInitUnicodeString (
            &PerflibSubKeyString,
            PerflibKey);


        //
        // Initialize the OBJECT_ATTRIBUTES structure and open the key.
        //
        InitializeObjectAttributes(
                &Obja,
                &PerflibSubKeyString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );


        Status = NtOpenKey(
                    &hPerflibKey,
                    MAXIMUM_ALLOWED | ACCESS_SYSTEM_SECURITY,
                    &Obja
                    );

        if ( ! NT_SUCCESS( Status )) {
            Status = NtOpenKey(
                    &hPerflibKey,
                    MAXIMUM_ALLOWED,
                    &Obja
                    );
            bGetSACL = FALSE;
        }

        if ( ! NT_SUCCESS( Status )) {
            KdPrint (("\nPERFLIB: Unable to open Perflib Key. Status: %d", Status));
        } else {

            *lpcbSecurityDescriptor = 0;

            if (bGetSACL == FALSE) {
                //
                // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP
                // and DACL. These three are always accessible (or inaccesible)
                // as a set.
                //
                Status = NtQuerySecurityObject(
                        hPerflibKey,
                        OWNER_SECURITY_INFORMATION
                        | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION,
                        &SecurityDescriptor,
                        0,
                        lpcbSecurityDescriptor
                        );
            } else {
                //
                // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP,
                // DACL, and SACL.
                //
                Status = NtQuerySecurityObject(
                            hPerflibKey,
                            OWNER_SECURITY_INFORMATION
                            | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION
                            | SACL_SECURITY_INFORMATION,
                            &SecurityDescriptor,
                            0,
                            lpcbSecurityDescriptor
                            );
            }

            if( Status != STATUS_BUFFER_TOO_SMALL ) {
                *lpcbSecurityDescriptor = 0;
            } else {
                // this is expected so set status to success
                Status = STATUS_SUCCESS;
            }

            NtClose(hPerflibKey);
        }
        if (NT_SUCCESS( Status )) {
            PerfStatus = ERROR_SUCCESS;
        } else {
            // return error
            PerfStatus = (DWORD)RtlNtStatusToDosError(Status);
        }
    } // else return status


    return PerfStatus;
}


LONG
PerfRegEnumValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )

/*++

Routine Description:

    Enumerates Values under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    USHORT cbNameSize;

    // table of names used by enum values
    UNICODE_STRING ValueNames[NUM_VALUES];

    ValueNames [0].Length = (WORD)(lstrlenW (GLOBAL_STRING) * sizeof(WCHAR));
    ValueNames [0].MaximumLength = (WORD)(ValueNames [0].Length + sizeof(UNICODE_NULL));
    ValueNames [0].Buffer =  (LPWSTR)GLOBAL_STRING;
    ValueNames [1].Length = (WORD)(lstrlenW(COSTLY_STRING) * sizeof(WCHAR));
    ValueNames [1].MaximumLength = (WORD)(ValueNames [1].Length + sizeof(UNICODE_NULL));
    ValueNames [1].Buffer = (LPWSTR)COSTLY_STRING;

    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(PerfEnumTextValue(hKey,
                                  dwIndex,
                                  lpValueName,
                                  lpReserved,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen));
    }

    if ( dwIndex >= NUM_VALUES ) {

        //
        // This is a request for data from a non-existent value name
        //

        *lpcbData = 0;

        return ERROR_NO_MORE_ITEMS;
    }

    cbNameSize = ValueNames[dwIndex].Length;

    if ( lpValueName->MaximumLength < cbNameSize ) {
        return ERROR_MORE_DATA;
    } else {

         lpValueName->Length = cbNameSize;
         RtlCopyUnicodeString(lpValueName, &ValueNames[dwIndex]);

         if (ARGUMENT_PRESENT (lpType)) {
            *lpType = REG_BINARY;
         }

         return PerfRegQueryValue(hKey,
                                  lpValueName,
                                  NULL,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen);

    }
}


LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

Routine Description:

    Enumerates Values under Perflib\lang

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING FullValueName;
    LONG            lReturn = ERROR_SUCCESS;

    //
    // Only two values, "Counter" and "Help"
    //
    if (dwIndex==0) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, CounterValue);
    } else if (dwIndex==1) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, HelpValue);
    } else {
        return(ERROR_NO_MORE_ITEMS);
    }
    RtlCopyUnicodeString(lpValueName, &FullValueName);

    //
    // We need to NULL terminate the name to make RPC happy.
    //
    if (lpValueName->Length+sizeof(WCHAR) <= lpValueName->MaximumLength) {
        lpValueName->Buffer[lpValueName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        lpValueName->Length += sizeof(UNICODE_NULL);
    }

    lReturn = PerfRegQueryValue(hKey,
                             &FullValueName,
                             lpReserved,
                             lpType,
                             lpData,
                             lpcbData,
                             lpcbLen);

    return lReturn;

}


DWORD
CloseExtObjectLibrary (
    pExtObject  pObj,
    BOOL        bCloseNow
)
/*++

  CloseExtObjectLibrary
    Closes and unloads the specified performance counter library and
    deletes all references to the functions.

    The unloader is "lazy" in that it waits for the library to be
    inactive for a specified time before unloading. This is due to the
    fact that Perflib can not ever be certain that no thread will need
    this library from one call to the next. In order to prevent "thrashing"
    due to constantly loading and unloading of the library, the unloading
    is delayed to make sure it's not really needed.

    This function expects locked and exclusive access to the object while
    it is opening. This must be provided by the calling function.

 Arguments:

    pObj    -- pointer to the object information structure of the
                perf object to close

    bCloseNow -- the flag to indicate the library should be closed
                immediately. This is the result of the calling function
                closing the registry key.

--*/
{
    DWORD       Status = ERROR_SUCCESS;
    LONGLONG    TimeoutTime;

    if (pObj->hLibrary != NULL) {
        // get current time to test timeout
        TimeoutTime = GetTimeAsLongLong();
        // timeout time is in ms
        TimeoutTime -= dwThreadAndLibraryTimeout;

        // don't close the library unless the object hasn't been accessed for
        // a while or the caller is closing the key

        if ((TimeoutTime > pObj->llLastUsedTime) || bCloseNow) {

            // don't toss if this library has the "keep" flag set and this
            // isn't a "close now" case

            if (!bCloseNow && (pObj->dwFlags & PERF_EO_KEEP_RESIDENT)) {
                // keep it loaded until the key is closed.
            } else {
                // then this is the last one to close the library
                // free library

                try {
                    // call close function for this DLL
                    Status = (*pObj->CloseProc)();
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                }

                FreeLibrary (pObj->hLibrary);
                pObj->hLibrary = NULL;

                // clear all pointers that are now invalid
                pObj->OpenProc = NULL;
                pObj->CollectProc = NULL;
                pObj->QueryProc = NULL;
                pObj->CloseProc = NULL;
                InterlockedIncrement((LONG *)&pObj->dwCloseCount);

                pObj->llLastUsedTime = 0;
            }
        }

        Status = ERROR_SUCCESS;
    } else {
        // already closed
        Status = ERROR_SUCCESS;
    }

    return Status;
}


DWORD
OpenExtObjectLibrary (
    pExtObject  pObj
)
/*++

 OpenExtObjectLibrary

    Opens the specified library and looks up the functions used by
    the performance library. If the library is successfully
    loaded and opened then the open procedure is called to initialize
    the object.

    This function expects locked and exclusive access to the object while
    it is opening. This must be provided by the calling function.

 Arguments:

    pObj    -- pointer to the object information structure of the
                perf object to close

--*/
{
    DWORD   FnStatus = ERROR_SUCCESS;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwOpenEvent;
    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwValue;

    // variables used for event logging
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    ULONG_PTR   dwRawDataDwords[8];
    LPWSTR  szMessageArray[8];

    HANDLE  hTimeOutEvent = NULL;
    HANDLE  hPerflibFuncTimer = NULL;
    DllValidationData   CurrentDllData;

    OPEN_PROC_WAIT_INFO opwInfo;
    UINT    nErrorMode;

    BOOL    bUseTimer;
    // check to see if the library has already been opened

    if (pObj->dwFlags & PERF_EO_DISABLED) return ERROR_SERVICE_DISABLED;

    if (pObj->hLibrary == NULL) {
        // library isn't loaded yet, so
        // check to see if this function is enabled

        dwType = 0;
        dwSize = sizeof (dwValue);
        dwValue = 0;
        Status = PrivateRegQueryValueExW (
            pObj->hPerfKey,
            DisablePerformanceCounters,
            NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);

        if ((Status == ERROR_SUCCESS) &&
            (dwType == REG_DWORD) &&
            (dwValue == 1)) {
            // then DON'T Load this library
            pObj->dwFlags |= PERF_EO_DISABLED;
        } else {
            // set the error status & the flag value
            Status = ERROR_SUCCESS;
            pObj->dwFlags &= ~PERF_EO_DISABLED;
        }

        if ((Status == ERROR_SUCCESS)  && 
            (pObj->LibData.FileSize > 0)) {

            if (ServiceIsTrustedByDefault(pObj->szServiceName)) {
                // then set as trusted and continue
                pObj->dwFlags |= PERF_EO_TRUSTED;
            } else {
                // see if this is a trusted file or a file that has been updated
                // get the file information
                memset (&CurrentDllData, 0, sizeof(CurrentDllData));
                Status = GetPerfDllFileInfo (
                    pObj->szLibraryName,
                    &CurrentDllData);

                if (Status == ERROR_SUCCESS) {
                    // compare file data to registry data and update flags
                    if ((*(LONGLONG *)&pObj->LibData.CreationDate) ==
                        (*(LONGLONG *)&CurrentDllData.CreationDate) &&
                        (pObj->LibData.FileSize == CurrentDllData.FileSize)) {
                        pObj->dwFlags |= PERF_EO_TRUSTED;
                    } else {
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;

                        ReportEvent (hEventLog,
                            EVENTLOG_WARNING_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)PERFLIB_NOT_TRUSTED_FILE,  // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            0,                          // sizeof raw data
                            szMessageArray,             // message text array
                            NULL);                       // raw data
                    }
                }
            }
        }

        if ((Status == ERROR_SUCCESS) && (!(pObj->dwFlags & PERF_EO_DISABLED))) {
            //  go ahead and load it
            nErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);
            // then load library & look up functions
            pObj->hLibrary = LoadLibraryExW (pObj->szLibraryName,
                NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

            if (pObj->hLibrary != NULL) {
                // lookup function names
                pObj->OpenProc = (OPENPROC)GetProcAddress(
                    pObj->hLibrary, pObj->szOpenProcName);
                if (pObj->OpenProc == NULL) {
                    if (lEventLogLevel >= LOG_USER) {
                        Status = GetLastError();
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        dwRawDataDwords[dwDataIndex++] =
                            (ULONG_PTR)Status;
                        szMessageArray[wStringIndex++] =
                            // BUGBUG: the ansi name should be converted for
                            // the message
                            (LPWSTR)L" ";
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;

                        ReportEvent (hEventLog,
                            EVENTLOG_ERROR_TYPE,        // error type
                            0,                          // category (not used)
                            (DWORD)PERFLIB_OPEN_PROC_NOT_FOUND,              // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                            szMessageArray,             // message text array
                            (LPVOID)&dwRawDataDwords[0]);           // raw data

                    }
                    DisablePerfLibrary (pObj);
                }

                if (Status == ERROR_SUCCESS) {
                    if (pObj->dwFlags & PERF_EO_QUERY_FUNC) {
                        pObj->QueryProc = (QUERYPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->CollectProc = (COLLECTPROC)pObj->QueryProc;
                    } else {
                        pObj->CollectProc = (COLLECTPROC)GetProcAddress (
                            pObj->hLibrary, pObj->szCollectProcName);
                        pObj->QueryProc = (QUERYPROC)pObj->CollectProc;
                    }

                    if (pObj->CollectProc == NULL) {
                        if (lEventLogLevel >= LOG_USER) {
                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (ULONG_PTR)Status;
                            szMessageArray[wStringIndex++] =
                                // BUGBUG: the ansi name should be converted for
                                // the message
                                (LPWSTR)L" ";
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEvent (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)PERFLIB_COLLECT_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }
                        DisablePerfLibrary (pObj);
                    }
                }

                if (Status == ERROR_SUCCESS) {
                    pObj->CloseProc = (CLOSEPROC)GetProcAddress (
                        pObj->hLibrary, pObj->szCloseProcName);

                    if (pObj->CloseProc == NULL) {
                        if (lEventLogLevel >= LOG_USER) {
                            Status = GetLastError();
                            // load data for eventlog message
                            dwDataIndex = wStringIndex = 0;
                            dwRawDataDwords[dwDataIndex++] =
                                (ULONG_PTR)Status;
                            szMessageArray[wStringIndex++] =
                                // BUGBUG: the ansi name should be converted for
                                // the message
                                (LPWSTR)L" ";
                            szMessageArray[wStringIndex++] =
                                pObj->szLibraryName;
                            szMessageArray[wStringIndex++] =
                                pObj->szServiceName;

                            ReportEvent (hEventLog,
                                EVENTLOG_ERROR_TYPE,        // error type
                                0,                          // category (not used)
                                (DWORD)PERFLIB_CLOSE_PROC_NOT_FOUND,              // event,
                                NULL,                       // SID (not used),
                                wStringIndex,               // number of strings
                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                szMessageArray,             // message text array
                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                        }

                        DisablePerfLibrary (pObj);
                    }
                }

                bUseTimer = TRUE;   // default
                if (!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) {
                    if (pObj->dwFlags & PERF_EO_TRUSTED) {
                        bUseTimer = FALSE;   // Trusted DLL's are not timed
                    }
                } else {
                    // disable DLL testing
                    bUseTimer = FALSE;   // Timing is disabled as well
                }

                if (Status == ERROR_SUCCESS) {
                    try {
                        // start timer
                        opwInfo.pNext = NULL;
                        opwInfo.szLibraryName = pObj->szLibraryName;
                        opwInfo.szServiceName = pObj->szServiceName;
                        opwInfo.dwWaitTime = pObj->dwOpenTimeout;
                        opwInfo.dwEventMsg = PERFLIB_OPEN_PROC_TIMEOUT;
                        opwInfo.pData = (LPVOID)pObj;
                        if (bUseTimer) {
                            hPerflibFuncTimer = StartPerflibFunctionTimer(&opwInfo);
                            // if no timer, continue anyway, even though things may
                            // hang, it's better than not loading the DLL since they
                            // usually load OK
                            //
                            if (hPerflibFuncTimer == NULL) {
                                // unable to get a timer entry
                                KdPrint (("\nPERFLIB: Unable to acquire timer for Open Proc"));
                            }
                        } else {
                            hPerflibFuncTimer = NULL;
                        }

                        // call open procedure to initialize DLL
                        FnStatus = (*pObj->OpenProc)(pObj->szLinkageString);
                        // check the result.
                        if (FnStatus != ERROR_SUCCESS) {
                            dwOpenEvent = PERFLIB_OPEN_PROC_FAILURE;
                        } else {
                            InterlockedIncrement((LONG *)&pObj->dwOpenCount);
                        }

                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        FnStatus = GetExceptionCode();
                        dwOpenEvent = PERFLIB_OPEN_PROC_EXCEPTION;
                    }

                    if (hPerflibFuncTimer != NULL) {
                        // kill timer
                        Status = KillPerflibFunctionTimer (hPerflibFuncTimer);
                        hPerflibFuncTimer = NULL;
                    }

                    if (FnStatus != ERROR_SUCCESS) {
                        // load data for eventlog message
                        dwDataIndex = wStringIndex = 0;
                        dwRawDataDwords[dwDataIndex++] =
                            (ULONG_PTR)FnStatus;
                        szMessageArray[wStringIndex++] =
                            pObj->szServiceName;
                        szMessageArray[wStringIndex++] =
                            pObj->szLibraryName;

                        ReportEventW (hEventLog,
                            (WORD)EVENTLOG_ERROR_TYPE, // error type
                            0,                          // category (not used)
                            dwOpenEvent,                // event,
                            NULL,                       // SID (not used),
                            wStringIndex,               // number of strings
                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                            szMessageArray,                // message text array
                            (LPVOID)&dwRawDataDwords[0]);           // raw data

                        if (dwOpenEvent == PERFLIB_OPEN_PROC_EXCEPTION) {
                            DisablePerfLibrary (pObj);
                        }
                    }
                }

                if (FnStatus != ERROR_SUCCESS) {
                    // clear fields
                    pObj->OpenProc = NULL;
                    pObj->CollectProc = NULL;
                    pObj->QueryProc = NULL;
                    pObj->CloseProc = NULL;
                    if (pObj->hLibrary != NULL) {
                        FreeLibrary (pObj->hLibrary);
                        pObj->hLibrary = NULL;
                    }
                    Status = FnStatus;
                } else {
                    pObj->llLastUsedTime = GetTimeAsLongLong();
                }
            } else {
                Status = GetLastError();
            }
            SetErrorMode (nErrorMode);
        }
    } else {
        // else already open so bump the ref count
        pObj->llLastUsedTime = GetTimeAsLongLong();
    }

    return Status;
}


pExtObject
AllocateAndInitializeExtObject (
    HKEY    hServicesKey,
    HKEY    hPerfKey,
    LPWSTR  szServiceName
)
/*++

 AllocateAndInitializeExtObject

    allocates and initializes an extensible object information entry
    for use by the performance library.

    a pointer to the initialized block is returned if all goes well,
    otherwise no memory is allocated and a null pointer is returned.

    The calling function must close the open handles and free this
    memory block when it is no longer needed.

 Arguments:

    hServicesKey    -- open registry handle to the
        HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services hey

    hPerfKey -- the open registry key to the Performance sub-key under
        the selected service

    szServiceName -- The name of the service

--*/
{
    LONG    Status;
    HKEY    hKeyLinkage;

    BOOL    bUseQueryFn = FALSE;

    pExtObject  pReturnObject = NULL;

    DWORD   dwType;
    DWORD   dwSize;
    DWORD   dwFlags = 0;
    DWORD   dwKeep;
    DWORD   dwObjectArray[MAX_PERF_OBJECTS_IN_QUERY_FUNCTION];
    DWORD   dwObjIndex = 0;
    DWORD   dwMemBlockSize = sizeof(ExtObject);
    DWORD   dwLinkageStringLen = 0;

    CHAR    szOpenProcName[MAX_PATH];
    CHAR    szCollectProcName[MAX_PATH];
    CHAR    szCloseProcName[MAX_PATH];
    WCHAR   szLibraryString[MAX_PATH];
    WCHAR   szLibraryExpPath[MAX_PATH];
    WCHAR   mszObjectList[MAX_PATH];
    WCHAR   szLinkageKeyPath[MAX_PATH];
    LPWSTR  szLinkageString = NULL;     // max path wasn't enough for some paths

    DllValidationData   DllVD;
    FILETIME    LocalftLastGoodDllFileDate;

    DWORD   dwOpenTimeout;
    DWORD   dwCollectTimeout;

    LPWSTR  szThisObject;
    LPWSTR  szThisChar;

    LPSTR   pNextStringA;
    LPWSTR  pNextStringW;

    WCHAR   szMutexName[MAX_PATH];
    WCHAR   szPID[32];
  
    // read the performance DLL name

    dwType = 0;
    dwSize = sizeof(szLibraryString);
    memset (szLibraryString, 0, sizeof(szLibraryString));
    memset (szLibraryString, 0, sizeof(szLibraryExpPath));

    Status = PrivateRegQueryValueExW (hPerfKey,
                            DLLValue,
                            NULL,
                            &dwType,
                            (LPBYTE)szLibraryString,
                            &dwSize);

    if (Status == ERROR_SUCCESS) {
        if (dwType == REG_EXPAND_SZ) {
            // expand any environment vars
            dwSize = ExpandEnvironmentStringsW(
                szLibraryString,
                szLibraryExpPath,
                MAX_PATH);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);
            }
        } else if (dwType == REG_SZ) {
            // look for dll and save full file Path
            dwSize = SearchPathW (
                NULL,   // use standard system search path
                szLibraryString,
                NULL,
                MAX_PATH,
                szLibraryExpPath,
                NULL);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                Status = ERROR_INVALID_DLL;
            } else {
                dwSize += 1;
                dwSize *= sizeof(WCHAR);
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);
            }
        } else {
            Status = ERROR_INVALID_DLL;
        }

        if (Status == ERROR_SUCCESS) {
            // we have the DLL name so get the procedure names
            dwType = 0;
            dwSize = sizeof(szOpenProcName);
            memset (szOpenProcName, 0, sizeof(szOpenProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    OpenValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szOpenProcName,
                                    &dwSize);
        }

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += DWORD_MULTIPLE(dwSize);

            // we have the procedure name so get the timeout value
            dwType = 0;
            dwSize = sizeof(dwOpenTimeout);
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    OpenTimeout,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwOpenTimeout,
                                    &dwSize);

            // if error, then apply default
            if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                dwOpenTimeout = dwExtCtrOpenProcWaitMs;
                Status = ERROR_SUCCESS;
            }

        }

        if (Status == ERROR_SUCCESS) {
            // get next string

            dwType = 0;
            dwSize = sizeof(szCloseProcName);
            memset (szCloseProcName, 0, sizeof(szCloseProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    CloseValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCloseProcName,
                                    &dwSize);
        }

        if (Status == ERROR_SUCCESS) {
            // add in size of previous string
            // the size value includes the Term. NULL
            dwMemBlockSize += DWORD_MULTIPLE(dwSize);

            // try to look up the query function which is the
            // preferred interface if it's not found, then
            // try the collect function name. If that's not found,
            // then bail
            dwType = 0;
            dwSize = sizeof(szCollectProcName);
            memset (szCollectProcName, 0, sizeof(szCollectProcName));
            Status = PrivateRegQueryValueExA (hPerfKey,
                                    QueryValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szCollectProcName,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                // add in size of the Query Function Name
                // the size value includes the Term. NULL
                dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                // get next string

                bUseQueryFn = TRUE;
                // the query function can support a static object list
                // so look it up

            } else {
                // the QueryFunction wasn't found so look up the
                // Collect Function name instead
                dwType = 0;
                dwSize = sizeof(szCollectProcName);
                memset (szCollectProcName, 0, sizeof(szCollectProcName));
                Status = PrivateRegQueryValueExA (hPerfKey,
                                        CollectValue,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)szCollectProcName,
                                        &dwSize);

                if (Status == ERROR_SUCCESS) {
                    // add in size of Collect Function Name
                    // the size value includes the Term. NULL
                    dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                }
            }

            if (Status == ERROR_SUCCESS) {
                // we have the procedure name so get the timeout value
                dwType = 0;
                dwSize = sizeof(dwCollectTimeout);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        CollectTimeout,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwCollectTimeout,
                                        &dwSize);

                // if error, then apply default
                if ((Status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwCollectTimeout = dwExtCtrOpenProcWaitMs;
                    Status = ERROR_SUCCESS;
                }

            }
            // get the list of supported objects if provided by the registry

            dwType = 0;
            dwSize = sizeof(mszObjectList);
            memset (mszObjectList, 0, sizeof(mszObjectList));
            Status = PrivateRegQueryValueExW (hPerfKey,
                                    ObjListValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)mszObjectList,
                                    &dwSize);

            if (Status == ERROR_SUCCESS) {
                if (dwType != REG_MULTI_SZ) {
                    // convert space delimited list to msz
                    for (szThisChar = mszObjectList; *szThisChar != 0; szThisChar++) {
                        if (*szThisChar == L' ') *szThisChar = L'\0';
                    }
                    ++szThisChar;
                    *szThisChar = 0; // add MSZ term Null
                }
                for (szThisObject = mszObjectList, dwObjIndex = 0;
                    (*szThisObject != 0) && (dwObjIndex < MAX_PERF_OBJECTS_IN_QUERY_FUNCTION);
                    szThisObject += lstrlenW(szThisObject) + 1) {
                    dwObjectArray[dwObjIndex] = wcstoul(szThisObject, NULL, 10);
                    dwObjIndex++;
                }
                if (*szThisObject != 0) {
                    // BUGBUG: log error idicating too many object ID's are
                    // in the list.
                }
            } else {
                // reset status since not having this is
                //  not a showstopper
                Status = ERROR_SUCCESS;
            }

            if (Status == ERROR_SUCCESS) {
                dwType = 0;
                dwKeep = 0;
                dwSize = sizeof(dwKeep);
                Status = PrivateRegQueryValueExW (hPerfKey,
                                        KeepResident,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)&dwKeep,
                                        &dwSize);

                if ((Status == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                    if (dwKeep == 1) {
                        dwFlags |= PERF_EO_KEEP_RESIDENT;
                    } else {
                        // no change.
                    }
                } else {
                    // not fatal, just use the defaults.
                    Status = ERROR_SUCCESS;
                }

            }
        }
    }

    if (Status == ERROR_SUCCESS) {
        // get Library validation time
        dwType = 0;
        dwSize = sizeof(DllVD);
        memset (&DllVD, 0, sizeof(DllVD));
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszLibraryValidationData,
                                NULL,
                                &dwType,
                                (LPBYTE)&DllVD,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) || 
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (DllVD))){
            // then set this entry to be 0
            memset (&DllVD, 0, sizeof(DllVD));
            // and clear the error
            Status = ERROR_SUCCESS;
        } 
    }

    if (Status == ERROR_SUCCESS) {
        // get the file timestamp of the last successfully accessed file
        dwType = 0;
        dwSize = sizeof(LocalftLastGoodDllFileDate);
        memset (&LocalftLastGoodDllFileDate, 0, sizeof(LocalftLastGoodDllFileDate));
        Status = PrivateRegQueryValueExW (hPerfKey,
                                cszSuccessfulFileData,
                                NULL,
                                &dwType,
                                (LPBYTE)&LocalftLastGoodDllFileDate,
                                &dwSize);

        if ((Status != ERROR_SUCCESS) || 
            (dwType != REG_BINARY) ||
            (dwSize != sizeof (LocalftLastGoodDllFileDate))) {
            // then set this entry to be Invalid
            memset (&LocalftLastGoodDllFileDate, 0xFF, sizeof(LocalftLastGoodDllFileDate));
            // and clear the error
            Status = ERROR_SUCCESS;
        } 
    }

    if (Status == ERROR_SUCCESS) {
        lstrcpyW (szLinkageKeyPath, szServiceName);
        lstrcatW (szLinkageKeyPath, LinkageKey);

        Status = RegOpenKeyExW (
            hServicesKey,
            szLinkageKeyPath,
            0L,
            KEY_READ,
            &hKeyLinkage);

        if (Status == ERROR_SUCCESS) {
            // look up[ export value string
            dwSize = 0;
            dwType = 0;
            Status = PrivateRegQueryValueExW (
                hKeyLinkage,
                ExportValue,
                NULL,
                &dwType,
                NULL,
                &dwSize);
            // get size of string
            if (((Status != ERROR_SUCCESS) && (Status != ERROR_MORE_DATA)) ||
                ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                dwLinkageStringLen = 0;
                szLinkageString = NULL;
                // not finding a linkage key is not fatal so correct
                // status
                Status = ERROR_SUCCESS;
            } else {
                // allocate buffer
                szLinkageString = (LPWSTR)ALLOCMEM(dwSize);
                
                if (szLinkageString != NULL) {
                    // read string into buffer
                    dwType = 0;
                    Status = PrivateRegQueryValueExW (
                        hKeyLinkage,
                        ExportValue,
                        NULL,
                        &dwType,
                        (LPBYTE)szLinkageString,
                        &dwSize);

                    if ((Status != ERROR_SUCCESS) ||
                        ((dwType != REG_SZ) && (dwType != REG_MULTI_SZ))) {
                        // clear & release buffer
                        FREEMEM (szLinkageString);
                        szLinkageString = NULL;
                        dwLinkageStringLen = 0;
                        // not finding a linkage key is not fatal so correct
                        // status
                        Status = ERROR_SUCCESS;
                    } else {
                        // add size of linkage string to buffer
                        // the size value includes the Term. NULL
                        dwLinkageStringLen = dwSize;
                        dwMemBlockSize += DWORD_MULTIPLE(dwSize);
                    }
                } else {
                    // clear & release buffer
                    FREEMEM (szLinkageString);
                    szLinkageString = NULL;
                    dwLinkageStringLen = 0;
                    Status = ERROR_OUTOFMEMORY;
                }
            }
            RegCloseKey (hKeyLinkage);
        } else {
            // not finding a linkage key is not fatal so correct
            // status
            // clear & release buffer
            szLinkageString = NULL;
            dwLinkageStringLen = 0;
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS) {
        // add in size of service name
        dwSize = lstrlenW (szServiceName);
        dwSize += 1;
        dwSize *= sizeof(WCHAR);
        dwMemBlockSize += DWORD_MULTIPLE(dwSize);

        // allocate and initialize a new ext. object block
        pReturnObject = ALLOCMEM (dwMemBlockSize);

        if (pReturnObject != NULL) {
            // copy values to new buffer (all others are NULL)
            pNextStringA = (LPSTR)&pReturnObject[1];

            // copy Open Procedure Name
            pReturnObject->szOpenProcName = pNextStringA;
            lstrcpyA (pNextStringA, szOpenProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_DWORD(pNextStringA);

            pReturnObject->dwOpenTimeout = dwOpenTimeout;

            // copy collect function or query function, depending
            pReturnObject->szCollectProcName = pNextStringA;
            lstrcpyA (pNextStringA, szCollectProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_DWORD(pNextStringA);

            pReturnObject->dwCollectTimeout = dwCollectTimeout;

            // copy Close Procedure Name
            pReturnObject->szCloseProcName = pNextStringA;
            lstrcpyA (pNextStringA, szCloseProcName);

            pNextStringA += lstrlenA (pNextStringA) + 1;
            pNextStringA = ALIGN_ON_DWORD(pNextStringA);

            // copy Library path
            pNextStringW = (LPWSTR)pNextStringA;
            pReturnObject->szLibraryName = pNextStringW;
            lstrcpyW (pNextStringW, szLibraryExpPath);

            pNextStringW += lstrlenW (pNextStringW) + 1;
            pNextStringW = ALIGN_ON_DWORD(pNextStringW);

            // copy Linkage String if there is one
            if (szLinkageString != NULL) {
                pReturnObject->szLinkageString = pNextStringW;
                memcpy (pNextStringW, szLinkageString, dwLinkageStringLen);

                // length includes extra NULL char and is in BYTES
                pNextStringW += (dwLinkageStringLen / sizeof (WCHAR));
                pNextStringW = ALIGN_ON_DWORD(pNextStringW);
                // release the buffer now that it's been copied
                FREEMEM (szLinkageString);
                szLinkageString = NULL;
            }

            // copy Service name
            pReturnObject->szServiceName = pNextStringW;
            lstrcpyW (pNextStringW, szServiceName);

            pNextStringW += lstrlenW (pNextStringW) + 1;
            pNextStringW = ALIGN_ON_DWORD(pNextStringW);

            // load flags
            if (bUseQueryFn) {
                dwFlags |= PERF_EO_QUERY_FUNC;
            }
            pReturnObject->dwFlags =  dwFlags;

            pReturnObject->hPerfKey = hPerfKey;

            pReturnObject->LibData = DllVD; // validation data
            pReturnObject->ftLastGoodDllFileDate = LocalftLastGoodDllFileDate;

            // the default test level is "all tests"
            // if the file and timestamp work out OK, this can
            // be reset to the system test level
            pReturnObject->dwValidationLevel = EXT_TEST_ALL; 

            // load Object array
            if (dwObjIndex > 0) {
                pReturnObject->dwNumObjects = dwObjIndex;
                memcpy (pReturnObject->dwObjList,
                    dwObjectArray, (dwObjIndex * sizeof(dwObjectArray[0])));
            }

            pReturnObject->llLastUsedTime = 0;

            // create Mutex name
            lstrcpyW (szMutexName, szServiceName);
            lstrcatW (szMutexName, (LPCWSTR)L"_Perf_Library_Lock_PID_");
            _ultow ((ULONG)GetCurrentProcessId(), szPID, 16);
            lstrcatW (szMutexName, szPID);
    
            pReturnObject->hMutex = CreateMutexW (NULL, FALSE, szMutexName);
        } else {
            Status = ERROR_OUTOFMEMORY;
        }
    }

    if ((Status == ERROR_SUCCESS) && (lpPerflibSectionAddr != NULL)) {
        pPerfDataSectionHeader  pHead;
        DWORD           dwEntry;
        pPerfDataSectionRecord  pEntry;
        // init perf data section
        pHead = (pPerfDataSectionHeader)lpPerflibSectionAddr;
        pEntry = (pPerfDataSectionRecord)lpPerflibSectionAddr;
        // get the entry first 
        // the "0" entry is the header
        if (pHead->dwEntriesInUse < pHead->dwMaxEntries) {
            dwEntry = ++pHead->dwEntriesInUse;
            pReturnObject->pPerfSectionEntry = &pEntry[dwEntry];
            lstrcpynW (pReturnObject->pPerfSectionEntry->szServiceName,
                pReturnObject->szServiceName, PDSR_SERVICE_NAME_LEN);
        } else {
            // the list is full so bump the missing entry count
            pHead->dwMissingEntries++;
            pReturnObject->pPerfSectionEntry = NULL;
        }
    }


    if (Status != ERROR_SUCCESS) {
        SetLastError (Status);
    }

    return pReturnObject;
}


void
OpenExtensibleObjects (
)

/*++

Routine Description:

    This routine will search the Configuration Registry for modules
    which will return data at data collection time.  If any are found,
    and successfully opened, data structures are allocated to hold
    handles to them.

    The global data access in this section is protected by the
    hGlobalDataMutex acquired by the calling function.

Arguments:

    None.
                  successful open.

Return Value:

    None.

--*/

{

    DWORD dwIndex;               // index for enumerating services
    ULONG KeyBufferLength;       // length of buffer for reading key data
    ULONG ValueBufferLength;     // length of buffer for reading value data
    ULONG ResultLength;          // length of data returned by Query call
    HANDLE hPerfKey;             // Root of queries for performance info
    HANDLE hServicesKey;         // Root of services
    REGSAM samDesired;           // access needed to query
    NTSTATUS Status;             // generally used for Nt call result status
    ANSI_STRING AnsiValueData;   // Ansi version of returned strings
    UNICODE_STRING ServiceName;  // name of service returned by enumeration
    UNICODE_STRING PathName;     // path name to services
    UNICODE_STRING PerformanceName;  // name of key holding performance data
    UNICODE_STRING ValueDataName;    // result of query of value is this name
    OBJECT_ATTRIBUTES ObjectAttributes;  // general use for opening keys
    PKEY_BASIC_INFORMATION KeyInformation;   // data from query key goes here

    WCHAR szServiceName[MAX_PATH];

    LPTSTR  szMessageArray[8];
    DWORD   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    DWORD   dwDefaultValue;

    HANDLE  hTimeOutEvent;

    pExtObject      pLastObject = NULL;
    pExtObject      pThisObject = NULL;

    //  Initialize do failure can deallocate if allocated

    ServiceName.Buffer = NULL;
    KeyInformation = NULL;
    ValueDataName.Buffer = NULL;
    AnsiValueData.Buffer = NULL;

    dwIndex = 0;

    RtlInitUnicodeString(&PathName, ExtPath);
    RtlInitUnicodeString(&PerformanceName, PerfSubKey);

    try {
        // get current event log level
        dwDefaultValue = LOG_USER;
        Status = GetPerflibKeyValue (
                    EventLogLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lEventLogLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = EXT_TEST_ALL;
        Status = GetPerflibKeyValue (
                    ExtCounterTestLevel,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&lExtCounterTestLevel,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = OPEN_PROC_WAIT_TIME;
        Status = GetPerflibKeyValue (
                    OpenProcedureWaitTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwExtCtrOpenProcWaitMs,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        dwDefaultValue = PERFLIB_TIMING_THREAD_TIMEOUT;
        Status = GetPerflibKeyValue (
                    LibraryUnloadTime,
                    REG_DWORD,
                    sizeof(DWORD),
                    (LPVOID)&dwThreadAndLibraryTimeout,
                    sizeof(DWORD),
                    (LPVOID)&dwDefaultValue);

        // register as an event log source if not already done.

        if (hEventLog == NULL) {
            hEventLog = RegisterEventSource (NULL, (LPCWSTR)TEXT("Perflib"));
        }

        if (ExtensibleObjects == NULL) {
            // create a list of the known performance data objects
            ServiceName.Length =
            ServiceName.MaximumLength = (WORD)(MAX_KEY_NAME_LENGTH +
                                        PerformanceName.MaximumLength +
                                        sizeof(UNICODE_NULL));

            ServiceName.Buffer = ALLOCMEM(ServiceName.MaximumLength);

            InitializeObjectAttributes(&ObjectAttributes,
                                    &PathName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

            samDesired = KEY_READ;

            Status = NtOpenKey(&hServicesKey,
                            samDesired,
                            &ObjectAttributes);


            KeyBufferLength = sizeof(KEY_BASIC_INFORMATION) + MAX_KEY_NAME_LENGTH;

            KeyInformation = ALLOCMEM(KeyBufferLength);

            ValueBufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) +
                                MAX_VALUE_NAME_LENGTH +
                                MAX_VALUE_DATA_LENGTH;

            ValueDataName.MaximumLength = MAX_VALUE_DATA_LENGTH;
            ValueDataName.Buffer = ALLOCMEM(ValueDataName.MaximumLength);

            AnsiValueData.MaximumLength = MAX_VALUE_DATA_LENGTH/sizeof(WCHAR);
            AnsiValueData.Buffer = ALLOCMEM(AnsiValueData.MaximumLength);

            //
            //  Check for successful NtOpenKey and allocation of dynamic buffers
            //

            if ( NT_SUCCESS(Status) &&
                ServiceName.Buffer != NULL &&
                KeyInformation != NULL &&
                ValueDataName.Buffer != NULL &&
                AnsiValueData.Buffer != NULL ) {

                dwIndex = 0;

                hTimeOutEvent = CreateEvent(NULL,TRUE,TRUE,NULL);

                // wait longer than the thread to give the timing thread
                // a chance to finish on it's own. This is really just a
                // failsafe step.

                while (TRUE) {

                    Status = NtEnumerateKey(hServicesKey,
                                            dwIndex,
                                            KeyBasicInformation,
                                            KeyInformation,
                                            KeyBufferLength,
                                            &ResultLength);

                    dwIndex++;  //  next time, get the next key

                    if( !NT_SUCCESS(Status) ) {
                        // This is the normal exit: Status should be
                        // STATUS_NO_MORE_VALUES
                        break;
                    }

                    // Concatenate Service name with "\\Performance" to form Subkey

                    if ( ServiceName.MaximumLength >=
                        (USHORT)( KeyInformation->NameLength + sizeof(UNICODE_NULL) ) ) {

                        ServiceName.Length = (USHORT) KeyInformation->NameLength;

                        RtlMoveMemory(ServiceName.Buffer,
                                    KeyInformation->Name,
                                    ServiceName.Length);

                        ServiceName.Buffer[(ServiceName.Length/sizeof(WCHAR))] = 0; // null term

                        lstrcpyW (szServiceName, ServiceName.Buffer);

                        // zero terminate the buffer if space allows

                        RtlAppendUnicodeStringToString(&ServiceName,
                                                    &PerformanceName);

                        // Open Service\Performance Subkey

                        InitializeObjectAttributes(&ObjectAttributes,
                                                &ServiceName,
                                                OBJ_CASE_INSENSITIVE,
                                                hServicesKey,
                                                NULL);

                        samDesired = KEY_WRITE | KEY_READ; // to be able to disable perf DLL's

                        Status = NtOpenKey(&hPerfKey,
                                        samDesired,
                                        &ObjectAttributes);

                        if(! NT_SUCCESS(Status) ) {
                            samDesired = KEY_READ; // try read only access

                            Status = NtOpenKey(&hPerfKey,
                                            samDesired,
                                            &ObjectAttributes);
                        }

                        if( NT_SUCCESS(Status) ) {
                            // this has a performance key so read the info
                            // and add the entry to the list
                            pThisObject = AllocateAndInitializeExtObject (
                                hServicesKey, hPerfKey, szServiceName);

                            if (pThisObject != NULL) {
                                if (ExtensibleObjects == NULL) {
                                    // set head pointer
                                    pLastObject =
                                        ExtensibleObjects = pThisObject;
                                    NumExtensibleObjects = 1;
                                } else {
                                    pLastObject->pNext = pThisObject;
                                    pLastObject = pThisObject;
                                    NumExtensibleObjects++;
                                }
                            } else {
                                // the object wasn't initialized so toss
                                // the perf subkey handle.
                                // otherwise keep it open for later
                                // use and it will be closed when
                                // this extensible object is closed
                                NtClose (hPerfKey);
                            }
                        } else {
                            // *** NEW FEATURE CODE ***
                            // unable to open the performance subkey
                            if (((Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                                (lEventLogLevel >= LOG_USER)) ||
                                (lEventLogLevel >= LOG_DEBUG)) {
                                // an error other than OBJECT_NOT_FOUND should be
                                // displayed if error logging is enabled
                                // if DEBUG level is selected, then write all
                                // non-success status returns to the event log
                                //
                                dwDataIndex = wStringIndex = 0;
                                dwRawDataDwords[dwDataIndex++] =
                                    (DWORD)RtlNtStatusToDosError(Status);
                                if (lEventLogLevel >= LOG_DEBUG) {
                                    // if this is DEBUG mode, then log
                                    // the NT status as well.
                                    dwRawDataDwords[dwDataIndex++] =
                                        (DWORD)Status;
                                }
                                szMessageArray[wStringIndex++] =
                                    szServiceName;
                                ReportEvent (hEventLog,
                                    EVENTLOG_WARNING_TYPE,        // error type
                                    0,                          // category (not used)
                                    (DWORD)PERFLIB_NO_PERFORMANCE_SUBKEY, // event,
                                    NULL,                       // SID (not used),
                                    wStringIndex,               // number of strings
                                    dwDataIndex*sizeof(DWORD),  // sizeof raw data
                                    szMessageArray,                // message text array
                                    (LPVOID)&dwRawDataDwords[0]);           // raw data
                            }
                        }
                    }
                }
                if (hTimeOutEvent != NULL) NtClose (hTimeOutEvent);
                NtClose (hServicesKey);
            }
        }
    } finally {
        if ( ServiceName.Buffer )
            FREEMEM(ServiceName.Buffer);
        if ( KeyInformation )
            FREEMEM(KeyInformation);
        if ( ValueDataName.Buffer )
            FREEMEM(ValueDataName.Buffer);
        if ( AnsiValueData.Buffer )
            FREEMEM(AnsiValueData.Buffer);
    }
}

#if 0
DWORD
CollectThreadFunction (
    LPDWORD dwArg
)
{
    DWORD   dwWaitStatus = 0;
    BOOL    bExit = FALSE;
    NTSTATUS   status = STATUS_SUCCESS;
    THREAD_BASIC_INFORMATION    tbiData;
    LONG    lOldPriority, lNewPriority;
    LONG    lStatus;

    UNREFERENCED_PARAMETER (dwArg);

//    KdPrint (("\nPERFLIB: Entering Data Collection Thread: PID: %d, TID: %d", 
//        GetCurrentProcessId(), GetCurrentThreadId()));
    // raise the priority of this thread
    status = NtQueryInformationThread (
        NtCurrentThread(),
        ThreadBasicInformation,
        &tbiData,
        sizeof(tbiData),
        NULL);

    if (NT_SUCCESS(status)) {
        lOldPriority = tbiData.Priority;
        lNewPriority = DEFAULT_THREAD_PRIORITY; // perfmon's favorite priority

        //
        //  Only RAISE the priority here. Don't lower it if it's high
        //
        if (lOldPriority < lNewPriority) {
            status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadPriority,
                    &lNewPriority,
                    sizeof(lNewPriority)
                    );
            if (status != STATUS_SUCCESS) {
                KdPrint (("\nPERFLIB: Set Thread Priority failed: 0x%8.8x", status));
            }
        }
    }

    // wait for flags
    while (!bExit) {
        dwWaitStatus = WaitForMultipleObjects (
            COLLECT_THREAD_LOOP_EVENT_COUNT,
            hCollectEvents,
            FALSE, // wait for ANY event to go
            INFINITE); // wait for ever
        // see why the wait returned:
        if (dwWaitStatus == (WAIT_OBJECT_0 + COLLECT_THREAD_PROCESS_EVENT)) {
            // the event is cleared automatically
            // collect data
            lStatus = QueryExtensibleData (
                &CollectThreadData);
            CollectThreadData.lReturnValue = lStatus;
            SetEvent (hCollectEvents[COLLECT_THREAD_DONE_EVENT]);
        } else if (dwWaitStatus == (WAIT_OBJECT_0 + COLLECT_THREAD_EXIT_EVENT)) {
            bExit = TRUE;
            continue;   // go up and bail out
        } else {
            // who knows, so output message
            KdPrint (("\nPERFLILB: Collect Thread wait returned unknown value: 0x%8.8x",dwWaitStatus));
            bExit = TRUE;
            continue;
        }
    }
//    KdPrint (("\nPERFLIB: Leaving Data Collection Thread: PID: %d, TID: %d", 
//        GetCurrentProcessId(), GetCurrentThreadId()));
    return ERROR_SUCCESS;
}
#endif

LONG
QueryExtensibleData (
    COLLECT_THREAD_DATA * pArgs
)
/*++
  QueryExtensibleData -    Get data from extensible objects

      Inputs:

          dwQueryType         - Query type (GLOBAL, COSTLY, item list, etc.)

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD   dwQueryType = pArgs->dwQueryType;
    LPWSTR  lpValueName = pArgs->lpValueName;
    LPBYTE  lpData = pArgs->lpData;
    LPDWORD lpcbData = pArgs->lpcbData;
    LPVOID  *lppDataDefinition = pArgs->lppDataDefinition;
    
    DWORD Win32Error=ERROR_SUCCESS;          //  Failure code
    DWORD BytesLeft;
    DWORD InitialBytesLeft;
    DWORD NumObjectTypes;

    LPVOID  lpExtDataBuffer = NULL;
    LPVOID  lpCallBuffer = NULL;
    LPVOID  lpLowGuardPage = NULL;
    LPVOID  lpHiGuardPage = NULL;
    LPVOID  lpEndPointer = NULL;
    LPVOID  lpBufferBefore = NULL;
    LPVOID  lpBufferAfter = NULL;
    LPDWORD lpCheckPointer;
    LARGE_INTEGER   liStartTime, liEndTime, liWaitTime;

    pExtObject  pThisExtObj = NULL;
    DWORD   dwLibEntry;

    BOOL    bGuardPageOK;
    BOOL    bBufferOK;
    BOOL    bException;
    BOOL    bUseSafeBuffer;
    BOOL    bUnlockObjData;

    LPTSTR  szMessageArray[8];
    ULONG_PTR   dwRawDataDwords[8];     // raw data buffer
    DWORD   dwDataIndex;
    WORD    wStringIndex;
    LONG    lReturnValue = ERROR_SUCCESS;

    LONG    lDllTestLevel;

    LONG                lInstIndex;
    DWORD               lCtrIndex;
    PERF_OBJECT_TYPE    *pObject, *pNextObject;
    PERF_INSTANCE_DEFINITION    *pInstance;
    PERF_COUNTER_DEFINITION     *pCounterDef;
    PERF_DATA_BLOCK     *pPerfData;
    BOOL                bForeignDataBuffer;

    DWORD           dwItemsInArray = 0;
    DWORD           dwItemsInList = 0;
    volatile PEXT_OBJ_LIST   pQueryList = NULL;
    LPWSTR          pwcThisChar;

    DWORD           dwThisNumber;
    DWORD           dwIndex, dwEntry;
    BOOL            bFound;
    BOOL            bDisabled = FALSE;
    BOOL            bUseTimer;
    DWORD           dwType = 0; 
    DWORD           dwValue = 0;
    DWORD           dwSize = sizeof(DWORD);
    DWORD           status = 0;
    DWORD           dwObjectBufSize;

    OPEN_PROC_WAIT_INFO opwInfo;
    HANDLE  hPerflibFuncTimer;

    HEAP_PROBE();

    if ((ULONG_PTR)*lppDataDefinition & (ULONG)0x00000007) {
        KdPrint (("\nPERFLIB: Processing a data buffer that is not 8-byte aligned."));
    }

    // see if perf data has been disabled 
    // this is to prevent crashing WINLOGON if the
    // system has installed a bogus DLL

    assert (ghKeyPerflib != NULL);
    dwSize = sizeof(dwValue);
    dwValue = dwType = 0;
    status = PrivateRegQueryValueExW (
        ghKeyPerflib,
        DisablePerformanceCounters,
        NULL,
        &dwType,
        (LPBYTE)&dwValue,
        &dwSize);

    if ((status == ERROR_SUCCESS) &&
        (dwType == REG_DWORD) &&
        (dwValue == 1)) {
        // then DON'T Load any libraries and unload any that have been
        // loaded
        bDisabled = TRUE;
    }

    // if data collection is disabled and there's a collection thread
    // then close it
    if (bDisabled && (hCollectThread != NULL)) {
        pArgs->dwActionFlags = CTD_AF_CLOSE_THREAD;
    } else if (!bDisabled && 
        ((hCollectThread == NULL) && (dwCollectionFlags && COLL_FLAG_USE_SEPARATE_THREAD))) {
        // then data collection is enabled and they want a separate collection
        // thread, but there's no thread at the moment, so create it here
        pArgs->dwActionFlags = CTD_AF_OPEN_THREAD;
    }

    lReturnValue = RegisterExtObjListAccess();

    if (lReturnValue == ERROR_SUCCESS) {

        if ((dwQueryType == QUERY_ITEMS) && (!bDisabled)) {
            // alloc the call list
            pwcThisChar = lpValueName;
            dwThisNumber = 0;

            // read the value string and build an object ID list

            while (*pwcThisChar != 0) {
                dwThisNumber = GetNextNumberFromList (
                    pwcThisChar, &pwcThisChar);
                if (dwThisNumber != 0) {
                    if (dwItemsInList >= dwItemsInArray) {
                        dwItemsInArray += 16;   // starting point for # of objects
                        if (pQueryList == NULL) {
                            // alloc a new buffer
                            pQueryList = ALLOCMEM ((sizeof(EXT_OBJ_LIST) * dwItemsInArray));
                        } else {
                            // realloc a new buffer
                            pQueryList = REALLOCMEM(pQueryList, 
                                (sizeof(EXT_OBJ_LIST) * dwItemsInArray));
                        }
                        if (pQueryList == NULL) {
                            // unable to alloc memory so bail
                            return ERROR_OUTOFMEMORY;
                        }
                    }

                    // then add to the list
                    pQueryList[dwItemsInList].dwObjId = dwThisNumber;
                    pQueryList[dwItemsInList].dwFlags = 0;
                    dwItemsInList++;
                }
            }

            if (Win32Error == ERROR_SUCCESS) {
                //
                //  Walk through list of ext. objects and tag the ones to call
                //  as the query objects are found
                //
                for (pThisExtObj = ExtensibleObjects, dwLibEntry = 0;
                    pThisExtObj != NULL;
                    pThisExtObj = pThisExtObj->pNext, dwLibEntry++) {

                    if (pThisExtObj->dwNumObjects > 0) {
                        // then examine list
                        for (dwIndex = 0; dwIndex < pThisExtObj->dwNumObjects; dwIndex++) {
                            // look at each entry in the list
                            for (dwEntry = 0; dwEntry < dwItemsInList; dwEntry++) {
                                if (pQueryList[dwEntry].dwObjId == pThisExtObj->dwObjList[dwIndex]) {
                                    // tag this entry as found
                                    pQueryList[dwEntry].dwFlags |= PERF_EOL_ITEM_FOUND;
                                    // tag the object as needed
                                    pThisExtObj->dwFlags |= PERF_EO_OBJ_IN_QUERY;
                                }
                            }
                        }
                    } else {
                        // this entry doesn't list it's supported objects
                    }
                }

                assert (dwLibEntry == NumExtensibleObjects);

                // see if any in the query list do not have entries

                bFound = TRUE;
                for (dwEntry = 0; dwEntry < dwItemsInList; dwEntry++) {
                    if (!(pQueryList[dwEntry].dwFlags & PERF_EOL_ITEM_FOUND)) {
                        // no matching object found
                        bFound = FALSE;
                        break;
                    }
                }

                if (!bFound) {
                    // at least one of the object ID's in the query list was
                    // not found in an object that supports an object list
                    // then tag all entries that DO NOT support an object list
                    // to be called and hope one of them supports it/them.
                    for (pThisExtObj = ExtensibleObjects;
                         pThisExtObj != NULL;
                         pThisExtObj = pThisExtObj->pNext) {
                        if (pThisExtObj->dwNumObjects == 0) {
                            // tag this one so it will be called
                            pThisExtObj->dwFlags |= PERF_EO_OBJ_IN_QUERY;
                        }
                    }
                }
            } // end if first scan was successful

            if (pQueryList != NULL) FREEMEM (pQueryList);
        } // end if QUERY_ITEMS


        if (lReturnValue == ERROR_SUCCESS) {
            for (pThisExtObj = ExtensibleObjects;
                 pThisExtObj != NULL;
                 pThisExtObj = pThisExtObj->pNext) {

                // set the current ext object pointer
                pArgs->pCurrentExtObject = pThisExtObj;
                // convert timeout value
                liWaitTime.QuadPart = MakeTimeOutValue (pThisExtObj->dwCollectTimeout);

                // close the unused Perf DLL's IF:
                //  the perflib key is disabled or this is an item query
                //  and this is an Item (as opposed to a global or foreign)  query or
                //      the requested objects are not it this library or this library is disabled
                //  and this library has been opened
                //
                if (((dwQueryType == QUERY_ITEMS) || bDisabled) &&
                    (bDisabled || (!(pThisExtObj->dwFlags & PERF_EO_OBJ_IN_QUERY)) || (pThisExtObj->dwFlags & PERF_EO_DISABLED)) &&
                    (pThisExtObj->hLibrary != NULL)) {
                    // then free this object
                    if (pThisExtObj->hMutex != NULL) {
                        Win32Error =  NtWaitForSingleObject (
                            pThisExtObj->hMutex,
                            FALSE,
                            &liWaitTime);
                        if (Win32Error != WAIT_TIMEOUT) {
                            // then we got a lock
                            CloseExtObjectLibrary (pThisExtObj, bDisabled);
                            ReleaseMutex (pThisExtObj->hMutex);
                        } else {
                            pThisExtObj->dwLockoutCount++;
                            KdPrint (("\nPERFLIB: Unable to Lock object for %ws to close in Query", pThisExtObj->szServiceName));
                        }
                    } else {
                        Win32Error = ERROR_LOCK_FAILED;
                        KdPrint (("\nPERFLIB: No Lock found for %ws", pThisExtObj->szServiceName));
                    }

                    if (hCollectThread != NULL) {
                        // close the collection thread

                    }
                } else if (((dwQueryType == QUERY_FOREIGN) ||
                            (dwQueryType == QUERY_GLOBAL) ||
                            (dwQueryType == QUERY_COSTLY) ||
                            ((dwQueryType == QUERY_ITEMS) &&
                             (pThisExtObj->dwFlags & PERF_EO_OBJ_IN_QUERY))) && 
                           (!(pThisExtObj->dwFlags & PERF_EO_DISABLED))) {

                    // initialize values to pass to the extensible counter function
                    NumObjectTypes = 0;
                    BytesLeft = (DWORD) (*lpcbData - ((LPBYTE) *lppDataDefinition - lpData));
                    bException = FALSE;

                    if ((pThisExtObj->hLibrary == NULL) ||
                        (dwQueryType == QUERY_GLOBAL) ||
                        (dwQueryType == QUERY_COSTLY)) {
                        // lock library object
                        if (pThisExtObj->hMutex != NULL) {
                            Win32Error =  NtWaitForSingleObject (
                                pThisExtObj->hMutex,
                                FALSE,
                                &liWaitTime);
                            if (Win32Error != WAIT_TIMEOUT) {
                                // if this is a global or costly query, then reset the "in query"
                                // flag for this object. The next ITEMS query will restore it.
                                if ((dwQueryType == QUERY_GLOBAL) ||
                                    (dwQueryType == QUERY_COSTLY)) {
                                    pThisExtObj->dwFlags &= ~PERF_EO_OBJ_IN_QUERY;
                                }
                                // if necessary, open the library
                                if (pThisExtObj->hLibrary == NULL) {
                                    // make sure the library is open
                                    Win32Error = OpenExtObjectLibrary(pThisExtObj);
                                    if (Win32Error != ERROR_SUCCESS) {
                                        if (Win32Error != ERROR_SERVICE_DISABLED) {
                                            // SERVICE_DISABLED is returned when the  
                                            // service has been disabled via ExCtrLst.
                                            // so no point in complaining about it.
                                            // assume error has been posted
                                            KdPrint (("\nPERFLIB: Unable to open perf counter library for %ws, Error: 0x%8.8x",
                                                pThisExtObj->szServiceName, Win32Error));
                                        }
                                        ReleaseMutex (pThisExtObj->hMutex);
                                        continue; // to next entry
                                    }
                                }
                                ReleaseMutex (pThisExtObj->hMutex);
                            } else {
                                pThisExtObj->dwLockoutCount++;
                                KdPrint (("\nPERFLIB: Unable to Lock object for %ws to open for Query", pThisExtObj->szServiceName));
                            }
                        } else {
                            Win32Error = ERROR_LOCK_FAILED;
                            KdPrint (("\nPERFLIB: No Lock found for %ws", pThisExtObj->szServiceName));
                        }
                    } else {
                        // library should be ready to use
                    }

                    // if this dll is trusted, then use the system 
                    // defined test level, otherwise, test it
                    // thorourghly
                    bUseTimer = TRUE;   // default
                    if (!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) {
                        if (pThisExtObj->dwFlags & PERF_EO_TRUSTED) {
                            lDllTestLevel = lExtCounterTestLevel;
                            bUseTimer = FALSE;   // Trusted DLL's are not timed
                        } else {
                            // not trusted so use full test
                            lDllTestLevel = EXT_TEST_ALL;
                        }
                    } else {
                        // disable DLL testing
                        lDllTestLevel = EXT_TEST_NOMEMALLOC;
                        bUseTimer = FALSE;   // Timing is disabled as well
                    }

                    if (lDllTestLevel < EXT_TEST_NOMEMALLOC) {
                        bUseSafeBuffer = TRUE;
                    } else {
                        bUseSafeBuffer = FALSE;
                    }

                    // allocate a local block of memory to pass to the
                    // extensible counter function.

                    if (bUseSafeBuffer) {
                        lpExtDataBuffer = ALLOCMEM (BytesLeft + (2*GUARD_PAGE_SIZE));
                    } else {
                        lpExtDataBuffer =
                            lpCallBuffer = *lppDataDefinition;
                    }

                    if (lpExtDataBuffer != NULL) {

                        if (bUseSafeBuffer) {
                            // set buffer pointers
                            lpLowGuardPage = lpExtDataBuffer;
                            lpCallBuffer = (LPBYTE)lpExtDataBuffer + GUARD_PAGE_SIZE;
                            lpHiGuardPage = (LPBYTE)lpCallBuffer + BytesLeft;
                            lpEndPointer = (LPBYTE)lpHiGuardPage + GUARD_PAGE_SIZE;

                            // initialize GuardPage Data

                            memset (lpLowGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                            memset (lpHiGuardPage, GUARD_PAGE_CHAR, GUARD_PAGE_SIZE);
                        }

                        lpBufferBefore = lpCallBuffer;
                        lpBufferAfter = NULL;
                        hPerflibFuncTimer = NULL;

                        try {
                            //
                            //  Collect data from extensible objects
                            //

                            bUnlockObjData = FALSE;
                            if (pThisExtObj->hMutex != NULL) {
                                Win32Error =  NtWaitForSingleObject (
                                    pThisExtObj->hMutex,
                                    FALSE,
                                    &liWaitTime);
                                if ((Win32Error != WAIT_TIMEOUT)  &&
                                    (pThisExtObj->CollectProc != NULL)) {

                                    bUnlockObjData = TRUE;

                                    opwInfo.pNext = NULL;
                                    opwInfo.szLibraryName = pThisExtObj->szLibraryName;
                                    opwInfo.szServiceName = pThisExtObj->szServiceName;
                                    opwInfo.dwWaitTime = pThisExtObj->dwCollectTimeout;
                                    opwInfo.dwEventMsg = PERFLIB_COLLECTION_HUNG;
                                    opwInfo.pData = (LPVOID)pThisExtObj;
                                    if (bUseTimer) {
                                        hPerflibFuncTimer = StartPerflibFunctionTimer(&opwInfo);
                                        // if no timer, continue anyway, even though things may
                                        // hang, it's better than not loading the DLL since they
                                        // usually load OK
                                        //
                                        if (hPerflibFuncTimer == NULL) {
                                            // unable to get a timer entry
                                            KdPrint (("\nPERFLIB: Unable to acquire timer for Collect Proc"));
                                        }
                                    } else {
                                        hPerflibFuncTimer = NULL;
                                    }

                                    InitialBytesLeft = BytesLeft;

                                    QueryPerformanceCounter (&liStartTime);

                                        Win32Error =  (*pThisExtObj->CollectProc) (
                                            lpValueName,
                                            &lpCallBuffer,
                                            &BytesLeft,
                                            &NumObjectTypes);

                                    QueryPerformanceCounter (&liEndTime);

                                    if (hPerflibFuncTimer != NULL) {
                                        // kill timer
                                        KillPerflibFunctionTimer (hPerflibFuncTimer);
                                        hPerflibFuncTimer = NULL;
                                    }

                                    // update statistics

                                    pThisExtObj->dwLastBufferSize = BytesLeft;
                                
                                    if (BytesLeft > pThisExtObj->dwMaxBufferSize) {
                                        pThisExtObj->dwMaxBufferSize = BytesLeft;
                                    }

                                    if ((Win32Error == ERROR_MORE_DATA) &&
                                        (InitialBytesLeft > pThisExtObj->dwMaxBufferRejected)) {
                                        pThisExtObj->dwMaxBufferRejected = InitialBytesLeft;
                                    }

                                    lpBufferAfter = lpCallBuffer;

                                    pThisExtObj->llLastUsedTime = GetTimeAsLongLong();

                                    ReleaseMutex (pThisExtObj->hMutex);
                                    bUnlockObjData = FALSE;
                                } else {
                                    if ((pThisExtObj->CollectProc != NULL)) {
                                        KdPrint (("\nPERFLIB: Unable to Lock object for %ws to Collect data", pThisExtObj->szServiceName));
                                        dwDataIndex = wStringIndex = 0;
                                        dwRawDataDwords[dwDataIndex++] = BytesLeft;
                                        dwRawDataDwords[dwDataIndex++] =
                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                        szMessageArray[wStringIndex++] =
                                            pThisExtObj->szServiceName;
                                        szMessageArray[wStringIndex++] =
                                            pThisExtObj->szLibraryName;
                                        ReportEvent (hEventLog,
                                            EVENTLOG_WARNING_TYPE,      // error type
                                            0,                          // category (not used)
                                            (DWORD)PERFLIB_COLLECTION_HUNG,   // event,
                                            NULL,                       // SID (not used),
                                            wStringIndex,              // number of strings
                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                            szMessageArray,                // message text array
                                            (LPVOID)&dwRawDataDwords[0]);           // raw data

                                        pThisExtObj->dwLockoutCount++;
                                    } else {
                                        // else it's not open so ignore.
                                        BytesLeft = 0;
                                        NumObjectTypes = 0;
                                    }
                                }
                            } else {
                                Win32Error = ERROR_LOCK_FAILED;
                                KdPrint (("\nPERFLIB: No Lock found for %ws", pThisExtObj->szServiceName));
                            }

                            if ((Win32Error == ERROR_SUCCESS) && (BytesLeft > 0)) {
                                // increment perf counters
                                if (BytesLeft > InitialBytesLeft) {
                                    // memory error
                                    dwDataIndex = wStringIndex = 0;
                                    dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)InitialBytesLeft;
                                    dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)BytesLeft;
                                    szMessageArray[wStringIndex++] =
                                        pThisExtObj->szServiceName;
                                    szMessageArray[wStringIndex++] =
                                        pThisExtObj->szLibraryName;
                                    ReportEvent (hEventLog,
                                        EVENTLOG_ERROR_TYPE,      // error type
                                        0,                          // category (not used)
                                        (DWORD)PERFLIB_INVALID_SIZE_RETURNED,   // event,
                                        NULL,                       // SID (not used),
                                        wStringIndex,              // number of strings
                                        dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                        szMessageArray,                // message text array
                                        (LPVOID)&dwRawDataDwords[0]);           // raw data

                                    // disable the dll unless:
                                    //      testing has been disabled.
                                    //      or this is a trusted DLL (which are never disabled)
                                    //  the event log message should be reported in any case since
                                    //  this is a serious error
                                    //
                                    if ((!(lPerflibConfigFlags & PLCF_NO_DLL_TESTING)) && 
                                        (!(pThisExtObj->dwFlags & PERF_EO_TRUSTED))) {
                                        DisablePerfLibrary (pThisExtObj);
                                    }
                                    // set error values to correct entries
                                    BytesLeft = 0;
                                    NumObjectTypes = 0;
                                } else {
                                    // the buffer seems ok so far, so validate it
                                        
                                    InterlockedIncrement ((LONG *)&pThisExtObj->dwCollectCount);
                                    pThisExtObj->llElapsedTime +=
                                        liEndTime.QuadPart - liStartTime.QuadPart;

                                    // test all returned buffers for correct alignment
                                    if ((((ULONG_PTR)BytesLeft & (ULONG_PTR)0x07)) &&
                                        !(lPerflibConfigFlags & PLCF_NO_ALIGN_ERRORS)) {
                                        if ((pThisExtObj->dwFlags & PERF_EO_ALIGN_ERR_POSTED) == 0) {
                                            KdPrint (("\nPERFLIB: %ws returned a buffer that is not 8-byte aligned.",
                                                pThisExtObj->szServiceName));
                                            dwDataIndex = wStringIndex = 0;
                                            dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)lpCallBuffer;
                                            dwRawDataDwords[dwDataIndex++] = (ULONG_PTR)BytesLeft;
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szServiceName;
                                            szMessageArray[wStringIndex++] =
                                                pThisExtObj->szLibraryName;
                                            ReportEvent (hEventLog,
                                                EVENTLOG_WARNING_TYPE,      // error type
                                                0,                          // category (not used)
                                                (DWORD)PERFLIB_BUFFER_ALIGNMENT_ERROR,   // event,
                                                NULL,                       // SID (not used),
                                                wStringIndex,              // number of strings
                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                szMessageArray,                // message text array
                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                            pThisExtObj->dwFlags |= PERF_EO_ALIGN_ERR_POSTED;
                                        }
                                    }

                                    if (bUseSafeBuffer) {
                                        // a data buffer was returned and
                                        // the function returned OK so see how things
                                        // turned out...
                                        //
                                        //
                                        // check for buffer corruption here
                                        //
                                        bBufferOK = TRUE; // assume it's ok until a check fails
                                        //
                                        if (lDllTestLevel <= EXT_TEST_BASIC) {
                                            //
                                            //  check 1: bytes left should be the same as
                                            //      new data buffer ptr - orig data buffer ptr
                                            //
                                            if (BytesLeft != (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore)) {
                                                if (lEventLogLevel >= LOG_USER) {
                                                    // issue WARNING, that bytes left param is incorrect
                                                    // load data for eventlog message
                                                    // since this error is correctable (though with
                                                    // some risk) this won't be reported at LOG_USER
                                                    // level
                                                    dwDataIndex = wStringIndex = 0;
                                                    dwRawDataDwords[dwDataIndex++] = BytesLeft;
                                                    dwRawDataDwords[dwDataIndex++] =
                                                        (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szServiceName;
                                                    szMessageArray[wStringIndex++] =
                                                        pThisExtObj->szLibraryName;
                                                    ReportEvent (hEventLog,
                                                        EVENTLOG_WARNING_TYPE,      // error type
                                                        0,                          // category (not used)
                                                        (DWORD)PERFLIB_BUFFER_POINTER_MISMATCH,   // event,
                                                        NULL,                       // SID (not used),
                                                        wStringIndex,              // number of strings
                                                        dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                        szMessageArray,                // message text array
                                                        (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                }

                                                // toss this buffer
                                                bBufferOK = FALSE;
                                                DisablePerfLibrary (pThisExtObj);
                                                // <<old code>>
                                                // we'll keep the buffer, since the returned bytes left
                                                // value is ignored anyway, in order to make the
                                                // rest of this function work, we'll fix it here
                                                // BytesLeft = (DWORD)((LPBYTE)lpBufferAfter - (LPBYTE)lpBufferBefore);
                                                // << end old code >>
                                            }
                                            //
                                            //  check 2: buffer after ptr should be < hi Guard page ptr
                                            //
                                            if (((LPBYTE)lpBufferAfter > (LPBYTE)lpHiGuardPage) && bBufferOK) {
                                                // see if they exceeded the allocated memory
                                                if ((LPBYTE)lpBufferAfter >= (LPBYTE)lpEndPointer) {
                                                    // this is very serious since they've probably trashed
                                                    // the heap by overwriting the heap sig. block
                                                    // issue ERROR, buffer overrun
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] =
                                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_HEAP_ERROR,  // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,               // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,             // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }
                                                } else {
                                                    // issue ERROR, buffer overrun
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] =
                                                            (ULONG_PTR)((LPBYTE)lpBufferAfter - (LPBYTE)lpHiGuardPage);
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_BUFFER_OVERFLOW,     // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }
                                                }
                                                bBufferOK = FALSE;
                                                DisablePerfLibrary (pThisExtObj);
                                                // since the DLL overran the buffer, the buffer
                                                // must be too small (no comments about the DLL
                                                // will be made here) so the status will be
                                                // changed to ERROR_MORE_DATA and the function
                                                // will return.
                                                Win32Error = ERROR_MORE_DATA;
                                            }
                                            //
                                            //  check 3: check lo guard page for corruption
                                            //
                                            if (bBufferOK) {
                                                bGuardPageOK = TRUE;
                                                for (lpCheckPointer = (LPDWORD)lpLowGuardPage;
                                                        lpCheckPointer < (LPDWORD)lpBufferBefore;
                                                    lpCheckPointer++) {
                                                    if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                                        bGuardPageOK = FALSE;
                                                            break;
                                                    }
                                                }
                                                if (!bGuardPageOK) {
                                                    // issue ERROR, Lo Guard Page corrupted
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_GUARD_PAGE_VIOLATION, // event
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data


                                                    }
                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                            }
                                            //
                                            //  check 4: check hi guard page for corruption
                                            //
                                            if (bBufferOK) {
                                                bGuardPageOK = TRUE;
                                                for (lpCheckPointer = (LPDWORD)lpHiGuardPage;
                                                    lpCheckPointer < (LPDWORD)lpEndPointer;
                                                    lpCheckPointer++) {
                                                        if (*lpCheckPointer != GUARD_PAGE_DWORD) {
                                                            bGuardPageOK = FALSE;
                                                        break;
                                                    }
                                                }
                                                if (!bGuardPageOK) {
                                                    // issue ERROR, Hi Guard Page corrupted
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_GUARD_PAGE_VIOLATION, // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,              // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,                // message text array
                                                            (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                    }

                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                            }
                                            //
                                            if ((lDllTestLevel <= EXT_TEST_ALL) && bBufferOK) {
                                                //
                                                //  Internal consistency checks
                                                //
                                                //
                                                //  Check 5: Check object length field values
                                                //
                                                // first test to see if this is a foreign
                                                // computer data block or not
                                                //
                                                pPerfData = (PERF_DATA_BLOCK *)lpBufferBefore;
                                                if ((pPerfData->Signature[0] == (WCHAR)'P') &&
                                                    (pPerfData->Signature[1] == (WCHAR)'E') &&
                                                    (pPerfData->Signature[2] == (WCHAR)'R') &&
                                                    (pPerfData->Signature[3] == (WCHAR)'F')) {
                                                    // if this is a foreign computer data block, then the
                                                    // first object is after the header
                                                    pObject = (PERF_OBJECT_TYPE *) (
                                                        (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    bForeignDataBuffer = TRUE;
                                                } else {
                                                    // otherwise, if this is just a buffer from
                                                    // an extensible counter, the object starts
                                                    // at the beginning of the buffer
                                                    pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    bForeignDataBuffer = FALSE;
                                                }
                                                // go to where the pointers say the end of the
                                                // buffer is and then see if it's where it
                                                // should be
                                                dwObjectBufSize = 0;
                                                for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                    dwObjectBufSize += pObject->TotalByteLength;
                                                    pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                        pObject->TotalByteLength);
                                                }
                                                if (((LPBYTE)pObject != (LPBYTE)lpCallBuffer) || 
                                                    (dwObjectBufSize > BytesLeft)) {
                                                    // then a length field is incorrect. This is FATAL
                                                    // since it can corrupt the rest of the buffer
                                                    // and render the buffer unusable.
                                                    if (lEventLogLevel >= LOG_USER) {
                                                        // load data for eventlog message
                                                        dwDataIndex = wStringIndex = 0;
                                                        dwRawDataDwords[dwDataIndex++] = NumObjectTypes;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szLibraryName;
                                                        szMessageArray[wStringIndex++] =
                                                            pThisExtObj->szServiceName;
                                                        ReportEvent (hEventLog,
                                                            EVENTLOG_ERROR_TYPE,        // error type
                                                            0,                          // category (not used)
                                                            (DWORD)PERFLIB_INCORRECT_OBJECT_LENGTH, // event,
                                                            NULL,                       // SID (not used),
                                                            wStringIndex,               // number of strings
                                                            dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                            szMessageArray,             // message text array
                                                            (LPVOID)&dwRawDataDwords[0]); // raw data
                                                    }
                                                    bBufferOK = FALSE;
                                                    DisablePerfLibrary (pThisExtObj);
                                                }
                                                //
                                                //  Test 6: Test Object definitions fields
                                                //
                                                if (bBufferOK) {
                                                    // set object pointer
                                                    if (bForeignDataBuffer) {
                                                        pObject = (PERF_OBJECT_TYPE *) (
                                                            (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    } else {
                                                        // otherwise, if this is just a buffer from
                                                        // an extensible counter, the object starts
                                                        // at the beginning of the buffer
                                                        pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    }

                                                    for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                        pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                            pObject->DefinitionLength);

                                                        if (pObject->NumCounters != 0) {
                                                            pCounterDef = (PERF_COUNTER_DEFINITION *)
                                                                ((LPBYTE)pObject + pObject->HeaderLength);
                                                            lCtrIndex = 0;
                                                            while (lCtrIndex < pObject->NumCounters) {
                                                                if ((LPBYTE)pCounterDef < (LPBYTE)pNextObject) {
                                                                    // still ok so go to next counter
                                                                    pCounterDef = (PERF_COUNTER_DEFINITION *)
                                                                        ((LPBYTE)pCounterDef + pCounterDef->ByteLength);
                                                                    lCtrIndex++;
                                                                } else {
                                                                    bBufferOK = FALSE;
                                                                    break;
                                                                }
                                                            }
                                                            if ((LPBYTE)pCounterDef != (LPBYTE)pNextObject) {
                                                                bBufferOK = FALSE;
                                                            }
                                                        }

                                                        if (!bBufferOK) {
                                                            break;
                                                        } else {
                                                            pObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                                pObject->TotalByteLength);
                                                        }
                                                    }

                                                    if (!bBufferOK) {
                                                        if (lEventLogLevel >= LOG_USER) {
                                                            // load data for eventlog message
                                                            dwDataIndex = wStringIndex = 0;
                                                            dwRawDataDwords[dwDataIndex++] = pObject->ObjectNameTitleIndex;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szLibraryName;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szServiceName;
                                                            ReportEvent (hEventLog,
                                                                EVENTLOG_ERROR_TYPE,        // error type
                                                                0,                          // category (not used)
                                                                (DWORD)PERFLIB_INVALID_DEFINITION_BLOCK, // event,
                                                                NULL,                       // SID (not used),
                                                                wStringIndex,              // number of strings
                                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                                szMessageArray,                // message text array
                                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                        }
                                                        DisablePerfLibrary (pThisExtObj);
                                                    }

                                                }
                                                //
                                                //  Test 7: Test instance field size values
                                                //
                                                if (bBufferOK) {
                                                    // set object pointer
                                                    if (bForeignDataBuffer) {
                                                        pObject = (PERF_OBJECT_TYPE *) (
                                                            (LPBYTE)pPerfData + pPerfData->HeaderLength);
                                                    } else {
                                                        // otherwise, if this is just a buffer from
                                                        // an extensible counter, the object starts
                                                        // at the beginning of the buffer
                                                        pObject = (PERF_OBJECT_TYPE *)lpBufferBefore;
                                                    }

                                                    for (dwIndex = 0; dwIndex < NumObjectTypes; dwIndex++) {
                                                        pNextObject = (PERF_OBJECT_TYPE *)((LPBYTE)pObject +
                                                            pObject->TotalByteLength);

                                                        if (pObject->NumInstances != PERF_NO_INSTANCES) {
                                                            pInstance = (PERF_INSTANCE_DEFINITION *)
                                                                ((LPBYTE)pObject + pObject->DefinitionLength);
                                                            lInstIndex = 0;
                                                            while (lInstIndex < pObject->NumInstances) {
                                                                PERF_COUNTER_BLOCK *pCounterBlock;

                                                                pCounterBlock = (PERF_COUNTER_BLOCK *)
                                                                    ((PCHAR) pInstance + pInstance->ByteLength);

                                                                pInstance = (PERF_INSTANCE_DEFINITION *)
                                                                    ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);

                                                                lInstIndex++;
                                                            }
                                                            if ((LPBYTE)pInstance > (LPBYTE)pNextObject) {
                                                                bBufferOK = FALSE;
                                                            }
                                                        }

                                                        if (!bBufferOK) {
                                                            break;
                                                        } else {
                                                            pObject = pNextObject;
                                                        }
                                                    }

                                                    if (!bBufferOK) {
                                                        if (lEventLogLevel >= LOG_USER) {
                                                            // load data for eventlog message
                                                            dwDataIndex = wStringIndex = 0;
                                                            dwRawDataDwords[dwDataIndex++] = pObject->ObjectNameTitleIndex;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szLibraryName;
                                                            szMessageArray[wStringIndex++] =
                                                                pThisExtObj->szServiceName;
                                                            ReportEvent (hEventLog,
                                                                EVENTLOG_ERROR_TYPE,        // error type
                                                                0,                          // category (not used)
                                                                (DWORD)PERFLIB_INCORRECT_INSTANCE_LENGTH, // event,
                                                                NULL,                       // SID (not used),
                                                                wStringIndex,              // number of strings
                                                                dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                                                szMessageArray,                // message text array
                                                                (LPVOID)&dwRawDataDwords[0]);           // raw data
                                                        }
                                                        DisablePerfLibrary (pThisExtObj);
                                                    }
                                                }
                                            }
                                        }
                                        //
                                        // if all the tests pass,then copy the data to the
                                        // original buffer and update the pointers
                                        if (bBufferOK) {
                                            RtlMoveMemory (*lppDataDefinition,
                                                lpBufferBefore,
                                                BytesLeft); // returned buffer size
                                        } else {
                                            NumObjectTypes = 0; // since this buffer was tossed
                                            BytesLeft = 0; // reset the size value since the buffer wasn't used
                                        }
                                    } else {
                                        // function already copied data to caller's buffer
                                        // so no further action is necessary
                                    }
                                    *lppDataDefinition = (LPVOID)((LPBYTE)(*lppDataDefinition) + BytesLeft);    // update data pointer
                                }
                            } else {
                                if (Win32Error != ERROR_SUCCESS) {
                                    InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                                }
                                if (bUnlockObjData) {
                                    ReleaseMutex (pThisExtObj->hMutex);
                                }

                                NumObjectTypes = 0; // clear counter
                            }// end if function returned successfully

                        } except (EXCEPTION_EXECUTE_HANDLER) {
                            Win32Error = GetExceptionCode();
                            InterlockedIncrement ((LONG *)&pThisExtObj->dwErrorCount);
                            bException = TRUE;

                            if (bUnlockObjData) {
                                ReleaseMutex (pThisExtObj->hMutex);
                                bUnlockObjData = FALSE;
                            }

                            if (hPerflibFuncTimer != NULL) {
                                // kill timer
                                KillPerflibFunctionTimer (hPerflibFuncTimer);
                                hPerflibFuncTimer = NULL;
                            }
                        }
                        if (bUseSafeBuffer) {
                            FREEMEM (lpExtDataBuffer);
                        }
                    } else {
                        // unable to allocate memory so set error value
                        Win32Error = ERROR_OUTOFMEMORY;
                    } // end if temp buffer allocated successfully
                    //
                    //  Update the count of the number of object types
                    //
                    ((PPERF_DATA_BLOCK) lpData)->NumObjectTypes += NumObjectTypes;

                    if ( Win32Error != ERROR_SUCCESS) {
                        if (bException ||
                            !((Win32Error == ERROR_MORE_DATA) ||
                              (Win32Error == WAIT_TIMEOUT))) {
                            // inform on exceptions & illegal error status only
                            if (lEventLogLevel >= LOG_USER) {
                                // load data for eventlog message
                                dwDataIndex = wStringIndex = 0;
                                dwRawDataDwords[dwDataIndex++] = Win32Error;
                                szMessageArray[wStringIndex++] =
                                    pThisExtObj->szServiceName;
                                szMessageArray[wStringIndex++] =
                                    pThisExtObj->szLibraryName;
                                ReportEvent (hEventLog,
                                    EVENTLOG_ERROR_TYPE,        // error type
                                    0,                          // category (not used)
                                    (DWORD)PERFLIB_COLLECT_PROC_EXCEPTION,   // event,
                                    NULL,                       // SID (not used),
                                    wStringIndex,              // number of strings
                                    dwDataIndex*sizeof(ULONG_PTR),  // sizeof raw data
                                    szMessageArray,                // message text array
                                    (LPVOID)&dwRawDataDwords[0]);           // raw data

                            } else {
                                if (bException) {
                                    KdPrint (("\nPERFLIB: Extensible Counter %d generated an exception code: 0x%8.8x (%dL)",
                                        NumObjectTypes, Win32Error, Win32Error));
                                } else {
                                    KdPrint (("\nPERFLIB: Extensible Counter %d returned error code: 0x%8.8x (%dL)",
                                        NumObjectTypes, Win32Error, Win32Error));
                                }
                            }
                            if (bException) {
                                DisablePerfLibrary (pThisExtObj);
                            }
                        }
                        // the ext. dll is only supposed to return:
                        //  ERROR_SUCCESS even if it encountered a problem, OR
                        //  ERROR_MODE_DATA if the buffer was too small.
                        // if it's ERROR_MORE_DATA, then break and return the
                        // error now, since it'll just be returned again and again.
                        if (Win32Error == ERROR_MORE_DATA) {
                            lReturnValue = Win32Error;
                            break;
                        }
                    }

                    // update perf data in global section
                    if (pThisExtObj->pPerfSectionEntry != NULL) {
                        pThisExtObj->pPerfSectionEntry->llElapsedTime = 
                            pThisExtObj->llElapsedTime;

                        pThisExtObj->pPerfSectionEntry->dwCollectCount =
                            pThisExtObj->dwCollectCount; 

                        pThisExtObj->pPerfSectionEntry->dwOpenCount =
                            pThisExtObj->dwOpenCount;

                        pThisExtObj->pPerfSectionEntry->dwCloseCount =
                            pThisExtObj->dwCloseCount;   

                        pThisExtObj->pPerfSectionEntry->dwLockoutCount =
                            pThisExtObj->dwLockoutCount;    

                        pThisExtObj->pPerfSectionEntry->dwErrorCount =
                            pThisExtObj->dwErrorCount;

                        pThisExtObj->pPerfSectionEntry->dwLastBufferSize =
                            pThisExtObj->dwLastBufferSize; 

                        pThisExtObj->pPerfSectionEntry->dwMaxBufferSize = 
                            pThisExtObj->dwMaxBufferSize; 

                        pThisExtObj->pPerfSectionEntry->dwMaxBufferRejected =
                            pThisExtObj->dwMaxBufferRejected; 

                    } else {
                        // no data section was initialized so skip
                    }
                } // end if this object is to be called
            } // end for each object
        } // else an error occurred so unable to call functions
        Win32Error = DeRegisterExtObjListAccess();
    } // else unable to access ext object list

    HEAP_PROBE();

    if (bDisabled) lReturnValue = ERROR_SERVICE_DISABLED;
    return lReturnValue;
}
