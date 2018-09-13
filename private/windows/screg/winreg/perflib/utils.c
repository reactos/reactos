/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1994-1997   Microsoft Corporation

Module Name:

    utils.c

Abstract:

        Utility functions used by the performance library functions

Author:

    Russ Blake  11/15/91

Revision History:


--*/
#define UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <prflbmsg.h>
#include <regrpc.h>
#include "ntconreg.h"
#include "utils.h"

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < '0') ? INVALID : \
     (c > '9') ? INVALID : \
     DIGIT)

#define MAX_KEYWORD_LEN   (sizeof (ADDHELP_STRING) / sizeof(WCHAR))
const   WCHAR GLOBAL_STRING[]     = L"GLOBAL";
const   WCHAR FOREIGN_STRING[]    = L"FOREIGN";
const   WCHAR COSTLY_STRING[]     = L"COSTLY";
const   WCHAR COUNTER_STRING[]    = L"COUNTER";
const   WCHAR HELP_STRING[]       = L"EXPLAIN";
const   WCHAR HELP_STRING2[]      = L"HELP";
const   WCHAR ADDCOUNTER_STRING[] = L"ADDCOUNTER";
const   WCHAR ADDHELP_STRING[]    = L"ADDEXPLAIN";
const   WCHAR ONLY_STRING[]       = L"ONLY";
const   WCHAR DisablePerformanceCounters[] = L"Disable Performance Counters";

// minimum length to hold a value name understood by Perflib

const   DWORD VALUE_NAME_LENGTH = ((sizeof(COSTLY_STRING) * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

#define PL_TIMER_START_EVENT    0
#define PL_TIMER_EXIT_EVENT     1
#define PL_TIMER_NUM_OBJECTS    2

static HANDLE   hTimerHandles[PL_TIMER_NUM_OBJECTS] = {NULL,NULL};

static  HANDLE  hTimerDataMutex = NULL;
static  HANDLE  hPerflibTimingThread   = NULL;
static  LPOPEN_PROC_WAIT_INFO   pTimerItemListHead = NULL;
#define PERFLIB_TIMER_INTERVAL  200     // 200 ms Timer

extern HANDLE hEventLog;


//
//  Perflib functions:
//
LONG
GetPerflibKeyValue (
    LPCWSTR szItem,
    DWORD   dwRegType,
    DWORD   dwMaxSize,      // ... of pReturnBuffer in bytes
    LPVOID  pReturnBuffer,
    DWORD   dwDefaultSize,  // ... of pDefault in bytes
    LPVOID  pDefault
)
/*++

    read and return the current value of the specified value
    under the Perflib registry key. If unable to read the value
    return the default value from the argument list.

    the value is returned in the pReturnBuffer.

--*/
{

    HKEY                    hPerflibKey;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    UNICODE_STRING          PerflibSubKeyString;
    UNICODE_STRING          ValueNameString;
    LONG                    lReturn = STATUS_SUCCESS;
    PKEY_VALUE_PARTIAL_INFORMATION  pValueInformation;
    LONG                    ValueBufferLength;
    LONG                    ResultLength;
    BOOL                    bUseDefault = TRUE;

    // initialize UNICODE_STRING structures used in this function

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        (LPCWSTR)L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");

    RtlInitUnicodeString (
        &ValueNameString,
        (LPWSTR)szItem);

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
                KEY_READ,
                &Obja
                );

    if (NT_SUCCESS( Status )) {
        // read value of desired entry

        ValueBufferLength = ResultLength = 1024;
        pValueInformation = ALLOCMEM(ResultLength);

        if (pValueInformation != NULL) {
            while ( (Status = NtQueryValueKey(hPerflibKey,
                                            &ValueNameString,
                                            KeyValuePartialInformation,
                                            pValueInformation,
                                            ValueBufferLength,
                                            &ResultLength))
                    == STATUS_BUFFER_OVERFLOW ) {

                pValueInformation = REALLOCMEM(pValueInformation,
                                                        ResultLength);
                if ( pValueInformation == NULL) {
                    break;
                } else {
                    ValueBufferLength = ResultLength;
                }
            }

            if (NT_SUCCESS(Status)) {
                // check to see if it's the desired type
                if (pValueInformation->Type == dwRegType) {
                    // see if it will fit
                    if (pValueInformation->DataLength <= dwMaxSize) {
                        memcpy (pReturnBuffer, &pValueInformation->Data[0],
                            pValueInformation->DataLength);
                        bUseDefault = FALSE;
                        lReturn = STATUS_SUCCESS;
                    }
                }
            } else {
                // return the default value
                lReturn = Status;
            }
            // release temp buffer
            FREEMEM (pValueInformation);
        } else {
            // unable to allocate memory for this operation so
            // just return the default value
        }
        // close the registry key
        NtClose(hPerflibKey);
    } else {
        // return default value
    }

    if (bUseDefault) {
        memcpy (pReturnBuffer, pDefault, dwDefaultSize);
        lReturn = STATUS_SUCCESS;
    }

    return lReturn;
}

BOOL
MatchString (
    IN LPCWSTR lpValueArg,
    IN LPCWSTR lpNameArg
)
/*++

MatchString

    return TRUE if lpName is in lpValue.  Otherwise return FALSE

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

    IN lpName
        string for one of the keyword names

Return TRUE | FALSE

--*/
{
    BOOL    bFound      = TRUE; // assume found until contradicted
    LPWSTR  lpValue     = (LPWSTR)lpValueArg;
    LPWSTR  lpName      = (LPWSTR)lpNameArg;

    // check to the length of the shortest string

    while ((*lpValue != 0) && (*lpName != 0)) {
        if (*lpValue++ != *lpName++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    return (bFound);
}

DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    QUERY_COUNTER
        if lpValue == pointer to "Counter" string

    QUERY_HELP
        if lpValue == pointer to "Explain" string

    QUERY_ADDCOUNTER
        if lpValue == pointer to "Addcounter" string

    QUERY_ADDHELP
        if lpValue == pointer to "Addexplain" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   LocalBuff[MAX_KEYWORD_LEN+1];
    WORD    i;

    if (lpValue == 0 || *lpValue == 0)
        return QUERY_GLOBAL;

    // convert the input string to Upper case before matching
    for (i=0; i < MAX_KEYWORD_LEN; i++) {
        if (*lpValue == TEXT(' ') || *lpValue == TEXT('\0')) {
            break;
        }
        LocalBuff[i] = *lpValue ;
        if (*lpValue >= TEXT('a') && *lpValue <= TEXT('z')) {
            LocalBuff[i]  = LocalBuff[i] - TEXT('a') + TEXT('A');
        }
        lpValue++ ;
    }
    LocalBuff[i] = TEXT('\0');

    // check for "Global" request
    if (MatchString (LocalBuff, GLOBAL_STRING))
        return QUERY_GLOBAL ;

    // check for "Foreign" request
    if (MatchString (LocalBuff, FOREIGN_STRING))
        return QUERY_FOREIGN ;

    // check for "Costly" request
    if (MatchString (LocalBuff, COSTLY_STRING))
        return QUERY_COSTLY;

    // check for "Counter" request
    if (MatchString (LocalBuff, COUNTER_STRING))
        return QUERY_COUNTER;

    // check for "Help" request
    if (MatchString (LocalBuff, HELP_STRING))
        return QUERY_HELP;

    if (MatchString (LocalBuff, HELP_STRING2))
        return QUERY_HELP;

    // check for "AddCounter" request
    if (MatchString (LocalBuff, ADDCOUNTER_STRING))
        return QUERY_ADDCOUNTER;

    // check for "AddHelp" request
    if (MatchString (LocalBuff, ADDHELP_STRING))
        return QUERY_ADDHELP;

    // None of the above, then it must be an item list
    return QUERY_ITEMS;

}

DWORD
GetNextNumberFromList (
    IN LPWSTR   szStartChar,
    IN LPWSTR   *szNextChar
)
/*++

 Reads a character string from the szStartChar to the next
 delimiting space character or the end of the string and returns
 the value of the decimal number found. If no valid number is found
 then 0 is returned. The pointer to the next character in the
 string is returned in the szNextChar parameter. If the character
 referenced by this pointer is 0, then the end of the string has
 been reached.

--*/
{
    DWORD   dwThisNumber    = 0;
    WCHAR   *pwcThisChar    = szStartChar;
    WCHAR   wcDelimiter     = L' ';
    BOOL    bValidNumber    = FALSE;

    if (szStartChar != 0) {
        while (TRUE) {
            switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
                case DIGIT:
                    // if this is the first digit after a delimiter, then
                    // set flags to start computing the new number
                    bValidNumber = TRUE;
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                    break;

                case DELIMITER:
                    // a delimter is either the delimiter character or the
                    // end of the string ('\0') if when the delimiter has been
                    // reached a valid number was found, then return it
                    //
                    if (bValidNumber || (*pwcThisChar == 0)) {
                        *szNextChar = pwcThisChar;
                        return dwThisNumber;
                    } else {
                        // continue until a non-delimiter char or the
                        // end of the file is found
                    }
                    break;

                case INVALID:
                    // if an invalid character was encountered, ignore all
                    // characters up to the next delimiter and then start fresh.
                    // the invalid number is not compared.
                    bValidNumber = FALSE;
                    break;

                default:
                    break;

            }
            pwcThisChar++;
        }
    } else {
        *szNextChar = szStartChar;
        return 0;
    }
}

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;

    while (*pwcThisChar != 0) {
        dwThisNumber = GetNextNumberFromList (
            pwcThisChar, &pwcThisChar);
        if (dwNumber == dwThisNumber) return TRUE;
    }
    // if here, then the number wasn't found
    return FALSE;

}   // IsNumberInUnicodeList

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
)
/*++

    MonBuildPerfDataBlock -     build the PERF_DATA_BLOCK structure

        Inputs:

            pBuffer         -   where the data block should be placed

            pBufferNext     -   where pointer to next byte of data block
                                is to begin; DWORD aligned

            NumObjectTypes  -   number of types of objects being reported

            DefaultObject   -   object to display by default when
                                this system is selected; this is the
                                object type title index
--*/

{
    // Initialize Signature and version ID for this data structure

    pBuffer->Signature[0] = L'P';
    pBuffer->Signature[1] = L'E';
    pBuffer->Signature[2] = L'R';
    pBuffer->Signature[3] = L'F';

    pBuffer->LittleEndian = TRUE;

    pBuffer->Version = PERF_DATA_VERSION;
    pBuffer->Revision = PERF_DATA_REVISION;

    //
    //  The next field will be filled in at the end when the length
    //  of the return data is known
    //

    pBuffer->TotalByteLength = 0;

    pBuffer->NumObjectTypes = NumObjectTypes;
    pBuffer->DefaultObject = DefaultObject;
    GetSystemTime(&pBuffer->SystemTime);
    NtQueryPerformanceCounter(&pBuffer->PerfTime,&pBuffer->PerfFreq);
    GetSystemTimeAsFileTime ((FILETIME *)&pBuffer->PerfTime100nSec.QuadPart);

    if ( ComputerNameLength ) {

        //  There is a Computer name: i.e., the network is installed

        pBuffer->SystemNameLength = ComputerNameLength;
        pBuffer->SystemNameOffset = sizeof(PERF_DATA_BLOCK);
        RtlMoveMemory(&pBuffer[1],
               pComputerName,
               ComputerNameLength);
        *pBufferNext = (PVOID) ((PCHAR) &pBuffer[1] +
                                QWORD_MULTIPLE(ComputerNameLength));
        pBuffer->HeaderLength = (DWORD)((PCHAR) *pBufferNext - (PCHAR) pBuffer);
    } else {

        // Member of Computers Anonymous

        pBuffer->SystemNameLength = 0;
        pBuffer->SystemNameOffset = 0;
        *pBufferNext = &pBuffer[1];
        pBuffer->HeaderLength = sizeof(PERF_DATA_BLOCK);
    }

    return 0;
}

//
// Timer functions
//
DWORD
PerflibTimerFunction (
    LPDWORD dwArg
)
/*++

 PerflibTimerFunction

    Timing thread used to write an event log message if the timer expires.

    This thread runs until the Exit event is set or the wait for the
    Exit event times out.

    While the start event is set, then the timer checks the current events
    to be timed and reports on any that have expired. It then sleeps for
    the duration of the timing interval after which it checks the status
    of the start & exit events to begin the next cycle.

    The timing events are added and deleted from the list only by the
    StartPerflibFunctionTimer and KillPerflibFunctionTimer functions.

 Arguments

    dwArg -- Not Used

--*/
{

    LONG                    lStatus = ERROR_SUCCESS;
    BOOL                    bKeepTiming = TRUE;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    LPWSTR                  szMessageArray[2];
    LARGE_INTEGER           liWaitTime;

    UNREFERENCED_PARAMETER (dwArg);

//    KdPrint (("\nPERFLIB: Entering Timing Thread: PID: %d, TID: %d", 
//        GetCurrentProcessId(), GetCurrentThreadId()));

    if (lStatus == ERROR_SUCCESS) {
        while (bKeepTiming) {
            liWaitTime.QuadPart =
                MakeTimeOutValue((PERFLIB_TIMING_THREAD_TIMEOUT));
            // wait for either the start or exit event flags to be set
            lStatus = NtWaitForMultipleObjects (
                PL_TIMER_NUM_OBJECTS,
                &hTimerHandles[0],
                WaitAny,          //wait for either one to be set
                FALSE,            // not alertable
                &liWaitTime);

            if (lStatus != WAIT_TIMEOUT) {
                if ((lStatus - WAIT_OBJECT_0) == PL_TIMER_EXIT_EVENT ) {
//                    KdPrint (("\nPERFLIB: Timing Thread received Exit Event (1): PID: %d, TID: %d", 
//                        GetCurrentProcessId(), GetCurrentThreadId()));

                    // then that's all
                    bKeepTiming = FALSE;
                    break;
                } else if ((lStatus - WAIT_OBJECT_0) == PL_TIMER_START_EVENT) {
//                    KdPrint (("\nPERFLIB: Timing Thread received Start Event: PID: %d, TID: %d", 
//                        GetCurrentProcessId(), GetCurrentThreadId()));
                    // then the timer is running so wait the interval period
                    // wait on exit event here to prevent hanging
                    liWaitTime.QuadPart =
                        MakeTimeOutValue((PERFLIB_TIMER_INTERVAL));
                    lStatus = NtWaitForSingleObject (
                        hTimerHandles[PL_TIMER_EXIT_EVENT],
                        FALSE,
                        &liWaitTime);

                    if (lStatus == WAIT_TIMEOUT) {
                        // then the wait time expired without being told
                        // to terminate the thread so
                        // now evaluate the list of timed events
                        // lock the data mutex
//                        KdPrint (("\nPERFLIB: Timing Thread Evaluating Entries: PID: %d, TID: %d", 
//                            GetCurrentProcessId(), GetCurrentThreadId()));

                        liWaitTime.QuadPart =
                            MakeTimeOutValue((PERFLIB_TIMER_INTERVAL * 2));
                        lStatus = NtWaitForSingleObject (
                            hTimerDataMutex,
                            FALSE,
                            &liWaitTime);

                        for (pLocalInfo = pTimerItemListHead;
                            pLocalInfo != NULL;
                            pLocalInfo = pLocalInfo->pNext) {

//                            KdPrint (("\nPERFLIB: Timing Thread Entry %d. count %d: PID: %d, TID: %d", 
//                                (DWORD)pLocalInfo, pLocalInfo->dwWaitTime,
//                                GetCurrentProcessId(), GetCurrentThreadId()));

                            if (pLocalInfo->dwWaitTime > 0) {
                                if (pLocalInfo->dwWaitTime == 1) {
                                    // then this is the last interval so log error
                                    // if this DLL hasn't already been disabled

                                    szMessageArray[0] = pLocalInfo->szServiceName;
                                    szMessageArray[1] = pLocalInfo->szLibraryName;

                                    ReportEvent (hEventLog,
                                        EVENTLOG_ERROR_TYPE,        // error type
                                        0,                          // category (not used)
                                        (DWORD)pLocalInfo->dwEventMsg, // event,
                                        NULL,                       // SID (not used),
                                        2,                          // number of strings
                                        0,                          // sizeof raw data
                                        szMessageArray,             // message text array
                                        NULL);                      // raw data

                                    if (pLocalInfo->pData != NULL) {
                                        if (lPerflibConfigFlags & PLCF_ENABLE_TIMEOUT_DISABLE) {
                                            if (!(((pExtObject)pLocalInfo->pData)->dwFlags & PERF_EO_DISABLED)) {
                                                // then pData is an extensible counter data block
                                                // disable the ext. counter
                                                DisablePerfLibrary ((pExtObject)pLocalInfo->pData);
                                            } // end if not already disabled
                                        } // end if disable DLL on Timeouts is enabled
                                    } // data is NULL so skip
                                } 
                                pLocalInfo->dwWaitTime--;
                            }
                        }
                        ReleaseMutex (hTimerDataMutex);
                    } else {
//                        KdPrint (("\nPERFLIB: Timing Thread received Exit Event (2): PID: %d, TID: %d", 
//                            GetCurrentProcessId(), GetCurrentThreadId()));

                        // we've been told to exit so
                        lStatus = ERROR_SUCCESS;
                        bKeepTiming = FALSE;
                        break;
                    }
                } else {
                    // some unexpected error was returned
                    assert (FALSE);
                }
            } else {
//                KdPrint (("\nPERFLIB: Timing Thread Timed out: PID: %d, TID: %d", 
//                    GetCurrentProcessId(), GetCurrentThreadId()));
                // the wait timed out so it's time to go
                lStatus = ERROR_SUCCESS;
                bKeepTiming = FALSE;
                break;
            }
        }
    }

//    KdPrint (("\nPERFLIB: Leaving Timing Thread: PID: %d, TID: %d", 
//        GetCurrentProcessId(), GetCurrentThreadId()));

    return lStatus;
}

HANDLE
StartPerflibFunctionTimer (
    IN  LPOPEN_PROC_WAIT_INFO pInfo
)
/*++

    Starts a timing event by adding it to the list of timing events.
    If the timer thread is not running, then the is started as well.

    If this is the first event in the list then the Start Event is
    set indicating that the timing thread can begin processing timing
    event(s).

--*/
{
    LONG    Status = ERROR_SUCCESS;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    DWORD   dwLibNameLen;
    DWORD   dwBufferLength = sizeof (OPEN_PROC_WAIT_INFO);
    LARGE_INTEGER   liWaitTime;
    HANDLE  hReturn = NULL;

    if (pInfo == NULL) {
        // no required argument
        Status = ERROR_INVALID_PARAMETER;
    } else {
        // check on or create sync objects

        // allocate timing events for the timing thread
        if (hTimerHandles[PL_TIMER_START_EVENT] == NULL) {
            // create the event as NOT signaled since we're not ready to start
            hTimerHandles[PL_TIMER_START_EVENT] = CreateEvent (NULL, TRUE, FALSE, NULL);
            if (hTimerHandles[PL_TIMER_START_EVENT] == NULL) {
                Status = GetLastError();
            }
        }

        if (hTimerHandles[PL_TIMER_EXIT_EVENT] == NULL) {
            hTimerHandles[PL_TIMER_EXIT_EVENT] = CreateEvent (NULL, TRUE, FALSE, NULL);
            if (hTimerHandles[PL_TIMER_EXIT_EVENT] == NULL) {
            Status = GetLastError();
            }
        }

        // create data sync mutex if it hasn't already been created
        if (hTimerDataMutex  == NULL) {
            hTimerDataMutex = CreateMutex (NULL, FALSE, NULL);
            if (hTimerDataMutex == NULL) {
                Status = GetLastError();
            }
        }
    }

    if (Status == ERROR_SUCCESS) {
        // continue creating timer entry
        if (hPerflibTimingThread != NULL) {
    	    // see if the handle is valid (i.e the thread is alive)
            Status = WaitForSingleObject (hPerflibTimingThread, 0);
    	    if (Status == WAIT_OBJECT_0) {
                // the thread has terminated so close the handle
                CloseHandle (hPerflibTimingThread);
    	        hPerflibTimingThread = NULL;
    	        Status = ERROR_SUCCESS;
    	    } else if (Status == WAIT_TIMEOUT) {
		// the thread is still running so continue
		Status = ERROR_SUCCESS;
    	    } else {
		// some other, probably serious, error
		// so pass it on through
	    }
        } else {
	        // the thread has never been created yet so continue
        }

        if (hPerflibTimingThread == NULL) {
            // create the timing thread

            assert (pTimerItemListHead == NULL);    // there should be no entries, yet

            // everything is ready for the timer thread

            hPerflibTimingThread = CreateThread (
                NULL, 0,
                (LPTHREAD_START_ROUTINE)PerflibTimerFunction,
                NULL, 0, NULL);

            assert (hPerflibTimingThread != NULL);
            if (hPerflibTimingThread == NULL) {
                Status = GetLastError();
            }
        }

        if (Status == ERROR_SUCCESS) {

            // compute the length of the required buffer;

            dwLibNameLen = (lstrlenW (pInfo->szLibraryName) + 1) * sizeof(WCHAR);
            dwBufferLength += dwLibNameLen;
            dwBufferLength += (lstrlenW (pInfo->szServiceName) + 1) * sizeof(WCHAR);
            dwBufferLength = DWORD_MULTIPLE (dwBufferLength);

            pLocalInfo = ALLOCMEM (dwBufferLength);

            // copy the arg buffer to the local list

            pLocalInfo->szLibraryName = (LPWSTR)&pLocalInfo[1];
            lstrcpyW (pLocalInfo->szLibraryName, pInfo->szLibraryName);
            pLocalInfo->szServiceName = (LPWSTR)
                ((LPBYTE)pLocalInfo->szLibraryName + dwLibNameLen);
            lstrcpyW (pLocalInfo->szServiceName, pInfo->szServiceName);
            // convert wait time in milliseconds to the number of "loops"
            pLocalInfo->dwWaitTime = pInfo->dwWaitTime / PERFLIB_TIMER_INTERVAL;
            if (pLocalInfo->dwWaitTime  == 0) pLocalInfo->dwWaitTime =1; // have at least 1 loop
            pLocalInfo->dwEventMsg = pInfo->dwEventMsg;
            pLocalInfo->pData = pInfo->pData;

            // wait for access to the data
            if (hTimerDataMutex != NULL) {
                liWaitTime.QuadPart =
                    MakeTimeOutValue((PERFLIB_TIMER_INTERVAL * 2));

                Status = NtWaitForSingleObject (
                    hTimerDataMutex,
                    FALSE,
                    &liWaitTime);
            } else {
                Status = ERROR_NOT_READY;
            }

            if (Status == WAIT_OBJECT_0) {
//                KdPrint (("\nPERFLIB: Timing Thread Adding Entry: %d (%d) PID: %d, TID: %d", 
//                    (DWORD)pLocalInfo, pLocalInfo->dwWaitTime,
//                    GetCurrentProcessId(), GetCurrentThreadId()));

                // we have access to the data so add this item to the front of the list
                pLocalInfo->pNext = pTimerItemListHead;
                pTimerItemListHead = pLocalInfo;
                ReleaseMutex (hTimerDataMutex);

                if (pLocalInfo->pNext == NULL) {
                    // then the list was empty before this call so start the timer
                    // going
                    SetEvent (hTimerHandles[PL_TIMER_START_EVENT]);
                }

                hReturn = (HANDLE)pLocalInfo;
            } else {
                SetLastError (Status);
            }
        } else {
            // unable to create thread
            SetLastError (Status);
        }
    } else {
        // unable to start timer
        SetLastError (Status);
    }

    return hReturn;
}

DWORD
KillPerflibFunctionTimer (
    IN  HANDLE  hPerflibTimer
)
/*++

    Terminates a timing event by removing it from the list. When the last
    item is removed from the list the Start event is reset so the timing
    thread will wait for either the next start event, exit event or it's
    timeout to expire.

--*/
{
    DWORD   Status;
    LPOPEN_PROC_WAIT_INFO   pArg = (LPOPEN_PROC_WAIT_INFO)hPerflibTimer;
    LPOPEN_PROC_WAIT_INFO   pLocalInfo;
    BOOL                    bFound = FALSE;
    LARGE_INTEGER           liWaitTime;
    DWORD   dwReturn = ERROR_SUCCESS;

    if (hTimerDataMutex == NULL) {
        dwReturn = ERROR_NOT_READY;
    } else if (pArg == NULL) {
	dwReturn = ERROR_INVALID_HANDLE;
    } else {
	// so far so good
        // wait for access to the data
        liWaitTime.QuadPart =
            MakeTimeOutValue((PERFLIB_TIMER_INTERVAL * 2));
        Status = NtWaitForSingleObject (
            hTimerDataMutex,
            FALSE,
            &liWaitTime);

        if (Status == WAIT_OBJECT_0) {
            // we have access to the list so walk down the list and remove the
            // specified item
            // see if it's the first one in the list

//            KdPrint (("\nPERFLIB: Timing Thread Removing Entry: %d (%d) PID: %d, TID: %d", 
//                (DWORD)pArg, pArg->dwWaitTime,
//                GetCurrentProcessId(), GetCurrentThreadId()));

            if (pArg == pTimerItemListHead) {
                // then remove it
                pTimerItemListHead = pArg->pNext;
                bFound = TRUE;
            } else {
                for (pLocalInfo = pTimerItemListHead;
                    pLocalInfo != NULL;
                    pLocalInfo = pLocalInfo->pNext) {
                    if (pLocalInfo->pNext == pArg) {
                        pLocalInfo->pNext = pArg->pNext;
                        bFound = TRUE;
                        break;
                    }
                }
            }
            assert (bFound);

            if (bFound) {
                // it's out of the list so release the lock
                ReleaseMutex (hTimerDataMutex);

                if (pTimerItemListHead == NULL) {
                    // then the list is empty now so stop timing
                    // going
                    ResetEvent (hTimerHandles[PL_TIMER_START_EVENT]);
                }

                // free memory

                FREEMEM (pArg);
                dwReturn = ERROR_SUCCESS;
            } else {
                dwReturn = ERROR_NOT_FOUND;
            }
        } else {
            dwReturn = ERROR_TIMEOUT;
        }
    }
    return dwReturn;
}

DWORD
DestroyPerflibFunctionTimer (
)
/*++

    Terminates the timing thread and cancels any current timer events.

--*/
{
    LONG    Status;
    LPOPEN_PROC_WAIT_INFO   pThisItem;
    LPOPEN_PROC_WAIT_INFO   pNextItem;
    LARGE_INTEGER           liWaitTime;

    // wait for data mutex
    liWaitTime.QuadPart =
        MakeTimeOutValue((PERFLIB_TIMER_INTERVAL * 2));

    Status = NtWaitForSingleObject (
        hTimerDataMutex,
        FALSE,
        &liWaitTime);

    assert (Status != WAIT_TIMEOUT);

    // free all entries in the list

    for (pNextItem = pTimerItemListHead;
        pNextItem != NULL;) {
        pThisItem = pNextItem;
        pNextItem = pThisItem->pNext;
        FREEMEM (pThisItem);
    }
    // all items have been freed so clear header
    pTimerItemListHead = NULL;

    // set exit event
    SetEvent (hTimerHandles[PL_TIMER_EXIT_EVENT]);

    // wait for thread to terminate
    liWaitTime.QuadPart =
        MakeTimeOutValue((PERFLIB_TIMER_INTERVAL * 5));

    Status = NtWaitForSingleObject (
        hPerflibTimingThread,
        FALSE,
        &liWaitTime);

    assert (Status != WAIT_TIMEOUT);

    if (hPerflibTimingThread != NULL) {
    	CloseHandle (hPerflibTimingThread);
	    hPerflibTimingThread = NULL;
    }

    if (hTimerDataMutex != NULL) {
        // cloes handles and leave
    	ReleaseMutex (hTimerDataMutex);
        CloseHandle (hTimerDataMutex);
        hTimerDataMutex = NULL;
    }

    if (hTimerHandles[PL_TIMER_START_EVENT] != NULL) {
        CloseHandle (hTimerHandles[PL_TIMER_START_EVENT]);
        hTimerHandles[PL_TIMER_START_EVENT] = NULL;
    }

    if (hTimerHandles[PL_TIMER_EXIT_EVENT] != NULL) {
        CloseHandle (hTimerHandles[PL_TIMER_EXIT_EVENT]);
        hTimerHandles[PL_TIMER_EXIT_EVENT] = NULL;
    }

    return ERROR_SUCCESS;
}

LONG
PrivateRegQueryValueExT (
    HKEY    hKey,
    LPVOID  lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData,
    BOOL    bUnicode
)
/*
    wrapper function to allow RegQueryValues while inside a RegQueryValue

*/
{
    LONG    ReturnStatus;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
	BOOL	bStatus;

    UNICODE_STRING      usLocal = {0,0,NULL};
    PSTR                AnsiValueBuffer;
    ULONG               AnsiValueLength;
    PWSTR               UnicodeValueBuffer;
    ULONG               UnicodeValueLength;
    ULONG               Index;
    ULONG               cbAnsi = 0;

    PKEY_VALUE_PARTIAL_INFORMATION  pValueInformation;
    LONG                    ValueBufferLength;
    LONG                    ResultLength;


    UNREFERENCED_PARAMETER (lpReserved);

    if (bUnicode) {
        bStatus = RtlCreateUnicodeString (&usLocal, (LPCWSTR)lpValueName);
    } else {
        bStatus = RtlCreateUnicodeStringFromAsciiz (&usLocal, (LPCSTR)lpValueName);
    }

    if (bStatus) {

        ValueBufferLength =
		ResultLength =
			sizeof(KEY_VALUE_PARTIAL_INFORMATION) + *lpcbData;
        pValueInformation = ALLOCMEM(ResultLength);

        if (pValueInformation != NULL) {
            ntStatus = NtQueryValueKey(
                hKey,
                &usLocal,
                KeyValuePartialInformation,
                pValueInformation,
                ValueBufferLength,
                &ResultLength);

            if ((NT_SUCCESS(ntStatus) || ntStatus == STATUS_BUFFER_OVERFLOW)) {
                // return data
                if (ARGUMENT_PRESENT(lpType)) {
                    *lpType = pValueInformation->Type;
                }

                if (ARGUMENT_PRESENT(lpcbData)) {
                    *lpcbData = pValueInformation->DataLength;
                }

                if (NT_SUCCESS(ntStatus)) {
                    if (ARGUMENT_PRESENT(lpData)) {
                        if (!bUnicode &&
                            (pValueInformation->Type == REG_SZ ||
                            pValueInformation->Type == REG_EXPAND_SZ ||
                            pValueInformation->Type == REG_MULTI_SZ)
                        ) {
                            // then convert the unicode return to an
                            // ANSI string before returning
                            // the local wide buffer used

                            UnicodeValueLength  = ResultLength;
                            UnicodeValueBuffer  = (LPWSTR)&pValueInformation->Data[0];

                            AnsiValueBuffer = (LPSTR)lpData;
                            AnsiValueLength = ARGUMENT_PRESENT( lpcbData )?
                                                     *lpcbData : 0;
                            Index = 0;
                            ntStatus = RtlUnicodeToMultiByteN(
                                AnsiValueBuffer,
                                AnsiValueLength,
                                &Index,
                                UnicodeValueBuffer,
                                UnicodeValueLength);

                            if (NT_SUCCESS( ntStatus ) &&
                                (ARGUMENT_PRESENT( lpcbData ))) {
                                *lpcbData = Index;
                            }
                        } else {
                            if (pValueInformation->DataLength <= *lpcbData) {
                                // copy the buffer to the user's buffer
                                memcpy (lpData, &pValueInformation->Data[0],
                                    pValueInformation->DataLength);
                                ntStatus = STATUS_SUCCESS;
                             } else {
                                 ntStatus = STATUS_BUFFER_OVERFLOW;
                             }
                             *lpcbData = pValueInformation->DataLength;
                        }
                    }
                }
            }

            if (pValueInformation != NULL) {
                // release temp buffer
                FREEMEM (pValueInformation);
            }
        } else {
            // unable to allocate memory for this operation so
            ntStatus = STATUS_NO_MEMORY;
        }

        RtlFreeUnicodeString (&usLocal);
    } else {
		// this is a guess at the most likely cause for the string
		// creation to fail.
		ntStatus = STATUS_NO_MEMORY;
	}

    ReturnStatus = RtlNtStatusToDosError(ntStatus);

    return ReturnStatus;
}

LONG
GetPerfDllFileInfo (
    LPCWSTR             szFileName,
    pDllValidationData  pDllData
)
{
    WCHAR   szFullPath[MAX_PATH*2];
    DWORD   dwStatus = ERROR_FILE_NOT_FOUND;
    DWORD   dwRetValue;
    HANDLE  hFile;
    BOOL    bStatus;
    LARGE_INTEGER   liSize;

    dwRetValue = SearchPathW (
        NULL,
        szFileName,
        NULL,
        sizeof(szFullPath) / sizeof(szFullPath[0]),
        szFullPath,
        NULL);

    if (dwRetValue > 0) {
        //then the file was found so open it.
        hFile = CreateFileW (
            szFullPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL, 
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            // get file creation date/time
            bStatus = GetFileTime (
                hFile,
                &pDllData->CreationDate,
                NULL, NULL);
            if (bStatus)  {
                // get file size
                liSize.LowPart  =  GetFileSize (
                    hFile, (LONG *)&liSize.HighPart);
                if (liSize.LowPart != 0xFFFFFFFF) {
                    pDllData->FileSize = liSize.QuadPart;
                    dwStatus = ERROR_SUCCESS;
                } else {
                    dwStatus = GetLastError();
                }
            } else {
                dwStatus = GetLastError();
            } 

            CloseHandle (hFile);
        } else {
            dwStatus = GetLastError();
        }
    } else {
        dwStatus = GetLastError();
    }

    return dwStatus;
}

DWORD
DisablePerfLibrary (
    pExtObject  pObj
)
{
    DWORD   dwValue, dwSize;
    DWORD   dwFnStatus = ERROR_SUCCESS;
    WORD    wStringIndex = 0;
    LPWSTR  szMessageArray[2];
 
    // continue only if the "Disable" feature is enabled and
    // if this library hasn't already been disabled.
    if ((!(lPerflibConfigFlags & PLCF_NO_DISABLE_DLLS)) &&
        (!(pObj->dwFlags & PERF_EO_DISABLED))) {

        // set the disabled bit in the info
        pObj->dwFlags |= PERF_EO_DISABLED;
        // disable perf library entry in the service key
        dwSize = sizeof(dwValue);
        dwValue = 1;
        dwFnStatus = RegSetValueExW (
            pObj->hPerfKey,
            DisablePerformanceCounters,
            0L,
            REG_DWORD,
            (LPBYTE)&dwValue,
            dwSize);
        // report error

        if (dwFnStatus == ERROR_SUCCESS) {
            // system disabled
            szMessageArray[wStringIndex++] =
                pObj->szServiceName;

            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFLIB_LIBRARY_DISABLED,              // event,
                NULL,                       // SID (not used),
                wStringIndex,               // number of strings
                0,                          // sizeof raw data
                szMessageArray,             // message text array
                NULL);                      // raw data
        } else {
            // local disable only
            szMessageArray[wStringIndex++] =
                pObj->szServiceName;

            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,        // error type
                0,                          // category (not used)
                (DWORD)PERFLIB_LIBRARY_TEMP_DISABLED,              // event,
                NULL,                       // SID (not used),
                wStringIndex,               // number of strings
                0,                          // sizeof raw data
                szMessageArray,             // message text array
                NULL);                      // raw data
        }
    }
    return ERROR_SUCCESS;
}