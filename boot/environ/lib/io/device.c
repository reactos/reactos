/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/device.c
 * PURPOSE:         Boot Library Device Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

typedef struct _BL_DEVICE_IO_INFORMATION
{
    ULONGLONG ReadCount;
    ULONGLONG WriteCount;
} BL_DEVICE_IO_INFORMATION, *PBL_DEVICE_IO_INFORMATION;

LIST_ENTRY DmRegisteredDevices;
ULONG DmTableEntries;
LIST_ENTRY DmRegisteredDevices;
PVOID* DmDeviceTable;

BL_DEVICE_IO_INFORMATION DmDeviceIoInformation;

/* FUNCTIONS *****************************************************************/

typedef struct _BL_REGISTERED_DEVICE
{
    LIST_ENTRY ListEntry;
    BL_DEVICE_CALLBACKS Callbacks;
} BL_REGISTERED_DEVICE, *PBL_REGISTERED_DEVICE;

PVOID* BlockIoDeviceTable;
ULONG BlockIoDeviceTableEntries;

ULONG BlockIoFirmwareRemovableDiskCount;
ULONG BlockIoFirmwareRawDiskCount;
ULONG BlockIoFirmwareCdromCount;

PVOID BlockIopAlignedBuffer;
ULONG BlockIopAlignedBufferSize;

PVOID BlockIopPartialBlockBuffer;
ULONG BlockIopPartialBlockBufferSize;

PVOID BlockIopPrefetchBuffer;

PVOID BlockIopReadBlockBuffer;
ULONG BlockIopReadBlockBufferSize;

ULONG HashTableId;

BOOLEAN BlockIoInitialized;

NTSTATUS
BlockIoOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    );

NTSTATUS
BlockIoGetInformation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    );

NTSTATUS
BlockIoSetInformation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    );

NTSTATUS
BlockIoRead (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_ PULONG BytesRead
    );

BL_DEVICE_CALLBACKS BlockIoDeviceFunctionTable =
{
    NULL,
    BlockIoOpen,
    NULL,
    BlockIoRead,
    NULL,
    BlockIoGetInformation,
    BlockIoSetInformation
};

NTSTATUS
BlockIoFirmwareWrite (
    _In_ PBL_BLOCK_DEVICE BlockDevice,
    _In_ PVOID Buffer,
    _In_ ULONGLONG Block,
    _In_ ULONGLONG BlockCount
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIoFirmwareRead (
    _In_ PBL_BLOCK_DEVICE BlockDevice,
    _In_ PVOID Buffer,
    _In_ ULONGLONG Block,
    _In_ ULONGLONG BlockCount
    )
{
    NTSTATUS Status;
    EFI_BLOCK_IO *BlockProtocol;
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    ULONG FailureCount;

    for (FailureCount = 0, Status = STATUS_SUCCESS;
         FailureCount < 2 && NT_SUCCESS(Status);
         FailureCount++)
    {
        BlockProtocol = BlockDevice->Protocol;

        OldMode = CurrentExecutionContext->Mode;
        if (CurrentExecutionContext->Mode != 1)
        {
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        //EfiPrintf(L"EFI Reading BLOCK %d off media %lx (%d blocks)\r\n",
                 //Block, BlockProtocol->Media->MediaId, BlockCount);
        EfiStatus = BlockProtocol->ReadBlocks(BlockProtocol,
                                              BlockProtocol->Media->MediaId,
                                              Block,
                                              BlockProtocol->Media->BlockSize * BlockCount,
                                              Buffer);
        if (EfiStatus == EFI_SUCCESS)
        {
            //EfiPrintf(L"EFI Read complete into buffer\r\n");
            //EfiPrintf(L"Buffer data: %lx %lx %lx %lx\r\n", *(PULONG)Buffer, *((PULONG)Buffer + 1), *((PULONG)Buffer + 2), *((PULONG)Buffer + 3));
        }

        if (OldMode != 1)
        {
            BlpArchSwitchContext(OldMode);
        }

        Status = EfiGetNtStatusCode(EfiStatus);
        if (Status != STATUS_MEDIA_CHANGED)
        {
            break;
        }

        EfiCloseProtocol(BlockDevice->Handle, &EfiBlockIoProtocol);

        Status = EfiOpenProtocol(BlockDevice->Handle,
                                 &EfiBlockIoProtocol,
                                 (PVOID*)BlockDevice->Protocol);
    }

    return Status;
}

NTSTATUS
BlockIopFirmwareOperation (
    PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONGLONG Block,
    _In_ ULONGLONG BlockCount,
    _In_ ULONG OperationType
    )
{
    ULONG FailureCount;
    PBL_BLOCK_DEVICE BlockDevice;
    NTSTATUS Status;

    BlockDevice = DeviceEntry->DeviceSpecificData;

    if (OperationType == 1)
    {
        for (FailureCount = 0; FailureCount < 3; FailureCount++)
        {
            Status = BlockIoFirmwareWrite(BlockDevice, Buffer, Block, BlockCount);
            if (Status >= 0)
            {
                break;
            }
        }
    }
    else
    {
        for (FailureCount = 0; FailureCount < 3; FailureCount++)
        {
            Status = BlockIoFirmwareRead(BlockDevice, Buffer, Block, BlockCount);
            if (Status >= 0)
            {
                break;
            }
        }
    }
    return Status;
}

NTSTATUS
BlockIopFreeAlignedBuffer (
    _Inout_ PVOID* Buffer,
    _Inout_ PULONG BufferSize
    )
{
    NTSTATUS Status;

    if (*BufferSize)
    {
        Status = MmPapFreePages(*Buffer, BL_MM_INCLUDE_MAPPED_ALLOCATED);

        *Buffer = NULL;
        *BufferSize = 0;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
BlockIopAllocateAlignedBuffer (
    _Inout_ PVOID* Buffer,
    _Inout_ PULONG BufferSize,
    _In_ ULONG Size,
    _In_ ULONG Alignment
    )
{
    NTSTATUS Status;

    if (!Alignment)
    {
        ++Alignment;
    }

    Status = STATUS_SUCCESS;
    if ((Size > *BufferSize) || ((Alignment - 1) & (ULONG_PTR)*Buffer))
    {
        BlockIopFreeAlignedBuffer(Buffer, BufferSize);

        *BufferSize = ROUND_TO_PAGES(Size);

        Status = MmPapAllocatePagesInRange(Buffer,
                                           BlLoaderDeviceMemory,
                                           *BufferSize >> PAGE_SHIFT,
                                           0,
                                           Alignment >> PAGE_SHIFT,
                                           NULL,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            *BufferSize = 0;
        }
    }

    return Status;
}

NTSTATUS
BlockIopReadUsingPrefetch (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG BlockCount
    )
{
    EfiPrintf(L"No prefetch support\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIopOperation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG BlockCount,
    _In_ ULONG OperationType
    )
{
    PBL_BLOCK_DEVICE BlockDevice;
    ULONG BufferSize, Alignment;
    ULONGLONG Offset;
    NTSTATUS Status;

    BlockDevice = DeviceEntry->DeviceSpecificData;
    BufferSize = BlockDevice->BlockSize * BlockCount;
    Offset = BlockDevice->Block + BlockDevice->StartOffset;
    if ((BlockDevice->LastBlock + 1) < (BlockDevice->Block + BlockCount))
    {
        EfiPrintf(L"Read past end of device\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    Alignment = BlockDevice->Alignment;
    if (!(Alignment) || !((Alignment - 1) & (ULONG_PTR)Buffer))
    {
        Status = BlockIopFirmwareOperation(DeviceEntry,
                                           Buffer,
                                           Offset,
                                           BlockCount,
                                           OperationType);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"EFI op failed: %lx\r\n", Status);
            return Status;
        }

        return STATUS_SUCCESS;
    }

    Status = BlockIopAllocateAlignedBuffer(&BlockIopAlignedBuffer,
                                           &BlockIopAlignedBufferSize,
                                           BufferSize,
                                           BlockDevice->Alignment);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"No memory for align\r\n");
        return STATUS_NO_MEMORY;
    }

    if (OperationType == 1)
    {
        RtlCopyMemory(BlockIopAlignedBuffer, Buffer, BufferSize);
    }

    Status = BlockIopFirmwareOperation(DeviceEntry,
                                       BlockIopAlignedBuffer,
                                       Offset,
                                       BlockCount,
                                       OperationType);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!OperationType)
    {
        RtlCopyMemory(Buffer, BlockIopAlignedBuffer, BufferSize);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BlockIopReadWriteVirtualDevice (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _In_ ULONG Operation,
    _Out_ PULONG BytesRead
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIopReadPhysicalDevice (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_ PULONG BytesRead
    )
{
    PBL_BLOCK_DEVICE BlockDevice;
    PVOID ReadBuffer;
    ULONGLONG OffsetEnd, AlignedOffsetEnd, Offset;
    NTSTATUS Status;

    BlockDevice = DeviceEntry->DeviceSpecificData;
    ReadBuffer = Buffer;
    OffsetEnd = Size + BlockDevice->Offset;
    if (OffsetEnd < Size)
    {
        OffsetEnd = -1;
        return STATUS_INTEGER_OVERFLOW;
    }

    AlignedOffsetEnd = ~(BlockDevice->BlockSize - 1) & (OffsetEnd + BlockDevice->BlockSize - 1);
    if (AlignedOffsetEnd < OffsetEnd)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    if ((BlockDevice->Offset) || (Size != AlignedOffsetEnd))
    {
        Status = BlockIopAllocateAlignedBuffer(&BlockIopReadBlockBuffer,
                                               &BlockIopReadBlockBufferSize,
                                               AlignedOffsetEnd,
                                               BlockDevice->Alignment);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"Failed to allocate buffer: %lx\r\n", Status);
            return Status;
        }

        ReadBuffer = BlockIopReadBlockBuffer;
    }

    Offset = AlignedOffsetEnd / BlockDevice->BlockSize;

    if (BlockDevice->Unknown & 2)
    {
        Status = BlockIopReadUsingPrefetch(DeviceEntry,
                                           ReadBuffer,
                                           AlignedOffsetEnd / BlockDevice->BlockSize);
        if (NT_SUCCESS(Status))
        {
            goto ReadComplete;
        }
    }

    Status = BlockIopOperation(DeviceEntry, ReadBuffer, Offset, 0);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Block I/O failed: %lx\r\n", Status);
        return Status;
    }

    BlockDevice->Block += Offset;

ReadComplete:
    if (ReadBuffer != Buffer)
    {
        RtlCopyMemory(Buffer,
                      (PVOID)((ULONG_PTR)ReadBuffer +
                              (ULONG_PTR)BlockDevice->Offset),
                      Size);
    }

    if (BytesRead)
    {
        *BytesRead = Size;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BlockIopBlockInformationCheck (
    _In_ PBL_BLOCK_DEVICE BlockDevice,
    _In_opt_ PULONG DesiredSize,
    _Out_opt_ PULONG Size,
    _Out_opt_ PULONG OutputAdjustedSize
    )
{
    ULONG RealSize;
    ULONGLONG Offset, LastOffset, RemainingOffset, MaxOffset;
    NTSTATUS Status;

    RealSize = 0;

    Offset = (BlockDevice->Offset * BlockDevice->BlockSize) + BlockDevice->Block;

    if (Offset > ((BlockDevice->LastBlock + 1) * BlockDevice->BlockSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    LastOffset = (BlockDevice->LastBlock * BlockDevice->BlockSize) + BlockDevice->BlockSize - 1;

    MaxOffset = BlockDevice->LastBlock;
    if (MaxOffset < BlockDevice->BlockSize)
    {
        MaxOffset = BlockDevice->BlockSize;
    }

    if (LastOffset < MaxOffset)
    {

        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    if (Offset > LastOffset)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    RemainingOffset = LastOffset - Offset + 1;

    if (DesiredSize != FALSE)
    {
        RealSize = *DesiredSize;
    }
    else
    {
        RealSize = ULONG_MAX;
    }

    if (RemainingOffset < RealSize)
    {
        if (Size == FALSE)
        {
            RealSize = 0;
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        RealSize = RemainingOffset;
    }

    Status = STATUS_SUCCESS;

Quickie:
    if (Size)
    {
        *Size = RealSize;
    }

    return Status;
}

NTSTATUS
BlockIoRead (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_ PULONG BytesRead
    )
{
    PBL_BLOCK_DEVICE BlockDevice;
    NTSTATUS Status;

    /* Get the device-specific data, which is our block device descriptor */
    BlockDevice = DeviceEntry->DeviceSpecificData;

    /* Make sure that the buffer and size is valid */
    Status = BlockIopBlockInformationCheck(BlockDevice, &Size, BytesRead, &Size);
    if (NT_SUCCESS(Status))
    {
        /* Check if this is a virtual device or a physical device */
        if (BlockDevice->DeviceFlags & BL_BLOCK_DEVICE_VIRTUAL_FLAG)
        {
            /* Do a virtual read or write */
            Status = BlockIopReadWriteVirtualDevice(DeviceEntry, Buffer, Size, 0, BytesRead);
        }
        else
        {
            /* Do a physical read or write */
            Status = BlockIopReadPhysicalDevice(DeviceEntry, Buffer, Size, BytesRead);
        }
    }
    else if (BytesRead)
    {
        /* We failed, if the caller wanted bytes read, return 0 */
        *BytesRead = 0;
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BlockIoSetInformation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    )
{
    PBL_BLOCK_DEVICE BlockDevice;
    ULONGLONG Offset;

    BlockDevice = DeviceEntry->DeviceSpecificData;

    /* Take the current block number and block-offset and conver to full offset */
    Offset = DeviceInformation->BlockDeviceInfo.Block * BlockDevice->BlockSize +
             DeviceInformation->BlockDeviceInfo.Offset;

    /* Make sure that the full offset is still within the bounds of the device */
    if (Offset > ((BlockDevice->LastBlock + 1) * BlockDevice->BlockSize - 1))
    {
        EfiPrintf(L"Offset out of bounds\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Convery the full raw offset into a block number and block-offset */
    BlockDevice->Block = Offset / BlockDevice->BlockSize;
    BlockDevice->Offset = Offset % BlockDevice->BlockSize;

    /* Return the unknown */
    BlockDevice->Unknown = DeviceInformation->BlockDeviceInfo.Unknown;

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
BlockIoGetInformation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    )
{
    /* Copy the device specific data into the block device information */
    RtlCopyMemory(&DeviceInformation->BlockDeviceInfo,
                   DeviceEntry->DeviceSpecificData,
                   sizeof(DeviceInformation->BlockDeviceInfo));

    /* Hardcode the device type */
    DeviceInformation->DeviceType = DiskDevice;
    return STATUS_SUCCESS;
}

BOOLEAN
BlDeviceIsVirtualPartitionDevice (
    _In_ PBL_DEVICE_DESCRIPTOR InputDevice,
    _Outptr_ PBL_DEVICE_DESCRIPTOR* VirtualDevice
    )
{
    BOOLEAN IsVirtual;
    PBL_LOCAL_DEVICE ParentDisk;

    /* Assume it isn't */
    IsVirtual = FALSE;

    /* Check if this is a partition device */
    if ((InputDevice->DeviceType == LegacyPartitionDevice) ||
        (InputDevice->DeviceType == PartitionDevice))
    {
        /* Check if the parent disk is a VHD */
        ParentDisk = &InputDevice->Partition.Disk;
        if (ParentDisk->Type == VirtualDiskDevice)
        {
            /* This is a virtual partition device -- does the caller want it? */
            IsVirtual = TRUE;
            if (VirtualDevice)
            {
                *VirtualDevice = (PBL_DEVICE_DESCRIPTOR)(&ParentDisk->VirtualHardDisk + 1);
            }
        }
    }

    /* Return */
    return IsVirtual;
}

NTSTATUS
BlDeviceSetInformation (
    _In_ ULONG DeviceId,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    )
{
    PBL_DEVICE_ENTRY DeviceEntry;

    /* This parameter is not optional */
    if (!DeviceInformation)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the device ID is valid */
    if (DmTableEntries <= DeviceId)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the device entry */
    DeviceEntry = DmDeviceTable[DeviceId];
    if (!DeviceEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the device is open */
    if (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Set the device information */
    return DeviceEntry->Callbacks.SetInformation(DeviceEntry, DeviceInformation);
}

NTSTATUS
BlDeviceGetInformation (
    _In_ ULONG DeviceId,
    _Out_ PBL_DEVICE_INFORMATION DeviceInformation
    )
{
    PBL_DEVICE_ENTRY DeviceEntry;

    /* This parameter is not optional */
    if (!DeviceInformation)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the device ID is valid */
    if (DmTableEntries <= DeviceId)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the device entry */
    DeviceEntry = DmDeviceTable[DeviceId];
    if (!DeviceEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the device is open */
    if (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the device information */
    DeviceInformation->DeviceType = DeviceEntry->DeviceDescriptor->DeviceType;
    return DeviceEntry->Callbacks.GetInformation(DeviceEntry, DeviceInformation);
}

NTSTATUS
BlDeviceRead (
    _In_ ULONG DeviceId,
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_opt_ PULONG BytesRead
    )
{
    PBL_DEVICE_ENTRY DeviceEntry;
    NTSTATUS Status;
    ULONG BytesTransferred;

    /* Make sure we have a buffer, and the device ID is valid */
    if (!(Buffer) || (DmTableEntries <= DeviceId))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the device entry for it */
    DeviceEntry = DmDeviceTable[DeviceId];
    if (!DeviceEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure this is a device opened for read access */
    if (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED) ||
        !(DeviceEntry->Flags & BL_DEVICE_ENTRY_READ_ACCESS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Issue the read */
    Status = DeviceEntry->Callbacks.Read(DeviceEntry,
                                         Buffer,
                                         Size,
                                         &BytesTransferred);
    if (!DeviceEntry->Unknown)
    {
        /* Update performance counters */
        DmDeviceIoInformation.ReadCount += BytesTransferred;
    }

    /* Return back how many bytes were read, if caller wants to know */
    if (BytesRead)
    {
        *BytesRead = BytesTransferred;
    }

    /* Return read result */
    return Status;
}

NTSTATUS
BlDeviceReadAtOffset (
    _In_ ULONG DeviceId,
    _In_ ULONG Size,
    _In_ ULONGLONG Offset,
    _In_ PVOID Buffer,
    _Out_ PULONG BytesRead
    )
{
    NTSTATUS Status;
    BL_DEVICE_INFORMATION DeviceInfo;

    /* Get the current block and offset  */
    Status = BlDeviceGetInformation(DeviceId, &DeviceInfo);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the block and block-offset based on the new raw offset */
    DeviceInfo.BlockDeviceInfo.Block = Offset / DeviceInfo.BlockDeviceInfo.BlockSize;
    DeviceInfo.BlockDeviceInfo.Offset = Offset % DeviceInfo.BlockDeviceInfo.BlockSize;

    /* Update the block and offset */
    Status = BlDeviceSetInformation(DeviceId, &DeviceInfo);
    if (NT_SUCCESS(Status))
    {
        /* Now issue a read, with this block and offset configured */
        Status = BlDeviceRead(DeviceId, Buffer, Size, BytesRead);
    }

    /* All good, return the caller */
    return Status;
}

BOOLEAN
BlpDeviceCompare (
    _In_ PBL_DEVICE_DESCRIPTOR Device1,
    _In_ PBL_DEVICE_DESCRIPTOR Device2
    )
{
    BOOLEAN DeviceMatch;
    ULONG DeviceSize;

    /* Assume failure */
    DeviceMatch = FALSE;

    /* Check if the two devices exist and are identical in type */
    if ((Device1) && (Device2) && (Device1->DeviceType == Device2->DeviceType))
    {
        /* Take the bigger of the two sizes */
        DeviceSize = max(Device1->Size, Device2->Size);
        if (DeviceSize >= (ULONG)FIELD_OFFSET(BL_DEVICE_DESCRIPTOR, Local))
        {
            /* Compare the two devices up to their size */
            if (RtlEqualMemory(&Device1->Local,
                               &Device2->Local,
                               DeviceSize - FIELD_OFFSET(BL_DEVICE_DESCRIPTOR, Local)))
            {
                /* They match! */
                DeviceMatch = TRUE;
            }
        }
    }

    /* Return matching state */
    return DeviceMatch;
}

NTSTATUS
BlockIopFreeAllocations (
    _In_ PBL_BLOCK_DEVICE BlockDevice
    )
{
    /* If a block device was passed in, free it */
    if (BlockDevice)
    {
        BlMmFreeHeap(BlockDevice);
    }

    /* Nothing else to do */
    return STATUS_SUCCESS;
}

NTSTATUS
BlockIoEfiGetBlockIoInformation (
    _In_ PBL_BLOCK_DEVICE BlockDevice
    )
{
    NTSTATUS Status;
    EFI_BLOCK_IO_MEDIA *Media;

    /* Open the Block I/O protocol on this device */
    Status = EfiOpenProtocol(BlockDevice->Handle,
                             &EfiBlockIoProtocol,
                             (PVOID*)&BlockDevice->Protocol);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get information on the block media */
    Media = BlockDevice->Protocol->Media;

    /* Set the appropriate device flags */
    BlockDevice->DeviceFlags = 0;
    if (Media->RemovableMedia)
    {
        BlockDevice->DeviceFlags = BL_BLOCK_DEVICE_REMOVABLE_FLAG;
    }
    if (Media->MediaPresent)
    {
        BlockDevice->DeviceFlags |= BL_BLOCK_DEVICE_PRESENT_FLAG;
    }

    /* No clue */
    BlockDevice->Unknown = 0;

    /* Set the block size */
    BlockDevice->BlockSize = Media->BlockSize;

    /* Make sure there's a last block value */
    if (!Media->LastBlock)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Don't let it be too high */
    if (Media->LastBlock > 0xFFFFFFFFFFE)
    {
        BlockDevice->LastBlock = 0xFFFFFFFFFFE;
    }
    else
    {
        BlockDevice->LastBlock = Media->LastBlock;
    }

    /* Make the alignment the smaller of the I/O alignment or the block size */
    if (Media->IoAlign >= Media->BlockSize)
    {
        BlockDevice->Alignment = Media->IoAlign;
    }
    else
    {
        BlockDevice->Alignment = Media->BlockSize;
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
BlockIoEfiGetChildHandle (
    _In_ PBL_PROTOCOL_HANDLE ProtocolInterface,
    _In_ PBL_PROTOCOL_HANDLE ChildProtocolInterface)
{
    NTSTATUS Status;
    ULONG i, DeviceCount;
    EFI_DEVICE_PATH *DevicePath, *ParentDevicePath;
    EFI_HANDLE *DeviceHandles;
    EFI_HANDLE Handle;

    /* Find all the Block I/O device handles on the system */
    DeviceCount = 0;
    DeviceHandles = 0;
    Status = EfiLocateHandleBuffer(ByProtocol,
                                   &EfiBlockIoProtocol,
                                   &DeviceCount,
                                   &DeviceHandles);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to enumerate, bail out */
        return Status;
    }

    /* Loop all the handles */
    for (i = 0; i < DeviceCount; i++)
    {
        /* Check if this is the device itself */
        Handle = DeviceHandles[i];
        if (Handle == ProtocolInterface->Handle)
        {
            /* Skip it */
            continue;
        }

        /* Get the device path of this device */
        Status = EfiOpenProtocol(Handle,
                                 &EfiDevicePathProtocol,
                                 (PVOID*)&DevicePath);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, skip it */
            continue;
        }

        /* See if we are its parent  */
        ParentDevicePath = EfiIsDevicePathParent(ProtocolInterface->Interface,
                                                 DevicePath);
        if (ParentDevicePath == ProtocolInterface->Interface)
        {
            /* Yup, return back to caller */
            ChildProtocolInterface->Handle = Handle;
            ChildProtocolInterface->Interface = DevicePath;
            Status = STATUS_SUCCESS;
            goto Quickie;
        }

        /* Close the device path */
        EfiCloseProtocol(Handle, &EfiDevicePathProtocol);
    }

    /* If we got here, nothing was found */
    Status = STATUS_NO_SUCH_DEVICE;

Quickie:
    /* Free the handle array buffer */
    BlMmFreeHeap(DeviceHandles);
    return Status;
}

NTSTATUS
BlockIoGetGPTDiskSignature (
    _In_ PBL_DEVICE_ENTRY DeviceEntry,
    _Out_ PGUID DiskSignature
    )
{
    EfiPrintf(L"GPT not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIoEfiGetDeviceInformation (
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR Device;
    PBL_BLOCK_DEVICE BlockDevice;
    EFI_DEVICE_PATH *LeafNode;
    BL_PROTOCOL_HANDLE Protocol[2];
    ACPI_HID_DEVICE_PATH *AcpiPath;
    HARDDRIVE_DEVICE_PATH *DiskPath;
    BOOLEAN Found;
    ULONG i;

    /* Extract the identifier, and the block device object */
    Device = DeviceEntry->DeviceDescriptor;
    BlockDevice = (PBL_BLOCK_DEVICE)DeviceEntry->DeviceSpecificData;

    /* Initialize protocol handles */
    Protocol[0].Handle = BlockDevice->Handle;
    Protocol[1].Handle = 0;

    /* Open this device */
    Status = EfiOpenProtocol(Protocol[0].Handle,
                             &EfiDevicePathProtocol,
                             &Protocol[0].Interface);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return Status;
    }

    /* Iterate twice -- once for the top level, once for the bottom */
    for (i = 0, Found = FALSE; Found == FALSE && Protocol[i].Handle; i++)
    {
        /* Check what kind of leaf node device this is */
        LeafNode = EfiGetLeafNode(Protocol[i].Interface);
        EfiPrintf(L"Pass %d, Leaf node: %p Type: %d\r\n", i, LeafNode, LeafNode->Type);
        if (LeafNode->Type == ACPI_DEVICE_PATH)
        {
            /* We only support floppy drives */
            AcpiPath = (ACPI_HID_DEVICE_PATH*)LeafNode;
            if ((AcpiPath->HID == EISA_PNP_ID(0x604)) ||
                (AcpiPath->HID == EISA_PNP_ID(0x700)))
            {
                /* Set the boot library specific device types */
                Device->DeviceType = LocalDevice;
                Device->Local.Type = FloppyDevice;

                /* The ACPI UID is the drive number */
                Device->Local.FloppyDisk.DriveNumber = AcpiPath->UID;

                /* We found a match */
                Found = TRUE;
            }
        }
        else if ((LeafNode->Type == MEDIA_DEVICE_PATH) && (i == 1))
        {
            /* Extract the disk path and check if it's a physical disk */
            DiskPath = (HARDDRIVE_DEVICE_PATH*)LeafNode;
            EfiPrintf(L"Disk path: %p Type: %lx\r\n", DiskPath, LeafNode->SubType);
            if (LeafNode->SubType == MEDIA_HARDDRIVE_DP)
            {
                /* Set this as a local  device */
                Device->Local.Type = LocalDevice;

                /* Check if this is an MBR partition */
                if (DiskPath->SignatureType == SIGNATURE_TYPE_MBR)
                {
                    /* Set that this is a local partition */
                    Device->DeviceType = LegacyPartitionDevice;
                    Device->Partition.Disk.Type = LocalDevice;

                    /* Write the MBR partition signature */
                    BlockDevice->PartitionType = MbrPartition;
                    BlockDevice->Disk.Mbr.Signature = *(PULONG)&DiskPath->Signature[0];
                    Found = TRUE;
                }
                else if (DiskPath->SignatureType == SIGNATURE_TYPE_GUID)
                {
                    /* Set this as a GPT partition */
                    BlockDevice->PartitionType = GptPartition;
                    Device->Local.HardDisk.PartitionType = GptPartition;

                    /* Get the GPT signature */
                    Status = BlockIoGetGPTDiskSignature(DeviceEntry,
                                                        &Device->Local.HardDisk.Gpt.PartitionSignature);
                    if (NT_SUCCESS(Status))
                    {
                        /* Copy it */
                        RtlCopyMemory(&BlockDevice->Disk.Gpt.Signature,
                                      &Device->Local.HardDisk.Gpt.PartitionSignature,
                                      sizeof(BlockDevice->Disk.Gpt.Signature));
                        Found = TRUE;
                    }
                }

                /* Otherwise, this is a raw disk */
                BlockDevice->PartitionType = RawPartition;
                Device->Local.HardDisk.PartitionType = RawPartition;
                Device->Local.HardDisk.Raw.DiskNumber = BlockIoFirmwareRawDiskCount++;
            }
            else if (LeafNode->SubType == MEDIA_CDROM_DP)
            {
                /* Set block device information */
                EfiPrintf(L"Found CD-ROM\r\n");
                BlockDevice->PartitionType = RawPartition;
                BlockDevice->Type = CdRomDevice;

                /* Set CDROM data */
                Device->Local.Type = CdRomDevice;
                Device->Local.FloppyDisk.DriveNumber = 0;
                Found = TRUE;
            }
        }
        else if ((LeafNode->Type != MEDIA_DEVICE_PATH) &&
                 (LeafNode->Type != ACPI_DEVICE_PATH) &&
                 (i == 0))
        {
            /* This is probably a messaging device node. Are we under it? */
            Status = BlockIoEfiGetChildHandle(Protocol, &Protocol[1]);
            EfiPrintf(L"Pass 0, non DP/ACPI path. Child handle obtained: %lx\r\n", Protocol[1].Handle);
            if (!NT_SUCCESS(Status))
            {
                /* We're not. So this must be a raw device */
                Device->DeviceType = LocalDevice;
                Found = TRUE;

                /* Is it a removable raw device? */
                if (BlockDevice->DeviceFlags & BL_BLOCK_DEVICE_REMOVABLE_FLAG)
                {
                    /* This is a removable (CD or Floppy or USB) device */
                    BlockDevice->Type = FloppyDevice;
                    Device->Local.Type = FloppyDevice;
                    Device->Local.FloppyDisk.DriveNumber = BlockIoFirmwareRemovableDiskCount++;
                    EfiPrintf(L"Found Floppy\r\n");
                }
                else
                {
                    /* It's a fixed device */
                    BlockDevice->Type = DiskDevice;
                    Device->Local.Type = DiskDevice;

                    /* Set it as a raw partition */
                    Device->Local.HardDisk.PartitionType = RawPartition;
                    Device->Local.HardDisk.Mbr.PartitionSignature = BlockIoFirmwareRawDiskCount++;
                    EfiPrintf(L"Found raw disk\r\n");
                }
            }
        }
    }

    /* Close any protocols that we opened for each handle */
    while (i)
    {
        EfiCloseProtocol(Protocol[--i].Handle, &EfiDevicePathProtocol);
    }

    /* Return appropriate status */
    return Found ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

NTSTATUS
BlockIoEfiReset (
    VOID
    )
{
    EfiPrintf(L"not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIoEfiFlush (
    VOID
    )
{
    EfiPrintf(L"not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlockIoEfiCreateDeviceEntry (
    _In_ PBL_DEVICE_ENTRY *DeviceEntry,
    _Out_ PVOID Handle
    )
{
    PBL_DEVICE_ENTRY IoDeviceEntry;
    PBL_BLOCK_DEVICE BlockDevice;
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR Device;

    /* Allocate the entry for this device and zero it out */
    IoDeviceEntry = BlMmAllocateHeap(sizeof(*IoDeviceEntry));
    if (!IoDeviceEntry)
    {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(IoDeviceEntry, sizeof(*IoDeviceEntry));

    /* Allocate the device descriptor for this device and zero it out */
    Device = BlMmAllocateHeap(sizeof(*Device));
    if (!Device)
    {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(Device, sizeof(*Device));

    /* Allocate the block device specific data, and zero it out */
    BlockDevice = BlMmAllocateHeap(sizeof(*BlockDevice));
    if (!BlockDevice)
    {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(BlockDevice, sizeof(*BlockDevice));

    /* Save the descriptor and block device specific data */
    IoDeviceEntry->DeviceSpecificData = BlockDevice;
    IoDeviceEntry->DeviceDescriptor = Device;

    /* Set the size of the descriptor */
    Device->Size = sizeof(*Device);

    /* Copy the standard I/O callbacks */
    RtlCopyMemory(&IoDeviceEntry->Callbacks,
                  &BlockIoDeviceFunctionTable,
                  sizeof(IoDeviceEntry->Callbacks));

    /* Add the two that are firmware specific */
    IoDeviceEntry->Callbacks.Reset = BlockIoEfiReset;
    IoDeviceEntry->Callbacks.Flush = BlockIoEfiFlush;

    /* Save the EFI handle */
    BlockDevice->Handle = Handle;

    /* Get information on this device from EFI, caching it in the device */
    Status = BlockIoEfiGetBlockIoInformation(BlockDevice);
    if (NT_SUCCESS(Status))
    {
        /* Build the descriptor structure for this device */
        Status = BlockIoEfiGetDeviceInformation(IoDeviceEntry);
        if (NT_SUCCESS(Status))
        {
            /* We have a fully constructed device, return it */
            *DeviceEntry = IoDeviceEntry;
            return STATUS_SUCCESS;
        }
    }

    /* Failure path, free the descriptor if we allocated one */
    if (IoDeviceEntry->DeviceDescriptor)
    {
        BlMmFreeHeap(IoDeviceEntry->DeviceDescriptor);
    }

    /* Free any other specific allocations */
    BlockIopFreeAllocations(IoDeviceEntry->DeviceSpecificData);

    /* Free the device entry itself and return the failure code */
    BlMmFreeHeap(IoDeviceEntry);
    EfiPrintf(L"Failed: %lx\r\n", Status);
    return Status;
}

NTSTATUS
BlockIoEfiCompareDevice (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ EFI_HANDLE Handle
    )
{
    PBL_LOCAL_DEVICE LocalDeviceInfo, EfiLocalDeviceInfo;
    PBL_DEVICE_ENTRY DeviceEntry;
    PBL_DEVICE_DESCRIPTOR EfiDevice;
    NTSTATUS Status;

    DeviceEntry = NULL;

    /* Check if no device was given */
    if (!Device)
    {
        /* Fail the comparison */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check if this is a local disk device */
    if (Device->DeviceType != DiskDevice)
    {
        /* Nope -- is it a partition device? */
        if ((Device->DeviceType != LegacyPartitionDevice) &&
            (Device->DeviceType != PartitionDevice))
        {
            /* Nope, so we can't compare */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* If so, return the device information for the parent disk */
        LocalDeviceInfo = &Device->Partition.Disk;
    }
    else
    {
        /* Just return the disk information itself */
        LocalDeviceInfo = &Device->Local;
    }

    /* Create an EFI device entry for the EFI device handle */
    Status = BlockIoEfiCreateDeviceEntry(&DeviceEntry, Handle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the descriptor and assume failure for now */
    EfiDevice = DeviceEntry->DeviceDescriptor;
    Status = STATUS_UNSUCCESSFUL;

    /* Check if the EFI device is a disk */
    if (EfiDevice->DeviceType != DiskDevice)
    {
        /* Nope, is it a partition? */
        if ((EfiDevice->DeviceType != LegacyPartitionDevice) &&
            (EfiDevice->DeviceType != PartitionDevice))
        {
            /* Neither, invalid handle so bail out */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        /* Yes, so get the information of the parent disk */
        EfiLocalDeviceInfo = &EfiDevice->Partition.Disk;
    }
    else
    {
        /* It's a disk, so get the disk information itself */
        EfiLocalDeviceInfo = &EfiDevice->Local;
    }

    /* Are the two devices the same type? */
    if (EfiLocalDeviceInfo->Type != LocalDeviceInfo->Type)
    {
        /* Nope, that was easy */
        goto Quickie;
    }

    /* Yes, what kind of device is the EFI side? */
    switch (EfiLocalDeviceInfo->Type)
    {
        case LocalDevice:

            /* Local hard drive, compare the signature */
            if (RtlCompareMemory(&EfiLocalDeviceInfo->HardDisk,
                                 &LocalDeviceInfo->HardDisk,
                                 sizeof(LocalDeviceInfo->HardDisk)) ==
                sizeof(LocalDeviceInfo->HardDisk))
            {
                Status = STATUS_SUCCESS;
            }
            break;

        case FloppyDevice:
        case CdRomDevice:

            /* Removable floppy or CD, compare the disk number */
            if (RtlCompareMemory(&EfiLocalDeviceInfo->FloppyDisk,
                                 &LocalDeviceInfo->FloppyDisk,
                                 sizeof(LocalDeviceInfo->FloppyDisk)) ==
                sizeof(LocalDeviceInfo->FloppyDisk))
            {
                Status = STATUS_SUCCESS;
            }
            break;

        case RamDiskDevice:

            /* RAM disk, compare the size and base information */
            if (RtlCompareMemory(&EfiLocalDeviceInfo->RamDisk,
                                 &LocalDeviceInfo->RamDisk,
                                 sizeof(LocalDeviceInfo->RamDisk)) ==
                sizeof(LocalDeviceInfo->RamDisk))
            {
                Status = STATUS_SUCCESS;
            }
            break;

        case FileDevice:

            /* File, compare the file identifier */
            if (RtlCompareMemory(&EfiLocalDeviceInfo->File,
                                 &LocalDeviceInfo->File,
                                 sizeof(LocalDeviceInfo->File)) ==
                sizeof(LocalDeviceInfo->File))
            {
                Status = STATUS_SUCCESS;
            }
            break;

        /* Something else we don't support */
        default:
            break;
    }

Quickie:
    /* All done, did we have an EFI device entry? */
    if (DeviceEntry)
    {
        /* Free it, since we only needed it locally for comparison */
        BlMmFreeHeap(DeviceEntry->DeviceDescriptor);
        BlockIopFreeAllocations(DeviceEntry->DeviceSpecificData);
        BlMmFreeHeap(DeviceEntry);
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BlockIoFirmwareOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_BLOCK_DEVICE BlockIoDevice
    )
{
    NTSTATUS Status;
    BOOLEAN DeviceMatch;
    BL_HASH_ENTRY HashEntry;
    ULONG i, Id, DeviceCount;
    PBL_DEVICE_ENTRY DeviceEntry;
    EFI_HANDLE* DeviceHandles;

    /* Initialize everything */
    DeviceEntry = NULL;
    DeviceCount = 0;
    DeviceHandles = 0;
    DeviceEntry = NULL;

    /* Ask EFI for handles to all block devices */
    Status = EfiLocateHandleBuffer(ByProtocol,
                                   &EfiBlockIoProtocol,
                                   &DeviceCount,
                                   &DeviceHandles);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    /* Build a hash entry, with the value inline */
    HashEntry.Flags = BL_HT_VALUE_IS_INLINE;
    HashEntry.Size = sizeof(EFI_HANDLE);

    /* Loop each device we got */
    DeviceMatch = FALSE;
    Status = STATUS_NO_SUCH_DEVICE;
    for (i = 0; i < DeviceCount; i++)
    {
        /* Check if we have a match in the device hash table */
        HashEntry.Value = DeviceHandles[i];
        Status = BlHtLookup(HashTableId, &HashEntry, 0);
        if (NT_SUCCESS(Status))
        {
            /* We already know about this device */
            EfiPrintf(L"Device is known\r\n");
            continue;
        }

        /* New device, store it in the hash table */
        Status = BlHtStore(HashTableId,
                           &HashEntry,
                           DeviceHandles[i],
                           sizeof(DeviceHandles[i]));
        if (!NT_SUCCESS(Status))
        {
            /* Free the array and fail */
            BlMmFreeHeap(DeviceHandles);
            break;
        }

        /* Create an entry for this device*/
        Status = BlockIoEfiCreateDeviceEntry(&DeviceEntry, DeviceHandles[i]);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"EFI create failed: %lx\r\n", Status);
            continue;
        }

        /* Add the device entry to the device table */
        Status = BlTblSetEntry(&BlockIoDeviceTable,
                               &BlockIoDeviceTableEntries,
                               DeviceEntry,
                               &Id,
                               TblDoNotPurgeEntry);
        if (!NT_SUCCESS(Status))
        {
            /* Remove it from teh hash table */
            BlHtDelete(HashTableId, &HashEntry);

            /* Free the block I/O device data */
            BlockIopFreeAllocations(DeviceEntry->DeviceSpecificData);

            /* Free the descriptor */
            BlMmFreeHeap(DeviceEntry->DeviceDescriptor);

            /* Free the entry */
            BlMmFreeHeap(DeviceEntry);
            break;
        }

        /* Does this device match what we're looking for? */
        DeviceMatch = BlpDeviceCompare(DeviceEntry->DeviceDescriptor, Device);
        if (DeviceMatch)
        {
            /* Yep, return the data back */
            RtlCopyMemory(BlockIoDevice,
                          DeviceEntry->DeviceSpecificData,
                          sizeof(*BlockIoDevice));
            Status = STATUS_SUCCESS;
            break;
        }
    }

    /* Free the device handle buffer array */
    BlMmFreeHeap(DeviceHandles);

    /* Return status */
    return Status;
}

NTSTATUS
PartitionOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VhdFileDeviceOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DiskClose (
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    NTSTATUS Status, LocalStatus;
    PBL_BLOCK_DEVICE BlockDevice;

    /* Assume success */
    Status = STATUS_SUCCESS;
    BlockDevice = DeviceEntry->DeviceSpecificData;

    /* Close the protocol */
    LocalStatus = EfiCloseProtocol(BlockDevice->Handle, &EfiBlockIoProtocol);
    if (!NT_SUCCESS(LocalStatus))
    {
        /* Only inherit failures */
        Status = LocalStatus;
    }

    /* Free the block device allocations */
    LocalStatus = BlockIopFreeAllocations(BlockDevice);
    if (!NT_SUCCESS(LocalStatus))
    {
        /* Only inherit failures */
        Status = LocalStatus;
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
DiskOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    NTSTATUS Status;

    /* Use firmware-specific functions to open the disk */
    Status = BlockIoFirmwareOpen(Device, DeviceEntry->DeviceSpecificData);
    if (NT_SUCCESS(Status))
    {
        /* Overwrite with our own close routine */
        DeviceEntry->Callbacks.Close = DiskClose;
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
RdDeviceOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FileDeviceOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SpOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
UdpOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BL_DEVICE_CALLBACKS FileDeviceFunctionTable =
{
    NULL,
    FileDeviceOpen,
    NULL,
};

BL_DEVICE_CALLBACKS PartitionDeviceFunctionTable =
{
    NULL,
    PartitionOpen,
    NULL,
};

BL_DEVICE_CALLBACKS RamDiskDeviceFunctionTable =
{
    NULL,
    RdDeviceOpen,
    NULL,
};

BL_DEVICE_CALLBACKS DiskDeviceFunctionTable =
{
    NULL,
    DiskOpen,
    NULL,
};

BL_DEVICE_CALLBACKS VirtualDiskDeviceFunctionTable =
{
    NULL,
    VhdFileDeviceOpen,
    NULL,
};

BL_DEVICE_CALLBACKS UdpFunctionTable =
{
    NULL,
    UdpOpen,
    NULL,
};

BL_DEVICE_CALLBACKS SerialPortFunctionTable =
{
    NULL,
    SpOpen,
    NULL,
};

BOOLEAN
DeviceTableCompare (
    _In_ PVOID Entry,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _Inout_ PVOID Argument3,
    _Inout_ PVOID Argument4
    )
{
    BOOLEAN Found;
    PBL_DEVICE_DESCRIPTOR Device = (PBL_DEVICE_DESCRIPTOR)Argument1;
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Entry;
    ULONG Flags = *(PULONG)Argument2;
    ULONG Unknown = *(PULONG)Argument3;

    /* Assume failure */
    Found = FALSE;

    /* Compare the device descriptor */
    if (BlpDeviceCompare(DeviceEntry->DeviceDescriptor, Device))
    {
        /* Compare something */
        if (DeviceEntry->Unknown == Unknown)
        {
            /* Compare flags */
            if ((!(Flags & BL_DEVICE_READ_ACCESS) || (DeviceEntry->Flags & BL_DEVICE_ENTRY_READ_ACCESS)) &&
                (!(Flags & BL_DEVICE_WRITE_ACCESS) || (DeviceEntry->Flags & BL_DEVICE_ENTRY_WRITE_ACCESS)))
            {
                /* And more flags */
                if (((Flags & 8) || !(DeviceEntry->Flags & 8)) &&
                    (!(Flags & 8) || (DeviceEntry->Flags & 8)))
                {
                    /* Found a match! */
                    Found = TRUE;
                }
            }
        }
    }

    /* Return matching state */
    return Found;
}

NTSTATUS
DeviceTableDestroyEntry (
    _In_ PVOID Entry,
    _In_ ULONG DeviceId
    )
{
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Entry;
    NTSTATUS Status;

    /* Call the close routine for this entry */
    Status = DeviceEntry->Callbacks.Close(DmDeviceTable[DeviceId]);

    /* Free the descriptor, and the device itself */
    BlMmFreeHeap(DeviceEntry->DeviceDescriptor);
    BlMmFreeHeap(DeviceEntry);

    /* Clear out the netry, and return */
    DmDeviceTable[DeviceId] = NULL;
    return Status;
}

NTSTATUS
DeviceTablePurge (
    _In_ PVOID Entry
    )
{
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Entry;
    NTSTATUS Status;

    /* Check if the device is opened */
    if (DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED)
    {
        /* It is, so can't purge it */
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* It isn't, so destroy the entry */
        Status = DeviceTableDestroyEntry(DeviceEntry, DeviceEntry->DeviceId);
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
BlockIoDeviceTableDestroyEntry (
    _In_ PVOID Entry,
    _In_ ULONG DeviceId
    )
{
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Entry;
    NTSTATUS Status;

    /* Call the close routine for this entry */
    Status = DeviceEntry->Callbacks.Close(DeviceEntry);

    /* Free the descriptor, and the device itself */
    BlMmFreeHeap(DeviceEntry->DeviceDescriptor);
    BlMmFreeHeap(DeviceEntry);

    /* Clear out the netry, and return */
    BlockIoDeviceTable[DeviceId] = NULL;
    return Status;
}

NTSTATUS
BlockIoDeviceTableDestroy (
    VOID
    )
{
    NTSTATUS Status;

    /* Call the entry destructor on each entry in the table */
    Status = BlTblMap(BlockIoDeviceTable,
                      BlockIoDeviceTableEntries,
                      BlockIoDeviceTableDestroyEntry);

    /* Free the table and return */
    BlMmFreeHeap(BlockIoDeviceTable);
    return Status;
}

NTSTATUS
BlockIopDestroy (
    VOID
    )
{
    /* Free the prefetch buffer */
    BlMmFreeHeap(BlockIopPrefetchBuffer);

    /* Set state to non initialized */
    BlockIoInitialized = FALSE;

    /* Return back */
    return STATUS_SUCCESS;
}

ULONG
BlockIoEfiHashFunction (
    _In_ PBL_HASH_ENTRY Entry,
    _In_ ULONG TableSize
    )
{
    /* Get rid of the alignment bits to have a more unique number */
    return ((ULONG_PTR)Entry->Value >> 3) % TableSize;
}

NTSTATUS
BlockIopInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Allocate the block device table and zero it out */
    BlockIoDeviceTableEntries = 8;
    BlockIoDeviceTable = BlMmAllocateHeap(sizeof(PVOID) *
                                          BlockIoDeviceTableEntries);
    if (!BlockIoDeviceTableEntries)
    {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(BlockIoDeviceTable, sizeof(PVOID) * BlockIoDeviceTableEntries);

    /* Register our destructor */
    Status = BlpIoRegisterDestroyRoutine(BlockIoDeviceTableDestroy);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Initialize all counters */
    BlockIoFirmwareRemovableDiskCount = 0;
    BlockIoFirmwareRawDiskCount = 0;
    BlockIoFirmwareCdromCount = 0;

    /* Initialize the buffers and their sizes */
    BlockIopAlignedBuffer = NULL;
    BlockIopAlignedBufferSize = 0;
    BlockIopPartialBlockBuffer = NULL;
    BlockIopPartialBlockBufferSize = 0;
    BlockIopPrefetchBuffer = NULL;
    BlockIopReadBlockBuffer = NULL;
    BlockIopReadBlockBufferSize = 0;

    /* Allocate the prefetch buffer */
    Status = MmPapAllocatePagesInRange(&BlockIopPrefetchBuffer,
                                       BlLoaderDeviceMemory,
                                       0x100,
                                       0,
                                       0,
                                       NULL,
                                       0);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the block cache */
        Status = BcInitialize();
        if (NT_SUCCESS(Status))
        {
            /* Initialize the block device hash table */
            Status = BlHtCreate(29, BlockIoEfiHashFunction, NULL, &HashTableId);
            if (NT_SUCCESS(Status))
            {
                /* Register our destructor */
                Status = BlpIoRegisterDestroyRoutine(BlockIopDestroy);
                if (NT_SUCCESS(Status))
                {
                    /* We're good */
                    BlockIoInitialized = TRUE;
                }
            }
        }
    }

    /* Check if this is the failure path */
    if (!NT_SUCCESS(Status))
    {
        /* Free the prefetch buffer is one was allocated */
        if (BlockIopPrefetchBuffer)
        {
            MmPapFreePages(BlockIopPrefetchBuffer, BL_MM_INCLUDE_MAPPED_ALLOCATED);
        }
    }

    /* Return back to the caller */
    return Status;
}

BOOLEAN
BlockIoDeviceTableCompare (
    _In_ PVOID Entry,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PVOID Argument3,
    _In_ PVOID Argument4
    )
{
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Entry;
    PBL_DEVICE_DESCRIPTOR Device = (PBL_DEVICE_DESCRIPTOR)Argument1;

    /* Compare the two devices */
    return BlpDeviceCompare(DeviceEntry->DeviceDescriptor, Device);
}

NTSTATUS
BlockIoOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    NTSTATUS Status;
    PBL_BLOCK_DEVICE BlockDevice;
    PBL_DEVICE_ENTRY FoundDeviceEntry;
    ULONG Dummy;

    /* Check if the block I/O manager is initialized */
    if (!BlockIoInitialized)
    {
        /* First call, initialize it now */
        Status = BlockIopInitialize();
        if (!NT_SUCCESS(Status))
        {
            /* Failed to initialize block I/O */
            return Status;
        }
    }

    /* Copy a function table for block I/O devices */
    RtlCopyMemory(&DeviceEntry->Callbacks,
                  &BlockIoDeviceFunctionTable,
                  sizeof(DeviceEntry->Callbacks));

    /* Allocate a block I/O device */
    BlockDevice = BlMmAllocateHeap(sizeof(*BlockDevice));
    if (!BlockDevice)
    {
        return STATUS_NO_MEMORY;
    }

    /* Set this as the device-specific data for this device entry */
    Status = STATUS_SUCCESS;
    DeviceEntry->DeviceSpecificData = BlockDevice;

    /* Check if we already have this device in our device table */
    FoundDeviceEntry = BlTblFindEntry(BlockIoDeviceTable,
                                      BlockIoDeviceTableEntries,
                                      &Dummy,
                                      BlockIoDeviceTableCompare,
                                      Device,
                                      NULL,
                                      NULL,
                                      NULL);
    if (FoundDeviceEntry)
    {
        /* We already found a device, so copy its device data and callbacks */
        //EfiPrintf(L"Block I/O Device entry found: %p\r\n", FoundDeviceEntry);
        RtlCopyMemory(BlockDevice, FoundDeviceEntry->DeviceSpecificData, sizeof(*BlockDevice));
        RtlCopyMemory(&DeviceEntry->Callbacks,
                      &FoundDeviceEntry->Callbacks,
                      sizeof(DeviceEntry->Callbacks));
        return Status;
    }

    /* Zero out the device for now */
    RtlZeroMemory(BlockDevice, sizeof(*BlockDevice));

    /* Is this a disk? */
    if (Device->DeviceType == DiskDevice)
    {
        /* What type of disk is it? */
        switch (Device->Local.Type)
        {
            /* Is it a raw physical disk? */
            case LocalDevice:
            case FloppyDevice:
            case CdRomDevice:
                /* Open a disk device */
                Status = DiskDeviceFunctionTable.Open(Device, DeviceEntry);
                break;

            /* Is it a RAM disk? */
            case RamDiskDevice:
                /* Open a RAM disk */
                Status = RamDiskDeviceFunctionTable.Open(Device, DeviceEntry);
                break;

            /* Is it a file? */
            case FileDevice:
                /* Open a file */
                Status = FileDeviceFunctionTable.Open(Device, DeviceEntry);
                break;

            /* Is it a VHD? */
            case VirtualDiskDevice:
                /* Open a virtual disk */
                Status = VirtualDiskDeviceFunctionTable.Open(Device, DeviceEntry);
                break;

            /* Is it something else? */
            default:
                /* Not supported */
                Status = STATUS_INVALID_PARAMETER;
                break;
        }
    }
    else if ((Device->DeviceType == LegacyPartitionDevice) ||
             (Device->DeviceType == PartitionDevice))
    {
        /* This is a partition on a disk, open it as such */
        Status = PartitionDeviceFunctionTable.Open(Device, DeviceEntry);
    }
    else
    {
        /* Other devices are not supported */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Free any allocations for this device */
        BlockIopFreeAllocations(BlockDevice);
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BlpDeviceResolveLocate (
    _In_ PBL_DEVICE_DESCRIPTOR InputDevice,
    _Out_ PBL_DEVICE_DESCRIPTOR* LocateDevice
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlDeviceClose (
    _In_ ULONG DeviceId
    )
{
    PBL_DEVICE_ENTRY DeviceEntry;

    /* Validate the device ID */
    if (DmTableEntries <= DeviceId)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure there's a device there */
    DeviceEntry = DmDeviceTable[DeviceId];
    if (DeviceEntry == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the device is active */
    if (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Drop a reference and check if it's the last one */
    DeviceEntry->ReferenceCount--;
    if (!DeviceEntry->ReferenceCount)
    {
        /* Mark the device as inactive */
        DeviceEntry->Flags = ~BL_DEVICE_ENTRY_OPENED;
    }

    /* We're good */
    return STATUS_SUCCESS;
}

NTSTATUS
BlpDeviceOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ ULONG Flags,
    _In_ ULONG Unknown,
    _Out_ PULONG DeviceId
    )
{
    NTSTATUS Status;
    PBL_DEVICE_ENTRY DeviceEntry;
    PBL_DEVICE_DESCRIPTOR LocateDeviceDescriptor;
    PBL_REGISTERED_DEVICE RegisteredDevice;
    PLIST_ENTRY NextEntry, ListHead;

    DeviceEntry = NULL;

    /* Check for missing parameters */
    if (!(Device) || !(DeviceId) || !(Device->Size))
    {
        /* Bail out */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Make sure both read and write access are set */
    if (!(Flags & (BL_DEVICE_READ_ACCESS | BL_DEVICE_WRITE_ACCESS)))
    {
        /* Bail out */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check if the boot device is being opened */
    if (Device->DeviceType == BootDevice)
    {
        /* Select it */
        Device = BlpBootDevice;
    }

    /* Check if the 'locate' device is being opened */
    if (Device->DeviceType == LocateDevice)
    {
        /* Go find it */
        Status = BlpDeviceResolveLocate(Device, &LocateDeviceDescriptor);
        if (!NT_SUCCESS(Status))
        {
            /* Not found, bail out */
            goto Quickie;
        }

        /* Select it */
        Device = LocateDeviceDescriptor;
    }

    /* Check if the device isn't ready yet */
    if (Device->Flags & 1)
    {
        /* Return a failure */
        Status = STATUS_DEVICE_NOT_READY;
        goto Quickie;
    }

    /* Check if we already have an entry for the device */
    DeviceEntry = BlTblFindEntry(DmDeviceTable,
                                 DmTableEntries,
                                 DeviceId,
                                 DeviceTableCompare,
                                 Device,
                                 &Flags,
                                 &Unknown,
                                 NULL);
    if (DeviceEntry)
    {
        /* Return it, taking a reference on it */
        *DeviceId = DeviceEntry->DeviceId;
        ++DeviceEntry->ReferenceCount;
        DeviceEntry->Flags |= BL_DEVICE_ENTRY_OPENED;
        return STATUS_SUCCESS;
    }

    /* We don't, allocate one */
    DeviceEntry = BlMmAllocateHeap(sizeof(*DeviceEntry));
    if (!DeviceEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Fill it out */
    RtlZeroMemory(DeviceEntry, sizeof(*DeviceEntry));
    DeviceEntry->ReferenceCount = 1;
    DeviceEntry->Flags |= (BL_DEVICE_ENTRY_OPENED |
                           BL_DEVICE_ENTRY_READ_ACCESS |
                           BL_DEVICE_ENTRY_WRITE_ACCESS);
    DeviceEntry->Unknown = Unknown;

    /* Save flag 8 if needed */
    if (Flags & 8)
    {
        DeviceEntry->Flags |= 8;
    }

    /* Allocate a device descriptor for the device */
    DeviceEntry->DeviceDescriptor = BlMmAllocateHeap(Device->Size);
    if (!DeviceEntry->DeviceDescriptor)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Copy the descriptor that was passed in */
    RtlCopyMemory(DeviceEntry->DeviceDescriptor, Device, Device->Size);

    /* Now loop the list of dynamically registered devices */
    ListHead = &DmRegisteredDevices;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the device */
        RegisteredDevice = CONTAINING_RECORD(NextEntry,
                                             BL_REGISTERED_DEVICE,
                                             ListEntry);

        /* Open the device */
        Status = RegisteredDevice->Callbacks.Open(Device, DeviceEntry);
        if (NT_SUCCESS(Status))
        {
            /* The device was opened, so we have the right one */
            goto DeviceOpened;
        }

        /* Nope, keep trying */
        NextEntry = NextEntry->Flink;
    }

    /* Well, it wasn't a dynamic device. Is it a block device? */
    if ((Device->DeviceType == PartitionDevice) ||
        (Device->DeviceType == DiskDevice) ||
        (Device->DeviceType == LegacyPartitionDevice))
    {
        /* Call the Block I/O handler */
        Status = BlockIoDeviceFunctionTable.Open(Device, DeviceEntry);
    }
    else if (Device->DeviceType == SerialDevice)
    {
        /* It's a serial device, call the serial device handler */
        Status = SerialPortFunctionTable.Open(Device, DeviceEntry);
    }
    else if (Device->DeviceType == UdpDevice)
    {
        /* It's a network device, call the UDP device handler */
        Status = UdpFunctionTable.Open(Device, DeviceEntry);
    }
    else
    {
        /* Unsupported type of device */
        Status = STATUS_NOT_IMPLEMENTED;
    }

    /* Check if the device was opened successfully */
    if (NT_SUCCESS(Status))
    {
DeviceOpened:
        /* Save the entry in the device table */
        Status = BlTblSetEntry(&DmDeviceTable,
                               &DmTableEntries,
                               DeviceEntry,
                               DeviceId,
                               DeviceTablePurge);
        if (NT_SUCCESS(Status))
        {
            /* It worked -- return the ID in the table to the caller */
            EfiPrintf(L"Device ID: %lx\r\n", *DeviceId);
            DeviceEntry->DeviceId = *DeviceId;
            return STATUS_SUCCESS;
        }
    }

Quickie:
    /* Failure path -- did we allocate a device entry? */
    EfiPrintf(L"Block failure: %lx\r\n", Status);
    if (DeviceEntry)
    {
        /* Yep -- did it have a descriptor? */
        if (DeviceEntry->DeviceDescriptor)
        {
            /* Free it */
            BlMmFreeHeap(DeviceEntry->DeviceDescriptor);
        }

        /* Free the entry */
        BlMmFreeHeap(DeviceEntry);
    }

    /* Return the failure */
    return Status;
}

NTSTATUS
BlpDeviceInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Initialize the table count and list of devices */
    DmTableEntries = 8;
    InitializeListHead(&DmRegisteredDevices);

    /* Initialize device information */
    DmDeviceIoInformation.ReadCount = 0;
    DmDeviceIoInformation.WriteCount = 0;

    /* Allocate the device table */
    DmDeviceTable = BlMmAllocateHeap(DmTableEntries * sizeof(PVOID));
    if (DmDeviceTable)
    {
        /* Clear it */
        RtlZeroMemory(DmDeviceTable, DmTableEntries * sizeof(PVOID));
#if BL_BITLOCKER_SUPPORT
        /* Initialize BitLocker support */
        Status = FvebInitialize();
#else
        Status = STATUS_SUCCESS;
#endif
    }
    else
    {
        /* No memory, we'll fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return initialization state */
    return Status;
}

