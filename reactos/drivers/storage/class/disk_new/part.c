/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    disk.c

Abstract:

    SCSI disk class driver

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "disk.h"

#define PtCache ClassDebugExternal1

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, DiskReadPartitionTableEx)
#pragma alloc_text(PAGE, DiskWritePartitionTableEx)
#pragma alloc_text(PAGE, DiskSetPartitionInformationEx)
#endif

ULONG DiskBreakOnPtInval = FALSE;

//
// By default, 64-bit systems can see GPT disks and 32-bit systems
// cannot. This will likely change in the future.
//

#if defined(_WIN64)
ULONG DiskDisableGpt = FALSE;
#else
ULONG DiskDisableGpt = TRUE;
#endif

NTSTATUS
NTAPI
DiskReadPartitionTableEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN BOOLEAN BypassCache,
    OUT PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    )
/*++

Routine Description:

    This routine will return the current layout information for the disk.
    If the cached information is still valid then it will be returned, 
    otherwise the layout will be retrieved from the kernel and cached for 
    future use.
    
    This routine must be called with the partitioning lock held.  The 
    partition list which is returned is not guaranteed to remain valid 
    once the lock has been released.
    
Arguments:

    Fdo - a pointer to the FDO for the disk.
    
    DriveLayout - a location to store a pointer to the drive layout information.    

Return Value:

    STATUS_SUCCESS if successful or an error status indicating what failed.
    
--*/        
    
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;
    NTSTATUS status;
    PDRIVE_LAYOUT_INFORMATION_EX layoutEx;

    layoutEx = NULL;

    if(BypassCache) {
        diskData->CachedPartitionTableValid = FALSE;
        DebugPrint((PtCache, "DiskRPTEx: cache bypassed and invalidated for "
                             "FDO %#p\n", Fdo));
    }

    //
    // If the cached partition table is present then return a copy of it.
    //

    if (diskData->CachedPartitionTableValid != FALSE) {

        ULONG partitionNumber;
        PDRIVE_LAYOUT_INFORMATION_EX layout = diskData->CachedPartitionTable;

        //
        // Clear the partition numbers from the list entries
        //

        for(partitionNumber = 0;
            partitionNumber < layout->PartitionCount;
            partitionNumber++) {
            layout->PartitionEntry[partitionNumber].PartitionNumber = 0;
        }

        *DriveLayout = diskData->CachedPartitionTable;

        DebugPrint((PtCache, "DiskRPTEx: cached PT returned (%#p) for "
                             "FDO %#p\n", 
                    *DriveLayout, Fdo));
                    
        return STATUS_SUCCESS;
    }

    ASSERTMSG("DiskReadPartitionTableEx is not using cached partition table",
              (DiskBreakOnPtInval == FALSE));

    //
    // If there's a cached partition table still around then free it.
    //

    if(diskData->CachedPartitionTable) {
        DebugPrint((PtCache, "DiskRPTEx: cached PT (%#p) freed for FDO %#p\n",
                    diskData->CachedPartitionTable, Fdo));

        ExFreePool(diskData->CachedPartitionTable);
        diskData->CachedPartitionTable = NULL;
    }

    //
    // By default, X86 disables recognition of GPT disks. Instead we
    // return the protective MBR partition. Use IoReadPartitionTable
    // to get this.
    //
    
    status = IoReadPartitionTableEx(Fdo->DeviceObject, &layoutEx);

    if (DiskDisableGpt) {
        PDRIVE_LAYOUT_INFORMATION layout;

        if (NT_SUCCESS (status) &&
            layoutEx->PartitionStyle == PARTITION_STYLE_GPT) {

            //
            // ISSUE - 2000/29/08 - math: Remove from final product.
            // Leave this debug print in for a while until everybody
            // has had a chance to convert their GPT disks to MBR.
            //
            
            DbgPrint ("DISK: Disk %p recognized as a GPT disk on a system without GPT support.\n"
                      "      Disk will appear as RAW.\n",
                      Fdo->DeviceObject);

            ExFreePool (layoutEx);
            status = IoReadPartitionTable(Fdo->DeviceObject,
                                          Fdo->DiskGeometry.BytesPerSector,
                                          FALSE,
                                          &layout);
            if (NT_SUCCESS (status)) {
                layoutEx = DiskConvertLayoutToExtended(layout);
                ExFreePool (layout);
            }
        }
    }

    diskData->CachedPartitionTable = layoutEx;

    //
    // If the routine fails make sure we don't have a stale partition table 
    // pointer.  Otherwise indicate that the table is now valid.
    //

    if(!NT_SUCCESS(status)) {
        diskData->CachedPartitionTable = NULL;
    } else {
        diskData->CachedPartitionTableValid = TRUE;
    }

    *DriveLayout = diskData->CachedPartitionTable;

    DebugPrint((PtCache, "DiskRPTEx: returning PT %#p for FDO %#p with status "
                         "%#08lx.  PT is %scached\n",
                *DriveLayout,
                Fdo,
                status,
                (diskData->CachedPartitionTableValid ? "" : "not ")));


    return status;
}

NTSTATUS
NTAPI
DiskWritePartitionTableEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN PDRIVE_LAYOUT_INFORMATION_EX DriveLayout
    )
/*++

Routine Description:

    This routine will invalidate the cached partition table.  It will then 
    write the new drive layout to disk.  
    
Arguments:

    Fdo - the FDO for the disk getting the new partition table.
    
    DriveLayout - the new drive layout.
    
Return Value:
    
    status
    
--*/        
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;

    //
    // Invalidate the cached partition table.  Do not free it as it may be 
    // the very drive layout that was passed in to us.
    //

    diskData->CachedPartitionTableValid = FALSE;

    DebugPrint((PtCache, "DiskWPTEx: Invalidating PT cache for FDO %#p\n",
                Fdo));

    if (DiskDisableGpt) {
        if (DriveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
            return STATUS_NOT_SUPPORTED;
        }
    }
        
    return IoWritePartitionTableEx(Fdo->DeviceObject, DriveLayout);
}

NTSTATUS
NTAPI
DiskSetPartitionInformationEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN ULONG PartitionNumber,
    IN struct _SET_PARTITION_INFORMATION_EX* PartitionInfo
    )
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;

    diskData->CachedPartitionTableValid = FALSE;
    DebugPrint((PtCache, "DiskSPIEx: Invalidating PT cache for FDO %#p\n",
                Fdo));

    if (DiskDisableGpt) {
        if (PartitionInfo->PartitionStyle == PARTITION_STYLE_GPT) {
            return STATUS_NOT_SUPPORTED;
        }
    }
        
    return IoSetPartitionInformationEx(Fdo->DeviceObject, 
                                       PartitionNumber, 
                                       PartitionInfo);
}

NTSTATUS
NTAPI
DiskSetPartitionInformation(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    )
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;

    diskData->CachedPartitionTableValid = FALSE;
    DebugPrint((PtCache, "DiskSPI: Invalidating PT cache for FDO %#p\n",
                Fdo));

    return IoSetPartitionInformation(Fdo->DeviceObject, 
                                     SectorSize,
                                     PartitionNumber, 
                                     PartitionType);
}

BOOLEAN
NTAPI
DiskInvalidatePartitionTable(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN BOOLEAN PartitionLockHeld
    )
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;
    BOOLEAN wasValid;

    wasValid = (BOOLEAN) (diskData->CachedPartitionTableValid ? TRUE : FALSE);
    diskData->CachedPartitionTableValid = FALSE;

    DebugPrint((PtCache, "DiskIPT: Invalidating PT cache for FDO %#p\n",
                Fdo));

    if((PartitionLockHeld) && (diskData->CachedPartitionTable != NULL)) {
        DebugPrint((PtCache, "DiskIPT: Freeing PT cache (%#p) for FDO %#p\n",
                    diskData->CachedPartitionTable, Fdo));
        ExFreePool(diskData->CachedPartitionTable);
        diskData->CachedPartitionTable = NULL;
    }

    return wasValid;
}

NTSTATUS
NTAPI
DiskVerifyPartitionTable(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo,
    IN BOOLEAN FixErrors
    )
{
    PDISK_DATA diskData = Fdo->CommonExtension.DriverData;

    if(FixErrors) {
        diskData->CachedPartitionTableValid = FALSE;
        DebugPrint((PtCache, "DiskWPTEx: Invalidating PT cache for FDO %#p\n",
                    Fdo));
    }

    return IoVerifyPartitionTable(Fdo->DeviceObject, FixErrors);
}
