/*++ 

Copyright (c) 1996  Microsoft Corporation

Module Name:

    perfcach.c

Abstract:

    This file implements an Performance Object that presents
    File System Cache data

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
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfos.h"
#include "datacach.h"

//
//  The following special defines are used to produce numbers for
//  cache measurement counters
//

#define SYNC_ASYNC(FLD) ((SysPerfInfo.FLD##Wait) + (SysPerfInfo.FLD##NoWait))

//
// Hit Rate Macro
//
#define HITRATE(FLD) (((Changes = SysPerfInfo.FLD) == 0) ? 0 :                                         \
                      ((Changes < (Misses = SysPerfInfo.FLD##Miss)) ? 0 :                              \
                      (Changes - Misses) ))

//
// Hit Rate Macro combining Sync and Async cases
//

#define SYNC_ASYNC_HITRATE(FLD) (((Changes = SYNC_ASYNC(FLD)) == 0) ? 0 : \
                                   ((Changes < \
                                    (Misses = SysPerfInfo.FLD##WaitMiss + \
                                              SysPerfInfo.FLD##NoWaitMiss) \
                                   ) ? 0 : \
                                  (Changes - Misses) ))


DWORD APIENTRY
CollectCacheObjectData (
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
    DWORD  TotalLen;            //  Length of the total return block
    DWORD  *pdwCounter;
    DWORD  Changes;             //  Used by macros to compute cache
    DWORD  Misses;              //  ...statistics

    DWORD   dwReturnedBufferSize;
    DWORD       dwQueryType;
    NTSTATUS    ntStatus;

    PCACHE_DATA_DEFINITION  pCacheDataDefinition;
    PCACHE_COUNTER_DATA     pCCD;

    //
    //  Check for enough space for cache data block
    //

    pCacheDataDefinition = (CACHE_DATA_DEFINITION *) *lppData;

    TotalLen = sizeof(CACHE_DATA_DEFINITION) +
                sizeof(CACHE_COUNTER_DATA);

    TotalLen = QWORD_MULTIPLE(TotalLen);

    if ( *lpcbTotalBytes < TotalLen ) {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    //
    //  Define cache data block
    //


    memcpy (pCacheDataDefinition,
        &CacheDataDefinition,
        sizeof(CACHE_DATA_DEFINITION));

    //
    //  Format and collect memory data
    //

    pCCD = (PCACHE_COUNTER_DATA)&pCacheDataDefinition[1];

    pCCD->CounterBlock.ByteLength = sizeof(CACHE_COUNTER_DATA);

    //
    //  The Data Map counter is the sum of the Wait/NoWait cases
    //

    pCCD->DataMaps = SYNC_ASYNC(CcMapData);

    pCCD->SyncDataMaps = SysPerfInfo.CcMapDataWait;
    pCCD->AsyncDataMaps = SysPerfInfo.CcMapDataNoWait;

    //
    //  The Data Map Hits is a percentage of Data Maps that hit
    //  the cache; second counter is the base (divisor)
    //

    pCCD->DataMapHits = SYNC_ASYNC_HITRATE(CcMapData);
    pCCD->DataMapHitsBase = SYNC_ASYNC(CcMapData);

    //
    //  The next pair of counters forms a percentage of
    //  Pins as a portion of Data Maps
    //

    pCCD->DataMapPins = SysPerfInfo.CcPinMappedDataCount;
    pCCD->DataMapPinsBase = SYNC_ASYNC(CcMapData);

    pCCD->PinReads = SYNC_ASYNC(CcPinRead);
    pCCD->SyncPinReads = SysPerfInfo.CcPinReadWait;
    pCCD->AsyncPinReads = SysPerfInfo.CcPinReadNoWait;

    //
    //  The Pin Read Hits is a percentage of Pin Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    pCCD->PinReadHits = SYNC_ASYNC_HITRATE(CcPinRead);
    pCCD->PinReadHitsBase = SYNC_ASYNC(CcPinRead);


    pCCD->CopyReads = SYNC_ASYNC(CcCopyRead);
    pCCD->SyncCopyReads = SysPerfInfo.CcCopyReadWait;
    pCCD->AsyncCopyReads = SysPerfInfo.CcCopyReadNoWait;

    //
    //  The Copy Read Hits is a percentage of Copy Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    pCCD->CopyReadHits = SYNC_ASYNC_HITRATE(CcCopyRead);
    pCCD->CopyReadHitsBase = SYNC_ASYNC(CcCopyRead);


    pCCD->MdlReads = SYNC_ASYNC(CcMdlRead);
    pCCD->SyncMdlReads = SysPerfInfo.CcMdlReadWait;
    pCCD->AsyncMdlReads = SysPerfInfo.CcMdlReadNoWait;

    //
    //  The Mdl Read Hits is a percentage of Mdl Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    pCCD->MdlReadHits = SYNC_ASYNC_HITRATE(CcMdlRead);
    pCCD->MdlReadHitsBase = SYNC_ASYNC(CcMdlRead);

    pCCD->ReadAheads = SysPerfInfo.CcReadAheadIos;

    pCCD->FastReads = SYNC_ASYNC(CcFastRead);
    pCCD->SyncFastReads = SysPerfInfo.CcFastReadWait;
    pCCD->AsyncFastReads = SysPerfInfo.CcFastReadNoWait;

    pCCD->FastReadResourceMiss = SysPerfInfo.CcFastReadResourceMiss;
    pCCD->FastReadNotPossibles = SysPerfInfo.CcFastReadNotPossible;
    pCCD->LazyWriteFlushes = SysPerfInfo.CcLazyWriteIos;
    pCCD->LazyWritePages = SysPerfInfo.CcLazyWritePages;
    pCCD->DataFlushes = SysPerfInfo.CcDataFlushes;
    pCCD->DataPages = SysPerfInfo.CcDataPages;

    *lppData = (LPVOID)&pCCD[1];

    // round up buffer to the nearest QUAD WORD
    
    *lppData = ALIGN_ON_QWORD (*lppData);

    *lpcbTotalBytes =
        pCacheDataDefinition->CacheObjectType.TotalByteLength =
            (DWORD)((LPBYTE)*lppData - (LPBYTE)pCacheDataDefinition);

    *lpNumObjectTypes = 1;

    return ERROR_SUCCESS;
}
