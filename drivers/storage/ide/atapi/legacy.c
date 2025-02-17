/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Legacy (non-PnP) IDE controllers support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

typedef struct _ATA_LEGACY_CHANNEL
{
    ULONG IoBase;
    ULONG Irq;
} ATA_LEGACY_CHANNEL, *PATA_LEGACY_CHANNEL;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("INIT")
VOID
AtaLegacyFetchIdentify(
    _In_ PIDE_REGISTERS Registers)
{
    UCHAR Buffer[512];

    ATA_READ_BLOCK_16((PUSHORT)Registers->Data,
                      (PUSHORT)Buffer,
                      sizeof(Buffer) / sizeof(USHORT));
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyIdentifyDevice(
    _In_ PIDE_REGISTERS Registers,
    _In_ ULONG DeviceNumber,
    _In_ UCHAR Command)
{
    UCHAR IdeStatus;

    /* Select the device */
    ATA_WRITE(Registers->Device, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
    ATA_IO_WAIT();

    IdeStatus = ATA_READ(Registers->Status);
    if (!ATA_WAIT_ON_BUSY(Registers, &IdeStatus, ATA_TIME_BUSY_IDENTIFY))
    {
        TRACE("Timeout, status 0x%02x\n", IdeStatus);
        return FALSE;
    }

    ATA_WRITE(Registers->Features, 0);
    ATA_WRITE(Registers->SectorCount, 0);
    ATA_WRITE(Registers->LbaLow, 0);
    ATA_WRITE(Registers->LbaMid, 0);
    ATA_WRITE(Registers->LbaHigh, 0);
    ATA_WRITE(Registers->Command, Command);

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    /* Now wait for busy to clear */
    IdeStatus = ATA_READ(Registers->Status);
    if (!ATA_WAIT_ON_BUSY(Registers, &IdeStatus, ATA_TIME_BUSY_IDENTIFY))
    {
        TRACE("Timeout, status 0x%02x\n", IdeStatus);
        return FALSE;
    }
    if (IdeStatus & ((IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT)))
    {
        TRACE("Command 0x%02x aborted, status 0x%02x, error 0x%02x\n",
              Command,
              IdeStatus,
              ATA_READ(Registers->Error));
        return FALSE;
    }
    if (!(IdeStatus & IDE_STATUS_DRQ))
    {
        ERR("DRQ not set, status 0x%02x\n", IdeStatus);
        return FALSE;
    }

    /* Receive parameter information from the device to complete the data transfer */
    AtaLegacyFetchIdentify(Registers);

    /* All data has been transferred, wait for DRQ to clear */
    IdeStatus = ATA_READ(Registers->Status);
    if (!ATA_WAIT_FOR_IDLE(Registers, &IdeStatus))
    {
        ERR("DRQ not cleared, status 0x%02x\n", IdeStatus);
        return FALSE;
    }

    return TRUE;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyPerformSoftwareReset(
    _In_ PIDE_REGISTERS Registers,
    _In_ ULONG DeviceNumber)
{
    UCHAR IdeStatus;
    ULONG i;

    /* Perform a software reset */
    ATA_WRITE(Registers->Control, IDE_DC_RESET_CONTROLLER);
    KeStallExecutionProcessor(20);
    ATA_WRITE(Registers->Control, IDE_DC_DISABLE_INTERRUPTS);
    KeStallExecutionProcessor(20);

    /* The reset will cause the master device to be selected */
    if (DeviceNumber != 0)
    {
        for (i = ATA_TIME_RESET_SELECT; i > 0; --i)
        {
            /* Select the device again */
            ATA_WRITE(Registers->Device, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
            ATA_IO_WAIT();

            /* Check whether the selection was successful */
            ATA_WRITE(Registers->ByteCountLow, 0xAA);
            ATA_WRITE(Registers->ByteCountLow, 0x55);
            ATA_WRITE(Registers->ByteCountLow, 0xAA);
            if (ATA_READ(Registers->ByteCountLow) == 0xAA)
                break;

            KeStallExecutionProcessor(10);
        }
        if (i == 0)
        {
            TRACE("Selection timeout\n");
            return FALSE;
        }
    }

    /* Now wait for busy to clear */
    IdeStatus = ATA_READ(Registers->Status);
    if (!ATA_WAIT_ON_BUSY(Registers, &IdeStatus, ATA_TIME_BUSY_RESET))
    {
        TRACE("Timeout, status 0x%02x\n", IdeStatus);
        return FALSE;
    }

    return FALSE;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyFindAtaDevice(
    _In_ PIDE_REGISTERS Registers,
    _In_ ULONG DeviceNumber)
{
    UCHAR IdeStatus, SignatureLow, SignatureHigh;

    /* Select the device */
    ATA_WRITE(Registers->Device, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
    ATA_IO_WAIT();

    /* Do a quick check first */
    IdeStatus = ATA_READ(Registers->Status);
    if (IdeStatus == 0xFF || IdeStatus == 0x7F)
        return FALSE;

    /* Look at controller */
    ATA_WRITE(Registers->ByteCountLow, 0x55);
    ATA_WRITE(Registers->ByteCountLow, 0xAA);
    ATA_WRITE(Registers->ByteCountLow, 0x55);
    if (ATA_READ(Registers->ByteCountLow) != 0x55)
        return FALSE;
    ATA_WRITE(Registers->ByteCountHigh, 0xAA);
    ATA_WRITE(Registers->ByteCountHigh, 0x55);
    ATA_WRITE(Registers->ByteCountHigh, 0xAA);
    if (ATA_READ(Registers->ByteCountHigh) != 0xAA)
        return FALSE;

    /* Wait for busy to clear */
    if (!ATA_WAIT_ON_BUSY(Registers, &IdeStatus, ATA_TIME_BUSY_SELECT))
    {
        WARN("Device is busy, attempting to recover %02x\n", IdeStatus);

        if (!AtaLegacyPerformSoftwareReset(Registers, DeviceNumber))
        {
            ERR("Failed to reset device %02x\n", ATA_READ(Registers->Status));
            return FALSE;
        }
    }

    /* Check for ATA */
    if (AtaLegacyIdentifyDevice(Registers, DeviceNumber, IDE_COMMAND_IDENTIFY))
        return TRUE;

    SignatureLow = ATA_READ(Registers->SignatureLow);
    SignatureHigh = ATA_READ(Registers->SignatureHigh);

    TRACE("SL = 0x%02x, SH = 0x%02x\n", SignatureLow, SignatureHigh);

    /* Check for ATAPI */
    if (SignatureLow == 0x14 && SignatureHigh == 0xEB)
        return TRUE;

    /*
     * ATAPI devices abort the IDENTIFY command and return an ATAPI signature
     * in the task file registers. However the NEC CDR-260 drive doesn't return
     * the correct signature, but instead shows up with zeroes.
     * This drive also reports an ATA signature after device reset.
     * To overcome this behavior, we try the ATAPI IDENTIFY command.
     * It should be successfully completed or failed (aborted or time-out).
     */
    if (AtaLegacyIdentifyDevice(Registers, DeviceNumber, IDE_COMMAND_ATAPI_IDENTIFY))
        return TRUE;

    return FALSE;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyChannelPresent(
    _In_ PIDE_REGISTERS Registers)
{
    ULONG i;

    /* The on-board IDE interface is always assumed to be present */
    if (IsNEC_98)
        return TRUE;

    /*
     * The only reliable way to detect the legacy IDE channel
     * is to check for devices attached to the bus.
     */
    for (i = 0; i < CHANNEL_PCAT_MAX_DEVICES; ++i)
    {
        if (AtaLegacyFindAtaDevice(Registers, i))
        {
            TRACE("Found IDE device\n");
            return TRUE;
        }
    }

    return FALSE;
}

static
inline
CODE_SEG("INIT")
VOID
AtaLegacyMakePortResource(
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG IoBase,
    _In_ ULONG Length)
{
    Descriptor->Type = CmResourceTypePort;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
    Descriptor->u.Port.Start.u.LowPart = IoBase;
    Descriptor->u.Port.Length = Length;
}

static
inline
CODE_SEG("INIT")
VOID
AtaLegacyMakeInterruptResource(
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG Level)
{
    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
    Descriptor->u.Interrupt.Level = Level;
    Descriptor->u.Interrupt.Vector = Level;
    Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyChannelBuildResources(
    _In_ PATA_LEGACY_CHANNEL LegacyChannel,
    _Inout_ PCM_RESOURCE_LIST ResourceList)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG i;

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    if (IsNEC_98)
    {
        if (ConfigInfo->AtDiskPrimaryAddressClaimed || ConfigInfo->AtDiskSecondaryAddressClaimed)
            return FALSE;

#define CHANNEL_PC98_RESOURCE_COUNT    12
        /*
         * Create the resource list for the internal IDE interface:
         *
         * [ShareDisposition 1, Flags 11] IO:  Start 0:640, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:74C, Len 1
         * [ShareDisposition 1, Flags 1]  INT: Lev 9 Vec 9 Aff FFFFFFFF
         * [ShareDisposition 1, Flags 11] IO:  Start 0:642, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:644, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:646, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:648, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:64A, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:64C, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:64E, Len 1
         * [ShareDisposition 1, Flags 11] IO:  Start 0:432, Len 2
         * [ShareDisposition 1, Flags 11] IO:  Start 0:435, Len 1
         */
        AtaLegacyMakePortResource(Descriptor++, 0x640, 1);
        AtaLegacyMakePortResource(Descriptor++, 0x74C, 1);
        AtaLegacyMakeInterruptResource(Descriptor++, 9);
        for (i = 0; i < 7; ++i)
        {
            AtaLegacyMakePortResource(Descriptor++, 0x642 + i * 2, 1);
        }
        AtaLegacyMakePortResource(Descriptor++, 0x432, 2);
        AtaLegacyMakePortResource(Descriptor++, 0x435, 1);
    }
    else
    {
        if (LegacyChannel->IoBase == 0x1F0 && ConfigInfo->AtDiskPrimaryAddressClaimed)
            return FALSE;

        if (LegacyChannel->IoBase == 0x170 && ConfigInfo->AtDiskSecondaryAddressClaimed)
            return FALSE;

#define CHANNEL_PCAT_RESOURCE_COUNT    3
        /*
         * For example, the following resource list is created for the primary IDE channel:
         *
         * [ShareDisposition 1, Flags 11] IO:  Start 0:1F0, Len 8
         * [ShareDisposition 1, Flags 11] IO:  Start 0:3F6, Len 1
         * [ShareDisposition 1, Flags 1]  INT: Lev A Vec A Aff FFFFFFFF
         */
        AtaLegacyMakePortResource(Descriptor++, LegacyChannel->IoBase, 8);
        AtaLegacyMakePortResource(Descriptor++, LegacyChannel->IoBase + 0x206, 1);
        AtaLegacyMakeInterruptResource(Descriptor++, LegacyChannel->Irq);
    }

    return TRUE;
}

static
CODE_SEG("INIT")
VOID
AtaLegacyChannelTranslateResources(
    _Inout_ PCM_RESOURCE_LIST ResourceList,
    _In_ PUCHAR CommandPortBase,
    _In_ PUCHAR ControlPortBase)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    KIRQL Irql;

    Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];

    /* Move on to the interrupt descriptor */
    if (IsNEC_98)
    {
        Descriptor += 2;
    }
    else
    {
        Descriptor->u.Port.Start.QuadPart = (ULONG_PTR)CommandPortBase;
        ++Descriptor;

        Descriptor->u.Port.Start.QuadPart = (ULONG_PTR)ControlPortBase;
        ++Descriptor;
    }
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    Descriptor->u.Interrupt.Vector = HalGetInterruptVector(Isa,
                                                           0,
                                                           Descriptor->u.Interrupt.Level,
                                                           Descriptor->u.Interrupt.Vector,
                                                           &Irql,
                                                           &Descriptor->u.Interrupt.Affinity);
}

static
CODE_SEG("INIT")
PVOID
AtaTranslateBusAddress(
    _In_ ULONG Address,
    _In_ ULONG NumberOfBytes,
    _Out_ PBOOLEAN MappedAddress)
{
    ULONG AddressSpace;
    BOOLEAN Success;
    PHYSICAL_ADDRESS BusAddress, TranslatedAddress;

    BusAddress.QuadPart = Address;
    AddressSpace = 1; /* I/O space */
    Success = HalTranslateBusAddress(Isa,
                                     0,
                                     BusAddress,
                                     &AddressSpace,
                                     &TranslatedAddress);
    if (!Success)
        return NULL;

    /* I/O space */
    if (AddressSpace)
    {
        *MappedAddress = FALSE;
        return (PVOID)(ULONG_PTR)TranslatedAddress.QuadPart;
    }
    else
    {
        *MappedAddress = TRUE;
        return MmMapIoSpace(TranslatedAddress, NumberOfBytes, MmNonCached);
    }
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyClaimHardwareResources(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ULONG ResourceListSize)
{
    NTSTATUS Status;
    BOOLEAN ConflictDetected;

    Status = IoReportResourceForDetection(DriverObject,
                                          ResourceList,
                                          ResourceListSize,
                                          NULL,
                                          NULL,
                                          0,
                                          &ConflictDetected);
    /* HACK: We really need to fix a number of resource bugs in the kernel */
    if (IsNEC_98)
        return TRUE;

    if (!NT_SUCCESS(Status) || ConflictDetected)
        return FALSE;

    return TRUE;
}

static
CODE_SEG("INIT")
VOID
AtaLegacyReleaseHardwareResources(
    _In_ PDRIVER_OBJECT DriverObject)
{
    BOOLEAN Dummy;

    IoReportResourceForDetection(DriverObject, NULL, 0, NULL, NULL, 0, &Dummy);
}

static
CODE_SEG("INIT")
VOID
AtaLegacyDetectChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PATA_LEGACY_CHANNEL LegacyChannel,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ULONG ResourceListSize)
{
    NTSTATUS Status;
    BOOLEAN MappedAddress;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    IDE_REGISTERS Registers;
    ULONG Spare;

    TRACE("IDE Channel IO %lx, Irq %lu\n", LegacyChannel->IoBase, LegacyChannel->Irq);

    if (!AtaLegacyChannelBuildResources(LegacyChannel, ResourceList))
    {
        TRACE("Failed to build the resource list\n");
        return;
    }

    if (!AtaLegacyClaimHardwareResources(DriverObject, ResourceList, ResourceListSize))
    {
        TRACE("Failed to claim resources\n");
        return;
    }

    Status = STATUS_SUCCESS;
    MappedAddress = FALSE;

    if (IsNEC_98)
    {
        /* No translation required for the C-Bus I/O space */
        Registers.Data = (PVOID)0x640;
        Registers.Control = (PVOID)0x74C;

        Spare = 2;
    }
    else
    {
        Registers.Data = AtaTranslateBusAddress(LegacyChannel->IoBase,
                                                8,
                                                &MappedAddress);
        if (!Registers.Data)
        {
            TRACE("Failed to map command port\n");

            Status = STATUS_UNSUCCESSFUL;
            goto ReleaseResources;
        }

        Registers.Control = AtaTranslateBusAddress(LegacyChannel->IoBase + 0x206,
                                                   1,
                                                   &MappedAddress);
        if (!Registers.Control)
        {
            TRACE("Failed to map control port\n");

            Status = STATUS_UNSUCCESSFUL;
            goto ReleaseResources;
        }

        Spare = 1;
    }
    Registers.Error       = (PVOID)((ULONG_PTR)Registers.Data + 1 * Spare);
    Registers.SectorCount = (PVOID)((ULONG_PTR)Registers.Data + 2 * Spare);
    Registers.LbaLow      = (PVOID)((ULONG_PTR)Registers.Data + 3 * Spare);
    Registers.LbaMid      = (PVOID)((ULONG_PTR)Registers.Data + 4 * Spare);
    Registers.LbaHigh     = (PVOID)((ULONG_PTR)Registers.Data + 5 * Spare);
    Registers.Device      = (PVOID)((ULONG_PTR)Registers.Data + 6 * Spare);
    Registers.Status      = (PVOID)((ULONG_PTR)Registers.Data + 7 * Spare);

    if (!AtaLegacyChannelPresent(&Registers))
    {
        TRACE("No IDE devices found\n");
        Status = STATUS_UNSUCCESSFUL;
    }

ReleaseResources:
    AtaLegacyReleaseHardwareResources(DriverObject);

    if (!NT_SUCCESS(Status))
        goto Cleanup;

    PhysicalDeviceObject = NULL;
    Status = IoReportDetectedDevice(DriverObject,
                                    InterfaceTypeUndefined,
                                    (ULONG)-1,
                                    (ULONG)-1,
                                    ResourceList,
                                    NULL,
                                    0,
                                    &PhysicalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("IoReportDetectedDevice() failed with status 0x%lx\n", Status);
        goto Cleanup;
    }

    Status = AtaAddChannel(DriverObject, PhysicalDeviceObject, &ChanExt);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to add the legacy channel 0x%lx, status 0x%lx\n",
            LegacyChannel->IoBase, Status);
        goto Cleanup;
    }

    AtaLegacyChannelTranslateResources(ResourceList, Registers.Data, Registers.Control);

    Status = AtaFdoStartDevice(ChanExt, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to start the legacy channel 0x%lx, status 0x%lx\n",
            LegacyChannel->IoBase, Status);

        AtaFdoRemoveDevice(ChanExt, NULL, TRUE);
    }

Cleanup:
    if (MappedAddress)
    {
        if (Registers.Data)
            MmUnmapIoSpace(Registers.Data, 8);
        if (Registers.Control)
            MmUnmapIoSpace(Registers.Control, 1);
    }
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyShouldDetectChannels(VOID)
{
    NTSTATUS Status;
    ULONG KeyValue;
    BOOLEAN PerformDetection;
    HANDLE SoftKeyHandle, ParamsKeyHandle;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    UNICODE_STRING ParametersKeyName = RTL_CONSTANT_STRING(L"Parameters");
    static const WCHAR MapperKeyPath[] =
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Pnp";

    PAGED_CODE();

    /*
     * Read the firmware mapper key. If the firmware mapper is disabled,
     * it is the responsibility of PnP drivers (ACPI, PCI, and others)
     * to detect and enumerate IDE channels.
     */
    KeyValue = 0;
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"DisableFirmwareMapper";
    QueryTable[0].EntryContext = &KeyValue;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MapperKeyPath, QueryTable, NULL, NULL);
    if (NT_SUCCESS(Status) && (KeyValue != 0))
    {
        TRACE("Skipping legacy detection on a PnP system\n");
        return FALSE;
    }

    /* Open the driver's software key */
    Status = AtaOpenRegistryKey(&SoftKeyHandle,
                                NULL,
                                &AtapDriverRegistryPath,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the '%wZ' key, status 0x%lx\n", &AtapDriverRegistryPath, Status);
        return FALSE;
    }

    /* Open the 'Parameters' key */
    Status = AtaOpenRegistryKey(&ParamsKeyHandle,
                                SoftKeyHandle,
                                &ParametersKeyName,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the 'Parameters' key, status 0x%lx\n", Status);

        ZwClose(SoftKeyHandle);
        return FALSE;
    }

    /* Check whether it's the first time we detect IDE channels */
    KeyValue = 0;
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"LegacyDetection";
    QueryTable[0].EntryContext = &KeyValue;
    RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                           (PWSTR)ParamsKeyHandle,
                           QueryTable,
                           NULL,
                           NULL);
    PerformDetection = (KeyValue != 0);

    /* Don't detect devices again on subsequent boots after driver installation */
    if (PerformDetection)
    {
        KeyValue = 0;
        RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                              (PWSTR)ParamsKeyHandle,
                              L"LegacyDetection",
                              REG_DWORD,
                              &KeyValue,
                              sizeof(KeyValue));
    }

    ZwClose(ParamsKeyHandle);
    ZwClose(SoftKeyHandle);

    return PerformDetection;
}

CODE_SEG("INIT")
VOID
AtaDetectLegacyDevices(
    _In_ PDRIVER_OBJECT DriverObject)
{
    ATA_LEGACY_CHANNEL LegacyChannel[4 + 1] = { 0 };
    PCM_RESOURCE_LIST ResourceList;
    ULONG i, ListSize, ResourceCount;

    if (!AtaLegacyShouldDetectChannels())
        return;

    if (IsNEC_98)
    {
        /* Internal IDE interface */
        LegacyChannel[0].IoBase = 0x640;
        LegacyChannel[0].Irq = 9;

        ResourceCount = CHANNEL_PC98_RESOURCE_COUNT;
    }
    else
    {
        /* Primary IDE channel */
        LegacyChannel[0].IoBase = 0x1F0;
        LegacyChannel[0].Irq = 14;

        /* Secondary IDE channel */
        LegacyChannel[1].IoBase = 0x170;
        LegacyChannel[1].Irq = 15;

        /* Tertiary IDE channel */
        LegacyChannel[2].IoBase = 0x1E8;
        LegacyChannel[2].Irq = 11;

        /* Quaternary IDE channel */
        LegacyChannel[3].IoBase = 0x168;
        LegacyChannel[3].Irq = 10;

        ResourceCount = CHANNEL_PCAT_RESOURCE_COUNT;
    }

    ListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors) +
               sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * ResourceCount;

    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, ATAPORT_TAG);
    if (!ResourceList)
    {
        ERR("Failed to allocate the resource list\n");
        return;
    }
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].BusNumber = 0;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = ResourceCount;

    for (i = 0; LegacyChannel[i].IoBase; ++i)
    {
        AtaLegacyDetectChannel(DriverObject, &LegacyChannel[i], ResourceList, ListSize);
    }

    ExFreePoolWithTag(ResourceList, ATAPORT_TAG);
}
