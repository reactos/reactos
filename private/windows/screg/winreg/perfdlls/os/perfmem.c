/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfmem.c

Abstract:

    This file implements an Performance Object that presents
    System Memory Performance Object

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
#include <ntmmapi.h>
#include <windows.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfos.h"
#include "perfosmc.h"
#include "datamem.h"

static  DWORD   dwOpenCount = 0;        // count of "Open" threads


DWORD APIENTRY
CollectMemoryObjectData (
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
    NTSTATUS Status;
    DWORD  TotalLen;            //  Length of the total return block
    DWORD dwReturnedBufferSize;
    DWORD       dwQueryType;

    PMEMORY_DATA_DEFINITION         pMemoryDataDefinition;
    SYSTEM_FILECACHE_INFORMATION    FileCache;
    PMEMORY_COUNTER_DATA    pMCD;
    DWORD       LocalPageSize;

    pMemoryDataDefinition = (MEMORY_DATA_DEFINITION *) *lppData;

    //
    //  Check for enough space for memory data block
    //

    TotalLen = sizeof(MEMORY_DATA_DEFINITION) +
                sizeof(MEMORY_COUNTER_DATA);

    TotalLen = QWORD_MULTIPLE (TotalLen);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );

    if (!NT_SUCCESS(Status)) {
        ReportEvent (hEventLog,
            EVENTLOG_WARNING_TYPE,
            0,
            PERFOS_UNABLE_QUERY_FILE_CACHE_INFO,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&Status);
        memset (&FileCache, 0, sizeof(FileCache));
    }

    //
    //  Define memory data block
    //

    memcpy (pMemoryDataDefinition,
        &MemoryDataDefinition,
        sizeof(MEMORY_DATA_DEFINITION));

    //
    //  Format and collect memory data
    //

    LocalPageSize = BasicInfo.PageSize;

    pMCD = (PMEMORY_COUNTER_DATA)&pMemoryDataDefinition[1];

    pMCD->CounterBlock.ByteLength = sizeof (MEMORY_COUNTER_DATA);

    pMCD->AvailablePages = SysPerfInfo.AvailablePages;
    pMCD->AvailablePages *= LocalPageSize; // display as bytes
    pMCD->AvailableKBytes = pMCD->AvailablePages / 1024;
    pMCD->AvailableMBytes = pMCD->AvailableKBytes / 1024;
    pMCD->CommittedPages = SysPerfInfo.CommittedPages;
    pMCD->CommittedPages *= LocalPageSize;
    pMCD->CommitList    = SysPerfInfo.CommitLimit;
    pMCD->CommitList    *= LocalPageSize;
    pMCD->PageFaults    = SysPerfInfo.PageFaultCount;
    pMCD->WriteCopies   = SysPerfInfo.CopyOnWriteCount;
    pMCD->TransitionFaults  = SysPerfInfo.TransitionCount;
    pMCD->CacheFaults   = FileCache.PageFaultCount;
    pMCD->DemandZeroFaults  = SysPerfInfo.DemandZeroCount;
    pMCD->Pages         = SysPerfInfo.PageReadCount +
                            SysPerfInfo.DirtyPagesWriteCount;
    pMCD->PagesInput    = SysPerfInfo.PageReadCount;
    pMCD->PageReads     = SysPerfInfo.PageReadIoCount;
    pMCD->DirtyPages    = SysPerfInfo.DirtyPagesWriteCount;
    pMCD->DirtyWrites   = SysPerfInfo.DirtyWriteIoCount;
    pMCD->PagedPool     = SysPerfInfo.PagedPoolPages;
    pMCD->PagedPool     *= LocalPageSize;
    pMCD->NonPagedPool  = SysPerfInfo.NonPagedPoolPages;
    pMCD->NonPagedPool  *= LocalPageSize;
    pMCD->PagedPoolAllocs   = SysPerfInfo.PagedPoolAllocs -
                                SysPerfInfo.PagedPoolFrees;
    pMCD->NonPagedPoolAllocs = SysPerfInfo.NonPagedPoolAllocs -
                                SysPerfInfo.NonPagedPoolFrees;
    pMCD->FreeSystemPtes    = SysPerfInfo.FreeSystemPtes;
    pMCD->CacheBytes    = FileCache.CurrentSize;
    pMCD->PeakCacheBytes    = FileCache.PeakSize;
    pMCD->ResidentPagedPoolBytes = SysPerfInfo.ResidentPagedPoolPage;
    pMCD->ResidentPagedPoolBytes *= LocalPageSize;
    pMCD->TotalSysCodeBytes     = SysPerfInfo.TotalSystemCodePages;
    pMCD->TotalSysCodeBytes     *= LocalPageSize;
    pMCD->ResidentSysCodeBytes  = SysPerfInfo.ResidentSystemCodePage;
    pMCD->ResidentSysCodeBytes  *= LocalPageSize;
    pMCD->TotalSysDriverBytes   = SysPerfInfo.TotalSystemDriverPages;
    pMCD->TotalSysDriverBytes   *= LocalPageSize;
    pMCD->ResidentSysDriverBytes = SysPerfInfo.ResidentSystemDriverPage;
    pMCD->ResidentSysDriverBytes *= LocalPageSize;
    pMCD->ResidentSysCacheBytes = SysPerfInfo.ResidentSystemCachePage;
    pMCD->ResidentSysCacheBytes *= LocalPageSize;

    //  This is reported as a percentage of CommittedPages/CommitLimit.
    //  these value return a value in "page" units. Since this is a
    //  fraction, the page size (i.e. converting pages to bytes) will
    //  cancel out and as such can be ignored, saving some CPU cycles
    //
    pMCD->CommitBytesInUse  = SysPerfInfo.CommittedPages;
    pMCD->CommitBytesLimit  = SysPerfInfo.CommitLimit;

#if 0	// no longer supported
    // load the VLM counters - this should really be removed period.
    pMCD->SystemVlmCommitCharge = 0;
    pMCD->SystemVlmPeakCommitCharge = 0;
    pMCD->SystemVlmSharedCommitCharge = 0;
#endif
    *lppData = (LPVOID)&pMCD[1];

    // round up buffer to the nearest QUAD WORD
    
    *lppData = ALIGN_ON_QWORD (*lppData);

    *lpcbTotalBytes =
        pMemoryDataDefinition->MemoryObjectType.TotalByteLength =
            (DWORD)((LPBYTE)*lppData - (LPBYTE)pMemoryDataDefinition);

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}
