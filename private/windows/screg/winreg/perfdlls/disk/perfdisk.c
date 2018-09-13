/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfdisk.c

Abstract:


Author:

    Bob Watson (a-robw) Aug 95

Revision History:

--*/

// define the WMI Guids for this program
#ifndef INITGUID
#define INITGUID 1
#endif  

//
// Force everything to be UNICODE
//
#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <ole2.h>
#include <wmium.h>
#include <wmiguid.h>
#include <winperf.h>
#if DBG
#include <stdio.h>
#include <stdlib.h>
#endif
#include <ntprfctr.h>
#include <perfutil.h>
#include <assert.h>
#include "perfdisk.h"
#include "diskmsg.h"

// define this symbol to test if diskperf has installed itself
//  as an upper filter
// if this symbol is undefined, then check for diskperf before
//  returning any logical disk counters
//#define _DONT_CHECK_FOR_VOLUME_FILTER

#ifndef _DONT_CHECK_FOR_VOLUME_FILTER
#include <regstr.h>     // for REGSTR_VAL_UPPERFILTERS
#endif

// bit field definitions for collect function flags

#define POS_COLLECT_PDISK_DATA      ((DWORD)0x00000001)
#define POS_COLLECT_LDISK_DATA      ((DWORD)0x00000003)
#define POS_COLLECT_IGNORE          ((DWORD)0x80000000)

#define POS_COLLECT_GLOBAL_DATA     ((DWORD)0x00000003)
#define POS_COLLECT_FOREIGN_DATA    ((DWORD)0)
#define POS_COLLECT_COSTLY_DATA     ((DWORD)0)

#define INITIAL_NUM_VOL_LIST_ENTRIES    ((DWORD)0x0000001A)

// global variables to this DLL

HANDLE  ThisDLLHandle = NULL;
HANDLE  hEventLog     = NULL;
HANDLE  hLibHeap      = NULL;

BOOL    bShownDiskPerfMessage = FALSE;
BOOL    bShownDiskVolumeMessage = FALSE;

LPWSTR  wszTotal = NULL;

const WCHAR cszNT4InstanceNames[] = {L"NT4 Instance Names"};
const WCHAR cszRegKeyPath[] = {L"System\\CurrentControlSet\\Services\\PerfDisk\\Performance"};

const WCHAR cszVolumeKey[] = {L"SYSTEM\\CurrentControlSet\\Control\\Class\\{71A27CDD-812A-11D0-BEC7-08002BE2092F}"};
#define DISKPERF_SERVICE_NAME L"DiskPerf"
ULONG CheckVolumeFilter();

PDRIVE_VOLUME_ENTRY pPhysDiskList = NULL;
DWORD               dwNumPhysDiskListEntries = 0;
PDRIVE_VOLUME_ENTRY pVolumeList = NULL;
DWORD               dwNumVolumeListEntries = 0;
DWORD               dwWmiDriveCount = 0;
BOOL                bRemapDriveLetters = TRUE;
DWORD               dwMaxVolumeNumber = 0;

// start off with a big buffer then size according to return values
DWORD   WmiBufSize  = 0x10000;   // this can be smaller when the Diskperf.sys 
DWORD   WmiAllocSize = 0x10000;  // function is fixed to return the right status
LPBYTE  WmiBuffer   = NULL;

// variables local to this module

static POS_FUNCTION_INFO    posDataFuncInfo[] = {
    {LOGICAL_DISK_OBJECT_TITLE_INDEX,   POS_COLLECT_LDISK_DATA,     0, CollectLDiskObjectData},
    {PHYSICAL_DISK_OBJECT_TITLE_INDEX,  POS_COLLECT_PDISK_DATA,     0, CollectPDiskObjectData}
};

#define POS_NUM_FUNCS   (sizeof(posDataFuncInfo) / sizeof(posDataFuncInfo[1]))

static  bInitOk  = FALSE;
static  DWORD   dwOpenCount = 0;

WMIHANDLE   hWmiDiskPerf = NULL;

PM_OPEN_PROC    OpenDiskObject;
PM_COLLECT_PROC CollecDiskObjectData;
PM_CLOSE_PROC   CloseDiskObject;

DOUBLE      dSysTickTo100Ns;

#if DBG

#define DEBUG_BUFFER_LENGTH MAX_PATH*2

ULONG_PTR HeapUsed = 0;
ULONG oldPLSize = 0;
ULONG oldVLSize = 0;
ULONG wszSize = 0;

ULONG PerfDiskDebug = 0;
UCHAR PerfDiskDebugBuffer[DEBUG_BUFFER_LENGTH];

#endif


static
BOOL
WriteNewBootTimeEntry (
    LONGLONG *pBootTime
)
{
    LONG    lStatus;
    HKEY    hKeyPerfDiskPerf;
    DWORD   dwType, dwSize;
    BOOL    bReturn = FALSE;

    // try to read the registry value of the last time
    // this error was reported 
    lStatus = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,
        cszRegKeyPath,
        (DWORD)0,
        KEY_WRITE,
        &hKeyPerfDiskPerf);
    if (lStatus == ERROR_SUCCESS) {
        // read the key value
        dwType = REG_BINARY;
        dwSize = sizeof (*pBootTime);
        lStatus = RegSetValueExW (
            hKeyPerfDiskPerf,
            (LPCWSTR)L"SystemStartTimeOfLastErrorMsg",
            0L,  // reserved 
            dwType,
            (LPBYTE)pBootTime,
            dwSize);
        if (lStatus == ERROR_SUCCESS) {
            bReturn = TRUE;
        } else {
            // the value hasn't been written and 
            SetLastError (lStatus);
        } // else assume the value hasn't been written and 
          // return FALSE
        RegCloseKey (hKeyPerfDiskPerf);
    } else {
        // assume the value hasn't been written and 
        SetLastError (lStatus);
    }

    return bReturn;
}
static
BOOL
NT4NamesAreDefault ()
{
    LONG    lStatus;
    HKEY    hKeyPerfDiskPerf;
    DWORD   dwType, dwSize;
    DWORD   dwValue;
    BOOL    bReturn = FALSE;

    // try to read the registry value of the last time
    // this error was reported 
    lStatus = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,
        cszRegKeyPath,
        (DWORD)0,
        KEY_READ,
        &hKeyPerfDiskPerf);
    if (lStatus == ERROR_SUCCESS) {
        // read the key value
        dwType = 0;
        dwSize = sizeof (dwValue);
        lStatus = RegQueryValueExW (
            hKeyPerfDiskPerf,
            cszNT4InstanceNames,
            0L,  // reserved 
            &dwType,
            (LPBYTE)&dwValue,
            &dwSize);
        if ((lStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
            if (dwValue != 0) {
                bReturn = TRUE;
            } 
        } else {
            // the key is not present or not accessible so
            // leave default as is and 
            // return FALSE
        }
        RegCloseKey (hKeyPerfDiskPerf);
    } else {
        // the key could not be opened.
        SetLastError (lStatus);
    }

    return bReturn;
}

static
BOOL
SystemHasBeenRestartedSinceLastEntry (
    DWORD   dwReserved, // just in case we want to have multiple tests in the future
    LONGLONG *pBootTime // a buffer to receive the current boot time
)
{
    BOOL        bReturn = TRUE;
    NTSTATUS    ntStatus = ERROR_SUCCESS;
    SYSTEM_TIMEOFDAY_INFORMATION    SysTimeInfo;
    DWORD       dwReturnedBufferSize = 0;
    HKEY        hKeyPerfDiskPerf;
    LONG        lStatus;
    DWORD       dwType;
    DWORD       dwSize;
    LONGLONG    llLastErrorStartTime;

    DBG_UNREFERENCED_PARAMETER(dwReserved);

    // get the current system boot time (as a filetime)
    memset ((LPVOID)&SysTimeInfo, 0, sizeof(SysTimeInfo));

    ntStatus = NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &SysTimeInfo,
        sizeof(SysTimeInfo),
        &dwReturnedBufferSize
        );

    if (NT_SUCCESS(ntStatus)) {
        // try to read the registry value of the last time
        // this error was reported 
        lStatus = RegOpenKeyExW (
            HKEY_LOCAL_MACHINE,
            cszRegKeyPath,
            (DWORD)0,
            KEY_READ,
            &hKeyPerfDiskPerf);
        if (lStatus == ERROR_SUCCESS) {
            // read the key value
            dwType = 0;
            dwSize = sizeof (llLastErrorStartTime);
            lStatus = RegQueryValueExW (
                hKeyPerfDiskPerf,
                (LPCWSTR)L"SystemStartTimeOfLastErrorMsg",
                0L,  // reserved 
                &dwType,
                (LPBYTE)&llLastErrorStartTime,
                &dwSize);
            if (lStatus == ERROR_SUCCESS) {
                assert (dwType == REG_BINARY);  // this should be a binary type
                assert (dwSize == sizeof (LONGLONG)); // and it should be 8 bytes long
                // compare times
                // if the times are the same, then this message has already been
                // written since the last boot so we don't need to do it again.
                if (SysTimeInfo.BootTime.QuadPart ==
                    llLastErrorStartTime) {
                    bReturn = FALSE;
                } // else they are the different times so return FALSE
            } // else assume the value hasn't been written and 
              // return TRUE
            RegCloseKey (hKeyPerfDiskPerf);
        } // else assume the value hasn't been written and 
          // return TRUE

        // return the boot time if a buffer was passed in
        if (pBootTime != NULL) {
            // save the time
            *pBootTime = SysTimeInfo.BootTime.QuadPart;
        }
    } // else assume that it has been rebooted and return TRUE

    return bReturn;
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

    LARGE_INTEGER   liSysTick;

    UNREFERENCED_PARAMETER(DllHandle);

    // create heap for this library
    if (hLibHeap == NULL) hLibHeap = HeapCreate (0, 1, 0);
    assert (hLibHeap != NULL);

    if (hLibHeap == NULL) {
        return FALSE;
    }
    // open handle to the event log
    if (hEventLog == NULL) hEventLog = MonOpenEventLog((LPWSTR)L"PerfDisk");
    assert (hEventLog != NULL);

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
#if DBG
            HeapUsed += dwBufferSize;
            wszSize = dwBufferSize;
            DebugPrint((4,
                "DllAttach: wszTotal add %d to %d\n",
                dwBufferSize, HeapUsed));
#endif
        }
    } else {
        // unable to get string from registry so just use static buffer
        wszTotal = (LPWSTR)&szDefaultTotalString[0];
    }

    QueryPerformanceFrequency (&liSysTick);
    dSysTickTo100Ns = (DOUBLE)liSysTick.QuadPart;
    dSysTickTo100Ns /= 10000000.0;

    return bReturn;
}

static
BOOL
DllProcessDetach (
    IN  HANDLE DllHandle
)
{
    UNREFERENCED_PARAMETER(DllHandle);

    if (dwOpenCount > 0) {
        // then close the object now, since the DLL's being discarded
        // prematurely, this is our last chance.
        // this is to insure the object is closed.
        dwOpenCount = 1;
        CloseDiskObject();
    }

    if ((wszTotal != NULL) && (wszTotal != &szDefaultTotalString[0])) {
        FREEMEM (hLibHeap, 0, wszTotal);
#if DBG
        HeapUsed -= wszSize;
        DebugPrint((4,
            "DllDetach: wsz freed %d to %d\n",
            wszSize, HeapUsed));
        wszSize = 0;
#endif
        wszTotal = NULL;
    }

    if (HeapDestroy (hLibHeap)) {
        hLibHeap = NULL;
        pVolumeList = NULL;
        pPhysDiskList = NULL;
        dwNumVolumeListEntries = 0;
        dwNumPhysDiskListEntries = 0;
    }

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

DWORD APIENTRY
MapDriveLetters()
{
    DWORD   status = ERROR_SUCCESS;
    DWORD   dwLoopCount;   
    PDRIVE_VOLUME_ENTRY pTempPtr;
    DWORD   dwDriveCount;
    DWORD   dwThisEntry;

    if (pPhysDiskList != NULL) {
        HeapFree (hLibHeap, 0, pPhysDiskList);
#if DBG
        HeapUsed -= oldPLSize;
        DebugPrint((4,"MapDriveLetters: PL Freed %d to %d\n",
            oldPLSize, HeapUsed));
        oldPLSize = 0;
#endif
        pPhysDiskList = NULL;
    }
    dwNumPhysDiskListEntries = INITIAL_NUM_VOL_LIST_ENTRIES;

    // Initially allocate enough entries for drives A through Z
    pPhysDiskList = (PDRIVE_VOLUME_ENTRY)HeapAlloc (
        hLibHeap, HEAP_ZERO_MEMORY, 
        (dwNumPhysDiskListEntries * sizeof (DRIVE_VOLUME_ENTRY)));

#if DBG
    if (pPhysDiskList == NULL) {
        DebugPrint((2,
            "MapDriveLetters: pPhysDiskList alloc failure\n"));
    }
#endif

    if (pPhysDiskList != NULL) {
        // try until we get a big enough buffer
#if DBG
        ULONG oldsize = dwNumPhysDiskListEntries * sizeof(DRIVE_VOLUME_ENTRY);
        HeapUsed += oldsize;
        oldPLSize = oldsize;
        DebugPrint((4, "MapDriveLetter: Alloc %d to %d\n",
            oldsize, HeapUsed));
#endif
        dwLoopCount = 10;   // no more than 10 retries to get the right size
        while ((status = BuildPhysDiskList (
                hWmiDiskPerf,
                pPhysDiskList,
                &dwNumPhysDiskListEntries)) == ERROR_INSUFFICIENT_BUFFER) {

            DebugPrint ((3,
                "MapDriveLetters: BuildPhysDiskList returns: %d, requesting %d entries\n",
                status, dwNumPhysDiskListEntries));
#if DBG
            if (!HeapValidate(hLibHeap, 0, pPhysDiskList)) {
                DebugPrint((2,
                    "\tERROR! pPhysDiskList %X corrupted BuildPhysDiskList\n",
                    pPhysDiskList));
                DbgBreakPoint();
            }
#endif

            // if ERROR_INSUFFICIENT_BUFFER, then 
            // dwNumPhysDiskListEntries should contain the required size
            pPhysDiskList = (PDRIVE_VOLUME_ENTRY)HeapReAlloc (
                hLibHeap, HEAP_ZERO_MEMORY, 
                pPhysDiskList, (dwNumPhysDiskListEntries * sizeof (DRIVE_VOLUME_ENTRY)));

            if (pPhysDiskList == NULL) {
                // bail if the allocation failed
                DebugPrint((2,
                    "MapDriveLetters: pPhysDiskList realloc failure\n"));
                status = ERROR_OUTOFMEMORY;
                break;
            }
#if DBG
            else {
                HeapUsed -= oldsize; // subtract the old size and add new size
                oldPLSize = dwNumPhysDiskListEntries*sizeof(DRIVE_VOLUME_ENTRY);
                HeapUsed += oldPLSize;
                DebugPrint((4,
                    "MapDriveLetter: Realloc old %d new %d to %d\n",
                    oldsize, oldPLSize, HeapUsed));
            }
#endif
            dwLoopCount--;
            if (!dwLoopCount) {
                status = ERROR_OUTOFMEMORY;
                break;
            }
            DebugPrint ((3, "MapDriveLetters: %d retrying BuildPhysDiskList with %d entries\n", status, dwNumPhysDiskListEntries));
        }
    }

    else {      // do not bother going any further if no memory
        return ERROR_OUTOFMEMORY;
    }

    DebugPrint ((4,
        "MapDriveLetters: BuildPhysDiskList returns: %d\n", status));
#if DBG
    if (pPhysDiskList != NULL) {
        if (!HeapValidate(hLibHeap, 0, pPhysDiskList)) {
            DebugPrint((2, "\tERROR! pPhysDiskList %X corrupted after Builds\n",
                pPhysDiskList));
            DbgBreakPoint();
        }
    }
#endif

    if (pVolumeList != NULL) {
        // close any open handles
        dwThisEntry = dwNumVolumeListEntries;
        while (dwThisEntry != 0) {
            dwThisEntry--;
            if (pVolumeList[dwThisEntry].hVolume != NULL) {
                NtClose (pVolumeList[dwThisEntry].hVolume);
            }
        } 
#if DBG
        HeapUsed -= oldVLSize;
        DebugPrint((4,"MapDriveLetters: VL Freed %d to %d\n",
            oldVLSize, HeapUsed));
        oldVLSize = 0;
        if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
            DebugPrint((2, "\tERROR! pVolumeList %X is corrupted before free\n",
                pVolumeList));
            DbgBreakPoint();
        }
#endif
        HeapFree (hLibHeap, 0, pVolumeList);
        pVolumeList = NULL;
    }
    dwNumVolumeListEntries = INITIAL_NUM_VOL_LIST_ENTRIES;

    // Initially allocate enough entries for letters C through Z
    pVolumeList = (PDRIVE_VOLUME_ENTRY)HeapAlloc (
        hLibHeap, HEAP_ZERO_MEMORY, 
        (dwNumVolumeListEntries * sizeof (DRIVE_VOLUME_ENTRY)));

#if DBG
    if (pVolumeList == NULL) {
        DebugPrint((2,
            "MapDriveLetters: pPhysVolumeList alloc failure\n"));
    }
#endif

    if (pVolumeList != NULL) {
        // try until we get a big enough buffer
#if DBG
        ULONG oldsize = dwNumVolumeListEntries * sizeof (DRIVE_VOLUME_ENTRY);
        HeapUsed += oldsize;
        oldVLSize = oldsize;
        DebugPrint((4,
            "MapDriveLetter: Add %d HeapUsed %d\n", oldsize, HeapUsed));
#endif
        dwLoopCount = 10;   // no more than 10 retries to get the right size
        while ((status = BuildVolumeList (
                pVolumeList,
                &dwNumVolumeListEntries)) == ERROR_INSUFFICIENT_BUFFER) {
            // if ERROR_INSUFFICIENT_BUFFER, then 

            DebugPrint ((3, "MapDriveLetters: BuildVolumeList returns: %d, requesting %d entries\n", status, dwNumPhysDiskListEntries));

#if DBG
            if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
                DebugPrint((2, "\tERROR! pVolumeList %X corrupted in while\n",
                    pVolumeList));
                DbgBreakPoint();
            }
#endif
            // dwNumVolumeListEntries should contain the required size
            pVolumeList = (PDRIVE_VOLUME_ENTRY)HeapReAlloc (
                hLibHeap, HEAP_ZERO_MEMORY, 
                pVolumeList, (dwNumVolumeListEntries * sizeof (DRIVE_VOLUME_ENTRY)));

            if (pVolumeList == NULL) {
                // bail if the allocation failed
                DebugPrint((2,
                    "MapDriveLetters: pPhysVolumeList realloc failure\n"));
                status = ERROR_OUTOFMEMORY;
                break;
            }
#if DBG
            else {
                if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
                    DebugPrint((2, "\tpVolumeList %X corrupted - realloc\n",
                        pVolumeList));
                    DbgBreakPoint();
                }
                HeapUsed -= oldsize; // subtract the old size and add new size
                oldVLSize = dwNumVolumeListEntries*sizeof(DRIVE_VOLUME_ENTRY);
                HeapUsed += oldVLSize;
                DebugPrint((4,
                    "MapDriveLetter: Realloc old %d new %d to %d\n",
                    oldsize, oldVLSize, HeapUsed));
            }
#endif
            dwLoopCount--;
            if (!dwLoopCount) {
                status = ERROR_OUTOFMEMORY;
                break;
            }
            DebugPrint ((3, "MapDriveLetters: retrying BuildVolumeList with %d entries\n", status, dwNumPhysDiskListEntries));
        }

        DebugPrint ((4, "MapDriveLetters: BuildVolumeList returns %d\n", status));

#if DBG
        if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
            DebugPrint((2, "\tpVolumeList %X corrupted after build\n",
                pVolumeList));
            DbgBreakPoint();
        }
#endif
        // now map the disks to their drive letters 
        if (status == ERROR_SUCCESS) {
            status = MapLoadedDisks (
                hWmiDiskPerf,
                pVolumeList,
                &dwNumVolumeListEntries,
                &dwMaxVolumeNumber,
                &dwWmiDriveCount
                );

            DebugPrint ((4,
                "MapDriveLetters: MapLoadedDisks returns status %d %d MaxVol %d WmiDrive\n",
                status, dwNumVolumeListEntries,
                dwMaxVolumeNumber, dwWmiDriveCount));
        }
        
#if DBG
        if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
            DebugPrint((2, "\tpVolumeList %X corrupted by MapLoadedDisks\n",
                pVolumeList));
            DbgBreakPoint();
        }
#endif
        if (status == ERROR_SUCCESS) {
            // now assign drive letters to the phys disk list
            dwDriveCount = 0;
            status = MakePhysDiskInstanceNames (
                    pPhysDiskList,
                    dwNumPhysDiskListEntries,
                    &dwDriveCount,
                    pVolumeList,
                    dwNumVolumeListEntries);
        
#if DBG
        if (!HeapValidate(hLibHeap, 0, pPhysDiskList)) {
            DebugPrint((2, "\tpPhysList %X corrupted by MakePhysDiskInst\n",
                pPhysDiskList));
            DbgBreakPoint();
        }
#endif
            if (status == ERROR_SUCCESS) {
                // then compress this into an indexed table
                // save original pointer
                pTempPtr = pPhysDiskList;

                // the function returns the last Drive ID
                // so we need to add 1 here to the count to include
                // the "0" drive
                dwDriveCount += 1;

                DebugPrint ((4, "\tDrive count now = %d\n",
                    dwDriveCount));

                // and allocate just enough for the actual physical drives
                pPhysDiskList = (PDRIVE_VOLUME_ENTRY)HeapAlloc (
                    hLibHeap, HEAP_ZERO_MEMORY, 
                    (dwDriveCount * sizeof (DRIVE_VOLUME_ENTRY)));

                if (pPhysDiskList != NULL) {
                    status = CompressPhysDiskTable (
                        pTempPtr,
                        dwNumPhysDiskListEntries,
                        pPhysDiskList,
                        dwDriveCount);
        
#if DBG
        if (!HeapValidate(hLibHeap, 0, pPhysDiskList)) {
            DebugPrint((2, "\tpPhysList %X corrupted by CompressPhys\n",
                pPhysDiskList));
            DbgBreakPoint();
        }
#endif
                    if (status == ERROR_SUCCESS) {
                        dwNumPhysDiskListEntries = dwDriveCount;
                    }
                    else {  // free if cannot compress
                        HeapFree(hLibHeap, 0, pPhysDiskList);
#if DBG
                        HeapUsed -= dwDriveCount * sizeof(DRIVE_VOLUME_ENTRY);
                        DebugPrint((4,
                            "MapDriveLetters: Compress freed %d to %d\n",
                            dwDriveCount*sizeof(DRIVE_VOLUME_ENTRY), HeapUsed));
#endif
                        pPhysDiskList = NULL;
                    }
                } else {
                    DebugPrint((2,"MapDriveLetters: pPhysDiskList alloc fail for compress\n"));
                    status = ERROR_OUTOFMEMORY;
                }
                if (pTempPtr) {     // Free the previous list
                    HeapFree(hLibHeap, 0, pTempPtr);
#if DBG
                    HeapUsed -= oldPLSize;
                    DebugPrint((4,
                        "MapDriveLetters: tempPtr freed %d to %d\n",
                        oldPLSize, HeapUsed));
                    oldPLSize = 0;
#endif
                }
#if DBG
                if (status == ERROR_SUCCESS) {
                    oldPLSize = dwDriveCount * sizeof(DRIVE_VOLUME_ENTRY);
                    HeapUsed += oldPLSize;
                    DebugPrint((4,
                        "MapDriveLetters: Compress add %d to %d\n",
                        oldPLSize, HeapUsed));
                }
#endif
            }
        }
        if (status == ERROR_SUCCESS) {
            // clear the remap flag
            bRemapDriveLetters = FALSE;
        }
    } else {
        status = ERROR_OUTOFMEMORY;
    }

    // TODO: Need to keep track of different status for PhysDisk & Volumes
    //       If Physdisk succeeds whereas Volume fails, need to log event
    //       and try and continue with Physdisk counters
    // TODO Post W2K: Free stuff if status != ERROR_SUCCESS
    return status;
}

DWORD APIENTRY
OpenDiskObject (
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
    LONGLONG    llLastBootTime;
    BOOL        bWriteMessage;

    UNREFERENCED_PARAMETER (lpDeviceNames);

    DebugPrint((1, "BEGIN OpenDiskObject:\n",
                   status));
    if (dwOpenCount == 0) {
        status = WmiOpenBlock (
            (GUID *)&DiskPerfGuid,
            GENERIC_READ,
            &hWmiDiskPerf);

        DebugPrint((3, "WmiOpenBlock returns: %d\n",
                   status));

        if (status == ERROR_SUCCESS) {
            // build drive map
            status = MapDriveLetters();

            DebugPrint((3,
                "OpenDiskObject: MapDriveLetters returns: %d\n",
                status));
        }
        // determine instance name format
        bUseNT4InstanceNames = NT4NamesAreDefault();

        if (status == ERROR_SUCCESS) {
            bInitOk = TRUE;
        }
    }

    if (status != ERROR_SUCCESS) {
        // check to see if this is a WMI error and if so only 
        // write the error once per boot cycle

        if (status == ERROR_WMI_GUID_NOT_FOUND) {
            bWriteMessage = SystemHasBeenRestartedSinceLastEntry (
                0, &llLastBootTime);
    
            if (bWriteMessage) {
                // update registry time
                WriteNewBootTimeEntry (&llLastBootTime);
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    PERFDISK_UNABLE_QUERY_DISKPERF_INFO,
                    NULL,
                    0,
                    sizeof(DWORD),
                    NULL,
                    (LPVOID)&status);
            } // else it's already been written
        } else {
            // always write other messages
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFDISK_UNABLE_OPEN,
                NULL,
                0,
                sizeof(DWORD),
                NULL,
                (LPVOID)&status);
        }
    } else {
        dwOpenCount++;
#ifndef _DONT_CHECK_FOR_VOLUME_FILTER
        if (!CheckVolumeFilter()) {
            posDataFuncInfo[0].dwCollectFunctionBit |= POS_COLLECT_IGNORE;
        }
#endif
    }

#if DBG
    if (status == ERROR_SUCCESS) {
        if (pPhysDiskList) {
            DebugPrint((4, "\t Validating pPhysDiskList %X at end Open\n",
                pPhysDiskList));
            if (!HeapValidate(hLibHeap, 0, pPhysDiskList)) {
                DebugPrint((2, "OpenDiskObject: PhysDiskList heap corrupt!\n"));
                DbgBreakPoint();
            }
        }
        if (pVolumeList) {
            DebugPrint((4, "\t Validating pVolumeList %X at end Open\n",
                pVolumeList));
            if (!HeapValidate(hLibHeap, 0, pVolumeList)) {
                DebugPrint((2, "OpenDiskObject: VolumeList heap corrupt!\n"));
                DbgBreakPoint();
            }
        }
        if (WmiBuffer) {
            DebugPrint((4, "\t Validating WmiBuffer %X at end Open\n",
                WmiBuffer));
            if (!HeapValidate(hLibHeap, 0, WmiBuffer)) {
                DebugPrint((2, "OpenDiskObject: WmiBuffer heap corrupt!\n"));
                DbgBreakPoint();
            }
        }
    }
    DebugPrint((1, "END OpenDiskObject: \n\n"));
#endif
    return  status;
}

DWORD APIENTRY
CollectDiskObjectData (
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

    NTSTATUS    Status;

    // build bit mask of functions to call

    DWORD       dwQueryType;
    DWORD       FunctionCallMask = 0;
    DWORD       FunctionIndex;

    DWORD       dwNumObjectsFromFunction;
    DWORD       dwOrigBuffSize;
    DWORD       dwByteSize;

    DebugPrint((1, "BEGIN CollectDiskObject:\n"));
    if (!bInitOk) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
        bShownDiskPerfMessage = TRUE;
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

	// if either bit is set, collect data
	if (FunctionCallMask && POS_COLLECT_GLOBAL_DATA) {
		__try {
			// read the data from the diskperf driver

			// only one call at a time is permitted. This should be 
			// throttled by the perflib, but just in case we'll test it

			assert (WmiBuffer == NULL);

			if (WmiBuffer != NULL) {
				ReportEvent (hEventLog,
					EVENTLOG_ERROR_TYPE,
					0,
					PERFDISK_BUSY,
					NULL,
					0,
					0,
					NULL,
					NULL);
				*lpcbTotalBytes = (DWORD) 0;
				*lpNumObjectTypes = (DWORD) 0;
				lReturn = ERROR_SUCCESS;
				goto COLLECT_BAIL_OUT;
			} else {
				WmiBuffer = ALLOCMEM (hLibHeap, HEAP_ZERO_MEMORY, WmiAllocSize);
#if DBG
                if (WmiBuffer != NULL) {
                    HeapUsed += WmiAllocSize;
                    DebugPrint((4,
                        "CollecDiskObjectData: WmiBuffer added %d to %d\n",
                        WmiAllocSize, HeapUsed));
                }
#endif
			}

			// the buffer pointer should NOT be null if here

			if ( WmiBuffer == NULL ) {
				ReportEvent (hEventLog,
					EVENTLOG_WARNING_TYPE,
					0,
					PERFDISK_UNABLE_ALLOC_BUFFER,
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

			WmiBufSize = WmiAllocSize;
			Status = WmiQueryAllDataW (
				hWmiDiskPerf,
				&WmiBufSize,
				WmiBuffer);

			// if buffer size attempted is too big or too small, resize
			if ((WmiBufSize > 0) && (WmiBufSize != WmiAllocSize)) {
				WmiBuffer = REALLOCMEM (hLibHeap,
					HEAP_ZERO_MEMORY,
					WmiBuffer, WmiBufSize);

				if (WmiBuffer == NULL) {
					// reallocation failed so bail out
					Status = ERROR_OUTOFMEMORY;
				} else {
					// if the required buffer is larger than 
					// originally planned, bump it up some
#if DBG
                    HeapUsed += (WmiBufSize - WmiAllocSize);
                    DebugPrint((4,
                        "CollectDiskObjectData: Realloc old %d new %d to %d\n",
                        WmiAllocSize, WmiBufSize, HeapUsed));
#endif
					if (WmiBufSize > WmiAllocSize) {                    
						WmiAllocSize = WmiBufSize;
					}
				}
			}

			if (Status == ERROR_INSUFFICIENT_BUFFER) {
				// if it didn't work because it was too small the first time
				// try one more time
				Status = WmiQueryAllDataW (
					hWmiDiskPerf,
					&WmiBufSize,
					WmiBuffer);
            
			} else {
				// it either worked the fisrt time or it failed because of 
				// something other than a buffer size problem
			}

		} __except (EXCEPTION_EXECUTE_HANDLER) {
            DebugPrint((2, "\tWmiBuffer %X size %d set to NULL\n",
                WmiBuffer, WmiAllocSize));
            if (WmiBuffer != NULL) {
                if (HeapValidate(hLibHeap, 0, WmiBuffer)) {
                    FREEMEM(hLibHeap, 0, WmiBuffer);
                }
            } // else we are really in trouble with WmiBuffer!!
			WmiBuffer = NULL;
			Status = ERROR_OUTOFMEMORY;
		}
        DebugPrint((3,
            "WmiQueryAllData status return: %x Buffer %d bytes\n",
            Status, WmiBufSize));

    } else {
        // no data required so these counter objects must not be in 
        // the query list
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
        goto COLLECT_BAIL_OUT;
    }

    if (Status == ERROR_SUCCESS) {
        *lpNumObjectTypes = 0;
        dwOrigBuffSize = dwByteSize = *lpcbTotalBytes;
        *lpcbTotalBytes = 0;

        for (FunctionIndex = 0; FunctionIndex < POS_NUM_FUNCS; FunctionIndex++) {
            if (posDataFuncInfo[FunctionIndex].dwCollectFunctionBit &
                POS_COLLECT_IGNORE)
                continue;

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
#if DBG
            dwQueryType = HeapValidate(hLibHeap, 0, WmiBuffer);
            DebugPrint((4,
                "CollectDiskObjectData: Index %d HeapValid %d lReturn %d\n",
                FunctionIndex, dwQueryType, lReturn));
            if (!dwQueryType)
                DbgBreakPoint();
#endif
        }
    } else {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFDISK_UNABLE_QUERY_DISKPERF_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&Status);

        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        lReturn = ERROR_SUCCESS;
    }

    // *lppData is updated by each function
    // *lpcbTotalBytes is updated after each successful function
    // *lpNumObjects is updated after each successful function

COLLECT_BAIL_OUT:
    if (WmiBuffer != NULL) {
        FREEMEM (hLibHeap, 0, WmiBuffer);
#if DBG
        HeapUsed -= WmiBufSize;
        DebugPrint((4, "CollectDiskObjectData: Freed %d to %d\n",
            WmiBufSize, HeapUsed));
#endif
        WmiBuffer = NULL;
    }
    
    DebugPrint((1, "END CollectDiskObject\n\n"));
    return lReturn;
}

DWORD APIENTRY
CloseDiskObject (
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
    DWORD   dwThisEntry;

    DebugPrint((1, "BEGIN CloseDiskObject:\n"));
    if (--dwOpenCount == 0) {
        if (pVolumeList != NULL) {
            // close handles in volume list
            dwThisEntry = dwNumVolumeListEntries;
            while (dwThisEntry != 0) {
                dwThisEntry--;
                if (pVolumeList[dwThisEntry].hVolume != NULL) {
                    NtClose (pVolumeList[dwThisEntry].hVolume);
                }
            } 
            HeapFree (hLibHeap, 0, pVolumeList);
#if DBG
            HeapUsed -= oldVLSize;
            DebugPrint((4, "CloseDiskObject: Freed VL %d to %d\n",
                oldVLSize, HeapUsed));
            oldVLSize = 0;
#endif
            pVolumeList = NULL;
            dwNumVolumeListEntries = 0;
        }
        if (pPhysDiskList != NULL) {
            HeapFree (hLibHeap, 0, pPhysDiskList);
#if DBG
            HeapUsed -= oldPLSize;
            DebugPrint((4, "CloseDiskObject: Freed PL %d to %d\n",
                oldVLSize, HeapUsed));
            oldPLSize = 0;
#endif
            pPhysDiskList = NULL;
            dwNumPhysDiskListEntries = 0;
        }
        // close PDisk object
        if (hWmiDiskPerf != NULL) {
            status = WmiCloseBlock (hWmiDiskPerf);
            hWmiDiskPerf = NULL;
        }
    }
    DebugPrint((1, "END CloseDiskObject\n\n"));
    return  status;

}

#ifndef _DONT_CHECK_FOR_VOLUME_FILTER
ULONG
CheckVolumeFilter(
    )
/*++

Routine Description:

    This routine checks to see if diskperf is set to be an upper filter
    for Storage Volumes

Arguments:

    None.


Return Value:

    TRUE if there is a filter

--*/
{
    WCHAR Buffer[MAX_PATH+1];
    WCHAR *string = Buffer;
    DWORD dwSize = sizeof(Buffer);
    ULONG stringLength, diskperfLen, result, status;
    HKEY hKey;

    status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                cszVolumeKey,
                (DWORD) 0,
                KEY_QUERY_VALUE,
                &hKey
                );
    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegQueryValueExW(
                hKey,
                (LPCWSTR)REGSTR_VAL_UPPERFILTERS,
                NULL,
                NULL,
                (LPBYTE) Buffer,
                &dwSize);
    if (status != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }

    stringLength = wcslen(string);

    diskperfLen = wcslen((LPCWSTR)DISKPERF_SERVICE_NAME);

    result = FALSE;
    while(stringLength != 0) {

        if (diskperfLen == stringLength) {
            if(_wcsicmp(string, (LPCWSTR)DISKPERF_SERVICE_NAME) == 0) {
                result = TRUE;
                break;
            }
        } else {
            string += stringLength + 1;
            stringLength = wcslen(string);
        }
    }
    RegCloseKey(hKey);
    return result;
}
#endif

#if DBG
VOID
PerfDiskDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all PerfDisk

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    if ((DebugPrintLevel <= (PerfDiskDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & PerfDiskDebug)) {
        DbgPrint("%d:Perfdisk!", GetCurrentThreadId());
    }
    else
        return;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (PerfDiskDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & PerfDiskDebug)) {

        _vsnprintf((LPSTR)PerfDiskDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint((LPSTR)PerfDiskDebugBuffer);
    }

    va_end(ap);

}
#endif
