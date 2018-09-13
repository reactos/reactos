/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    logidisk.c

Abstract:

    This file implements a Performance Object that presents
    Logical Disk Performance object data

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
#include <ntdddisk.h>
#include <windows.h>
#include <ole2.h>
#include <wmium.h>
#include <assert.h>
#include <winperf.h>
#include <ntprfctr.h>
#include <perfutil.h>
#include "perfdisk.h"
#include "diskmsg.h"
#include "datalogi.h"

DWORD APIENTRY
CollectLDiskObjectData(
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the logical disk object

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
    PLDISK_DATA_DEFINITION      pLogicalDiskDataDefinition;
    DWORD  TotalLen;            //  Length of the total return block
    LDISK_COUNTER_DATA          lcdTotal;

    DWORD   dwStatus    = ERROR_SUCCESS;
    PPERF_INSTANCE_DEFINITION   pPerfInstanceDefinition;

    PWNODE_ALL_DATA WmiDiskInfo;
    DISK_PERFORMANCE            *pDiskPerformance;    //  Disk driver returns counters here

    PWCHAR  wszInstanceName;
    DWORD   dwInstanceNameOffset;

    DWORD   dwNumLogicalDisks;

    WCHAR   wszTempName[MAX_PATH];
    WORD    wNameLength;
    WCHAR   wszDriveName[MAX_PATH];
    DWORD   dwDriveNameSize;

    PLDISK_COUNTER_DATA         pLCD;

    BOOL    bMoreEntries;

    DWORD   dwReturn = ERROR_SUCCESS;

    LONGLONG    llTemp;
    DWORD       dwTemp;
    HANDLE      hVolume;

    FILE_FS_SIZE_INFORMATION FsSizeInformation;
    IO_STATUS_BLOCK status_block;       //  Disk driver status
    ULONG AllocationUnitBytes;
    LONGLONG    TotalBytes;
    LONGLONG    FreeBytes;

    DWORD       dwCurrentWmiObjCount = 0;
    DWORD       dwRemapCount = 10;

    DOUBLE      dReadTime, dWriteTime, dTransferTime;

    //
    //  Check for sufficient space for Logical Disk object
    //  type definition
    //

    do {
        dwNumLogicalDisks = 0;
        // make sure the drive letter map is up-to-date
        if (bRemapDriveLetters) {
            dwStatus = MapDriveLetters();
            // MapDriveLetters clears the remap flag when successful
            if (dwStatus != ERROR_SUCCESS) {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                return dwStatus;
            }
        }

        pLogicalDiskDataDefinition = (LDISK_DATA_DEFINITION *) *lppData;

        // clear the accumulator structure

        memset (&lcdTotal, 0, sizeof(lcdTotal));
        //
        //  Define Logical Disk data block
        //

        TotalLen = sizeof (LDISK_DATA_DEFINITION);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        memmove(pLogicalDiskDataDefinition,
               &LogicalDiskDataDefinition,
               sizeof(LDISK_DATA_DEFINITION));


        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pLogicalDiskDataDefinition[1];

        WmiDiskInfo = (PWNODE_ALL_DATA)WmiBuffer;

        // make sure the structure is valid
        if (WmiDiskInfo->WnodeHeader.BufferSize < sizeof(WNODE_ALL_DATA)) {
            bMoreEntries = FALSE;
            // just to make sure someone notices on a checked build
            assert (WmiDiskInfo->WnodeHeader.BufferSize >= sizeof(WNODE_ALL_DATA));
        } else {
            // make sure there are some entries to return
            bMoreEntries =
                (WmiDiskInfo->InstanceCount > 0) ? TRUE : FALSE;
        }

        while (bMoreEntries) {

            pDiskPerformance = (PDISK_PERFORMANCE)(
                        (PUCHAR)WmiDiskInfo +  WmiDiskInfo->DataBlockOffset);
            dwInstanceNameOffset = *((LPDWORD)(
                        (LPBYTE)WmiDiskInfo +  WmiDiskInfo->OffsetInstanceNameOffsets));
            wNameLength = *(WORD *)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset);
            if (wNameLength > 0) {
                wszInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                // copy to local buffer for processing
                if (wNameLength >= MAX_PATH) wNameLength = MAX_PATH-1; // truncate if necessary
                // copy text
                memcpy (wszTempName, wszInstanceName, wNameLength);
                // then null terminate
                wNameLength /= 2;
                wszTempName[wNameLength] = 0;

#if _DBG_PRINT_INSTANCES
                    OutputDebugStringW ((LPCWSTR)L"\nPERFDISK: Logical Disk Instance: ");
                    OutputDebugStringW (wszTempName);
#endif

                // see if this is a Physical Drive
                if (!IsPhysicalDrive(pDiskPerformance)) {
                    // it's not so get the name of it for this instance
                    dwDriveNameSize = sizeof (wszDriveName) / sizeof(wszDriveName[0]);
                    dwStatus = GetDriveNameString (
                        wszTempName, 
                        (DWORD)wNameLength,
                        pVolumeList,
                        dwNumVolumeListEntries,
                        wszDriveName,
                        &dwDriveNameSize,
                        (LPCWSTR)&(pDiskPerformance->StorageManagerName[0]),
                        pDiskPerformance->StorageDeviceNumber,
                        &hVolume);
                    if (dwStatus != ERROR_SUCCESS) {
                        // just so we have a name
                        lstrcpyW (wszDriveName, wszTempName);
                        dwDriveNameSize = lstrlenW(wszDriveName);
                    }
#if _DBG_PRINT_INSTANCES
                    OutputDebugStringW ((LPCWSTR)L" loaded as ");
                    OutputDebugStringW (wszDriveName);
#endif

                    TotalLen =
                        // space already used
                        (DWORD)((PCHAR) pPerfInstanceDefinition -
                            (PCHAR) pLogicalDiskDataDefinition)
                        // + estimate of this instance
                        +   sizeof(PERF_INSTANCE_DEFINITION)
                        +   (dwDriveNameSize + 1) * sizeof(WCHAR) ;
                    TotalLen = QWORD_MULTIPLE (TotalLen);
                    TotalLen += sizeof(LDISK_COUNTER_DATA);
                    TotalLen = QWORD_MULTIPLE (TotalLen);

                    if ( *lpcbTotalBytes < TotalLen ) {
                        *lpcbTotalBytes = (DWORD) 0;
                        *lpNumObjectTypes = (DWORD) 0;
                        dwReturn = ERROR_MORE_DATA;
                        break;
                    }

                    MonBuildInstanceDefinition(
                        pPerfInstanceDefinition,
                        (PVOID *) &pLCD,
                        0, 0,   // no parent
                        (DWORD)-1,// no unique ID
                        &wszDriveName[0]);

                    // insure quadword alignment of the data structure
                    assert (((DWORD)(pLCD) & 0x00000007) == 0);

                    //  Set up pointer for data collection

                    // the QueueDepth counter is only a byte so clear the unused bytes
                    pDiskPerformance->QueueDepth &= 0x000000FF;

                    //
                    //  Format and collect Physical data
                    //
                    lcdTotal.DiskCurrentQueueLength += pDiskPerformance->QueueDepth;
                    pLCD->DiskCurrentQueueLength = pDiskPerformance->QueueDepth;

                    llTemp = pDiskPerformance->ReadTime.QuadPart +
                             pDiskPerformance->WriteTime.QuadPart;

                    // these values are read in 100 NS units but are expected
                    // to be in sys perf freq (tick) units for the Sec/op ctrs 
                    // so convert them here

                    dReadTime = (DOUBLE)(pDiskPerformance->ReadTime.QuadPart);
                    dWriteTime = (DOUBLE)(pDiskPerformance->WriteTime.QuadPart);
                    dTransferTime = (DOUBLE)(llTemp);

                    dReadTime *= dSysTickTo100Ns;
                    dWriteTime *= dSysTickTo100Ns;
                    dTransferTime *= dSysTickTo100Ns;

                    pLCD->DiskTime = llTemp;
                    pLCD->DiskAvgQueueLength = llTemp;
                    lcdTotal.DiskAvgQueueLength += llTemp;
                    lcdTotal.DiskTime += llTemp;

                    pLCD->DiskReadTime = pDiskPerformance->ReadTime.QuadPart;
                    pLCD->DiskReadQueueLength = pDiskPerformance->ReadTime.QuadPart;
                    lcdTotal.DiskReadTime +=  pDiskPerformance->ReadTime.QuadPart;
                    lcdTotal.DiskReadQueueLength += pDiskPerformance->ReadTime.QuadPart;

                    pLCD->DiskWriteTime = pDiskPerformance->WriteTime.QuadPart;
                    pLCD->DiskWriteQueueLength = pDiskPerformance->WriteTime.QuadPart;

                    lcdTotal.DiskWriteTime += pDiskPerformance->WriteTime.QuadPart;
                    lcdTotal.DiskWriteQueueLength += pDiskPerformance->WriteTime.QuadPart;

                    pLCD->DiskAvgTime = (LONGLONG)dTransferTime;
                    lcdTotal.DiskAvgTime += (LONGLONG)dTransferTime;

                    dwTemp = pDiskPerformance->ReadCount +
                             pDiskPerformance->WriteCount;

                    lcdTotal.DiskTransfersBase1 += dwTemp;
                    pLCD->DiskTransfersBase1 = dwTemp;

                    lcdTotal.DiskAvgReadTime += (LONGLONG)dReadTime;
                    pLCD->DiskAvgReadTime = (LONGLONG)dReadTime;
                    lcdTotal.DiskReadsBase1 += pDiskPerformance->ReadCount;
                    pLCD->DiskReadsBase1 = pDiskPerformance->ReadCount;

                    lcdTotal.DiskAvgWriteTime += (LONGLONG)dWriteTime;
                    pLCD->DiskAvgWriteTime = (LONGLONG)dWriteTime;
                    lcdTotal.DiskWritesBase1 += pDiskPerformance->WriteCount;
                    pLCD->DiskWritesBase1 = pDiskPerformance->WriteCount;

                    lcdTotal.DiskTransfers += dwTemp;
                    pLCD->DiskTransfers = dwTemp;

                    lcdTotal.DiskReads += pDiskPerformance->ReadCount;
                    pLCD->DiskReads = pDiskPerformance->ReadCount;
                    lcdTotal.DiskWrites += pDiskPerformance->WriteCount;
                    pLCD->DiskWrites = pDiskPerformance->WriteCount;

                    llTemp = pDiskPerformance->BytesRead.QuadPart +
                             pDiskPerformance->BytesWritten.QuadPart;
                    lcdTotal.DiskBytes += llTemp;
                    pLCD->DiskBytes = llTemp;

                    lcdTotal.DiskReadBytes += pDiskPerformance->BytesRead.QuadPart;
                    pLCD->DiskReadBytes = pDiskPerformance->BytesRead.QuadPart;
                    lcdTotal.DiskWriteBytes += pDiskPerformance->BytesWritten.QuadPart;
                    pLCD->DiskWriteBytes = pDiskPerformance->BytesWritten.QuadPart;

                    lcdTotal.DiskAvgBytes += llTemp;
                    pLCD->DiskAvgBytes = llTemp;
                    lcdTotal.DiskTransfersBase2 += dwTemp;
                    pLCD->DiskTransfersBase2 = dwTemp;

                    lcdTotal.DiskAvgReadBytes += pDiskPerformance->BytesRead.QuadPart;
                    pLCD->DiskAvgReadBytes = pDiskPerformance->BytesRead.QuadPart;
                    lcdTotal.DiskReadsBase2 += pDiskPerformance->ReadCount;
                    pLCD->DiskReadsBase2 = pDiskPerformance->ReadCount;

                    lcdTotal.DiskAvgWriteBytes += pDiskPerformance->BytesWritten.QuadPart;
                    pLCD->DiskAvgWriteBytes = pDiskPerformance->BytesWritten.QuadPart;
                    lcdTotal.DiskWritesBase2 += pDiskPerformance->WriteCount;
                    pLCD->DiskWritesBase2 = pDiskPerformance->WriteCount;

                    pLCD->IdleTime = pDiskPerformance->IdleTime.QuadPart;
                    lcdTotal.IdleTime += pDiskPerformance->IdleTime.QuadPart;
                    pLCD->SplitCount = pDiskPerformance->SplitCount;
                    lcdTotal.SplitCount += pDiskPerformance->SplitCount;

                    pLCD->DiskTimeTimestamp = pDiskPerformance->QueryTime.QuadPart;
                    lcdTotal.DiskTimeTimestamp += pDiskPerformance->QueryTime.QuadPart;

                    dwStatus = ERROR_SUCCESS;
                    if (hVolume != NULL) {
                        dwStatus = NtQueryVolumeInformationFile(hVolume,
                                    &status_block,
                                    &FsSizeInformation,
                                    sizeof(FILE_FS_SIZE_INFORMATION),
                                    FileFsSizeInformation);
                    }

                    if ( hVolume && NT_SUCCESS(dwStatus) ) {
                        AllocationUnitBytes =
                            FsSizeInformation.BytesPerSector *
                            FsSizeInformation.SectorsPerAllocationUnit;
                        TotalBytes =  FsSizeInformation.TotalAllocationUnits.QuadPart *
                                                AllocationUnitBytes;

                        FreeBytes = FsSizeInformation.AvailableAllocationUnits.QuadPart *
                                                AllocationUnitBytes;

                        //  Express in megabytes, truncated

                        TotalBytes /= (1024*1024);
                        FreeBytes /= (1024*1024);

                        //  First two yield percentage of free space;
                        //  last is for raw count of free space in megabytes

                        lcdTotal.DiskFreeMbytes1 +=
                            pLCD->DiskFreeMbytes1 = (DWORD)FreeBytes;

                        lcdTotal.DiskTotalMbytes +=
                            pLCD->DiskTotalMbytes = (DWORD)TotalBytes;
                        lcdTotal.DiskFreeMbytes2 +=
                            pLCD->DiskFreeMbytes2 = (DWORD)FreeBytes;
                    } else {
                        if (!NT_SUCCESS (dwStatus)) {
                            if (!bShownDiskVolumeMessage) {
                                bShownDiskVolumeMessage = ReportEvent (hEventLog,
                                    EVENTLOG_WARNING_TYPE,
                                    0,
                                    PERFDISK_UNABLE_QUERY_VOLUME_INFO,
                                    NULL,
                                    0,
                                    sizeof(DWORD),
                                    NULL,
                                    (LPVOID)&dwStatus);
                            }
                        }
                        // Cannot get space information
                        pLCD->DiskFreeMbytes1 = 0;
                        pLCD->DiskTotalMbytes = 0;
                        pLCD->DiskFreeMbytes2 = 0;
                    }

                    // bump pointers in Perf Data Block
                    dwNumLogicalDisks++;
                    pLCD->CounterBlock.ByteLength = sizeof (LDISK_COUNTER_DATA);
                    pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)&pLCD[1];

                } else {
                    // this is a physical drive entry so skip it
#if _DBG_PRINT_INSTANCES
                    OutputDebugStringW ((LPCWSTR)L" (skipped)");
#endif
                }
                // count the number of items returned
                dwCurrentWmiObjCount++;
            } else {
                // 0 length name string so skip
            }
            // bump pointers inside WMI data block
            if (WmiDiskInfo->WnodeHeader.Linkage != 0) {
                // continue
                WmiDiskInfo = (PWNODE_ALL_DATA) (
                    (LPBYTE)WmiDiskInfo + WmiDiskInfo->WnodeHeader.Linkage);
            } else {
                // this is the end of the line
                bMoreEntries = FALSE;
            }

        } // end for each volume

        // see if number of WMI objects returned is different from
        // the last time the instance table was built, if so then 
        // remap the letters and redo the instances
        if (dwCurrentWmiObjCount != dwWmiDriveCount) {
            DebugPrint((1, "CollectLDisk: Remap Current %d Drive %d\n",
                dwCurrentWmiObjCount, dwWmiDriveCount));
            bRemapDriveLetters = TRUE;
            dwRemapCount--;
        }
    } while (bRemapDriveLetters && dwRemapCount);

    if (dwNumLogicalDisks > 0) {
        // see if there's room for the TOTAL entry....

        TotalLen =
            // space already used
            (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pLogicalDiskDataDefinition)
            // + estimate of this instance
            +   sizeof(PERF_INSTANCE_DEFINITION)
            +   (lstrlenW(wszTotal) + 1) * sizeof(WCHAR) ;
        TotalLen = QWORD_MULTIPLE (TotalLen);
        TotalLen += sizeof(LDISK_COUNTER_DATA);
        TotalLen = QWORD_MULTIPLE (TotalLen);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            dwReturn = ERROR_MORE_DATA;
        } else {
            // normalize the total times
            lcdTotal.DiskTime /= dwNumLogicalDisks;
            lcdTotal.DiskReadTime /= dwNumLogicalDisks;
            lcdTotal.DiskWriteTime /= dwNumLogicalDisks;
            lcdTotal.IdleTime /= dwNumLogicalDisks;
            lcdTotal.DiskTimeTimestamp /= dwNumLogicalDisks;

            MonBuildInstanceDefinition(
                pPerfInstanceDefinition,
                (PVOID *) &pLCD,
                0,
                0,
                (DWORD)-1,
                wszTotal);

            // update the total counters

            // insure quadword alignment of the data structure
            assert (((DWORD)(pLCD) & 0x00000007) == 0);
            memcpy (pLCD, &lcdTotal, sizeof (lcdTotal));
            pLCD->CounterBlock.ByteLength = sizeof(LDISK_COUNTER_DATA);

            // and update the "next byte" pointer
            pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)&pLCD[1];

            // update pointer to next available buffer...
            pLogicalDiskDataDefinition->DiskObjectType.NumInstances =
                dwNumLogicalDisks + 1; // add 1 for "Total" disk
        }
    } else {
        // there are  no instances so adjust the pointer for the 
        // rest of the code 
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pLogicalDiskDataDefinition[1];
    }

    if (dwReturn == ERROR_SUCCESS) {
        *lpcbTotalBytes =
            pLogicalDiskDataDefinition->DiskObjectType.TotalByteLength =
                (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pLogicalDiskDataDefinition);

#if DBG
        // sanity check on buffer size estimates
        if (*lpcbTotalBytes > TotalLen ) {
            DbgPrint ("\nPERFDISK: Logical Disk Perf Ctr. Instance Size Underestimated:");
            DbgPrint ("\nPERFDISK:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
        }
#endif

        *lppData = (LPVOID) pPerfInstanceDefinition;

        *lpNumObjectTypes = 1;

    }

    return dwReturn;
}

