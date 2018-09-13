/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfsprc.c

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
#include <perfutil.h>
#include "perfsprc.h"
#include "procmsg.h"

// bit field definitions for collect function flags
#define POS_READ_SYS_PROCESS_DATA       ((DWORD)0x00010000)
#define POS_READ_PROCESS_VM_DATA        ((DWORD)0x00020000)
#define	POS_READ_JOB_OBJECT_DATA		((DWORD)0x00040000)
#define POS_READ_JOB_DETAIL_DATA		((DWORD)0x00080000)

#define POS_COLLECT_PROCESS_DATA        ((DWORD)0x00010001)
#define POS_COLLECT_THREAD_DATA         ((DWORD)0x00010003)
#define POS_COLLECT_EXPROCESS_DATA      ((DWORD)0x00030004)
#define POS_COLLECT_IMAGE_DATA          ((DWORD)0x0003000C)
#define POS_COLLECT_LONG_IMAGE_DATA     ((DWORD)0x00030014)
#define POS_COLLECT_THREAD_DETAILS_DATA ((DWORD)0x00030024)
#define POS_COLLECT_JOB_OBJECT_DATA		((DWORD)0x00050040)
#define POS_COLLECT_JOB_DETAIL_DATA		((DWORD)0x00050081)

#define POS_COLLECT_FUNCTION_MASK       ((DWORD)0x000000FF)

#define POS_COLLECT_GLOBAL_DATA         ((DWORD)0x000500C3)
#define POS_COLLECT_FOREIGN_DATA        ((DWORD)0)
#define POS_COLLECT_COSTLY_DATA         ((DWORD)0x0003003C)

// global variables to this DLL

HANDLE  ThisDLLHandle = NULL;
HANDLE  hEventLog     = NULL;
LPWSTR  wszTotal = NULL;
HANDLE  hLibHeap = NULL;

LPBYTE  pProcessBuffer = NULL;
PPROCESS_VA_INFO     pProcessVaInfo = NULL;
PUNICODE_STRING pusLocalProcessNameBuffer = NULL;

SYSTEM_TIMEOFDAY_INFORMATION SysTimeInfo = {{0,0},{0,0},{0,0},0,0};

const WCHAR IDLE_PROCESS[] = L"Idle";
const WCHAR SYSTEM_PROCESS[] = L"System";

extern DWORD bOpenJobErrorLogged;

//
//  Value to decide if process names should be collected from:
//      the SystemProcessInfo structure (fastest)
//          -- or --
//      the process's image file (slower, but shows Unicode filenames)
//
LONG    lProcessNameCollectionMethod = PNCM_NOT_DEFINED;

// variables local to this module

static POS_FUNCTION_INFO    posDataFuncInfo[] = {
    {PROCESS_OBJECT_TITLE_INDEX,    POS_COLLECT_PROCESS_DATA,   0,  CollectProcessObjectData},
    {THREAD_OBJECT_TITLE_INDEX,     POS_COLLECT_THREAD_DATA,    0,  CollectThreadObjectData},
    {EXPROCESS_OBJECT_TITLE_INDEX,  POS_COLLECT_EXPROCESS_DATA, 0,  CollectExProcessObjectData},
    {IMAGE_OBJECT_TITLE_INDEX,      POS_COLLECT_IMAGE_DATA,     0,  CollectImageObjectData},
    {LONG_IMAGE_OBJECT_TITLE_INDEX, POS_COLLECT_LONG_IMAGE_DATA,0,  CollectLongImageObjectData},
    {THREAD_DETAILS_OBJECT_TITLE_INDEX, POS_COLLECT_THREAD_DETAILS_DATA, 0, CollectThreadDetailsObjectData},
	{JOB_OBJECT_TITLE_INDEX,		POS_COLLECT_JOB_OBJECT_DATA, 0,	CollectJobObjectData},
	{JOB_DETAILS_OBJECT_TITLE_INDEX, POS_COLLECT_JOB_DETAIL_DATA, 0, CollectJobDetailData}
};

#define POS_NUM_FUNCS   (sizeof(posDataFuncInfo) / sizeof(posDataFuncInfo[1]))

static  BOOL    bInitOk  = FALSE;
static  DWORD   dwOpenCount = 0;
static  DWORD   ProcessBufSize = LARGE_BUFFER_SIZE;

PM_OPEN_PROC    OpenSysProcessObject;
PM_COLLECT_PROC CollecSysProcessObjectData;
PM_CLOSE_PROC   CloseSysProcessObject;

BOOL
GetProcessExeName(
    HANDLE  hProcessID,
    PUNICODE_STRING pusName
)
{
    HANDLE                  hProcess;
    OBJECT_ATTRIBUTES       obProcess;
    CLIENT_ID               ClientId;
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    PLDR_DATA_TABLE_ENTRY   LdrEntry;
    LDR_DATA_TABLE_ENTRY    LdrEntryData;
    BOOL                    bReturn;
    WCHAR                   wszDllName[MAX_PATH];

    // open process for reading
    // get handle to process

    ClientId.UniqueThread = (HANDLE)NULL;
    ClientId.UniqueProcess = hProcessID;

    InitializeObjectAttributes(
        &obProcess,
        NULL,
        0,
        NULL,
        NULL
        );

    Status = NtOpenProcess(
        &hProcess,
        (ACCESS_MASK)PROCESS_ALL_ACCESS,
        &obProcess,
        &ClientId);

    if (! NT_SUCCESS(Status)){
        // unable to open the process,
        return FALSE;
    }

    // Get the process information

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        bReturn = FALSE;
    } else {
        Peb = BasicInfo.PebBaseAddress;

        //
        // get the loader information block
        //
        // Ldr = Peb->Ldr
        //

        if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
            // unable to read loader information
            bReturn = FALSE;
        } else {
            LdrHead = &Ldr->InMemoryOrderModuleList;

            //
            //  get the first memory block listed. this is the .EXE in NT
            //
            if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
                // unable to read memory link
                bReturn = FALSE;
            } else {
                LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

                if (!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL)) {
                    // unable to read image header
                    bReturn = FALSE;
                } else {
                    if (!ReadProcessMemory(hProcess,
                        LdrEntryData.BaseDllName.Buffer,
                        (LPVOID)&wszDllName[0],
                        sizeof(wszDllName), NULL)) {
                        // unable to read DLL buffer
                        bReturn = FALSE;
                    } else {
                        // copy the short name to the caller's buffer
                        RtlInitUnicodeString (
                            pusName,
                            wszDllName);
                        SetLastError(ERROR_SUCCESS);
                    }
                }
            }
        }
        NtClose (hProcess);
    }

    return TRUE;
}

LONG
GetProcessNameColMeth (
    VOID
)
{
    NTSTATUS            Status;
    HANDLE              hPerflibKey;
    OBJECT_ATTRIBUTES   oaPerflibKey;
    ACCESS_MASK         amPerflibKey;
    UNICODE_STRING      PerflibSubKeyString;
    UNICODE_STRING      NameInfoValueString;
    LONG                lReturn = PNCM_SYSTEM_INFO;
    PKEY_VALUE_PARTIAL_INFORMATION    pKeyInfo;
    DWORD               dwBufLen;
    DWORD               dwRetBufLen;
    PDWORD              pdwValue;

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        (LPCWSTR)L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");

    InitializeObjectAttributes(
            &oaPerflibKey,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    Status = NtOpenKey(
                &hPerflibKey,
                MAXIMUM_ALLOWED,
                &oaPerflibKey
                );

    if (NT_SUCCESS (Status)) {
        // registry key opened, now read value.
        // allocate enough room for the structure, - the last
        // UCHAR in the struct, but + the data buffer (a dword)

        dwBufLen = sizeof(KEY_VALUE_PARTIAL_INFORMATION) -
            sizeof(UCHAR) + sizeof (DWORD);

        pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCMEM (
            hLibHeap,
            HEAP_ZERO_MEMORY,
            dwBufLen);

        if (pKeyInfo != NULL) {
            // initialize value name string
            RtlInitUnicodeString (
                &NameInfoValueString,
                (LPCWSTR)L"CollectUnicodeProcessNames");

            dwRetBufLen = 0;
            Status = NtQueryValueKey (
                hPerflibKey,
                &NameInfoValueString,
                KeyValuePartialInformation,
                (PVOID)pKeyInfo,
                dwBufLen,
                &dwRetBufLen);

            if (NT_SUCCESS(Status)) {
                // check value of return data buffer
                pdwValue = (PDWORD)&pKeyInfo->Data[0];
                if (*pdwValue == PNCM_MODULE_FILE) {
                    lReturn = PNCM_MODULE_FILE;
                } else {
                    // all other values will cause this routine to return
                    // the default value of PNCM_SYSTEM_INFO;
                }
            }

            FREEMEM (hLibHeap, 0, pKeyInfo);
        }
        // close handle
        NtClose (hPerflibKey);
    }

    return lReturn;
}

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

    // open handle to the event log
    if (hEventLog == NULL) {
        hEventLog = MonOpenEventLog((LPWSTR)L"PerfProc");

        // create the local heap
        hLibHeap = HeapCreate (0, 1, 0);

        if (hLibHeap == NULL) {
            return FALSE;
        }

        if (lProcessNameCollectionMethod == PNCM_NOT_DEFINED) {
            // get desired process name collection method as defined in the
            // registry
            lProcessNameCollectionMethod = GetProcessNameColMeth ();
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
    if (dwOpenCount > 0) {
        // the Library is being unloaded before it was
        // closed so close it now as this is the last
        // chance to do it before the library is tossed.
        // if the value of dwOpenCount is > 1, set it to
        // one to insure everything will be closed when
        // the close function is called.
        if (dwOpenCount > 1) dwOpenCount = 1;
        CloseSysProcessObject();
    }

    if ((wszTotal != NULL) && (wszTotal != &szDefaultTotalString[0])) {
        FREEMEM (hLibHeap, 0, wszTotal);
        wszTotal = NULL;
    }

    if (HeapDestroy (hLibHeap)) hLibHeap = NULL;

    if (hEventLog != NULL) {
        MonCloseEventLog ();
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

PUNICODE_STRING
GetProcessSlowName (
    PSYSTEM_PROCESS_INFORMATION pProcess
)
/*++

GetProcessShortName

Inputs:
    PSYSTEM_PROCESS_INFORMATION pProcess

    address of System Process Information data structure.

Outputs:

    None

Returns:

    Pointer to an initialized Unicode string (created by this routine)
    that contains the short name of the process image or a numeric ID
    if no name is found.

    If unable to allocate memory for structure, then NULL is returned.

--*/
{
    PWCHAR  pPeriod;
    PWCHAR  pThisChar;

    WORD   wStringSize;

    WORD   wThisChar;


    // allocate Unicode String Structure and adjacent buffer  first

    wStringSize =  sizeof (UNICODE_STRING) +
                    MAX_INSTANCE_NAME * sizeof(WCHAR) +
                    sizeof (UNICODE_NULL);

    // this routine assumes that the allocated memory has been zero'd
    if (pusLocalProcessNameBuffer == NULL) {
        pusLocalProcessNameBuffer =
            ALLOCMEM (hLibHeap,
            HEAP_ZERO_MEMORY, // this will only 0 the buffer the first time!
            (DWORD)wStringSize);
    }

    if (pusLocalProcessNameBuffer == NULL) {
        return NULL;
    } else {
        pusLocalProcessNameBuffer->MaximumLength = (WORD)(wStringSize - (WORD)(sizeof (UNICODE_STRING)));
        pusLocalProcessNameBuffer->Length = 0;
        pusLocalProcessNameBuffer->Buffer = (PWCHAR)&pusLocalProcessNameBuffer[1];
        memset (     // buffer must be zero'd so we'll have a NULL Term
            pusLocalProcessNameBuffer->Buffer, 0,
            (DWORD)pusLocalProcessNameBuffer->MaximumLength);
    }

    // get the process name from the image file

    GetProcessExeName (pProcess->UniqueProcessId, pusLocalProcessNameBuffer);

    if (pusLocalProcessNameBuffer->Length > 0) {   // some name has been defined

        pPeriod = (PWCHAR)pusLocalProcessNameBuffer->Buffer;
        pThisChar = (PWCHAR)pusLocalProcessNameBuffer->Buffer;
        wThisChar = 0;

        //
        //  go from beginning to end and find last backslash and
        //  last period in name
        //

        while (*pThisChar != 0) { // go until null
            if (*pThisChar == L'.') {
                pPeriod = pThisChar;
            }
            pThisChar++;    // point to next char
            wThisChar += sizeof(WCHAR);
            if (wThisChar >= pusLocalProcessNameBuffer->Length) {
                break;
            }
        }

        // if pPeriod is still pointing to the beginning of the
        // string, then no period was found

        if (pPeriod == (PWCHAR)pusLocalProcessNameBuffer->Buffer) {
            pPeriod = pThisChar; // set to end of string;
        } else {
            // if a period was found, then see if the extension is
            // .EXE, if so leave it, if not, then use end of string
            // (i.e. include extension in name)

            if (lstrcmpiW(pPeriod, (LPCWSTR)L".EXE") != 0) {
                pPeriod = pThisChar;
            }
        }

        // copy characters between period (or end of string) and
        // slash (or start of string) to make image name

        wStringSize = (WORD)((PCHAR)pPeriod - (PCHAR)pusLocalProcessNameBuffer->Buffer);
        *pPeriod = 0; // null terminate buffer
        pusLocalProcessNameBuffer->Length = wStringSize; // adjust length

    } else {    // no name defined so use Process #

        // check  to see if this is a system process and give it
        // a name

        switch (HandleToUlong(pProcess->UniqueProcessId)) {
            case IDLE_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)IDLE_PROCESS);
                break;

            case SYSTEM_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)SYSTEM_PROCESS);
                break;

            // if the id is not a system process, then use the id as the name

            default:
            // try accessing via the "regular" interface
            return (GetProcessShortName (pProcess));

                break;
        }
    }

    return pusLocalProcessNameBuffer;
}

PUNICODE_STRING
GetProcessShortName (
    PSYSTEM_PROCESS_INFORMATION pProcess
)
/*++

GetProcessShortName

Inputs:
    PSYSTEM_PROCESS_INFORMATION pProcess

    address of System Process Information data structure.

Outputs:

    None

Returns:

    Pointer to an initialized Unicode string (created by this routine)
    that contains the short name of the process image or a numeric ID
    if no name is found.

    If unable to allocate memory for structure, then NULL is returned.

--*/
{
    PWCHAR  pSlash;
    PWCHAR  pPeriod;
    PWCHAR  pThisChar;

    WORD   wStringSize;

    WORD   wThisChar;

    // allocate Unicode String Structure and adjacent buffer  first

    if (pProcess->ImageName.Length > 0) {
        wStringSize =  (WORD)((WORD)(sizeof (UNICODE_STRING)) +
                        pProcess->ImageName.Length +
                        (WORD)(sizeof (UNICODE_NULL)));
    } else {
        wStringSize =  sizeof (UNICODE_STRING) +
                        MAX_INSTANCE_NAME * sizeof(WCHAR) +
                        sizeof (UNICODE_NULL);
    }

    // this routine assumes that the allocated memory has been zero'd
    if (pusLocalProcessNameBuffer == NULL) {
        pusLocalProcessNameBuffer =
            ALLOCMEM (hLibHeap,
            HEAP_ZERO_MEMORY, // this will only 0 the buffer the first time!
            (DWORD)wStringSize);
    }

    if (pusLocalProcessNameBuffer == NULL) {
        return NULL;
    } else {
        pusLocalProcessNameBuffer->MaximumLength = (WORD)(wStringSize - (WORD)sizeof (UNICODE_STRING));
        pusLocalProcessNameBuffer->Length = 0;
        pusLocalProcessNameBuffer->Buffer = (PWCHAR)&pusLocalProcessNameBuffer[1];
        memset (     // buffer must be zero'd so we'll have a NULL Term
            pusLocalProcessNameBuffer->Buffer, 0,
            (DWORD)pusLocalProcessNameBuffer->MaximumLength);
    }

    if (pProcess->ImageName.Buffer) {   // some name has been defined

        pSlash = (PWCHAR)pProcess->ImageName.Buffer;
        pPeriod = (PWCHAR)pProcess->ImageName.Buffer;
        pThisChar = (PWCHAR)pProcess->ImageName.Buffer;
        wThisChar = 0;

        //
        //  go from beginning to end and find last backslash and
        //  last period in name
        //

        while (*pThisChar != 0) { // go until null
            if (*pThisChar == L'\\') {
                pSlash = pThisChar;
            } else if (*pThisChar == L'.') {
                pPeriod = pThisChar;
            }
            pThisChar++;    // point to next char
            wThisChar += sizeof(WCHAR);
            if (wThisChar >= pProcess->ImageName.Length) {
                break;
            }
        }

        // if pPeriod is still pointing to the beginning of the
        // string, then no period was found

        if (pPeriod == (PWCHAR)pProcess->ImageName.Buffer) {
            pPeriod = pThisChar; // set to end of string;
        } else {
            // if a period was found, then see if the extension is
            // .EXE, if so leave it, if not, then use end of string
            // (i.e. include extension in name)

            if (lstrcmpiW(pPeriod, (LPCWSTR)L".EXE") != 0) {
                pPeriod = pThisChar;
            }
        }

        if (*pSlash == L'\\') { // if pSlash is pointing to a slash, then
            pSlash++;   // point to character next to slash
        }

        // copy characters between period (or end of string) and
        // slash (or start of string) to make image name

        wStringSize = (WORD)((PCHAR)pPeriod - (PCHAR)pSlash); // size in bytes

        memcpy (pusLocalProcessNameBuffer->Buffer, pSlash, wStringSize);
        pusLocalProcessNameBuffer->Length = wStringSize;

        // null terminate is
        // not necessary because allocated memory is zero-init'd

    } else {    // no name defined so use Process #

        // check  to see if this is a system process and give it
        // a name

        switch (HandleToUlong(pProcess->UniqueProcessId)) {
            case IDLE_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)IDLE_PROCESS);
                break;

            case SYSTEM_PROCESS_ID:
                RtlAppendUnicodeToString (pusLocalProcessNameBuffer,
                    (LPWSTR)SYSTEM_PROCESS);
                break;

            // if the id is not a system process, then use the id as the name

            default:
                RtlIntegerToUnicodeString (HandleToUlong(pProcess->UniqueProcessId),
                    10,
                    pusLocalProcessNameBuffer);

                break;
        }


    }

    return pusLocalProcessNameBuffer;
}

#pragma warning (disable : 4706)

static
DWORD
GetSystemProcessData (
)
{
    DWORD   dwReturnedBufferSize;
    NTSTATUS Status;

    //
    //  Get process data from system.
    //  if bGotProcessInfo is TRUE, that means we have the process
    //  info. collected earlier when we are checking for costly
    //  object types.
    //
    if (pProcessBuffer == NULL) {
        // allocate a new block
        pProcessBuffer = ALLOCMEM (hLibHeap, HEAP_ZERO_MEMORY,
            ProcessBufSize);
        if (pProcessBuffer == NULL) {
            return ERROR_OUTOFMEMORY;
        }
    }

    while( (Status = NtQuerySystemInformation(
                            SystemProcessInformation,
                            pProcessBuffer,
                            ProcessBufSize,
                            &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH ) {
        // expand buffer & retry
        ProcessBufSize += INCREMENT_BUFFER_SIZE;

        if ( !(pProcessBuffer = REALLOCMEM(hLibHeap, 0,
                                                    pProcessBuffer,
                                                    ProcessBufSize)) ) {
            Status = ERROR_OUTOFMEMORY;
            return (Status);
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        // convert to win32 error
        Status = (DWORD)RtlNtStatusToDosError(Status);
    } else {
        // get system time
        Status = NtQuerySystemInformation(
            SystemTimeOfDayInformation,
            &SysTimeInfo,
            sizeof(SysTimeInfo),
            &dwReturnedBufferSize
            );

        if (!NT_SUCCESS(Status)) {
            Status = (DWORD)RtlNtStatusToDosError(Status);
        }
    }

    return (Status);

}
#pragma warning (default : 4706)

DWORD APIENTRY
OpenSysProcessObject (
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

    if (dwOpenCount == 0) {
        // clear the job object open error flag
        bOpenJobErrorLogged = FALSE;
    }

    dwOpenCount++;

    bInitOk = TRUE;

    return  status;
}

DWORD APIENTRY
CollectSysProcessObjectData (
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

    NTSTATUS    status;

    // build bit mask of functions to call

    DWORD       dwQueryType;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;

    DWORD       dwNumObjectsFromFunction;
    DWORD       dwOrigBuffSize;
    DWORD       dwByteSize;

    DWORD       dwReturnedBufferSize;

    if (!bInitOk) {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFPROC_NOT_OPEN,
            NULL,
            0,
            0,
            NULL,
            NULL);
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

    // collect data  from system
    if (FunctionCallMask & POS_READ_SYS_PROCESS_DATA) {
        status = GetSystemProcessData ();

        if (!NT_SUCCESS(status)) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFPROC_UNABLE_QUERY_PROCESS_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
        }
    } else {
        status = ERROR_SUCCESS;
    }

    // collect data  from system
    if ((status == ERROR_SUCCESS) &&
        (pProcessBuffer != NULL) &&
        (FunctionCallMask & POS_READ_PROCESS_VM_DATA)) {
         pProcessVaInfo = GetSystemVaData (
              (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer);
        // call function

        if (pProcessVaInfo == NULL) {
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFPROC_UNABLE_QUERY_VM_INFO,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
            // zero buffer
        }
    } else {
        // zero buffer
    }

    // collect data
    *lpNumObjectTypes = 0;
    dwOrigBuffSize = dwByteSize = *lpcbTotalBytes;
    *lpcbTotalBytes = 0;

    // remove query bits
    FunctionCallMask &= POS_COLLECT_FUNCTION_MASK;

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

    // this list of data must be freed after use
    if (pProcessVaInfo != NULL) {

        FreeSystemVaData (pProcessVaInfo);
        pProcessVaInfo = NULL;

    }


    // *lppData is updated by each function
    // *lpcbTotalBytes is updated after each successful function
    // *lpNumObjects is updated after each successful function

COLLECT_BAIL_OUT:

    return lReturn;
}

DWORD APIENTRY
CloseSysProcessObject (
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
    DWORD   status = ERROR_SUCCESS;

    if (--dwOpenCount == 0) {
        if (hLibHeap != NULL) {
            // close
            if (pProcessBuffer != NULL) {
                FREEMEM (hLibHeap, 0, pProcessBuffer);
                pProcessBuffer = NULL;
            }

            if (pusLocalProcessNameBuffer != NULL) {
                FREEMEM (hLibHeap, 0, pusLocalProcessNameBuffer);
                pusLocalProcessNameBuffer = NULL;
            }
        }
    }
    return  status;

}
