/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/disk/scsiport.c
 * PURPOSE:         Interface for SCSI Emulation
 * PROGRAMMERS:     Hervé Poussineau  <hpoussin@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(SCSIPORT);

#define _SCSIPORT_

#include <srb.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <stdio.h>
#include <stdarg.h>

#undef ScsiPortLogError
#undef ScsiPortMoveMemory
#undef ScsiPortWritePortBufferUchar
#undef ScsiPortWritePortBufferUlong
#undef ScsiPortWritePortBufferUshort
#undef ScsiPortWritePortUchar
#undef ScsiPortWritePortUlong
#undef ScsiPortWritePortUshort
#undef ScsiPortWriteRegisterBufferUchar
#undef ScsiPortWriteRegisterBufferUlong
#undef ScsiPortWriteRegisterBufferUshort
#undef ScsiPortWriteRegisterUchar
#undef ScsiPortWriteRegisterUlong
#undef ScsiPortWriteRegisterUshort
#undef ScsiPortReadPortBufferUchar
#undef ScsiPortReadPortBufferUlong
#undef ScsiPortReadPortBufferUshort
#undef ScsiPortReadPortUchar
#undef ScsiPortReadPortUlong
#undef ScsiPortReadPortUshort
#undef ScsiPortReadRegisterBufferUchar
#undef ScsiPortReadRegisterBufferUlong
#undef ScsiPortReadRegisterBufferUshort
#undef ScsiPortReadRegisterUchar
#undef ScsiPortReadRegisterUlong
#undef ScsiPortReadRegisterUshort

#define SCSI_PORT_NEXT_REQUEST_READY  0x0008

#define TAG_SCSI_DEVEXT 'DscS'
#define TAG_SCSI_ACCESS_RANGES 'AscS'

/* GLOBALS ********************************************************************/

#ifdef _M_IX86
VOID NTAPI HalpInitializePciStubs(VOID);
VOID NTAPI HalpInitBusHandler(VOID);
#endif

typedef struct
{
    PVOID NonCachedExtension;

    ULONG BusNum;
    ULONG MaxTargedIds;

    ULONG InterruptFlags;

    /* SRB extension stuff */
    ULONG SrbExtensionSize;
    PVOID SrbExtensionBuffer;

    IO_SCSI_CAPABILITIES PortCapabilities;

    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_RESET_BUS HwResetBus;

    /* DMA related stuff */
    PADAPTER_OBJECT AdapterObject;

    ULONG CommonBufferLength;

    PVOID MiniPortDeviceExtension;
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;

typedef struct tagDISKCONTEXT
{
    /* Device ID */
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    /* Device characteristics */
    ULONG SectorSize;
    ULONGLONG SectorOffset;
    ULONGLONG SectorCount;
    ULONGLONG SectorNumber;
} DISKCONTEXT;

PSCSI_PORT_DEVICE_EXTENSION ScsiDeviceExtensions[SCSI_MAXIMUM_BUSES];

/* FUNCTIONS ******************************************************************/

ULONG
ntohl(
    IN ULONG Value)
{
    FOUR_BYTE Dest;
    PFOUR_BYTE Source = (PFOUR_BYTE)&Value;

    Dest.Byte0 = Source->Byte3;
    Dest.Byte1 = Source->Byte2;
    Dest.Byte2 = Source->Byte1;
    Dest.Byte3 = Source->Byte0;

    return Dest.AsULong;
}

static
BOOLEAN
SpiSendSynchronousSrb(
    IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb)
{
    BOOLEAN ret;

    ASSERT(!(Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE));

    /* HACK: handle lack of interrupts */
    while (!(DeviceExtension->InterruptFlags & SCSI_PORT_NEXT_REQUEST_READY))
    {
        KeStallExecutionProcessor(100 * 1000);
        if (!DeviceExtension->HwInterrupt(DeviceExtension->MiniPortDeviceExtension))
        {
            ExFreePool(Srb);
            return FALSE;
        }
    }

    DeviceExtension->InterruptFlags &= ~SCSI_PORT_NEXT_REQUEST_READY;
    Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

    if (!DeviceExtension->HwStartIo(
            DeviceExtension->MiniPortDeviceExtension,
            Srb))
    {
        ExFreePool(Srb);
        return FALSE;
    }

    /* HACK: handle lack of interrupts */
    while (Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)
    {
        KeStallExecutionProcessor(100 * 1000);
        if (!DeviceExtension->HwInterrupt(DeviceExtension->MiniPortDeviceExtension))
        {
            ExFreePool(Srb);
            return FALSE;
        }
    }

    ret = SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS;
    ExFreePool(Srb);

    return ret;
}

static ARC_STATUS DiskClose(ULONG FileId)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    ExFreePool(Context);
    return ESUCCESS;
}

static ARC_STATUS DiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(*Information));

    /*
     * The ARC specification mentions that for partitions, StartingAddress and
     * EndingAddress are the start and end positions of the partition in terms
     * of byte offsets from the start of the disk.
     * CurrentAddress is the current offset into (i.e. relative to) the partition.
     */
    Information->StartingAddress.QuadPart = Context->SectorOffset * Context->SectorSize;
    Information->EndingAddress.QuadPart   = (Context->SectorOffset + Context->SectorCount) * Context->SectorSize;
    Information->CurrentAddress.QuadPart  = Context->SectorNumber * Context->SectorSize;

    return ESUCCESS;
}

static ARC_STATUS DiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    READ_CAPACITY_DATA ReadCapacityBuffer;

    DISKCONTEXT* Context;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    ULONG ScsiBus, PathId, TargetId, Lun, Partition, PathSyntax;
    ULONG SectorSize;
    ULONGLONG SectorOffset = 0;
    ULONGLONG SectorCount;

    /* Parse ARC path */
    if (!DissectArcPath2(Path, &ScsiBus, &TargetId, &Lun, &Partition, &PathSyntax))
        return EINVAL;
    if (PathSyntax != 0) /* scsi() format */
        return EINVAL;
    DeviceExtension = ScsiDeviceExtensions[ScsiBus];
    PathId = ScsiBus - DeviceExtension->BusNum;

    /* Get disk capacity and sector size */
    Srb = ExAllocatePool(PagedPool, sizeof(SCSI_REQUEST_BLOCK));
    if (!Srb)
        return ENOMEM;
    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->PathId = (UCHAR)PathId;
    Srb->TargetId = (UCHAR)TargetId;
    Srb->Lun = (UCHAR)Lun;
    Srb->CdbLength = 10;
    Srb->SrbFlags = SRB_FLAGS_DATA_IN;
    Srb->DataTransferLength = sizeof(READ_CAPACITY_DATA);
    Srb->TimeOutValue = 5; /* in seconds */
    Srb->DataBuffer = &ReadCapacityBuffer;
    Cdb = (PCDB)Srb->Cdb;
    Cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;
    if (!SpiSendSynchronousSrb(DeviceExtension, Srb))
    {
        return EIO;
    }

    /* Transform result to host endianness */
    SectorCount = ntohl(ReadCapacityBuffer.LogicalBlockAddress);
    SectorSize = ntohl(ReadCapacityBuffer.BytesPerBlock);

    if (Partition != 0)
    {
        /* Need to offset start of disk and length */
        UNIMPLEMENTED;
        return EIO;
    }

    Context = ExAllocatePool(PagedPool, sizeof(DISKCONTEXT));
    if (!Context)
        return ENOMEM;
    Context->DeviceExtension = DeviceExtension;
    Context->PathId = (UCHAR)PathId;
    Context->TargetId = (UCHAR)TargetId;
    Context->Lun = (UCHAR)Lun;
    Context->SectorSize = SectorSize;
    Context->SectorOffset = SectorOffset;
    Context->SectorCount = SectorCount;
    Context->SectorNumber = 0;
    FsSetDeviceSpecific(*FileId, Context);

    return ESUCCESS;
}

static ARC_STATUS DiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    ULONG FullSectors, NbSectors;
    ULONG Lba;

    *Count = 0;

    if (N == 0)
        return ESUCCESS;

    FullSectors = N / Context->SectorSize;
    NbSectors = (N + Context->SectorSize - 1) / Context->SectorSize;
    if (Context->SectorNumber + NbSectors >= Context->SectorCount)
        return EINVAL;
    if (FullSectors > 0xffff)
        return EINVAL;

    /* Read full sectors */
    ASSERT(Context->SectorNumber < 0xFFFFFFFF);
    Lba = (ULONG)(Context->SectorOffset + Context->SectorNumber);
    if (FullSectors > 0)
    {
        Srb = ExAllocatePool(PagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (!Srb)
            return ENOMEM;

        RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb->PathId = Context->PathId;
        Srb->TargetId = Context->TargetId;
        Srb->Lun = Context->Lun;
        Srb->CdbLength = 10;
        Srb->SrbFlags = SRB_FLAGS_DATA_IN;
        Srb->DataTransferLength = FullSectors * Context->SectorSize;
        Srb->TimeOutValue = 5; /* in seconds */
        Srb->DataBuffer = Buffer;
        Cdb = (PCDB)Srb->Cdb;
        Cdb->CDB10.OperationCode = SCSIOP_READ;
        Cdb->CDB10.LogicalUnitNumber = Srb->Lun;
        Cdb->CDB10.LogicalBlockByte0 = (Lba >> 24) & 0xff;
        Cdb->CDB10.LogicalBlockByte1 = (Lba >> 16) & 0xff;
        Cdb->CDB10.LogicalBlockByte2 = (Lba >> 8) & 0xff;
        Cdb->CDB10.LogicalBlockByte3 = Lba & 0xff;
        Cdb->CDB10.TransferBlocksMsb = (FullSectors >> 8) & 0xff;
        Cdb->CDB10.TransferBlocksLsb = FullSectors & 0xff;
        if (!SpiSendSynchronousSrb(Context->DeviceExtension, Srb))
        {
            return EIO;
        }
        Buffer = (PUCHAR)Buffer + FullSectors * Context->SectorSize;
        N -= FullSectors * Context->SectorSize;
        *Count += FullSectors * Context->SectorSize;
        Context->SectorNumber += FullSectors;
        Lba += FullSectors;
    }

    /* Read incomplete last sector */
    if (N > 0)
    {
        PUCHAR Sector;

        Sector = ExAllocatePool(PagedPool, Context->SectorSize);
        if (!Sector)
            return ENOMEM;

        Srb = ExAllocatePool(PagedPool, sizeof(SCSI_REQUEST_BLOCK));
        if (!Srb)
        {
            ExFreePool(Sector);
            return ENOMEM;
        }

        RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb->PathId = Context->PathId;
        Srb->TargetId = Context->TargetId;
        Srb->Lun = Context->Lun;
        Srb->CdbLength = 10;
        Srb->SrbFlags = SRB_FLAGS_DATA_IN;
        Srb->DataTransferLength = Context->SectorSize;
        Srb->TimeOutValue = 5; /* in seconds */
        Srb->DataBuffer = Sector;
        Cdb = (PCDB)Srb->Cdb;
        Cdb->CDB10.OperationCode = SCSIOP_READ;
        Cdb->CDB10.LogicalUnitNumber = Srb->Lun;
        Cdb->CDB10.LogicalBlockByte0 = (Lba >> 24) & 0xff;
        Cdb->CDB10.LogicalBlockByte1 = (Lba >> 16) & 0xff;
        Cdb->CDB10.LogicalBlockByte2 = (Lba >> 8) & 0xff;
        Cdb->CDB10.LogicalBlockByte3 = Lba & 0xff;
        Cdb->CDB10.TransferBlocksMsb = 0;
        Cdb->CDB10.TransferBlocksLsb = 1;
        if (!SpiSendSynchronousSrb(Context->DeviceExtension, Srb))
        {
            ExFreePool(Sector);
            return EIO;
        }
        RtlCopyMemory(Buffer, Sector, N);
        *Count += N;
        /* Context->SectorNumber remains untouched (incomplete sector read) */
        ExFreePool(Sector);
    }

    return ESUCCESS;
}

static ARC_STATUS DiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += (Context->SectorNumber * Context->SectorSize);
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart & (Context->SectorSize - 1))
        return EINVAL;

    /* Convert in number of sectors */
    NewPosition.QuadPart /= Context->SectorSize;
    if (NewPosition.QuadPart >= Context->SectorCount)
        return EINVAL;

    Context->SectorNumber = NewPosition.QuadPart;
    return ESUCCESS;
}

static const DEVVTBL DiskVtbl =
{
    DiskClose,
    DiskGetFileInformation,
    DiskOpen,
    DiskRead,
    DiskSeek,
};

static
NTSTATUS
SpiCreatePortConfig(
    IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    IN PHW_INITIALIZATION_DATA HwInitData,
    OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN BOOLEAN ZeroStruct)
{
    ULONG Bus;

    /* Zero out the struct if told so */
    if (ZeroStruct)
    {
        /* First zero the portconfig */
        RtlZeroMemory(ConfigInfo, sizeof(PORT_CONFIGURATION_INFORMATION));

        /* Initialize the struct */
        ConfigInfo->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
        ConfigInfo->AdapterInterfaceType = HwInitData->AdapterInterfaceType;
        ConfigInfo->InterruptMode = Latched;
        ConfigInfo->DmaChannel = SP_UNINITIALIZED_VALUE;
        ConfigInfo->DmaPort = SP_UNINITIALIZED_VALUE;
        ConfigInfo->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
        ConfigInfo->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS_PER_BUS;

        /* Store parameters */
        ConfigInfo->NeedPhysicalAddresses = HwInitData->NeedPhysicalAddresses;
        ConfigInfo->MapBuffers = HwInitData->MapBuffers;
        ConfigInfo->AutoRequestSense = HwInitData->AutoRequestSense;
        ConfigInfo->ReceiveEvent = HwInitData->ReceiveEvent;
        ConfigInfo->TaggedQueuing = HwInitData->TaggedQueuing;
        ConfigInfo->MultipleRequestPerLu = HwInitData->MultipleRequestPerLu;

        /* Get the disk usage */
        ConfigInfo->AtdiskPrimaryClaimed = FALSE; // FIXME
        ConfigInfo->AtdiskSecondaryClaimed = FALSE; // FIXME

        /* Initiator bus id is not set */
        for (Bus = 0; Bus < 8; Bus++)
            ConfigInfo->InitiatorBusId[Bus] = (CCHAR)SP_UNINITIALIZED_VALUE;
    }

    ConfigInfo->NumberOfPhysicalBreaks = 17;

    return STATUS_SUCCESS;
}

VOID
__cdecl
ScsiDebugPrint(
    IN ULONG DebugPrintLevel,
    IN PCCHAR DebugMessage,
    IN ...)
{
    va_list ap;
    CHAR Buffer[512];
    ULONG Length;

    if (DebugPrintLevel > 10)
        return;

    va_start(ap, DebugMessage);

    /* Construct a string */
    Length = _vsnprintf(Buffer, 512, DebugMessage, ap);

    /* Check if we went past the buffer */
    if (Length == MAXULONG)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\0';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    /* Print the message */
    TRACE("%s", Buffer);

    /* Cleanup */
    va_end(ap);
}

VOID
NTAPI
ScsiPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus)
{
    // FIXME
    UNIMPLEMENTED;
}

#undef ScsiPortConvertPhysicalAddressToUlong
ULONG
NTAPI
ScsiPortConvertPhysicalAddressToUlong(
    IN SCSI_PHYSICAL_ADDRESS Address)
{
    return Address.LowPart;
}

SCSI_PHYSICAL_ADDRESS
NTAPI
ScsiPortConvertUlongToPhysicalAddress(
    IN ULONG_PTR UlongAddress)
{
    SCSI_PHYSICAL_ADDRESS Address;

    Address.QuadPart = UlongAddress;
    return Address;
}

VOID
NTAPI
ScsiPortFlushDma(
    IN PVOID DeviceExtension)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress)
{
    // Nothing to do
}

ULONG
NTAPI
ScsiPortGetBusData(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length)
{
    return HalGetBusDataByOffset(BusDataType, SystemIoBusNumber, SlotNumber, Buffer, 0, Length);
}

PVOID
NTAPI
ScsiPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace)
{
    PHYSICAL_ADDRESS TranslatedAddress;
    ULONG AddressSpace;

    AddressSpace = (ULONG)InIoSpace;
    if (HalTranslateBusAddress(BusType,
                               SystemIoBusNumber,
                               IoAddress,
                               &AddressSpace,
                               &TranslatedAddress) == FALSE)
    {
        return NULL;
    }

    /* I/O space */
    if (AddressSpace != 0)
        return (PVOID)TranslatedAddress.u.LowPart;

    // FIXME
    UNIMPLEMENTED;
    return (PVOID)IoAddress.LowPart;
}

PVOID
NTAPI
ScsiPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun)
{
    // FIXME
    UNIMPLEMENTED;
    return NULL;
}

SCSI_PHYSICAL_ADDRESS
NTAPI
ScsiPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN PVOID VirtualAddress,
    OUT ULONG *Length)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    SCSI_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG BufferLength = 0;
    ULONG Offset;

    TRACE("ScsiPortGetPhysicalAddress(%p %p %p %p)\n",
        HwDeviceExtension, Srb, VirtualAddress, Length);

    DeviceExtension = ((PSCSI_PORT_DEVICE_EXTENSION)HwDeviceExtension) - 1;

    if (Srb == NULL || Srb->SenseInfoBuffer == VirtualAddress)
    {
        /* Simply look it up in the allocated common buffer */
        Offset = (PUCHAR)VirtualAddress - (PUCHAR)DeviceExtension->SrbExtensionBuffer;

        BufferLength = DeviceExtension->CommonBufferLength - Offset;
        PhysicalAddress.QuadPart = Offset;
    }
    else
    {
        /* Nothing */
        PhysicalAddress.QuadPart = (LONGLONG)(SP_UNINITIALIZED_VALUE);
    }

    *Length = BufferLength;
    return PhysicalAddress;
}

PSCSI_REQUEST_BLOCK
NTAPI
ScsiPortGetSrb(
    IN PVOID DeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag)
{
    // FIXME
    UNIMPLEMENTED;
    return NULL;
}

static
NTSTATUS
SpiAllocateCommonBuffer(
    IN OUT PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG NonCachedSize)
{
    PVOID CommonBuffer;
    ULONG CommonBufferLength, BufSize;

    /* If size is 0, set it to 16 */
    if (!DeviceExtension->SrbExtensionSize)
        DeviceExtension->SrbExtensionSize = 16;

    /* Calculate size */
    BufSize = DeviceExtension->SrbExtensionSize;

    /* Round it */
    BufSize = (BufSize + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    /* Sum up into the total common buffer length, and round it to page size */
    CommonBufferLength =
        ROUND_TO_PAGES(NonCachedSize);

    /* Allocate it */
    if (!DeviceExtension->AdapterObject)
    {
        /* From nonpaged pool if there is no DMA */
        CommonBuffer = ExAllocatePool(NonPagedPool, CommonBufferLength);
    }
    else
    {
        /* Perform a full request since we have a DMA adapter*/
        UNIMPLEMENTED;
        CommonBuffer = NULL;
    }

    /* Fail in case of error */
    if (!CommonBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero it */
    RtlZeroMemory(CommonBuffer, CommonBufferLength);

    /* Store its size in Device Extension */
    DeviceExtension->CommonBufferLength = CommonBufferLength;

    /* SrbExtension buffer is located at the beginning of the buffer */
    DeviceExtension->SrbExtensionBuffer = CommonBuffer;

    /* Non-cached extension buffer is located at the end of
       the common buffer */
    if (NonCachedSize)
    {
        CommonBufferLength -=  NonCachedSize;
        DeviceExtension->NonCachedExtension = (PUCHAR)CommonBuffer + CommonBufferLength;
    }
    else
    {
        DeviceExtension->NonCachedExtension = NULL;
    }

    return STATUS_SUCCESS;
}

PVOID
NTAPI
ScsiPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    DEVICE_DESCRIPTION DeviceDescription;
    ULONG MapRegistersCount;
    NTSTATUS Status;

    TRACE("ScsiPortGetUncachedExtension(%p %p %lu)\n",
        HwDeviceExtension, ConfigInfo, NumberOfBytes);

    DeviceExtension = ((PSCSI_PORT_DEVICE_EXTENSION)HwDeviceExtension) - 1;

    /* Check for allocated common DMA buffer */
    if (DeviceExtension->SrbExtensionBuffer != NULL)
    {
        return NULL;
    }

    /* Check for DMA adapter object */
    if (DeviceExtension->AdapterObject == NULL)
    {
        /* Initialize DMA adapter description */
        RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

        DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        DeviceDescription.Master = ConfigInfo->Master;
        DeviceDescription.ScatterGather = ConfigInfo->ScatterGather;
        DeviceDescription.DemandMode = ConfigInfo->DemandMode;
        DeviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
        DeviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
        DeviceDescription.DmaChannel = ConfigInfo->DmaChannel;
        DeviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
        DeviceDescription.DmaWidth = ConfigInfo->DmaWidth;
        DeviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
        DeviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
        DeviceDescription.DmaPort = ConfigInfo->DmaPort;

        /* Get a DMA adapter object */
#if 0
        DeviceExtension->AdapterObject =
            HalGetAdapter(&DeviceDescription, &MapRegistersCount);

        /* Fail in case of error */
        if (DeviceExtension->AdapterObject == NULL)
        {
            return NULL;
        }
#else
        MapRegistersCount = 0;
#endif

        /* Set number of physical breaks */
        if (ConfigInfo->NumberOfPhysicalBreaks != 0 &&
            MapRegistersCount > ConfigInfo->NumberOfPhysicalBreaks)
        {
            DeviceExtension->PortCapabilities.MaximumPhysicalPages =
                ConfigInfo->NumberOfPhysicalBreaks;
        }
        else
        {
            DeviceExtension->PortCapabilities.MaximumPhysicalPages = MapRegistersCount;
        }
    }

    /* Update Srb extension size */
    if (DeviceExtension->SrbExtensionSize != ConfigInfo->SrbExtensionSize)
        DeviceExtension->SrbExtensionSize = ConfigInfo->SrbExtensionSize;

    /* Allocate a common DMA buffer */
    Status = SpiAllocateCommonBuffer(DeviceExtension, NumberOfBytes);

    if (!NT_SUCCESS(Status))
    {
        TRACE("SpiAllocateCommonBuffer() failed with Status = 0x%08X!\n", Status);
        return NULL;
    }

    return DeviceExtension->NonCachedExtension;
}

PVOID
NTAPI
ScsiPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
    // FIXME
    UNIMPLEMENTED;
    return NULL;
}

static
VOID
SpiScanDevice(
    IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    IN PCHAR ArcName,
    IN ULONG ScsiBus,
    IN ULONG TargetId,
    IN ULONG Lun)
{
    ULONG FileId, i;
    ARC_STATUS Status;
    NTSTATUS ret;
    struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer;
    CHAR PartitionName[64];

    /* Register device with partition(0) suffix */
    RtlStringCbPrintfA(PartitionName, sizeof(PartitionName), "%spartition(0)", ArcName);
    FsRegisterDevice(PartitionName, &DiskVtbl);

    /* Read device partition table */
    Status = ArcOpen(PartitionName, OpenReadOnly, &FileId);
    if (Status == ESUCCESS)
    {
        ret = HALDISPATCH->HalIoReadPartitionTable((PDEVICE_OBJECT)FileId, 512, FALSE, &PartitionBuffer);
        if (NT_SUCCESS(ret))
        {
            for (i = 0; i < PartitionBuffer->PartitionCount; i++)
            {
                if (PartitionBuffer->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED)
                {
                    RtlStringCbPrintfA(PartitionName,
                                       sizeof(PartitionName),
                                       "%spartition(%lu)",
                                       ArcName,
                                       PartitionBuffer->PartitionEntry[i].PartitionNumber);
                    FsRegisterDevice(PartitionName, &DiskVtbl);
                }
            }
            ExFreePool(PartitionBuffer);
        }
        ArcClose(FileId);
    }
}

static
VOID
SpiScanAdapter(
    IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    IN ULONG ScsiBus,
    IN UCHAR PathId)
{
    CHAR ArcName[64];
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    INQUIRYDATA InquiryBuffer;
    UCHAR TargetId;
    UCHAR Lun;

    if (!DeviceExtension->HwResetBus(DeviceExtension->MiniPortDeviceExtension, PathId))
    {
        return;
    }

    /* Remember the extension */
    ScsiDeviceExtensions[ScsiBus] = DeviceExtension;

    for (TargetId = 0; TargetId < DeviceExtension->MaxTargedIds; TargetId++)
    {
        Lun = 0;
        do
        {
            TRACE("Scanning SCSI device %d.%d.%d\n",
                ScsiBus, TargetId, Lun);

            Srb = ExAllocatePool(PagedPool, sizeof(SCSI_REQUEST_BLOCK));
            if (!Srb)
                break;
            RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
            Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
            Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            Srb->PathId = PathId;
            Srb->TargetId = TargetId;
            Srb->Lun = Lun;
            Srb->CdbLength = 6;
            Srb->SrbFlags = SRB_FLAGS_DATA_IN;
            Srb->DataTransferLength = INQUIRYDATABUFFERSIZE;
            Srb->TimeOutValue = 5; /* in seconds */
            Srb->DataBuffer = &InquiryBuffer;
            Cdb = (PCDB)Srb->Cdb;
            Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
            Cdb->CDB6INQUIRY.LogicalUnitNumber = Srb->Lun;
            Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)Srb->DataTransferLength;
            if (!SpiSendSynchronousSrb(DeviceExtension, Srb))
            {
                /* Don't check next LUNs */
                break;
            }

            /* Device exists, create its ARC name */
            if (InquiryBuffer.RemovableMedia)
            {
                sprintf(ArcName, "scsi(%ld)cdrom(%d)fdisk(%d)",
                    ScsiBus, TargetId, Lun);
                FsRegisterDevice(ArcName, &DiskVtbl);
            }
            else
            {
                sprintf(ArcName, "scsi(%ld)disk(%d)rdisk(%d)",
                    ScsiBus, TargetId, Lun);
                /* Now, check if it has partitions */
                SpiScanDevice(DeviceExtension, ArcName, PathId, TargetId, Lun);
            }

            /* Check next LUN */
            Lun++;
        } while (Lun < SCSI_MAXIMUM_LOGICAL_UNITS);
    }
}

static
VOID
SpiResourceToConfig(
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor,
    IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig)
{
    PACCESS_RANGE AccessRange;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialData;
    ULONG RangeNumber;
    ULONG Index;

    RangeNumber = 0;

    /* Loop through all entries */
    for (Index = 0; Index < ResourceDescriptor->PartialResourceList.Count; Index++)
    {
        PartialData = &ResourceDescriptor->PartialResourceList.PartialDescriptors[Index];

        switch (PartialData->Type)
        {
        case CmResourceTypePort:
            /* Copy access ranges */
            if (RangeNumber < HwInitializationData->NumberOfAccessRanges)
            {
                TRACE("Got port at 0x%I64x, len 0x%x\n",
                    PartialData->u.Port.Start.QuadPart, PartialData->u.Port.Length);
                AccessRange = &((*(PortConfig->AccessRanges))[RangeNumber]);

                AccessRange->RangeStart = PartialData->u.Port.Start;
                AccessRange->RangeLength = PartialData->u.Port.Length;

                AccessRange->RangeInMemory = FALSE;
                RangeNumber++;
            }
            break;

        case CmResourceTypeMemory:
            /* Copy access ranges */
            if (RangeNumber < HwInitializationData->NumberOfAccessRanges)
            {
                TRACE("Got memory at 0x%I64x, len 0x%x\n",
                    PartialData->u.Memory.Start.QuadPart, PartialData->u.Memory.Length);
                AccessRange = &((*(PortConfig->AccessRanges))[RangeNumber]);

                AccessRange->RangeStart = PartialData->u.Memory.Start;
                AccessRange->RangeLength = PartialData->u.Memory.Length;

                AccessRange->RangeInMemory = TRUE;
                RangeNumber++;
            }
            break;

        case CmResourceTypeInterrupt:
            /* Copy interrupt data */
            TRACE("Got interrupt level %d, vector %d\n",
                PartialData->u.Interrupt.Level, PartialData->u.Interrupt.Vector);
            PortConfig->BusInterruptLevel = PartialData->u.Interrupt.Level;
            PortConfig->BusInterruptVector = PartialData->u.Interrupt.Vector;

            /* Set interrupt mode accordingly to the resource */
            if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
            {
                PortConfig->InterruptMode = Latched;
            }
            else if (PartialData->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)
            {
                PortConfig->InterruptMode = LevelSensitive;
            }
            break;

        case CmResourceTypeDma:
            TRACE("Got DMA channel %d, port %d\n",
                PartialData->u.Dma.Channel, PartialData->u.Dma.Port);
            PortConfig->DmaChannel = PartialData->u.Dma.Channel;
            PortConfig->DmaPort = PartialData->u.Dma.Port;
            break;
        }
    }
}

static
BOOLEAN
SpiGetPciConfigData(
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
    IN ULONG BusNumber,
    IN OUT PPCI_SLOT_NUMBER NextSlotNumber)
{
    PCI_COMMON_CONFIG PciConfig;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG DataSize;
    ULONG DeviceNumber;
    ULONG FunctionNumber;
    CHAR VendorIdString[8];
    CHAR DeviceIdString[8];
    PCM_RESOURCE_LIST ResourceList = NULL;
    NTSTATUS Status;

    SlotNumber.u.AsULONG = 0;

    /* Loop through all devices */
    for (DeviceNumber = NextSlotNumber->u.bits.DeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
        SlotNumber.u.bits.DeviceNumber = DeviceNumber;

        /* Loop through all functions */
        for (FunctionNumber = NextSlotNumber->u.bits.FunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
        {
            SlotNumber.u.bits.FunctionNumber = FunctionNumber;

            /* Get PCI config bytes */
            DataSize = HalGetBusDataByOffset(
                PCIConfiguration,
                BusNumber,
                SlotNumber.u.AsULONG,
                &PciConfig,
                0,
                sizeof(ULONG));

            /* If result of HalGetBusData is 0, then the bus is wrong */
            if (DataSize == 0)
                return FALSE;

            /* If result is PCI_INVALID_VENDORID, then this device has no more
               "Functions" */
            if (PciConfig.VendorID == PCI_INVALID_VENDORID)
                break;

            sprintf(VendorIdString, "%04hx", PciConfig.VendorID);
            sprintf(DeviceIdString, "%04hx", PciConfig.DeviceID);

            if (_strnicmp(VendorIdString, HwInitializationData->VendorId, HwInitializationData->VendorIdLength) ||
                _strnicmp(DeviceIdString, HwInitializationData->DeviceId, HwInitializationData->DeviceIdLength))
            {
                /* It is not our device */
                continue;
            }

            TRACE( "Found device 0x%04hx 0x%04hx at %1lu %2lu %1lu\n",
                PciConfig.VendorID, PciConfig.DeviceID,
                BusNumber,
                SlotNumber.u.bits.DeviceNumber, SlotNumber.u.bits.FunctionNumber);

            Status = HalAssignSlotResources(NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            PCIBus,
                                            BusNumber,
                                            SlotNumber.u.AsULONG,
                                            &ResourceList);

            if (!NT_SUCCESS(Status))
                break;

            /* Create configuration information */
            SpiResourceToConfig(HwInitializationData,
                                ResourceList->List,
                                PortConfig);

            /* Free the resource list */
            ExFreePool(ResourceList);

            /* Set dev & fn numbers */
            NextSlotNumber->u.bits.DeviceNumber = DeviceNumber;
            NextSlotNumber->u.bits.FunctionNumber = FunctionNumber + 1;

            /* Save the slot number */
            PortConfig->SlotNumber = SlotNumber.u.AsULONG;

            return TRUE;
        }
        NextSlotNumber->u.bits.FunctionNumber = 0;
    }

    NextSlotNumber->u.bits.DeviceNumber = 0;

    return FALSE;
}

ULONG
NTAPI
ScsiPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext OPTIONAL)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    ULONG DeviceExtensionSize;
    PORT_CONFIGURATION_INFORMATION PortConfig;
    BOOLEAN Again;
    BOOLEAN FirstConfigCall = TRUE;
    PCI_SLOT_NUMBER SlotNumber;
    UCHAR ScsiBus;
    NTSTATUS Status;

    if (HwInitializationData->HwInitializationDataSize != sizeof(HW_INITIALIZATION_DATA))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check params for validity */
    if ((HwInitializationData->HwInitialize == NULL) ||
        (HwInitializationData->HwStartIo == NULL) ||
        (HwInitializationData->HwInterrupt == NULL) ||
        (HwInitializationData->HwFindAdapter == NULL) ||
        (HwInitializationData->HwResetBus == NULL))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Zero starting slot number */
    SlotNumber.u.AsULONG = 0;

    while (TRUE)
    {
        Again = FALSE;

        DeviceExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) + HwInitializationData->DeviceExtensionSize;
        DeviceExtension = FrLdrTempAlloc(DeviceExtensionSize, TAG_SCSI_DEVEXT);
        if (!DeviceExtension)
        {
            return STATUS_NO_MEMORY;
        }
        RtlZeroMemory(DeviceExtension, DeviceExtensionSize);
        DeviceExtension->InterruptFlags = SCSI_PORT_NEXT_REQUEST_READY;
        DeviceExtension->HwInitialize = HwInitializationData->HwInitialize;
        DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
        DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
        DeviceExtension->HwResetBus = HwInitializationData->HwResetBus;
        DeviceExtension->MiniPortDeviceExtension = (PVOID)(DeviceExtension + 1);

        Status = SpiCreatePortConfig(DeviceExtension,
                                     HwInitializationData,
                                     &PortConfig,
                                     FirstConfigCall);
        if (Status != STATUS_SUCCESS)
        {
            FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
            return Status;
        }

        PortConfig.NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;
        PortConfig.AccessRanges = FrLdrTempAlloc(sizeof(ACCESS_RANGE) * HwInitializationData->NumberOfAccessRanges,
                                                 TAG_SCSI_ACCESS_RANGES);
        if (!PortConfig.AccessRanges)
        {
           FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
           return STATUS_NO_MEMORY;
        }
        RtlZeroMemory(PortConfig.AccessRanges, sizeof(ACCESS_RANGE) * HwInitializationData->NumberOfAccessRanges);

        /* Search for matching PCI device */
        if ((HwInitializationData->AdapterInterfaceType == PCIBus) &&
            (HwInitializationData->VendorIdLength > 0) &&
            (HwInitializationData->VendorId != NULL) &&
            (HwInitializationData->DeviceIdLength > 0) &&
            (HwInitializationData->DeviceId != NULL))
        {
            PortConfig.BusInterruptLevel = 0;

            /* Get PCI device data */
            TRACE("VendorId '%.*s'  DeviceId '%.*s'\n",
                HwInitializationData->VendorIdLength,
                HwInitializationData->VendorId,
                HwInitializationData->DeviceIdLength,
                HwInitializationData->DeviceId);

            if (!SpiGetPciConfigData(HwInitializationData,
                                     &PortConfig,
                                     0, /* FIXME */
                                     &SlotNumber))
            {
                /* Continue to the next bus, nothing here */
                FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
                return STATUS_INTERNAL_ERROR;
            }

            if (!PortConfig.BusInterruptLevel)
            {
                /* Bypass this slot, because no interrupt was assigned */
                FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
                return STATUS_INTERNAL_ERROR;
            }
        }

        if (HwInitializationData->HwFindAdapter(
             DeviceExtension->MiniPortDeviceExtension,
             HwContext,
             NULL,
             NULL,
             &PortConfig,
             &Again) != SP_RETURN_FOUND)
        {
            FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
            return STATUS_INTERNAL_ERROR;
        }

        /* Copy all stuff which we ever need from PortConfig to the DeviceExtension */
        if (PortConfig.MaximumNumberOfTargets > SCSI_MAXIMUM_TARGETS_PER_BUS)
            DeviceExtension->MaxTargedIds = SCSI_MAXIMUM_TARGETS_PER_BUS;
        else
            DeviceExtension->MaxTargedIds = PortConfig.MaximumNumberOfTargets;

        DeviceExtension->BusNum = PortConfig.SystemIoBusNumber;

        TRACE("Adapter found: buses = %d, targets = %d\n",
                 PortConfig.NumberOfBuses, DeviceExtension->MaxTargedIds);

        /* Initialize adapter */
        if (!DeviceExtension->HwInitialize(DeviceExtension->MiniPortDeviceExtension))
        {
            FrLdrTempFree(DeviceExtension, TAG_SCSI_DEVEXT);
            return STATUS_INTERNAL_ERROR;
        }

        /* Scan bus */
        for (ScsiBus = 0; ScsiBus < PortConfig.NumberOfBuses; ScsiBus++)
        {
            SpiScanAdapter(DeviceExtension, PortConfig.SystemIoBusNumber, ScsiBus);
            PortConfig.SystemIoBusNumber++;
        }

        FirstConfigCall = FALSE;
        if (!Again)
        {
            break;
        }
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
ScsiPortIoMapTransfer(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID LogicalAddress,
    IN ULONG Length)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length)
{
    RtlMoveMemory(WriteBuffer, ReadBuffer, Length);
}

VOID
__cdecl
ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    IN ...)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_REQUEST_BLOCK Srb;
    va_list ap;

    DeviceExtension = ((PSCSI_PORT_DEVICE_EXTENSION)HwDeviceExtension) - 1;

    va_start(ap, HwDeviceExtension);

    switch (NotificationType)
    {
        case RequestComplete:
            /* Mask the SRB as completed */
            Srb = va_arg(ap, PSCSI_REQUEST_BLOCK);
            Srb->SrbFlags &= ~SRB_FLAGS_IS_ACTIVE;
            break;

        case NextRequest:
            /* Say that device is ready */
            DeviceExtension->InterruptFlags |= SCSI_PORT_NEXT_REQUEST_READY;
            break;

        default:
            // FIXME
            UNIMPLEMENTED;
    }

    va_end(ap);
}

VOID
NTAPI
ScsiPortReadPortBufferUchar(
    IN PUCHAR Port,
    OUT PUCHAR Buffer,
    IN ULONG Count)
{
    __inbytestring(H2I(Port), Buffer, Count);
}

VOID
NTAPI
ScsiPortReadPortBufferUlong(
    IN PULONG Port,
    OUT PULONG Buffer,
    IN ULONG Count)
{
    __indwordstring(H2I(Port), Buffer, Count);
}

VOID
NTAPI
ScsiPortReadPortBufferUshort(
    IN PUSHORT Port,
    OUT PUSHORT Buffer,
    IN ULONG Count)
{
    __inwordstring(H2I(Port), Buffer, Count);
}

UCHAR
NTAPI
ScsiPortReadPortUchar(
    IN PUCHAR Port)
{
    TRACE("ScsiPortReadPortUchar(%p)\n", Port);

    return READ_PORT_UCHAR(Port);
}

ULONG
NTAPI
ScsiPortReadPortUlong(
    IN PULONG Port)
{
    return READ_PORT_ULONG(Port);
}

USHORT
NTAPI
ScsiPortReadPortUshort(
    IN PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}

VOID
NTAPI
ScsiPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

UCHAR
NTAPI
ScsiPortReadRegisterUchar(
    IN PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}

ULONG
NTAPI
ScsiPortReadRegisterUlong(
    IN PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}

USHORT
NTAPI
ScsiPortReadRegisterUshort(
    IN PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}

ULONG
NTAPI
ScsiPortSetBusDataByOffset(
    IN PVOID DeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length)
{
    // FIXME
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
ScsiPortStallExecution(
    IN ULONG Delay)
{
    KeStallExecutionProcessor(Delay);
}

BOOLEAN
NTAPI
ScsiPortValidateRange(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN SCSI_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace)
{
    // FIXME
    UNIMPLEMENTED;
    return TRUE;
}

#if 0
// ScsiPortWmi*
#endif


VOID
NTAPI
ScsiPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    __outbytestring(H2I(Port), Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count)
{
    __outdwordstring(H2I(Port), Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    __outwordstring(H2I(Port), Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

VOID
NTAPI
ScsiPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}

VOID
NTAPI
ScsiPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    // FIXME
    UNIMPLEMENTED;
}

VOID
NTAPI
ScsiPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

extern char __ImageBase;

ULONG
LoadBootDeviceDriver(VOID)
{
    PIMAGE_NT_HEADERS NtHeaders;
    LIST_ENTRY ModuleListHead;
    PIMAGE_IMPORT_DESCRIPTOR ImportTable;
    ULONG ImportTableSize;
    PLDR_DATA_TABLE_ENTRY BootDdDTE, FreeldrDTE;
    CHAR NtBootDdPath[MAX_PATH];
    PVOID ImageBase = NULL;
    ULONG (NTAPI *EntryPoint)(IN PVOID DriverObject, IN PVOID RegistryPath);
    BOOLEAN Success;

    // FIXME: Must be done *INSIDE* the HAL!
#ifdef _M_IX86
    HalpInitializePciStubs();
    HalpInitBusHandler();
#endif

    /* Initialize the loaded module list */
    InitializeListHead(&ModuleListHead);

    /* Create full ntbootdd.sys path */
    strcpy(NtBootDdPath, FrLdrBootPath);
    strcat(NtBootDdPath, "\\NTBOOTDD.SYS");

    /* Load file */
    Success = PeLdrLoadImage(NtBootDdPath, LoaderBootDriver, &ImageBase);
    if (!Success)
    {
        /* That's OK, file simply doesn't exist */
        return ESUCCESS;
    }

    /* Allocate a DTE for ntbootdd */
    Success = PeLdrAllocateDataTableEntry(&ModuleListHead, "ntbootdd.sys",
                                          "NTBOOTDD.SYS", ImageBase, &BootDdDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        MmFreeMemory(ImageBase);
        return EIO;
    }

    /* Add the PE part of freeldr.sys to the list of loaded executables, it
       contains ScsiPort* exports, imported by ntbootdd.sys */
    Success = PeLdrAllocateDataTableEntry(&ModuleListHead, "scsiport.sys",
                                          "FREELDR.SYS", &__ImageBase, &FreeldrDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        PeLdrFreeDataTableEntry(BootDdDTE);
        MmFreeMemory(ImageBase);
        return EIO;
    }

    /* Fix imports */
    Success = PeLdrScanImportDescriptorTable(&ModuleListHead, "", BootDdDTE);
    if (!Success)
    {
        /* Cleanup and bail out */
        PeLdrFreeDataTableEntry(FreeldrDTE);
        PeLdrFreeDataTableEntry(BootDdDTE);
        MmFreeMemory(ImageBase);
        return EIO;
    }

    /* Now unlink the DTEs, they won't be valid later */
    RemoveEntryList(&BootDdDTE->InLoadOrderLinks);
    RemoveEntryList(&FreeldrDTE->InLoadOrderLinks);

    /* Change imports to PA */
    ImportTable = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(VaToPa(BootDdDTE->DllBase),
        TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportTableSize);
    for (;(ImportTable->Name != 0) && (ImportTable->FirstThunk != 0);ImportTable++)
    {
        PIMAGE_THUNK_DATA ThunkData = (PIMAGE_THUNK_DATA)VaToPa(RVA(BootDdDTE->DllBase, ImportTable->FirstThunk));

        while (((PIMAGE_THUNK_DATA)ThunkData)->u1.AddressOfData != 0)
        {
            ThunkData->u1.Function = (ULONG)VaToPa((PVOID)ThunkData->u1.Function);
            ThunkData++;
        }
    }

    /* Relocate image to PA */
    NtHeaders = RtlImageNtHeader(VaToPa(BootDdDTE->DllBase));
    if (!NtHeaders)
        return EIO;
    Success = (BOOLEAN)LdrRelocateImageWithBias(VaToPa(BootDdDTE->DllBase),
                                                NtHeaders->OptionalHeader.ImageBase - (ULONG_PTR)BootDdDTE->DllBase,
                                                "FreeLdr",
                                                TRUE,
                                                TRUE, /* In case of conflict still return success */
                                                FALSE);
    if (!Success)
        return EIO;

    /* Call the entrypoint */
    EntryPoint = VaToPa(BootDdDTE->EntryPoint);
    (*EntryPoint)(NULL, NULL);

    return ESUCCESS;
}

/* EOF */
