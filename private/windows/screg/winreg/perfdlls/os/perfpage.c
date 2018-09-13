/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfpage.c

Abstract:

    This file implements an Performance Object that presents
    system Page file performance data

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
#include <assert.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfos.h"
#include "perfosmc.h"
#include "datapage.h"

DWORD   dwPageOpenCount = 0;        // count of "Open" threads

static  PSYSTEM_PAGEFILE_INFORMATION pSysPageFileInfo = NULL;
static  DWORD  dwSysPageFileInfoSize = 0; // size of page file info array


DWORD APIENTRY
OpenPageFileObject (
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
    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    if (!dwPageOpenCount) {
        // allocate the memory for the Page file info

        dwSysPageFileInfoSize = LARGE_BUFFER_SIZE;

        pSysPageFileInfo = ALLOCMEM (
            hLibHeap, HEAP_ZERO_MEMORY,
            dwSysPageFileInfoSize);

        if (pSysPageFileInfo == NULL) {
            status = ERROR_OUTOFMEMORY;
            goto OpenExitPoint;
        }
    }

    dwPageOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}


DWORD APIENTRY
CollectPageFileObjectData (
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the XXX object

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
    DWORD   TotalLen;            //  Length of the total return block

    DWORD   PageFileNumber;
    DWORD   NumPageFileInstances;
    DWORD   dwReturnedBufferSize;

    NTSTATUS    status;

    PSYSTEM_PAGEFILE_INFORMATION    pThisPageFile;
    PPAGEFILE_DATA_DEFINITION       pPageFileDataDefinition;
    PPERF_INSTANCE_DEFINITION       pPerfInstanceDefinition;
    PPAGEFILE_COUNTER_DATA          pPFCD;
    PAGEFILE_COUNTER_DATA           TotalPFCD;

    //
    //  Check for sufficient space for the Pagefile object
    //  and counter type definition records, + one instance and
    //  one set of counter data
    //

    TotalLen = sizeof(PAGEFILE_DATA_DEFINITION) +
                sizeof(PERF_INSTANCE_DEFINITION) +
                MAX_PATH * sizeof(WCHAR) +
                sizeof(PAGEFILE_COUNTER_DATA);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    status = (NTSTATUS) -1; // so we go throug the loop at least once

    while ((status = NtQuerySystemInformation(
                SystemPageFileInformation,  // item id
                pSysPageFileInfo,           // address of buffer to get data
                dwSysPageFileInfoSize,      // size of buffer
                &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH) {
        dwSysPageFileInfoSize += INCREMENT_BUFFER_SIZE;
        pSysPageFileInfo = REALLOCMEM (hLibHeap,
            0, pSysPageFileInfo,
            dwSysPageFileInfoSize);

        if (pSysPageFileInfo == NULL) {
            status = ERROR_OUTOFMEMORY;
            break;
        }
    }

    if ( !NT_SUCCESS(status) ) {
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFOS_UNABLE_QUERY_PAGEFILE_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            &status);
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        status = (NTSTATUS)RtlNtStatusToDosError(status);
        return status;
    }

    pPageFileDataDefinition = (PPAGEFILE_DATA_DEFINITION) *lppData;
    //
    //  Define Page File data block
    //

    memcpy (pPageFileDataDefinition,
        &PagefileDataDefinition,
        sizeof(PAGEFILE_DATA_DEFINITION));

    // Now load data for each PageFile

    // clear the total fields
    memset (&TotalPFCD, 0, sizeof(TotalPFCD));
    TotalPFCD.CounterBlock.ByteLength = sizeof (PAGEFILE_COUNTER_DATA);

    PageFileNumber = 0;
    NumPageFileInstances = 0;

    pThisPageFile = pSysPageFileInfo;   // initialize pointer to list of pagefiles

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                            &pPageFileDataDefinition[1];

    // the check for NULL pointer is NOT the exit criteria for this loop,
    // merely a check to bail out if the first (or any subsequent) pointer
    // is NULL. Normally the loop will exit when the NextEntryOffset == 0

    while ( pThisPageFile != NULL ) {

        // compute the size required for the next instance record

        TotalLen =
            // current bytes already used
            (DWORD)((LPBYTE)pPerfInstanceDefinition -
                (LPBYTE)pPageFileDataDefinition)
            // + this instance definition
            + sizeof(PERF_INSTANCE_DEFINITION)
            // + the file (instance) name
            + QWORD_MULTIPLE(pThisPageFile->PageFileName.Length + sizeof(WCHAR))
            // + the data block
            + sizeof (PAGEFILE_COUNTER_DATA);

        TotalLen = QWORD_MULTIPLE(TotalLen+4); // round up to the next quadword

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        // Build an Instance

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *) &pPFCD,
            0,
            0,
            (DWORD)-1,
            pThisPageFile->PageFileName.Buffer);

        //
        //  Format the pagefile data
        //

        pPFCD->CounterBlock.ByteLength = sizeof (PAGEFILE_COUNTER_DATA);

        pPFCD->PercentInUse = pThisPageFile->TotalInUse;
        pPFCD->PeakUsageBase =
            pPFCD->PercentInUseBase = pThisPageFile->TotalSize;
        pPFCD->PeakUsage = pThisPageFile->PeakUsage;

        // update the total accumulators

        TotalPFCD.PeakUsageBase =
            TotalPFCD.PercentInUseBase += pThisPageFile->TotalSize;
        TotalPFCD.PeakUsage     += pThisPageFile->PeakUsage;
        TotalPFCD.PercentInUse  += pThisPageFile->TotalInUse;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pPFCD[1];
        NumPageFileInstances++;
        PageFileNumber++;

        if (pThisPageFile->NextEntryOffset != 0) {
            pThisPageFile = (PSYSTEM_PAGEFILE_INFORMATION)\
                        ((BYTE *)pThisPageFile + pThisPageFile->NextEntryOffset);
        } else {
            break;
        }
    }


    if (NumPageFileInstances > 0) {
        // compute the size required for the next instance record

        TotalLen =
            // current bytes already used
            (DWORD)((LPBYTE)pPerfInstanceDefinition -
                (LPBYTE)pPageFileDataDefinition)
            // + this instance definition
            + sizeof(PERF_INSTANCE_DEFINITION)
            // + the file (instance) name
            + QWORD_MULTIPLE((lstrlenW (wszTotal) + 1) * sizeof (WCHAR))
            // + the data block
            + sizeof (PAGEFILE_COUNTER_DATA);

        TotalLen = QWORD_MULTIPLE(TotalLen+4); // round up to the next quadword

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        // Build the Total Instance

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *)&pPFCD,
            0,
            0,
            (DWORD)-1,
            (LPWSTR)wszTotal);

        //
        //  copy the total data
        //

        memcpy (pPFCD, &TotalPFCD, sizeof (TotalPFCD));

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pPFCD[1];
        NumPageFileInstances++;
    }
    // Note number of PageFile instances

    pPageFileDataDefinition->PagefileObjectType.NumInstances =
        NumPageFileInstances;

    //
    //  update pointers for return
    //

    *lppData = (LPVOID) pPerfInstanceDefinition;

    // round up buffer to the nearest QUAD WORD

    *lppData = ALIGN_ON_QWORD (*lppData);

    *lpcbTotalBytes =
        pPageFileDataDefinition->PagefileObjectType.TotalByteLength =
            (DWORD)((PCHAR) *lppData -
            (PCHAR) pPageFileDataDefinition);

#if DBG
    if (*lpcbTotalBytes > TotalLen ) {
        DbgPrint ("\nPERFOS: Paging File Perf Ctr. Instance Size Underestimated:");
        DbgPrint ("\nPERFOS:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
    }
#endif

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}


#pragma warning (disable : 4706)
DWORD APIENTRY
ClosePageFileObject (
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
    if (dwPageOpenCount > 0) {
        if (!(--dwPageOpenCount)) { // when this is the last thread...
            // close stuff here
            if (hLibHeap != NULL) {
                if (pSysPageFileInfo != NULL) {
                    FREEMEM (hLibHeap, 0, pSysPageFileInfo);
                    pSysPageFileInfo = NULL;
                }
            }
        }
    } else {
        // if open count == 0, then this should be null
        assert (pSysPageFileInfo == NULL);
    }

    return ERROR_SUCCESS;

}
#pragma warning (default: 4706)
