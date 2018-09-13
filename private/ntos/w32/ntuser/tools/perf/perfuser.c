/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:    perfuser.c

Abstract:
    This file implements the Extensible Objects for the User object type

Revision History

    July 97     MCostea     created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winuser.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <string.h>
#include <wcstr.h>
#include "userctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "userdata.h"

#define ALL_PROCESSES_STRING L"_All Processes"

/*
 * Declared in winuserp.h
 */
#define QUC_PID_TOTAL       0xffffffff
#define QUERYUSER_TYPE_USER    0x1
#define QUERYUSER_TYPE_CS      0x2

/*
 *  The counters in CSSTATISTICS refer to the USER critical section:
 *      gCSExclusive counts how many times the CS was aquired exclusive
 *      gCSShared counts how many times the CS was aquired shared
 *      gCSTimeExclusive counts the time (NtQueryPerformanceCounter() units)
 *      spent in the resource since the last query.
 */
typedef struct _tagCSStatistics {
        DWORD   cExclusive;
        DWORD   cShared;
        __int64 i64TimeExclusive;
} CSSTATISTICS;

BOOL (WINAPI *QueryUserCounters)(DWORD, LPVOID, DWORD, LPVOID, DWORD );

/*
 *  References to constants which initialize the Object type definitions
 */
extern USER_DATA_DEFINITION UserDataDefinition;
extern CS_DATA_DEFINITION   CSDataDefinition;

/*
 * Global to store the process instances
 * This array is filled when the dll is attached (OpenUserPerformanceData)
 */
typedef struct _tagInstance {
    LPWSTR     pName;    // pointer to Unicode string
    DWORD      sizeName; // in bytes, including terminating NULL
    DWORD      id;       // client side ID
} ProcessInstance;

ProcessInstance *gaInstances;
int     gNumInstances;
PDWORD  gpPid;            // globals to store the allocate blocks of memory used
PDWORD  gpdwResult;       // as parameters in QueryUserCounters
DWORD   dwOpenCount;      // count of "Open" threads
HANDLE  ghHeap;
BOOL    gbInitOK;          // true = DLL initialized OK

/*
 * Function Prototypes
 * used to insure that the data collection functions
 * accessed by Perflib will have the correct calling format.
 */
DWORD GlobalCollect(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes,
    IN      DWORD   dwQueryType);
BOOL  FillInstances(VOID);

PM_OPEN_PROC    OpenUserPerformanceData;
PM_COLLECT_PROC CollectUserPerformanceData;
PM_CLOSE_PROC   CloseUserPerformanceData;

BOOL
__stdcall
DllInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
)
{
    char szUser32DllPath[MAX_PATH+15];
    HMODULE hUser32Module;
    ReservedAndUnused;
    /*
     *  this will prevent the DLL from getting the DLL_THREAD_* messages
     */
    DisableThreadLibraryCalls (DLLHandle);

    switch(Reason) {
        case DLL_PROCESS_ATTACH:

            if (!GetSystemDirectory(szUser32DllPath, MAX_PATH+1)) {
                return FALSE;
            }
            strcat( szUser32DllPath, "\\user32.dll");

            hUser32Module = GetModuleHandle(szUser32DllPath);
            if (!hUser32Module) {
                return FALSE;
            }
            QueryUserCounters = (BOOL (WINAPI  *)(DWORD, LPVOID, DWORD, LPVOID, DWORD ))
                            GetProcAddress(hUser32Module, "QueryUserCounters");
            if (!QueryUserCounters) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

DWORD APIENTRY OpenUserPerformanceData(
    LPWSTR lpDeviceNames    )
/*++
Routine Description:

    This routine opens the interface with the event log and
    initializes the data structures used to pass data back to the registry.
    It also calls FillInstances.

Arguments:
    Pointer to object ID of each device to be opened

Return Value:
    None.
--*/
{
    LONG    status;
    HKEY    hKeyDriverPerf;
    DWORD   size;
    DWORD   type;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;

    PDWORD  pCounterNameTitleIndex;
    LPWSTR  *pCounterNameTitle;
    PDWORD  pCounterHelpTitleIndex;
    LPWSTR  *pCounterHelpTitle;
    int     i;

    /*
     *  Since SCREG is multi-threaded and will call this routine in
     *  order to service remote performance queries, this library
     *  must keep track of how many times it has been opened (i.e.
     *  how many threads have accessed it). the registry routines will
     *  limit access to the initialization routine to only one thread
     *  at a time so synchronization (i.e. reentrancy) should not be
     *  a problem
     */
    if (!dwOpenCount) {

        hEventLog = MonOpenEventLog();  // open Eventlog interface
        /*
         * get counter and help index base values from registry
         *      Open key to registry entry
         *      read First Counter and First Help values
         *      update static data structures by adding base to
         *      offset value in structure.
         */
        status = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\PerfUser\\Performance",
            0L,
            KEY_ALL_ACCESS,
            &hKeyDriverPerf);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (USERPERF_UNABLE_OPEN_DRIVER_KEY, LOG_USER,
                &status, sizeof(status));
            /*
             * these failures are fatal, if we can't get the base values of the
             * counter or help names, then the names won't be available
             * to the requesting application  so there's not much
             * point in continuing.
             */
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                     &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (USERPERF_UNABLE_READ_FIRST_COUNTER, LOG_USER,
                &status, sizeof(status));
            goto OpenExitPoint;
        }

        status = RegQueryValueEx(
                    hKeyDriverPerf,
                    "First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
                    &size);

        if (status != ERROR_SUCCESS) {
            REPORT_ERROR_DATA (USERPERF_UNABLE_READ_FIRST_HELP, LOG_USER,
                &status, sizeof(status));
            goto OpenExitPoint;
        }

        UserDataDefinition.UserObjectType.ObjectNameTitleIndex += dwFirstCounter;
        UserDataDefinition.UserObjectType.ObjectHelpTitleIndex += dwFirstHelp;

        pCounterNameTitleIndex = &UserDataDefinition.NumTotals.CounterNameTitleIndex;
        pCounterHelpTitleIndex = &UserDataDefinition.NumTotals.CounterHelpTitleIndex;

        for (i = 0; i<NUM_USER_COUNTERS;
                i++,
                pCounterNameTitleIndex += sizeof(PERF_COUNTER_DEFINITION)/sizeof(DWORD),
                pCounterHelpTitleIndex += sizeof(PERF_COUNTER_DEFINITION)/sizeof(DWORD))
        {
            *pCounterNameTitleIndex += dwFirstCounter;
            *pCounterHelpTitleIndex += dwFirstHelp;
        }
        /*
         * Set CSDataDefinition indexes
         */
        CSDataDefinition.CSObjectType.ObjectNameTitleIndex += dwFirstCounter;
        CSDataDefinition.CSObjectType.ObjectHelpTitleIndex += dwFirstHelp;
        CSDataDefinition.CSExEnter.CounterNameTitleIndex += dwFirstCounter;
        CSDataDefinition.CSExEnter.CounterHelpTitleIndex += dwFirstHelp;
        CSDataDefinition.CSShEnter.CounterNameTitleIndex += dwFirstCounter;
        CSDataDefinition.CSShEnter.CounterHelpTitleIndex += dwFirstHelp;
        CSDataDefinition.CSExTime.CounterNameTitleIndex += dwFirstCounter;
        CSDataDefinition.CSExTime.CounterHelpTitleIndex += dwFirstHelp;

        RegCloseKey (hKeyDriverPerf); // close key to registry
        gbInitOK = TRUE;     // ok to use this function
    }

    dwOpenCount++;          // increment OPEN counter
    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}

DWORD APIENTRY CollectUserPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes)

/*++

Routine Description:    This routine will return the data for the User counters.

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
         OUT: the number of bytes added by this routine is written to the
            DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
         IN: the address of the DWORD to receive the number of objects added
            by this routine
         OUT: the number of objects added by this routine is written to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.
      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.
--*/
{

    //  Variables for reformatting the data
    PERF_COUNTER_BLOCK      *pPerfCounterBlock;
    USER_DATA_DEFINITION    *pUserDataDefinition;
    ULONG   SpaceNeeded;
    PDWORD  pdwCounter, dwProcList;
    DWORD   dwQueryType;
    DWORD   dwObjects;
    int     i;

    /*
     * before doing anything else, see if Open went OK
     */
    if (!gbInitOK) {
        // unable to continue because open failed.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    /*
     * see if this is a foreign (i.e. non-NT) computer data request
     */
    dwQueryType = GetQueryType (lpValueName);
    if (dwQueryType == QUERY_FOREIGN) {
        /*
         * this routine does not service requests for data from
         * Non-NT computers
         */
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }
    if (dwQueryType == QUERY_ITEMS) {
        if ( !(dwObjects = IsNumberInUnicodeList (lpValueName) )) {

            /*
             * request received for data object not provided by this routine
             */
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;

        }

        return GlobalCollect(
                lpValueName,
                lppData,
                lpcbTotalBytes,
                lpNumObjectTypes,
                dwObjects);
    }
    else {
        /*
         * General request to fill the instance combo-box
         * No need to actually retrieve the counter values
         */
        return GlobalCollect(
                lpValueName,
                lppData,
                lpcbTotalBytes,
                lpNumObjectTypes,
                QUERY_NOCOUNTERS);
    }

    return ERROR_SUCCESS;
}

DWORD APIENTRY
CloseUserPerformanceData()
/*++
Routine Description:
    This routine closes the open handles to win32k device performance counters

Arguments:
    None.

Return Value:
    ERROR_SUCCESS
--*/

{
    int i;
    if (!(--dwOpenCount)) { // when this is the last thread...
        MonCloseEventLog();
    }
    if (gpdwResult)
        FREE(gpdwResult);
    if (gpPid)
        FREE(gpPid);
    if (gaInstances) {
        for (i = 0; i<gNumInstances; i++) {
            FREE(gaInstances[i].pName);
        }
        FREE(gaInstances);
    }
    return ERROR_SUCCESS;
}

/*
 * As PerfMon calls GlobalCollect when it first loads the DLL
 * I will NOT call QueryUserCounters for it.
 * This is decided by the value of startMonitoring
 */
DWORD GlobalCollect(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes,
    IN      DWORD   dwQueryType)
{
    PERF_COUNTER_BLOCK      *pPerfCounterBlock;
    USER_DATA_DEFINITION    *pUserDataDefinition;
    CS_DATA_DEFINITION      *pCSDataDefinition;
    static CSSTATISTICS     PrevCSStatistics;
    static __int64          i64Frecv;
    PERF_INSTANCE_DEFINITION    *pPerfInstanceDefinition;
    PDWORD  pdwCounter;     // write pointer in the lppData
    DWORD   dwTotal, dwSpaceNeeded;
    int     i, counter;

    /*
     *  FillInstances() is quite expensive and should normally be called only
     *  once, in OpenUserPerformanceData().  The problem is that, when monitoring
     *  remote machines, the Open get called by screg.exe only once, no matter how
     *  many remote "probes" are attached.  So, if there are new processes on the
     *  target machine, they can not be seen by the probes that are added afterwards.
     */
    if (dwQueryType & (QUERY_USER | QUERY_NOCOUNTERS)) {
        if (!FillInstances()) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }
    /*
     * First make sure we have enough space
     * If the space will not sufice, we will get called again with a larger buffer
     */
    dwSpaceNeeded = 0;
    if (dwQueryType & (QUERY_USER | QUERY_NOCOUNTERS)) {
        dwSpaceNeeded += (gNumInstances)*(
                            sizeof(PERF_INSTANCE_DEFINITION) +
                            MAX_PATH * sizeof (WCHAR) +
                            sizeof(PERF_COUNTER_BLOCK) + SIZE_OF_USER_PERFORMANCE_DATA) +
                            sizeof(USER_DATA_DEFINITION);
    }
    if (dwQueryType & (QUERY_CS | QUERY_NOCOUNTERS)) {
        dwSpaceNeeded += SIZE_OF_CS_PERFORMANCE_DATA
                        + sizeof(CS_DATA_DEFINITION)
                        + sizeof(PERF_COUNTER_BLOCK);
    }
    if (*lpcbTotalBytes < dwSpaceNeeded) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
    }

    /*
     * fill the return data
     */

    *lpNumObjectTypes = 0;
    /*
     * pUserDataDefinition also keeps track of the buffer starting point
     */
    pUserDataDefinition = (USER_DATA_DEFINITION *) *lppData;
    pdwCounter = (DWORD *) *lppData;

    if (dwQueryType & (QUERY_USER | QUERY_NOCOUNTERS)) {
        /*
         * USER object is requested
         */
        (*lpNumObjectTypes) ++;

        if (dwQueryType & QUERY_USER) {
            if (!NT_SUCCESS(QueryUserCounters(QUERYUSER_TYPE_USER,
                                gpPid, gNumInstances*sizeof(DWORD),
                                gpdwResult, (NUM_USER_COUNTERS-1)*gNumInstances*sizeof(DWORD)))) {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return ERROR_SUCCESS;
            }
        }
        else {
            memset(gpdwResult, 0, sizeof(DWORD)*gNumInstances*NUM_USER_COUNTERS);
        }
        /*
         * Copy the (constant, initialized) Object Type and counter definitions
         * to the caller's data buffer
         */
        memmove(pUserDataDefinition, &UserDataDefinition, sizeof(USER_DATA_DEFINITION));
        pUserDataDefinition->UserObjectType.NumInstances = gNumInstances;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *) &pUserDataDefinition[1];
        for (i = 0; i < gNumInstances; i++) {

            /*
             * Collect and format all process instances
             */
            pPerfInstanceDefinition->ParentObjectTitleIndex = 0;
            pPerfInstanceDefinition->ParentObjectInstance = 0;
            pPerfInstanceDefinition->UniqueID = (LONG)gaInstances[i].id;
            pPerfInstanceDefinition->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
            pPerfInstanceDefinition->NameLength = gaInstances[i].sizeName;

            pPerfInstanceDefinition->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                                        pPerfInstanceDefinition->NameLength;

            wcscpy((wchar_t*)((PBYTE)pPerfInstanceDefinition + sizeof(PERF_INSTANCE_DEFINITION)),
                    gaInstances[i].pName);

            pPerfCounterBlock = (PERF_COUNTER_BLOCK *) ((PBYTE)pPerfInstanceDefinition
                                + pPerfInstanceDefinition->ByteLength);

            pPerfCounterBlock->ByteLength = SIZE_OF_USER_PERFORMANCE_DATA;
            pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

            for (counter = dwTotal = 0; counter<NUM_USER_COUNTERS-1; counter++) {
                dwTotal += gpdwResult[i*(NUM_USER_COUNTERS-1)+counter];
            }
            *pdwCounter++ = dwTotal;
            memmove(pdwCounter,  gpdwResult + i*(NUM_USER_COUNTERS-1), (NUM_USER_COUNTERS-1)*sizeof(DWORD));
            pdwCounter += NUM_USER_COUNTERS-1;

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)pdwCounter;
        }

        ((USER_DATA_DEFINITION *)*lppData)->UserObjectType.TotalByteLength =
                (ULONG)((PBYTE)pdwCounter - (PBYTE)pUserDataDefinition);
    }

    if (dwQueryType & (QUERY_CS | QUERY_NOCOUNTERS)) {
        /*
         * CS object is requested
         */
        (*lpNumObjectTypes) ++;

        /*
         * Write CS_DATA_DEFINITION object, no instances
         *
         * Copy the (constant, initialized) Object Type and counter definitions
         * to the caller's data buffer
         */
        pCSDataDefinition = (CS_DATA_DEFINITION *)pdwCounter;
        memmove(pCSDataDefinition, &CSDataDefinition, sizeof(CS_DATA_DEFINITION));

        pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pCSDataDefinition[1];
        pPerfCounterBlock->ByteLength = SIZE_OF_CS_PERFORMANCE_DATA;

        pdwCounter = (DWORD *) &pPerfCounterBlock[1];

        if (dwQueryType & QUERY_NOCOUNTERS) {
            memset(pdwCounter, 0, sizeof(DWORD)*NUM_CS_COUNTERS);
        }
        else {
            CSSTATISTICS CSCounters;

            /*
             * When entering for the first time, grab the hardware
             * cycle counter frequency (if any)
             */
            if (!PrevCSStatistics.i64TimeExclusive) {

                if (!QueryPerformanceFrequency((LARGE_INTEGER*)&i64Frecv))
                    i64Frecv = 0;
            }
            /*
             * Retrieve the CS counters
             */
             if (!QueryUserCounters(QUERYUSER_TYPE_CS, NULL, 0, &CSCounters, sizeof(CSSTATISTICS))) {
                REPORT_ERROR (USERPERF_CS_CANT_QUERY, LOG_USER);
             }
             /*
              * In case of overflow, the error will be of 1, no big deal
              */
             *pdwCounter = CSCounters.cExclusive - PrevCSStatistics.cExclusive;
             *(pdwCounter + 1) = CSCounters.cShared - PrevCSStatistics.cShared;

             if (i64Frecv) {
                 /*
                  * Translate the value in counts per milisecond
                  */
                  *(pdwCounter + 2) = (DWORD)((CSCounters.i64TimeExclusive-PrevCSStatistics.i64TimeExclusive)*1000/i64Frecv);
             }
             else {
                 /*
                  * No support for high resolution timer in the hardware
                  */
                  *(pdwCounter + 2) = 0;
             }

             PrevCSStatistics.cExclusive = CSCounters.cExclusive;
             PrevCSStatistics.cShared = CSCounters.cShared;
             PrevCSStatistics.i64TimeExclusive = CSCounters.i64TimeExclusive;
        }
        pdwCounter += NUM_CS_COUNTERS;
    }


    /*
     *  update arguments for return
     */
    *lpcbTotalBytes = (ULONG)((PBYTE)pdwCounter - (PBYTE)*lppData);
    *lppData = (PVOID) pdwCounter;

    return ERROR_SUCCESS;
}


VOID CleanUpInstances(VOID)
/*++
Routine Description:
    Clean-up previous allocated memory
    Helper function

--*/
{
    int i;

    if (gaInstances) {
        for (i = 0; i<gNumInstances; i++) {
            FREE(gaInstances[i].pName);
        }
        FREE(gaInstances);
        gaInstances = NULL;
    }
}

BOOL FillInstances(VOID)
/*++

Routine Description:
    This routine will fill the gaInstances array when the performance dll is opened
    The data in gaInstances will be used for every Collect call

Arguments:      none

Return Value:   success status

--*/
{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG   TotalOffset = 0, ResultLength;
    PWCHAR  p, pPeriod;
    DWORD   status;
    UCHAR   *pLargeBuffer;
    int     cNumAllocatedInstances;
    int     i, oldInstances;

    oldInstances  = gNumInstances;
    gNumInstances = 0;

    CleanUpInstances();

    if (!(pLargeBuffer = ALLOC(sizeof(UCHAR)*64*1024))) {
        REPORT_ERROR_DATA (STATUS_NO_MEMORY, LOG_USER,
                    &status, sizeof(status));
        return FALSE;
    }

    status = NtQuerySystemInformation(
                SystemProcessInformation,
                pLargeBuffer,
                sizeof(UCHAR)*64*1024,
                &ResultLength);
    if (!NT_SUCCESS(status)) {
        goto ExitNoMemory;
    }

    cNumAllocatedInstances = ResultLength/sizeof(SYSTEM_PROCESS_INFORMATION);
    gaInstances = ALLOC(cNumAllocatedInstances*sizeof(ProcessInstance));
    if (!gaInstances) {

        REPORT_ERROR_DATA (STATUS_NO_MEMORY, LOG_USER,
            &status, sizeof(status));
        goto ExitNoMemory;
    }

    /*
     * create the first instance named _All Processes so that it will usualy show
     * the first in the list
     */
    gaInstances[gNumInstances].sizeName = sizeof(ALL_PROCESSES_STRING);
    gaInstances[gNumInstances].id = QUC_PID_TOTAL;
    gaInstances[gNumInstances].pName = ALLOC(gaInstances[gNumInstances].sizeName);
    if (!gaInstances[gNumInstances].pName) {
        REPORT_ERROR_DATA (STATUS_NO_MEMORY, LOG_USER,
            &status, sizeof(status));
        goto ExitNoMemory;
    }
    wcscpy(gaInstances[gNumInstances].pName, ALL_PROCESSES_STRING);
    gNumInstances++;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pLargeBuffer;
    while (TRUE) {
        if ( ProcessInfo->ImageName.Buffer ) {
            p = wcsrchr(ProcessInfo->ImageName.Buffer, L'\\');
            if ( p ) {
                p++;
                }
            else {
                p = ProcessInfo->ImageName.Buffer;
                }
            /*
             *  remove .exe from the name
             */
            if (pPeriod =  wcsrchr(p, L'.'))
                *pPeriod = L'\0';
            }
        else {
                p = L"SystemProc";
            }
        gaInstances[gNumInstances].sizeName = wcslen(p);
        /*
         *  convert to bytes
         */
        gaInstances[gNumInstances].sizeName = (gaInstances[gNumInstances].sizeName + 1)
                                                * sizeof(WCHAR);
        gaInstances[gNumInstances].pName = ALLOC(gaInstances[gNumInstances].sizeName);
        if (!gaInstances[gNumInstances].pName) {
            REPORT_ERROR_DATA (STATUS_NO_MEMORY, LOG_USER,
                &status, sizeof(status));
            goto ExitNoMemory;
        }
        wcscpy(gaInstances[gNumInstances].pName, p);
        gaInstances[gNumInstances].id = (WORD)ProcessInfo->UniqueProcessId;
        gNumInstances++;

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)((BYTE*)pLargeBuffer + TotalOffset);
    }

    FREE(pLargeBuffer);
    pLargeBuffer = NULL;

    if (oldInstances != gNumInstances) {
        /*
         *  adjust the global buffers
         */
        if (gpdwResult)
            FREE(gpdwResult);
        if (gpPid)
            FREE(gpPid);

        if (!(gpdwResult = ALLOC(sizeof(DWORD)*gNumInstances*NUM_USER_COUNTERS))) {
            gpdwResult = NULL;
            goto ExitNoMemory;
        }

        if (!(gpPid = ALLOC(sizeof(DWORD)*gNumInstances))) {
            gpPid = NULL;
            goto ExitNoMemory;
        }
        for (i = 0; i < gNumInstances; i++) {
            gpPid[i] = gaInstances[i].id;
        }
    }
    return TRUE;

ExitNoMemory:

    CleanUpInstances();

    if (pLargeBuffer) {
        FREE(pLargeBuffer);
    }
    gbInitOK = FALSE;
    return FALSE;

}
