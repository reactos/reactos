/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    physdisk.c

Abstract:

    This file implements a Performance Object that presents
    Physical Disk Performance object data

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
#if _DBG_PRINT_INSTANCES
#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#endif
#include "diskmsg.h"
#include "dataphys.h"

DWORD APIENTRY
CollectPDiskObjectData(
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
    PPDISK_DATA_DEFINITION      pPhysicalDiskDataDefinition;
    DWORD  TotalLen;            //  Length of the total return block
    PDISK_COUNTER_DATA          pcdTotal;

    DWORD   dwStatus    = ERROR_SUCCESS;
    PPERF_INSTANCE_DEFINITION   pPerfInstanceDefinition = NULL;

    PWNODE_ALL_DATA WmiDiskInfo;
    DISK_PERFORMANCE            *pDiskPerformance;    //  Disk driver returns counters here

    PWCHAR  wszWmiInstanceName;
    WCHAR   wszInstanceName[MAX_PATH]; // the numbers shouldn't ever get this big
    DWORD   dwInstanceNameOffset;

    DWORD   dwNumPhysicalDisks = 0;

    PPDISK_COUNTER_DATA         pPCD;

    BOOL    bMoreEntries;

    LONGLONG    llTemp;
    DWORD       dwTemp;

    DWORD   dwReturn = ERROR_SUCCESS;
    WORD    wNameLength;

    BOOL    bSkip;

    DWORD       dwCurrentWmiObjCount = 0;
    DWORD       dwRemapCount = 10;

    DOUBLE      dReadTime, dWriteTime, dTransferTime;

    //
    //  Check for sufficient space for Physical Disk object
    //  type definition
    //

    do {
        dwNumPhysicalDisks = 0;
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

        pPhysicalDiskDataDefinition = (PDISK_DATA_DEFINITION *) *lppData;

        // clear the accumulator structure

        memset (&pcdTotal, 0, sizeof(pcdTotal));
        //
        //  Define Logical Disk data block
        //

        TotalLen = sizeof (PDISK_DATA_DEFINITION);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_MORE_DATA;
        }

        memmove(pPhysicalDiskDataDefinition,
               &PhysicalDiskDataDefinition,
               sizeof(PDISK_DATA_DEFINITION));

        // read the data from the diskperf driver

        if (dwStatus == ERROR_SUCCESS) {

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                        &pPhysicalDiskDataDefinition[1];

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
                    wszWmiInstanceName = (LPWSTR)((LPBYTE)WmiDiskInfo + dwInstanceNameOffset + sizeof(WORD));

                    if (IsPhysicalDrive(pDiskPerformance)) {
#if _DBG_PRINT_INSTANCES
                        WCHAR szOutputBuffer[512];
#endif  
                        // then the format is correct AND this is a physical
                        // partition so set the name string pointer and
                        // length for use in creating the instance.
                        memset (wszInstanceName, 0, sizeof(wszInstanceName));
                        GetPhysicalDriveNameString (
                            pDiskPerformance->StorageDeviceNumber,
                            pPhysDiskList,
                            dwNumPhysDiskListEntries,
                            wszInstanceName);
#if _DBG_PRINT_INSTANCES
                        swprintf (szOutputBuffer, (LPCWSTR)L"\nPERFDISK: [%d] PhysDrive [%8.8s,%d] is mapped as: ",
                            dwNumPhysDiskListEntries,
                            pDiskPerformance->StorageManagerName,
                            pDiskPerformance->StorageDeviceNumber);
                        OutputDebugStringW (szOutputBuffer);
                        OutputDebugStringW (wszInstanceName);
#endif  
                        bSkip = FALSE;
                    } else {
                        bSkip = TRUE;
                    }           
                
                    if (!bSkip) {
                        // first see if there's room for this entry....

                        TotalLen =
                            // space already used
                            (DWORD)((PCHAR) pPerfInstanceDefinition -
                            (PCHAR) pPhysicalDiskDataDefinition)
                            // + estimate of this instance
                            +   sizeof(PERF_INSTANCE_DEFINITION)
                            +   (lstrlenW(wszInstanceName) + 1) * sizeof(WCHAR) ;
                        TotalLen = QWORD_MULTIPLE (TotalLen);
                        TotalLen += sizeof(PDISK_COUNTER_DATA);
                        TotalLen = QWORD_MULTIPLE (TotalLen);

                        if ( *lpcbTotalBytes < TotalLen ) {
                            *lpcbTotalBytes = (DWORD) 0;
                            *lpNumObjectTypes = (DWORD) 0;
                            dwReturn = ERROR_MORE_DATA;
                            break;
                        }

                        MonBuildInstanceDefinition(
                            pPerfInstanceDefinition,
                            (PVOID *) &pPCD,
                            0, 0,   // no parent
                            (DWORD)-1,// no unique ID
                            wszInstanceName);

                        // clear counter data block
                        memset (pPCD, 0, sizeof(PDISK_COUNTER_DATA));
                        pPCD->CounterBlock.ByteLength = sizeof(PDISK_COUNTER_DATA);

//                      KdPrint (("PERFDISK: (P)   Entry %8.8x for: %ws\n", (DWORD)pPCD, wszWmiInstanceName));

                        // insure quadword alignment of the data structure
                        assert (((DWORD)(pPCD) & 0x00000007) == 0);

                        //  Set up pointer for data collection

                        // the QueueDepth counter is only a byte so clear the unused bytes
                        pDiskPerformance->QueueDepth &= 0x000000FF;

                        //
                        //  Format and collect Physical data
                        //
                        pcdTotal.DiskCurrentQueueLength += pDiskPerformance->QueueDepth;
                        pPCD->DiskCurrentQueueLength = pDiskPerformance->QueueDepth;

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

                        pPCD->DiskTime = llTemp;
                        pPCD->DiskAvgQueueLength = llTemp;
                        pcdTotal.DiskAvgQueueLength += llTemp;
                        pcdTotal.DiskTime += llTemp;

                        pPCD->DiskReadTime = pDiskPerformance->ReadTime.QuadPart;
                        pPCD->DiskReadQueueLength = pDiskPerformance->ReadTime.QuadPart;
                        pcdTotal.DiskReadTime +=  pDiskPerformance->ReadTime.QuadPart;
                        pcdTotal.DiskReadQueueLength += pDiskPerformance->ReadTime.QuadPart;

                        pPCD->DiskWriteTime = pDiskPerformance->WriteTime.QuadPart;
                        pPCD->DiskWriteQueueLength = pDiskPerformance->WriteTime.QuadPart;

                        pcdTotal.DiskWriteTime += pDiskPerformance->WriteTime.QuadPart;
                        pcdTotal.DiskWriteQueueLength += pDiskPerformance->WriteTime.QuadPart;

                        pPCD->DiskAvgTime = (LONGLONG)dTransferTime;
                        pcdTotal.DiskAvgTime += (LONGLONG)dTransferTime;

                        dwTemp = pDiskPerformance->ReadCount +
                                 pDiskPerformance->WriteCount;

                        pcdTotal.DiskTransfersBase1 += dwTemp;
                        pPCD->DiskTransfersBase1 = dwTemp;

                        pcdTotal.DiskAvgReadTime += (LONGLONG)dReadTime;
                        pPCD->DiskAvgReadTime = (LONGLONG)dReadTime;
                        pcdTotal.DiskReadsBase1 += pDiskPerformance->ReadCount;
                        pPCD->DiskReadsBase1 = pDiskPerformance->ReadCount;

                        pcdTotal.DiskAvgWriteTime += (LONGLONG)dWriteTime;
                        pPCD->DiskAvgWriteTime = (LONGLONG)dWriteTime;
                        pcdTotal.DiskWritesBase1 += pDiskPerformance->WriteCount;
                        pPCD->DiskWritesBase1 = pDiskPerformance->WriteCount;

                        pcdTotal.DiskTransfers += dwTemp;
                        pPCD->DiskTransfers = dwTemp;

                        pcdTotal.DiskReads += pDiskPerformance->ReadCount;
                        pPCD->DiskReads = pDiskPerformance->ReadCount;
                        pcdTotal.DiskWrites += pDiskPerformance->WriteCount;
                        pPCD->DiskWrites = pDiskPerformance->WriteCount;

                        llTemp = pDiskPerformance->BytesRead.QuadPart +
                                 pDiskPerformance->BytesWritten.QuadPart;
                        pcdTotal.DiskBytes += llTemp;
                        pPCD->DiskBytes = llTemp;

                        pcdTotal.DiskReadBytes += pDiskPerformance->BytesRead.QuadPart;
                        pPCD->DiskReadBytes = pDiskPerformance->BytesRead.QuadPart;
                        pcdTotal.DiskWriteBytes += pDiskPerformance->BytesWritten.QuadPart;
                        pPCD->DiskWriteBytes = pDiskPerformance->BytesWritten.QuadPart;

                        pcdTotal.DiskAvgBytes += llTemp;
                        pPCD->DiskAvgBytes = llTemp;
                        pcdTotal.DiskTransfersBase2 += dwTemp;
                        pPCD->DiskTransfersBase2 = dwTemp;

                        pcdTotal.DiskAvgReadBytes += pDiskPerformance->BytesRead.QuadPart;
                        pPCD->DiskAvgReadBytes = pDiskPerformance->BytesRead.QuadPart;
                        pcdTotal.DiskReadsBase2 += pDiskPerformance->ReadCount;
                        pPCD->DiskReadsBase2 = pDiskPerformance->ReadCount;

                        pcdTotal.DiskAvgWriteBytes += pDiskPerformance->BytesWritten.QuadPart;
                        pPCD->DiskAvgWriteBytes = pDiskPerformance->BytesWritten.QuadPart;
                        pcdTotal.DiskWritesBase2 += pDiskPerformance->WriteCount;
                        pPCD->DiskWritesBase2 = pDiskPerformance->WriteCount;

                        pPCD->IdleTime = pDiskPerformance->IdleTime.QuadPart;
                        pcdTotal.IdleTime += pDiskPerformance->IdleTime.QuadPart;
                        pPCD->SplitCount = pDiskPerformance->SplitCount;
                        pcdTotal.SplitCount += pDiskPerformance->SplitCount;

                        pPCD->DiskTimeTimeStamp = pDiskPerformance->QueryTime.QuadPart;
                        pcdTotal.DiskTimeTimeStamp += pDiskPerformance->QueryTime.QuadPart;

                        // move to the end of the buffer for the next instance
                        pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)&pPCD[1];
                        dwNumPhysicalDisks++;

                    } else {
//                      KdPrint (("PERFDISK: (P) Skipping Instance: %ws\n", wszWmiInstanceName));
                    }
                    // count the number of items returned by WMI
                    dwCurrentWmiObjCount++;
                } else {
                    // the name has 0 length so skip
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
                DebugPrint((1, "CollectPDisk: Remap Current %d Drive %d\n",
                    dwCurrentWmiObjCount, dwWmiDriveCount));
                bRemapDriveLetters = TRUE;
                dwRemapCount--;
            }
        } // end if mem init was successful
    } while (bRemapDriveLetters && dwRemapCount);


    if ((dwNumPhysicalDisks > 0) && (dwStatus == ERROR_SUCCESS)) {
        // see if there's room for this entry....

        TotalLen =
            // space already used
            (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pPhysicalDiskDataDefinition)
            // + estimate of this instance
            +   sizeof(PERF_INSTANCE_DEFINITION)
            +   (lstrlenW(wszTotal) + 1) * sizeof(WCHAR) ;
        TotalLen = QWORD_MULTIPLE (TotalLen);
        TotalLen += sizeof(PDISK_COUNTER_DATA);
        TotalLen = QWORD_MULTIPLE (TotalLen);

        if ( *lpcbTotalBytes < TotalLen ) {
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            dwReturn = ERROR_MORE_DATA;
        } else {
            // normalize the total times
            pcdTotal.DiskTime /= dwNumPhysicalDisks;
            pcdTotal.DiskReadTime /= dwNumPhysicalDisks;
            pcdTotal.DiskWriteTime /= dwNumPhysicalDisks;
            pcdTotal.IdleTime /= dwNumPhysicalDisks;
            pcdTotal.DiskTimeTimeStamp /= dwNumPhysicalDisks;

            MonBuildInstanceDefinition(
                pPerfInstanceDefinition,
                (PVOID *) &pPCD,
                0,
                0,
                (DWORD)-1,
                wszTotal);

            // update the total counters

            // insure quadword alignment of the data structure
            assert (((DWORD)(pPCD) & 0x00000007) == 0);
            memcpy (pPCD, &pcdTotal, sizeof (pcdTotal));
            pPCD->CounterBlock.ByteLength = sizeof(PDISK_COUNTER_DATA);

            // and update the "next byte" pointer
            pPerfInstanceDefinition = (PPERF_INSTANCE_DEFINITION)&pPCD[1];

            // update pointer to next available buffer...
            pPhysicalDiskDataDefinition->DiskObjectType.NumInstances =
                dwNumPhysicalDisks + 1; // add 1 for "Total" disk
        }
    } else {
        //  If we are diskless, then return no instances
        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pPhysicalDiskDataDefinition[1];
        pPhysicalDiskDataDefinition->DiskObjectType.NumInstances = 0;
    }

    if (dwReturn == ERROR_SUCCESS) {
        *lpcbTotalBytes =
            pPhysicalDiskDataDefinition->DiskObjectType.TotalByteLength =
                (DWORD)((PCHAR) pPerfInstanceDefinition -
                (PCHAR) pPhysicalDiskDataDefinition);

#if DBG
        // sanity check on buffer size estimates
        if (*lpcbTotalBytes > TotalLen ) {
            DbgPrint ("\nPERFDISK: Physical Disk Perf Ctr. Instance Size Underestimated:");
            DbgPrint ("\nPERFDISK:   Estimated size: %d, Actual Size: %d", TotalLen, *lpcbTotalBytes);
        }
#endif

        *lppData = (LPVOID) pPerfInstanceDefinition;

        *lpNumObjectTypes = 1;
    }

    return dwReturn;
}
