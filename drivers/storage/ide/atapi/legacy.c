/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Legacy (non-PnP) IDE controllers support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("INIT")
BOOLEAN
AtaDeviceSignatureValid(
    _In_ PIDE_REGISTERS Registers)
{
    UCHAR SignatureLow, SignatureHigh;

    SignatureLow = ATA_READ(Registers->SignatureLow);
    SignatureHigh = ATA_READ(Registers->SignatureHigh);

    TRACE("SL = 0x%02x, SH = 0x%02x\n", SignatureLow, SignatureHigh);

    /* ATA signature */
    if (SignatureLow == 0x00 && SignatureHigh == 0x00)
        return TRUE;

    /* ATAPI signature */
    if (SignatureLow == 0x14 && SignatureHigh == 0xEB)
        return TRUE;

    return FALSE;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyIdentifyDevice(
    _In_ ULONG DeviceNumber,
    _In_ PIDE_REGISTERS Registers,
    _In_ BOOLEAN TryAtapi)
{
    IDENTIFY_DEVICE_DATA IdentifyDeviceData;
    UCHAR Status, ErrorCode;
    ULONG i;

    /* Select the device */
    ATA_WRITE(Registers->DriveSelect, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
    ATA_IO_WAIT();

    /*
     * Signature check after device reset.
     * Note that the NEC CDR-260 drive reports an ATA signature.
     */
    if (AtaDeviceSignatureValid(Registers))
        return TRUE;

    /*
     * One last check to make sure the IDE device is there: send the identify command,
     * it should be successfully completed or failed (aborted or time-out).
     */
    ATA_WRITE(Registers->Feature, 0);
    ATA_WRITE(Registers->SectorCount, 0);
    ATA_WRITE(Registers->LowLba, 0);
    ATA_WRITE(Registers->MidLba, 0);
    ATA_WRITE(Registers->HighLba, 0);
    ATA_WRITE(Registers->Command, TryAtapi ? IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY);

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    /* Wait for busy being cleared and DRQ or ERR bit being set */
    for (i = 0; i < 100000; ++i)
    {
        Status = ATA_READ(Registers->Status);
        if (Status == 0xFF || Status == 0x7F)
            return FALSE;
        if (!(Status & IDE_STATUS_BUSY))
            break;

        KeStallExecutionProcessor(10);
    }
    if (Status & IDE_STATUS_BUSY)
    {
        TRACE("Timeout, status 0x%02x\n", Status);
        return FALSE;
    }
    if (Status & IDE_STATUS_ERROR)
    {
        ErrorCode = ATA_READ(Registers->Error);

        TRACE("Command 0x%02x aborted, status 0x%02x, error 0x%02x\n",
              TryAtapi ? IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY,
              Status,
              ErrorCode);
        return FALSE;
    }

    /* Wait for DRQ */
    if (!(Status & IDE_STATUS_DRQ))
    {
        for (i = 0; i < 100000; ++i)
        {
            Status = ATA_READ(Registers->Status);
            if (Status == 0xFF || Status == 0x7F)
                return FALSE;
            if (Status & IDE_STATUS_DRQ)
                break;

            KeStallExecutionProcessor(10);
        }
    }
    if (!(Status & IDE_STATUS_DRQ))
    {
        TRACE("Timeout, status 0x%02x\n", Status);
        return FALSE;
    }

    /* Receive parameter information from the device to complete the data transfer */
    ATA_READ_BLOCK_16((PUSHORT)Registers->Data,
                      (PUSHORT)&IdentifyDeviceData,
                      sizeof(IdentifyDeviceData) / sizeof(USHORT));

    /* Need to wait for a valid status */
    ATA_IO_WAIT();

    /* Wait for an idle state */
    for (i = 0; i < 100000; ++i)
    {
        Status = ATA_READ(Registers->Status);
        if (Status == 0xFF || Status == 0x7F)
            return FALSE;
        if (!(Status & (IDE_STATUS_BUSY | IDE_STATUS_DRQ)))
        {
            if (!(Status & IDE_STATUS_DRDY) && !TryAtapi)
                return FALSE;

            return TRUE;
        }

        KeStallExecutionProcessor(10);
    }

    TRACE("Timeout, status 0x%02x\n", Status);
    return FALSE;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyChannelPresent(
    _In_ PIDE_REGISTERS Registers)
{
    ULONG DeviceNumber, i;
    UCHAR Status;

    /* The on-board IDE interface is always assumed to be present */
    if (IsNEC_98)
        return TRUE;

    for (DeviceNumber = 0; DeviceNumber < CHANNEL_PCAT_MAX_DEVICES; ++DeviceNumber)
    {
        /* Select the device */
        ATA_WRITE(Registers->DriveSelect, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
        ATA_IO_WAIT();

        /* Do a quick check first */
        Status = ATA_READ(Registers->Status);
        if (Status == 0xFF || Status == 0x7F)
            continue;

        /* Look at controller */
        ATA_WRITE(Registers->ByteCountLow, 0x55);
        ATA_WRITE(Registers->ByteCountLow, 0xAA);
        ATA_WRITE(Registers->ByteCountLow, 0x55);
        if (ATA_READ(Registers->ByteCountLow) != 0x55)
            continue;
        ATA_WRITE(Registers->ByteCountHigh, 0xAA);
        ATA_WRITE(Registers->ByteCountHigh, 0x55);
        ATA_WRITE(Registers->ByteCountHigh, 0xAA);
        if (ATA_READ(Registers->ByteCountHigh) != 0xAA)
            continue;

        /* Perform a software reset so that we can check the device signature */
        ATA_WRITE(Registers->Control, IDE_DC_RESET_CONTROLLER);
        KeStallExecutionProcessor(20);
        ATA_WRITE(Registers->Control, IDE_DC_DISABLE_INTERRUPTS);
        KeStallExecutionProcessor(20);

        /* The reset will cause the master device to be selected */
        if (DeviceNumber != 0)
        {
            for (i = 200000; i > 0; --i)
            {
                /* Select the device again */
                ATA_WRITE(Registers->DriveSelect, ((DeviceNumber << 4) | IDE_DRIVE_SELECT));
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
                break;
            }
        }

        /* Now wait for busy to clear */
        for (i = 0; i < 500000; ++i)
        {
            Status = ATA_READ(Registers->Status);
            if (Status == 0xFF || Status == 0x7F)
                goto SkipDevice;
            if (!(Status & IDE_STATUS_BUSY))
                break;

            KeStallExecutionProcessor(10);
        }
        if (Status & IDE_STATUS_BUSY)
        {
            TRACE("Timeout, status 0x%02x\n", Status);
SkipDevice:
            continue;
        }

        /* Check for an ATA or ATAPI device on the channel */
        if (AtaLegacyIdentifyDevice(DeviceNumber, Registers, FALSE) ||
            AtaLegacyIdentifyDevice(DeviceNumber, Registers, TRUE))
        {
            TRACE("Found IDE device\n");
            return TRUE;
        }
    }

    return FALSE;
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

    if (IsNEC_98)
    {
        if (ConfigInfo->AtDiskPrimaryAddressClaimed ||
            ConfigInfo->AtDiskSecondaryAddressClaimed)
        {
            return FALSE;
        }

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
#define CHANNEL_PC98_RESOURCE_COUNT    12
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = 0x640;
        Descriptor->u.Port.Length = 1;
        ++Descriptor;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = 0x74C;
        Descriptor->u.Port.Length = 1;
        ++Descriptor;

        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->u.Interrupt.Level = 9;
        Descriptor->u.Interrupt.Vector = 9;
        Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
        ++Descriptor;

        for (i = 0; i < 7; ++i)
        {
            Descriptor->Type = CmResourceTypePort;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
            Descriptor->u.Port.Start.u.LowPart = 0x642 + i * 2;
            Descriptor->u.Port.Length = 1;
            ++Descriptor;
        }

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = 0x432;
        Descriptor->u.Port.Length = 2;
        ++Descriptor;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = 0x435;
        Descriptor->u.Port.Length = 1;
    }
    else
    {
        if (LegacyChannel->IoBase == 0x1F0 && ConfigInfo->AtDiskPrimaryAddressClaimed)
            return FALSE;

        if (LegacyChannel->IoBase == 0x170 && ConfigInfo->AtDiskSecondaryAddressClaimed)
            return FALSE;

        /*
         * For example, the following resource list is created for the primary IDE channel:
         *
         * [ShareDisposition 1, Flags 11] IO:  Start 0:168, Len 8
         * [ShareDisposition 1, Flags 11] IO:  Start 0:36E, Len 1
         * [ShareDisposition 1, Flags 1]  INT: Lev A Vec A Aff FFFFFFFF
         */
#define CHANNEL_PCAT_RESOURCE_COUNT    3
        Descriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = LegacyChannel->IoBase;
        Descriptor->u.Port.Length = 8;
        ++Descriptor;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;
        Descriptor->u.Port.Start.u.LowPart = LegacyChannel->IoBase + 0x206;
        Descriptor->u.Port.Length = 1;
        ++Descriptor;

        Descriptor->Type = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        Descriptor->u.Interrupt.Level = LegacyChannel->Irq;
        Descriptor->u.Interrupt.Vector = LegacyChannel->Irq;
        Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
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
    PATAPORT_CHANNEL_EXTENSION ChannelExtension;
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
        Registers.Data = AtaTranslateBusAddress(LegacyChannel->IoBase, 8, &MappedAddress);
        if (!Registers.Data)
        {
            TRACE("Failed to map command port\n");

            Status = STATUS_UNSUCCESSFUL;
            goto ReleaseResources;
        }

        Registers.Control = AtaTranslateBusAddress(LegacyChannel->IoBase + 0x206, 1, &MappedAddress);
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
    Registers.LowLba      = (PVOID)((ULONG_PTR)Registers.Data + 3 * Spare);
    Registers.MidLba      = (PVOID)((ULONG_PTR)Registers.Data + 4 * Spare);
    Registers.HighLba     = (PVOID)((ULONG_PTR)Registers.Data + 5 * Spare);
    Registers.DriveSelect = (PVOID)((ULONG_PTR)Registers.Data + 6 * Spare);
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

    Status = AtaAddChannel(DriverObject, PhysicalDeviceObject, &ChannelExtension);
    if (!NT_SUCCESS(Status))
    {
        TRACE("AtaAddChannel() failed with status 0x%lx\n", Status);
        goto Cleanup;
    }

    AtaLegacyChannelTranslateResources(ResourceList, Registers.Data, Registers.Control);

    Status = AtaFdoStartDevice(ChannelExtension, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to start the legacy channel, status 0x%lx\n", Status);
        AtaFdoRemoveDevice(ChannelExtension, NULL);
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
        TRACE("Skip legacy detection on a PnP system\n");
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
    ATA_LEGACY_CHANNEL LegacyChannel[4 + 1] = {0};
    PCM_RESOURCE_LIST ResourceList;
    ULONG ListSize, ResourceCount, i;

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

    ResourceList = ExAllocatePoolZero(PagedPool, ListSize, IDEPORT_TAG);
    if (!ResourceList)
    {
        INFO("Failed to allocate the resource list\n");
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

    ExFreePoolWithTag(ResourceList, IDEPORT_TAG);
}
