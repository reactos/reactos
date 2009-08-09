/*
 * PROJECT:         Ramdisk Class Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/storage/class/ramdisk/ramdisk.c
 * PURPOSE:         Main Driver Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <initguid.h>
#include <ntddk.h>
#include <ntdddisk.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>
#include <reactos/drivers/ntddrdsk.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef enum _RAMDISK_DEVICE_TYPE
{
    RamdiskFdo,
    RamdiskPdo
} RAMDISK_DEVICE_TYPE;

typedef enum _RAMDISK_DEVICE_STATE
{
    RamdiskStateUninitialized,
    RamdiskStateStarted,
    RamdiskStatePaused,
    RamdiskStateStopped,
    RamdiskStateRemoved,
    RamdiskStateBusRemoved,
} RAMDISK_DEVICE_STATE;

DEFINE_GUID(RamdiskBusInterface,
		    0x5DC52DF0,
			0x2F8A,
			0x410F,
			0x80, 0xE4, 0x05, 0xF8, 0x10, 0xE7, 0xA8, 0x8A);

typedef struct _RAMDISK_EXTENSION
{
    RAMDISK_DEVICE_TYPE Type;
    RAMDISK_DEVICE_STATE State;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    IO_REMOVE_LOCK RemoveLock;
    UNICODE_STRING SymbolicLinkName;
    FAST_MUTEX DiskListLock;
    LIST_ENTRY DiskListHead;
} RAMDISK_EXTENSION, *PRAMDISK_EXTENSION;

ULONG MaximumViewLength;
ULONG MaximumPerDiskViewLength;
ULONG ReportDetectedDevice;
ULONG MarkRamdisksAsRemovable;
ULONG MinimumViewCount;
ULONG DefaultViewCount;
ULONG MaximumViewCount;
ULONG MinimumViewLength;
ULONG DefaultViewLength;
UNICODE_STRING DriverRegistryPath;
BOOLEAN ExportBootDiskAsCd;
BOOLEAN IsWinPEBoot;
PDEVICE_OBJECT RamdiskBusFdo;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
QueryParameters(IN PUNICODE_STRING RegistryPath)
{
    ULONG MinView, DefView, MinViewLength, DefViewLength, MaxViewLength;
    RTL_QUERY_REGISTRY_TABLE QueryTable[10];
    
    //
    // Set defaults
    //
    MaximumViewLength = 0x10000000u;
    MaximumPerDiskViewLength = 0x10000000u;
    ReportDetectedDevice = 0;
    MarkRamdisksAsRemovable = 0;
    MinimumViewCount = 2;
    DefaultViewCount = 16;
    MaximumViewCount = 64;
    MinimumViewLength = 0x10000u;
    DefaultViewLength = 0x100000u;
    
    //
    // Setup the query table and query the registry
    //
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = 1;
    QueryTable[0].Name = L"Parameters";
    QueryTable[1].Flags = 32;
    QueryTable[1].Name = L"ReportDetectedDevice";
    QueryTable[1].EntryContext = &ReportDetectedDevice;
    QueryTable[2].Flags = 32;
    QueryTable[2].Name = L"MarkRamdisksAsRemovable";
    QueryTable[2].EntryContext = &MarkRamdisksAsRemovable;
    QueryTable[3].Flags = 32;
    QueryTable[3].Name = L"MinimumViewCount";
    QueryTable[3].EntryContext = &MinimumViewCount;
    QueryTable[4].Flags = 32;
    QueryTable[4].Name = L"DefaultViewCount";
    QueryTable[4].EntryContext = &DefaultViewCount;
    QueryTable[5].Flags = 32;
    QueryTable[5].Name = L"MaximumViewCount";
    QueryTable[5].EntryContext = &MaximumViewCount;
    QueryTable[6].Flags = 32;
    QueryTable[6].Name = L"MinimumViewLength";
    QueryTable[6].EntryContext = &MinimumViewLength;
    QueryTable[7].Flags = 32;
    QueryTable[7].Name = L"DefaultViewLength";
    QueryTable[7].EntryContext = &DefaultViewLength;
    QueryTable[8].Flags = 32;
    QueryTable[8].Name = L"MaximumViewLength";
    QueryTable[8].EntryContext = &MaximumViewLength;
    QueryTable[9].Flags = 32;
    QueryTable[9].Name = L"MaximumPerDiskViewLength";
    QueryTable[9].EntryContext = &MaximumPerDiskViewLength;
    RtlQueryRegistryValues(RTL_REGISTRY_OPTIONAL,
                           RegistryPath->Buffer,
                           QueryTable,
                           NULL,
                           NULL);
    
    //
    // Parse minimum view count, cannot be bigger than 256 or smaller than 2
    //
    MinView = MinimumViewCount;
    if (MinimumViewCount >= 2)
    {
        if (MinimumViewCount > 256) MinView = 256;
    }
    else
    {
        MinView = 2;
    }
    MinimumViewCount = MinView;

    //
    // Parse default view count, cannot be bigger than 256 or smaller than minimum
    //
    DefView = DefaultViewCount;
    if (DefaultViewCount >= MinView)
    {
        if (DefaultViewCount > 256) DefView = 256;
    }
    else
    {
        DefView = MinView;
    }
    DefaultViewCount = DefView;
    
    //
    // Parse maximum view count, cannot be bigger than 256 or smaller than default
    //
    if (MaximumViewCount >= DefView)
    {
        if (MaximumViewCount > 256) MaximumViewCount = 256;
    }
    else
    {
        MaximumViewCount = DefView;
    }
    
    //
    // Parse minimum view length, cannot be bigger than 1GB or smaller than 64KB
    //
    MinViewLength = MinimumViewLength;
    if (MinimumViewLength >= 0x10000)
    {
        if (MinimumViewLength > 0x40000000) MinViewLength = 0x40000000u;
    }
    else
    {
        MinViewLength = 0x10000u;
    }
    MinimumViewLength = MinViewLength;

    //
    // Parse default view length, cannot be bigger than 1GB or smaller than minimum
    //
    DefViewLength = DefaultViewLength;
    if (DefaultViewLength >= MinViewLength)
    {
        if (DefaultViewLength > 0x40000000) DefViewLength = 0x40000000u;
    }
    else
    {
        DefViewLength = MinViewLength;
    }
    DefaultViewLength = DefViewLength;

    //
    // Parse maximum view length, cannot be bigger than 1GB or smaller than default
    //
    MaxViewLength = MaximumViewLength;
    if (MaximumViewLength >= DefViewLength)
    {
        if (MaximumViewLength > 0x40000000) MaxViewLength = 0x40000000u;
    }
    else
    {
        MaxViewLength = DefViewLength;
    }
    MaximumViewLength = MaxViewLength;

    //
    // Parse maximum view length per disk, cannot be smaller than 16MB
    //
    if (MaximumPerDiskViewLength >= 0x1000000)
    {
        if (MaxViewLength > 0xFFFFFFFF) MaximumPerDiskViewLength = -1;
    }
    else
    {
        MaximumPerDiskViewLength = 0x1000000u;
    }
}

NTSTATUS
NTAPI
RamdiskOpenClose(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    //
    // Complete the IRP
    //
    Irp->IoStatus.Information = 1;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskCreateDiskDevice(IN PRAMDISK_EXTENSION DeviceExtension,
						IN PRAMDISK_CREATE_INPUT Input,
						IN BOOLEAN ValidateOnly,
						OUT PDEVICE_OBJECT *DeviceObject)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskCreateRamdisk(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
					 IN BOOLEAN ValidateOnly)
{
	PRAMDISK_CREATE_INPUT Input;
	ULONG Length;
	PRAMDISK_EXTENSION DeviceExtension; 
	ULONG DiskType;
	PWCHAR FileNameStart, FileNameEnd;
	NTSTATUS Status;
	PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
	
	//
	// Get the device extension and our input data
	//
	DeviceExtension = (PRAMDISK_EXTENSION)DeviceObject->DeviceExtension;
	Length = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	Input = (PRAMDISK_CREATE_INPUT)Irp->AssociatedIrp.SystemBuffer;
	
	//
	// Validate input parameters
	//
	if ((Length < sizeof(RAMDISK_CREATE_INPUT)) ||
		(Input->Version != sizeof(RAMDISK_CREATE_INPUT)))
	{
		//
		// Invalid input
		//
		return STATUS_INVALID_PARAMETER;
	}
	
	//
	// Validate the disk type
	//
	DiskType = Input->DiskType;
	if (DiskType == FILE_DEVICE_CONTROLLER) return STATUS_INVALID_PARAMETER;
	
	//
	// Look at the disk type
	//
	if (DiskType == FILE_DEVICE_CD_ROM_FILE_SYSTEM)
	{
		//
		// We only allow this as an early-init boot
		//
		if (!KeLoaderBlock) return STATUS_INVALID_PARAMETER;
		
		//
		// Save command-line flags
		//
		if (ExportBootDiskAsCd) Input->Options.ExportAsCd = TRUE;
		if (IsWinPEBoot) Input->Options.NoDriveLetter = TRUE;
	}
    
	//
	// Validate the disk type
	//
	if ((Input->Options.ExportAsCd) && (DiskType != FILE_DEVICE_CD_ROM_FILE_SYSTEM))
	{
		//
		// If the type isn't CDFS, it has to at least be raw CD
		//
		if (DiskType != FILE_DEVICE_CD_ROM) return STATUS_INVALID_PARAMETER;
	}
	
	//
	// Check if this is an actual file
	//
	if (DiskType <= FILE_DEVICE_CD_ROM)
	{
		//
		// Validate the file name
		//
		FileNameStart = (PWCHAR)((ULONG_PTR)Input + Length);
		FileNameEnd = Input->FileName + 1;
		while ((FileNameEnd < FileNameStart) && *(FileNameEnd)) FileNameEnd++;
		if (FileNameEnd == FileNameStart) return STATUS_INVALID_PARAMETER;
	}
    
	//
	// Create the actual device
	//
	Status = RamdiskCreateDiskDevice(DeviceExtension,
									 Input, 
									 ValidateOnly,
									 &DeviceObject);
	if (NT_SUCCESS(Status))
	{
		//
		// Invalidate and set success
		//
		IoInvalidateDeviceRelations(DeviceExtension->PhysicalDeviceObject, 0);
		Irp->IoStatus.Information = STATUS_SUCCESS;
	}
	
	//
	// We're done
	//
	return Status;
}

NTSTATUS
NTAPI
SendIrpToThread(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIO_WORKITEM WorkItem;
    
    //
    // Mark the IRP pending
    //
    IoMarkIrpPending(Irp);
    
    //
    // Allocate a work item
    //
    WorkItem = IoAllocateWorkItem(DeviceObject);
    if (WorkItem)
    {
        //
        // Queue it up
        //
        Irp->Tail.Overlay.DriverContext[0] = WorkItem;
        IoQueueWorkItem(WorkItem, RamdiskWorkerThread, DelayedWorkQueue, Irp);
        return STATUS_PENDING;
    }
    else
    {
        //
        // Fail
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
NTAPI
RamdiskReadWriteReal(IN PIRP Irp,
                     IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    PMDL Mdl;
    PVOID CurrentBase, SystemVa, BaseAddress;
    PIO_STACK_LOCATION IoStackLocation;
    LARGE_INTEGER CurrentOffset;
    ULONG BytesRead, BytesLeft, CopyLength;
    PVOID Source, Destination;
    NTSTATUS Status;
    
    //
    // Get the MDL and check if it's mapped
    //
    Mdl = Irp->MdlAddress;
    if (Mdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))
    {
        //
        // Use the mapped address
        //
        SystemVa = Mdl->MappedSystemVa;
    }
    else
    {
        //
        // Map it ourselves
        //
        SystemVa = MmMapLockedPagesSpecifyCache(Mdl,
                                                0,
                                                MmCached,
                                                NULL,
                                                0, 
                                                NormalPagePriority);
    }
    
    //
    // Make sure we were able to map it
    //
    CurrentBase = SystemVa;
    if (!SystemVa) return STATUS_INSUFFICIENT_RESOURCES;
    
    //
    // Initialize default
    //
    Irp->IoStatus.Information = 0;
    
    //
    // Get the I/O Stack Location and capture the data
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    CurrentOffset = IoStackLocation->Parameters.Read.ByteOffset;
    BytesLeft = IoStackLocation->Parameters.Read.Length;
    if (!BytesLeft) return STATUS_INVALID_PARAMETER;
    
    //
    // Do the copy loop
    //
    while (TRUE)
    {
        //
        // Map the pages
        //
        BaseAddress = RamdiskMapPages(DeviceExtension,
                                      CurrentOffset,
                                      BytesLeft,
                                      &BytesRead);
        if (!BaseAddress) return STATUS_INSUFFICIENT_RESOURCES;
        
        //
        // Update our lengths
        //
        Irp->IoStatus.Information += BytesRead;
        CopyLength = BytesRead;
        
        //
        // Check if this was a read or write
        //
        Status = STATUS_SUCCESS;
        if (IoStackLocation->MajorFunction == IRP_MJ_READ)
        {
            //
            // Set our copy parameters
            //
            Destination = CurrentBase;
            Source = BaseAddress;
            goto DoCopy;
        }
        else if (IoStackLocation->MajorFunction == IRP_MJ_WRITE)
        {
            //
            // Set our copy parameters
            //
            Destination = BaseAddress;
            Source = CurrentBase;
DoCopy:
            //
            // Copy the data
            //
            RtlCopyMemory(Destination, Source, CopyLength);
        }
        else
        {
            //
            // Prepare us for failure
            //
            BytesLeft = CopyLength;
            Status = STATUS_INVALID_PARAMETER;
        }
        
        //
        // Unmap the pages
        //
        RamdiskUnmapPages(DeviceExtension,
                          BaseAddress,
                          CurrentOffset,
                          BytesRead);
        
        //
        // Update offset and bytes left
        //
        BytesLeft -= BytesRead;
        CurrentOffset.QuadPart += BytesRead;
        CurrentBase = (PVOID)((ULONG_PTR)CurrentBase + BytesRead);
        
        //
        // Check if we're done
        //
        if (!BytesLeft) return Status;
    }
}

NTSTATUS
NTAPI
RamdiskGetPartitionInfo(IN PIRP Irp,
                        IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PPARTITION_INFORMATION PartitionInfo;
    PVOID BaseAddress;
    LARGE_INTEGER Zero = {{0, 0}};
    ULONG Length;
    PIO_STACK_LOCATION IoStackLocation;
    
    //
    // Validate the length
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.
        OutputBufferLength < sizeof(PARTITION_INFORMATION))
    {
        //
        // Invalid length
        //
        Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        return Status;
    }
    
    //
    // Map the partition table
    //
    BaseAddress = RamdiskMapPages(DeviceExtension, Zero, PAGE_SIZE, &Length);
    if (!BaseAddress)
    {
        //
        // No memory
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        return Status;
    }
    
    //
    // Fill out the information
    //
    PartitionInfo = Irp->AssociatedIrp.SystemBuffer;
    PartitionInfo->StartingOffset.QuadPart = DeviceExtension->BytesPerSector;
    PartitionInfo->PartitionLength.QuadPart = DeviceExtension->BytesPerSector *
                                              DeviceExtension->SectorsPerTrack *
                                              DeviceExtension->NumberOfHeads *
                                              DeviceExtension->Cylinders;
    PartitionInfo->HiddenSectors = DeviceExtension->HiddenSectors;
    PartitionInfo->PartitionNumber = 0;
    PartitionInfo->PartitionType = PARTITION_FAT32; //*((PCHAR)BaseAddress + 450);
    PartitionInfo->BootIndicator = (DeviceExtension->DiskType ==
                                    RAMDISK_BOOT_DISK) ? TRUE: FALSE;
    PartitionInfo->RecognizedPartition = IsRecognizedPartition(PartitionInfo->
                                                               PartitionType);
    PartitionInfo->RewritePartition = FALSE;

    //
    // Unmap the partition table
    //
    RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, Length);
    
    //
    // Done
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskOpenClose(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    //
    // Complete the IRP
    //
    Irp->IoStatus.Information = 1;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;
    ULONG Length;
    LARGE_INTEGER ByteOffset;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status, ReturnStatus;
    
    //
    // Get the device extension and make sure this isn't a bus
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Type == RamdiskBus)
    {
        //
        // Fail
        //
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }
    
    //
    // Capture parameters
    //
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Length = IoStackLocation->Parameters.Read.Length;
    ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
    
    //
    // FIXME: Validate offset
    //
    
    //
    // FIXME: Validate sector
    //
    
    //
    // Validate write
    //
    if ((IoStackLocation->MajorFunction == IRP_MJ_WRITE) &&
        (DeviceExtension->DiskOptions.Readonly))
    {
        //
        // Fail, this is read-only
        //
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto Complete;
    }
    
    //
    // See if we want to do this sync or async
    //
    if (DeviceExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
    {
        //
        // Do it sync
        //
        Status = RamdiskReadWriteReal(Irp, DeviceExtension);
        goto Complete;
    }
    
    //
    // Queue it to the worker
    //
    Status = SendIrpToThread(DeviceObject, Irp);
    ReturnStatus = STATUS_PENDING;
    
    //
    // Check if we're pending or not
    //
    if (Status != STATUS_PENDING)
    {
Complete:
        //
        // Complete the IRP
        //
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        ReturnStatus = Status;
    }
    
    //
    // Return to caller
    //
    return ReturnStatus;
}

NTSTATUS
NTAPI
RamdiskDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PRAMDISK_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    ULONG Information = 0;
    
    //
    // Grab the remove lock
    //
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail the IRP
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
    
    //
    // Check if this is an FDO or PDO
    //
    if (DeviceExtension->Type == RamdiskFdo)
    {
        //
        // Check what the request is
        //
        switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
        {
            //
            // Request to create a ramdisk
            //
            case FSCTL_CREATE_RAM_DISK:
                
                //
                // Do it
                //
                Status = RamdiskCreateRamdisk(DeviceObject, Irp, TRUE);
                if (!NT_SUCCESS(Status)) goto CompleteRequest;
                break;
                
            default:
                
                //
                // We don't handle anything else yet
                //
                ASSERT(FALSE);
                while (TRUE);
        }
    }
    else
    {
        //
        // PDO code not yet done
        //
        ASSERT(FALSE);
    }
    
    //
    // Queue the request to our worker thread
    //
    UNIMPLEMENTED;
    while (TRUE);
    
CompleteRequest:
    //
    // Release the lock
    //
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (Status != STATUS_PENDING)
    {
        //
        // Complete the request
        //
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = Information;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    //
    // Return status
    //
    return Status;
}

NTSTATUS
NTAPI
RamdiskPnp(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PRAMDISK_EXTENSION DeviceExtension;
    NTSTATUS Status;
    UCHAR Minor;
    
    //
    // Get the device extension and stack location
    //
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Minor = IoStackLocation->MinorFunction;
    
    //
    // Check if the bus is removed
    //
    if (DeviceExtension->State == RamdiskStateBusRemoved)
    {
        //
        // Only remove-device and query-id are allowed
        //
        if ((Minor != IRP_MN_REMOVE_DEVICE) || (Minor != IRP_MN_QUERY_ID))
        {
            //
            // Fail anything else
            //
            Status = STATUS_NO_SUCH_DEVICE;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }
    
    //
    // Acquire the remove lock
    //
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // Fail the IRP
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
    
    //
    // Query the IRP type
    //
    switch (Minor)
    {
        case IRP_MN_START_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_QUERY_STOP_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_CANCEL_STOP_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_STOP_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_QUERY_REMOVE_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_REMOVE_DEVICE:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_QUERY_ID:
            
            //
            // Are we a PDO?
            //
            if (DeviceExtension->Type == RamdiskPdo)
            {
                DPRINT1("PnP IRP: %lx\n", Minor);
                while (TRUE);
            }
            break;
            
        case IRP_MN_QUERY_BUS_INFORMATION:
            
            //
            // Are we a PDO?
            //
            if (DeviceExtension->Type == RamdiskPdo)
            {
                DPRINT1("PnP IRP: %lx\n", Minor);
                while (TRUE);
            }
            break;
            
        case IRP_MN_EJECT:
            
            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);
            break;
            
        case IRP_MN_QUERY_DEVICE_TEXT:
            
            //
            // Are we a PDO?
            //
            if (DeviceExtension->Type == RamdiskPdo)
            {
                DPRINT1("PnP IRP: %lx\n", Minor);
                while (TRUE);
            }
            break;
            
        case IRP_MN_QUERY_DEVICE_RELATIONS:

            DPRINT1("PnP IRP: %lx\n", Minor);
            while (TRUE);            
            break;
            
        case IRP_MN_QUERY_CAPABILITIES:
            
            //
            // Are we a PDO?
            //
            if (DeviceExtension->Type == RamdiskPdo)
            {
                DPRINT1("PnP IRP: %lx\n", Minor);
                while (TRUE);
            }
            break;
            
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            
            //
            // Complete immediately without touching it
            //
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto ReleaseAndReturn;
            
        default:
            
            DPRINT1("Illegal IRP: %lx\n", Minor);
            break;
    }
    
    //
    // Are we an FDO?
    //
    if (DeviceExtension->Type == RamdiskFdo)
    {
        //
        // Do we have an attached device?
        //
        if (DeviceExtension->AttachedDevice)
        {
            //
            // Forward the IRP
            //
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
    }
    
    //
    // Release the lock and return status
    //
ReleaseAndReturn:
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    return Status;
}

NTSTATUS
NTAPI
RamdiskPower(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskSystemControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskScsi(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
RamdiskUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
RamdiskAddDevice(IN PDRIVER_OBJECT DriverObject, 
                 IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	PRAMDISK_EXTENSION DeviceExtension;
	PDEVICE_OBJECT AttachedDevice;
	NTSTATUS Status; 
	UNICODE_STRING DeviceName;
	PDEVICE_OBJECT DeviceObject;
	
	//
	// Only create the FDO once
	//
	if (RamdiskBusFdo) return STATUS_DEVICE_ALREADY_ATTACHED;
	
	//
	// Create the FDO
	//
	RtlInitUnicodeString(&DeviceName, L"\\Device\\Ramdisk");
	Status = IoCreateDevice(DriverObject,
						    sizeof(RAMDISK_EXTENSION),
							&DeviceName,
							FILE_DEVICE_BUS_EXTENDER,
							FILE_DEVICE_SECURE_OPEN,
							0,
							&DeviceObject);
	if (NT_SUCCESS(Status))
	{
		//
		// Initialize the FDO extension
		//
	    DeviceExtension = (PRAMDISK_EXTENSION)DeviceObject->DeviceExtension;
	    RtlZeroMemory(DeviceObject->DeviceExtension, sizeof(RAMDISK_EXTENSION));

		//
		// Set FDO flags
		//
	    DeviceObject->Flags |= DO_POWER_PAGABLE | DO_DIRECT_IO;

		//
		// Setup the FDO extension
		//
	    DeviceExtension->Type = RamdiskFdo;
		ExInitializeFastMutex(&DeviceExtension->DiskListLock);
	    IoInitializeRemoveLock(&DeviceExtension->RemoveLock,
                               TAG('R', 'a', 'm', 'd'),
                               0,
                               1);
		InitializeListHead(&DeviceExtension->DiskListHead);
	    DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
	    DeviceExtension->DeviceObject = DeviceObject;

		//
		// Register the RAM disk device interface
		//
	    Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
										   &RamdiskBusInterface,
										   NULL,
										   &DeviceExtension->SymbolicLinkName);
	    if (!NT_SUCCESS(Status))
	    {
			//
			// Fail
			//
			IoDeleteDevice(DeviceObject);
			return Status;
	    }

		//
		// Attach us to the device stack
		//
	    AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
													 PhysicalDeviceObject);
	    DeviceExtension->AttachedDevice = AttachedDevice;
	    if (!AttachedDevice)
	    {
			//
			// Fail
			//
			IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, 0);
			RtlFreeUnicodeString(&DeviceExtension->SymbolicLinkName);
			IoDeleteDevice(DeviceObject);
			return STATUS_NO_SUCH_DEVICE;
	    }

		//
		// FDO is initialized
		//
	    RamdiskBusFdo = DeviceObject;

		//
		// Loop for loader block
		//
	    if (KeLoaderBlock)
	    {
			//
			// Are we being booted from setup? Not yet supported
			//
			//ASSERT(!KeLoaderBlock->SetupLdrBlock);
	    }

		//
		// All done
		//
	    DeviceObject->Flags &= DO_DEVICE_INITIALIZING;
	    Status = STATUS_SUCCESS;
	}

	//
	// Return status
	//
	return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PCHAR BootDeviceName, CommandLine;
    PDEVICE_OBJECT PhysicalDeviceObject = NULL;
    NTSTATUS Status;
    DPRINT1("RAM Disk Driver Initialized\n");
    
    //
    // Query ramdisk parameters
    //
    QueryParameters(RegistryPath);
    
    //
    // Save the registry path
    //
    DriverRegistryPath = *RegistryPath;
    DriverRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      RegistryPath->Length +
                                                      sizeof(WCHAR),
                                                      TAG('R', 'a', 'm', 'd'));
    if (!DriverRegistryPath.Buffer) return STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyUnicodeString(&DriverRegistryPath, RegistryPath);
    
    //
    // Set device routines
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = RamdiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RamdiskPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RamdiskSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = RamdiskScsi;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = RamdiskFlushBuffers;
    DriverObject->DriverExtension->AddDevice = RamdiskAddDevice;
    DriverObject->DriverUnload = RamdiskUnload;
    
    //
    // Check for a loader block
    //
    if (KeLoaderBlock)
    {
        //
        // Get the boot device name
        //
        BootDeviceName = KeLoaderBlock->ArcBootDeviceName;
        if (BootDeviceName)
        {
            //
            // Check if we're booting from ramdisk
            //
            if ((strlen(BootDeviceName) >= 10) &&
                !(_strnicmp(BootDeviceName, "ramdisk(0)", 10)))
            {
                //
                // We'll have to tell the PnP Manager
                //
                ReportDetectedDevice = TRUE;
                
                //
                // Check for a command line
                //
                CommandLine = KeLoaderBlock->LoadOptions;
                if (CommandLine)
                {
                    //
                    // Check if this is an ISO boot
                    //
                    if (strstr(CommandLine, "RDEXPORTASCD"))
                    {
                        //
                        // Remember for later
                        //
                        ExportBootDiskAsCd = TRUE;
                    }
                    
                    //
                    // Check if this is PE boot
                    //
                    if (strstr(CommandLine, "MININT"))
                    {
                        //
                        // Remember for later
                        //
                        IsWinPEBoot = TRUE;
                    }
                }
            }
            
        }
    }
    
    //
    // Installing from Ramdisk isn't supported yet
    //
    //ASSERT(!KeLoaderBlock->SetupLdrBlock);
    
    //
    // Are we reporting the device
    //
    if (ReportDetectedDevice)
    {
        //
        // Do it
        //
        Status = IoReportDetectedDevice(DriverObject,
                                        InterfaceTypeUndefined,
                                        0xFFFFFFFF,
                                        0xFFFFFFFF,
                                        NULL,
                                        NULL,
                                        0,
                                        &PhysicalDeviceObject);
        if (NT_SUCCESS(Status))
        {
            //
            // ReactOS Fix
            // The ReactOS Plug and Play Manager is broken and does not create
            // the required keys when reporting a detected device.
            // We hack around this ourselves.
            //
            RtlCreateUnicodeString(&((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode->InstancePath,
                                   L"Root\\UNKNOWN\\0000");
            
            //
            // Create the device object
            //
            Status = RamdiskAddDevice(DriverObject, PhysicalDeviceObject);
            if (NT_SUCCESS(Status))
            {
                //
                // We're done
                //
                PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                Status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        //
        // Done
        //
        Status = STATUS_SUCCESS;
    }

    //
    // Done
    //
    return Status;
}
