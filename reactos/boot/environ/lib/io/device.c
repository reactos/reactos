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

typedef struct _BL_DEVICE_INFORMATION
{
    ULONG Unknown0;
    ULONG Unknown1;
    ULONG Unknown2;
    ULONG Unknown3;
} BL_DEVICE_INFORMATION, *PBL_DEVICE_INFORMATION;

LIST_ENTRY DmRegisteredDevices;
ULONG DmTableEntries;
LIST_ENTRY DmRegisteredDevices;
PVOID* DmDeviceTable;

BL_DEVICE_INFORMATION DmDeviceIoInformation;

/* FUNCTIONS *****************************************************************/

struct _BL_DEVICE_ENTRY;

typedef
NTSTATUS
(*PBL_DEVICE_ENUMERATE_DEVICE_CLASS) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_OPEN) (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ struct _BL_DEVICE_ENTRY* DeviceEntry
    );

typedef
NTSTATUS
(*PBL_DEVICE_CLOSE) (
    _In_ struct _BL_DEVICE_ENTRY* DeviceEntry
    );

typedef
NTSTATUS
(*PBL_DEVICE_READ) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_WRITE) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_GET_INFORMATION) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_SET_INFORMATION) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_RESET) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_FLUSH) (
    VOID
    );

typedef
NTSTATUS
(*PBL_DEVICE_CREATE) (
    VOID
    );

typedef struct _BL_DEVICE_CALLBACKS
{
    PBL_DEVICE_ENUMERATE_DEVICE_CLASS EnumerateDeviceClass;
    PBL_DEVICE_OPEN Open;
    PBL_DEVICE_CLOSE Close;
    PBL_DEVICE_READ Read;
    PBL_DEVICE_WRITE Write;
    PBL_DEVICE_GET_INFORMATION GetInformation;
    PBL_DEVICE_SET_INFORMATION SetInformation;
    PBL_DEVICE_RESET Reset;
    PBL_DEVICE_FLUSH Flush;
    PBL_DEVICE_CREATE Create;
} BL_DEVICE_CALLBACKS, *PBL_DEVICE_CALLBACKS;

typedef struct _BL_DEVICE_ENTRY
{
    ULONG DeviceId;
    ULONG Flags;
    ULONG Unknown;
    ULONG ReferenceCount;
    BL_DEVICE_CALLBACKS Callbacks;
    PVOID DeviceSpecificData;
    PBL_DEVICE_DESCRIPTOR DeviceDescriptor;
} BL_DEVICE_ENTRY, *PBL_DEVICE_ENTRY;

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
DiskOpen (
    _In_ PBL_DEVICE_DESCRIPTOR Device,
    _In_ PBL_DEVICE_ENTRY DeviceEntry
    )
{
    EfiPrintf(L"Not implemented!\r\n");
    return STATUS_NOT_IMPLEMENTED;
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

BL_DEVICE_CALLBACKS BlockIoDeviceFunctionTable =
{
    NULL,
    BlockIoOpen,
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


ULONG BcpBlockAllocatorHandle;
ULONG BcpHashTableId;

NTSTATUS
BcpDestroy (
    VOID
    )
{
    //BcpPurgeCacheEntries();
    //return BlpMmDeleteBlockAllocator(BcpBlockAllocatorHandle);
    EfiPrintf(L"Destructor for block cache not yet implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
BcpCompareKey (
    _In_ PBL_HASH_ENTRY Entry1,
    _In_ PBL_HASH_ENTRY Entry2
    )
{
    PULONG Value1, Value2;

    Value1 = Entry1->Value;
    Value2 = Entry2->Value;
    return Entry1->Size == Entry2->Size && Entry1->Flags == Entry2->Flags && *Value1 == *Value2 && Value1[1] == Value2[1] && Value1[2] == Value2[2];
}

ULONG
BcpHashFunction (
    _In_ PBL_HASH_ENTRY Entry,
    _In_ ULONG TableSize
    )
{
    ULONG i, j, ValueHash;
    PUCHAR ValueBuffer;

    j = 0;
    ValueHash = 0;
    i = 0;

    ValueBuffer = Entry->Value;

    do
    {
        ValueHash += ValueBuffer[i++];
    }
    while (i < 8);

    do
    {
        ValueHash += ValueBuffer[j++ + 8];
    }
    while (j < 4);

    return ValueHash % TableSize;
}

NTSTATUS
BcInitialize (
    VOID
    )
{
    NTSTATUS Status;

    Status = BlHtCreate(50, BcpHashFunction, BcpCompareKey, &BcpHashTableId);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    BcpBlockAllocatorHandle = BlpMmCreateBlockAllocator();
    if (BcpBlockAllocatorHandle == -1)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    Status = BlpIoRegisterDestroyRoutine(BcpDestroy);
    if (Status >= 0)
    {
        return Status;
    }

Quickie:
    EfiPrintf(L"Failure path not yet implemented\n");
#if 0
    if (BcpHashTableId != -1)
    {
        BlHtDestroy(BcpHashTableId);
    }
    if (BcpBlockAllocatorHandle != -1)
    {
        BlpMmDeleteBlockAllocator(BcpBlockAllocatorHandle);
    }
#endif
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

    /* Check if the two devices exist and are identical in typ */
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
    PBL_DEVICE_DESCRIPTOR Device = (PBL_DEVICE_DESCRIPTOR)Entry;
    PBL_DEVICE_ENTRY DeviceEntry = (PBL_DEVICE_ENTRY)Argument1;
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
            if ((!(Flags & 1) || (DeviceEntry->Flags & 2)) &&
                (!(Flags & 2) || (DeviceEntry->Flags & 4)))
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
    if (DeviceEntry->Flags & 1)
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
    return ((ULONG)Entry->Value >> 3) % TableSize;
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
            EfiPrintf(L"Failure path not implemented %lx\r\n", Status);
            //MmPapFreePages(BlockIopPrefetchBuffer, 1);
        }
    }

    /* Return back to the caller */
    return Status;
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
        BlockDevice->DeviceFlags = 1;
    }
    if (Media->MediaPresent)
    {
        BlockDevice->DeviceFlags |= 2;
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
            break;
        }

        /* Close the device path */
        EfiCloseProtocol(Handle, &EfiDevicePathProtocol);
    }

    /* If we got here, nothing was found */
    Status = STATUS_NO_SUCH_DEVICE;

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
    EFI_HANDLE Handle;
    BL_PROTOCOL_HANDLE Protocol[2];
    ACPI_HID_DEVICE_PATH *AcpiPath;
    HARDDRIVE_DEVICE_PATH *DiskPath;
    BOOLEAN Found;
    ULONG i;

    Device = DeviceEntry->DeviceDescriptor;
    BlockDevice = (PBL_BLOCK_DEVICE)DeviceEntry->DeviceSpecificData;

    Found = FALSE;

    Handle = BlockDevice->Handle;

    Protocol[0].Handle = Handle;
    Protocol[1].Handle = 0;

    Status = EfiOpenProtocol(Handle,
                             &EfiDevicePathProtocol,
                             &Protocol[0].Interface);
    if (Status < 0)
    {
        return Status;
    }

    for (i = 0, Found = FALSE; Found == FALSE && Protocol[i].Handle; i++)
    {
        LeafNode = EfiGetLeafNode(Protocol[i].Interface);
        if (LeafNode->Type == ACPI_DEVICE_PATH)
        {
            /* We only support floppy drives */
            AcpiPath = (ACPI_HID_DEVICE_PATH*)LeafNode;
            if ((AcpiPath->HID == EISA_PNP_ID(0x604)) &&
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
            if (LeafNode->SubType == MEDIA_HARDDRIVE_DP)
            {
                Device->Local.Type = LocalDevice;

                /* Check if this is an MBR partition */
                if (DiskPath->SignatureType == SIGNATURE_TYPE_MBR)
                {
                    /* Set that this is a local partition */
                    Device->DeviceType = LegacyPartitionDevice;
                    Device->Partition.Disk.Type = LocalDevice;

                    BlockDevice->PartitionType = MbrPartition;
                    BlockDevice->Disk.Mbr.Signature = *(PULONG)&DiskPath->Signature[0];
                    Found = TRUE;
                }
                else if (DiskPath->SignatureType == SIGNATURE_TYPE_GUID)
                {
                    BlockDevice->PartitionType = 0;
                    Device->Local.HardDisk.PartitionType = GptPartition;

                    Status = BlockIoGetGPTDiskSignature(DeviceEntry,
                                                        &Device->Local.HardDisk.Gpt.PartitionSignature);
                    if (NT_SUCCESS(Status))
                    {
                        RtlCopyMemory(&BlockDevice->Disk.Gpt.Signature,
                                      &Device->Local.HardDisk.Gpt.PartitionSignature,
                                      sizeof(&BlockDevice->Disk.Gpt.Signature));
                        Found = TRUE;
                    }
                }

                /* Otherwise, raw boot is not supported */
                BlockDevice->PartitionType = RawPartition;
                Device->Local.HardDisk.PartitionType = RawPartition;
                Device->Local.HardDisk.Raw.DiskNumber = BlockIoFirmwareRawDiskCount++;;
            }
            else if (LeafNode->SubType == MEDIA_CDROM_DP)
            {
                /* Set block device information */
                BlockDevice->PartitionType = RawPartition;
                BlockDevice->Type = CdRomDevice;

                /* Set CDROM data */
                Device->Local.Type = CdRomDevice;
                Device->Local.FloppyDisk.DriveNumber = 0;
                Found = TRUE;
            }
        }
        else if ((LeafNode->Type != MEDIA_DEVICE_PATH) &&
                 (LeafNode->Type == ACPI_DEVICE_PATH) &&
                 (i == 0))
        {
            Status = BlockIoEfiGetChildHandle(Protocol, &Protocol[i]);
            if (!NT_SUCCESS(Status))
            {
                Device->DeviceType = LocalDevice;
                Found = 1;
                if (BlockDevice->DeviceFlags & 1)
                {
                    BlockDevice->Type = FloppyDevice;
                    Device->Local.HardDisk.PartitionType = BlockIoFirmwareRemovableDiskCount++;
                    Device->Local.Type = FloppyDevice;
                }
                else
                {
                    BlockDevice->Type = DiskDevice;
                    Device->Local.Type = DiskDevice;
                    Device->Local.HardDisk.PartitionType = RawPartition;
                    BlockDevice->PartitionType = RawPartition;
                    Device->Local.HardDisk.Mbr.PartitionSignature = BlockIoFirmwareRawDiskCount++;
                }
            }
        }
    }

    while (i)
    {
        EfiCloseProtocol(Protocol[--i].Handle, &EfiDevicePathProtocol);
    }

    if (Found)
    {
        Status = 0;
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
    }
    return Status;
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
            /* We have a fully constructed device, reuturn it */
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
    HashEntry.Flags = 1;
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
            break;
        }

        /* Add the device entry to the device table */
        Status = BlTblSetEntry(&BlockIoDeviceTable,
                               &BlockIoDeviceTableEntries,
                               DeviceEntry,
                               &Id,
                               TblDoNotPurgeEntry);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"Failure path not implemented: %lx\r\n", Status);
#if 0
            BlHtDelete(HashTableId, &HashKey);
#endif
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
    if (!(DeviceEntry->Flags & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Drop a reference and check if it's the last one */
    DeviceEntry->ReferenceCount--;
    if (!DeviceEntry->ReferenceCount)
    {
        /* Mark the device as inactive */
        DeviceEntry->Flags = ~1;
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

    /* Check for unsupported flags */
    if (!(Flags & 3))
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
        DeviceEntry->Flags |= 1;
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
    DeviceEntry->Flags |= 7;
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

    /* Check if the device was opened successfuly */
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
            DeviceEntry->DeviceId = *DeviceId;
            return STATUS_SUCCESS;
        }
    }

Quickie:
    /* Failure path -- did we allocate a device entry? */
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
    DmDeviceIoInformation.Unknown0 = 0;
    DmDeviceIoInformation.Unknown1 = 0;
    DmDeviceIoInformation.Unknown2 = 0;
    DmDeviceIoInformation.Unknown3 = 0;

    /* Allocate the device table */
    DmDeviceTable = BlMmAllocateHeap(DmTableEntries * sizeof(PVOID));
    if (DmDeviceTable)
    {
        /* Clear it */
        RtlZeroMemory(DmDeviceTable, DmTableEntries * sizeof(PVOID));
#if 0
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

