/*
 * PROJECT:     Legacy PATA Bus Enumerator Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * This driver detects non-PnP PATA controllers in the system.
 * The legacy PATA device detection should be done after PCI bus enumeration,
 * therefore this logic is implemented in a separate kernel-mode driver.
 */

/* INCLUDES *******************************************************************/

#include <ntddk.h>
#include <ide.h>
#include <ata.h>
#include <scsi.h>
#include <reactos/drivers/ata/ata_shared.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define ATAPORT_TAG             'PedI'

#define IDE_DRIVE_SELECT        0xA0

#define ATA_READ(Port)          READ_PORT_UCHAR((Port))
#define ATA_WRITE(Port, Value)  WRITE_PORT_UCHAR((Port), (Value))
#define ATA_IO_WAIT()           KeStallExecutionProcessor(1)

#define ATA_TIME_BUSY_SELECT    3000    // 30 ms
#define ATA_TIME_BUSY_IDENTIFY  500000  // 5 s
#define ATA_TIME_DRQ_CLEAR      1000    // 10 ms
#define ATA_TIME_RESET_SELECT   200000  // 2 s
#define ATA_TIME_BUSY_RESET     1000000 // 10 s

typedef struct _ATA_LEGACY_CHANNEL
{
    ULONG IoBase;
    ULONG Irq;
} ATA_LEGACY_CHANNEL, *PATA_LEGACY_CHANNEL;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("INIT")
UCHAR
AtaWait(
    _In_ PIDE_REGISTERS Registers,
    _In_range_(>, 0) ULONG Timeout,
    _In_ UCHAR Mask,
    _In_ UCHAR Value)
{
    UCHAR IdeStatus;
    ULONG i;

    for (i = 0; i < Timeout; ++i)
    {
        IdeStatus = ATA_READ(Registers->Status);
        if ((IdeStatus & Mask) == Value)
            break;

        if (IdeStatus == 0xFF)
            break;

        KeStallExecutionProcessor(10);
    }

    return IdeStatus;
}

static
CODE_SEG("INIT")
VOID
AtaLegacyFetchIdentifyData(
    _In_ PIDE_REGISTERS Registers)
{
    USHORT Buffer[256];

    READ_PORT_BUFFER_USHORT((PUSHORT)Registers->Data, Buffer, RTL_NUMBER_OF(Buffer));
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
    IdeStatus = AtaWait(Registers, ATA_TIME_BUSY_SELECT, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        DPRINT("Timeout, status 0x%02x\n", IdeStatus);
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
    IdeStatus = AtaWait(Registers, ATA_TIME_BUSY_IDENTIFY, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        DPRINT("Timeout, status 0x%02x\n", IdeStatus);
        return FALSE;
    }
    if (IdeStatus & ((IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT)))
    {
        DPRINT("Command 0x%02x aborted, status 0x%02x, error 0x%02x\n",
              Command,
              IdeStatus,
              ATA_READ(Registers->Error));
        return FALSE;
    }
    if (!(IdeStatus & IDE_STATUS_DRQ))
    {
        DPRINT1("DRQ not set, status 0x%02x\n", IdeStatus);
        return FALSE;
    }

    /* Complete the data transfer */
    AtaLegacyFetchIdentifyData(Registers);

    /* All data has been transferred, wait for DRQ to clear */
    IdeStatus = AtaWait(Registers,
                        ATA_TIME_DRQ_CLEAR,
                        IDE_STATUS_BUSY | IDE_STATUS_DRQ,
                        0);
    if (IdeStatus & (IDE_STATUS_BUSY | IDE_STATUS_DRQ))
    {
        DPRINT1("DRQ not cleared, status 0x%02x\n", IdeStatus);
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
    if ((DeviceNumber & 1) != 0)
    {
        for (i = ATA_TIME_RESET_SELECT; i > 0; i--)
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
            DPRINT("Selection timeout\n");
            return FALSE;
        }
    }

    /* Now wait for busy to clear */
    IdeStatus = AtaWait(Registers, ATA_TIME_BUSY_RESET, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        DPRINT1("Timeout, status 0x%02x\n", IdeStatus);
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
    IdeStatus = AtaWait(Registers, ATA_TIME_BUSY_SELECT, IDE_STATUS_BUSY, 0);
    if (IdeStatus & IDE_STATUS_BUSY)
    {
        DPRINT1("Device is busy, attempting to recover %02x\n", IdeStatus);

        if (!AtaLegacyPerformSoftwareReset(Registers, DeviceNumber))
        {
            DPRINT1("Failed to reset device %02x\n", ATA_READ(Registers->Status));
            return FALSE;
        }
    }

    /* Check for ATA */
    if (AtaLegacyIdentifyDevice(Registers, DeviceNumber, IDE_COMMAND_IDENTIFY))
        return TRUE;

    SignatureLow = ATA_READ(Registers->SignatureLow);
    SignatureHigh = ATA_READ(Registers->SignatureHigh);

    DPRINT("SL = 0x%02x, SH = 0x%02x\n", SignatureLow, SignatureHigh);

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
    return AtaLegacyIdentifyDevice(Registers, DeviceNumber, IDE_COMMAND_ATAPI_IDENTIFY);
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyChannelPresent(
    _In_ PIDE_REGISTERS Registers)
{
    ULONG i;

    /* Check for the PC-98 on-board IDE interface */
    if (IsNEC_98)
        return (ATA_READ((PUCHAR)0x432) != 0xFF);

    /*
     * The only reliable way to detect the legacy IDE channel
     * is to check for devices attached to the bus.
     */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        if (AtaLegacyFindAtaDevice(Registers, i))
        {
            DPRINT("Found IDE device\n");
            return TRUE;
        }
    }

    return FALSE;
}

static
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
         * Make a resource list for the internal IDE interface:
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
    Descriptor->u.Interrupt.Level = Irql;
}

static
CODE_SEG("INIT")
PVOID
AtaLegacyTranslateBusAddress(
    _In_ ULONG Address,
    _In_ ULONG NumberOfBytes,
    _Out_ PBOOLEAN IsAddressMmio)
{
    ULONG AddressSpace;
    BOOLEAN Success;
    PHYSICAL_ADDRESS BusAddress, TranslatedAddress;

    BusAddress.QuadPart = Address;
    AddressSpace = 1; // I/O space
    Success = HalTranslateBusAddress(Isa, 0, BusAddress, &AddressSpace, &TranslatedAddress);
    if (!Success)
        return NULL;

    /* I/O space */
    if (AddressSpace != 0)
    {
        *IsAddressMmio = FALSE;
        return (PVOID)(ULONG_PTR)TranslatedAddress.QuadPart;
    }
    else
    {
        *IsAddressMmio = TRUE;
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
BOOLEAN
AtaLegacyDetectChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PPCIIDEX_LEGACY_CONTROLLER_INTERFACE ControllerInferface,
    _In_ PATA_LEGACY_CHANNEL LegacyChannel,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ULONG ResourceListSize)
{
    BOOLEAN IsAddressMmio[2];
    PDEVICE_OBJECT PhysicalDeviceObject;
    PVOID ControllerContext;
    IDE_REGISTERS Registers;
    NTSTATUS Status;
    ULONG Spare;

    DPRINT("IDE Channel IO %lx, Irq %lu\n", LegacyChannel->IoBase, LegacyChannel->Irq);

    if (!AtaLegacyChannelBuildResources(LegacyChannel, ResourceList))
    {
        DPRINT("Failed to build the resource list\n");
        return FALSE;
    }

    if (!AtaLegacyClaimHardwareResources(DriverObject, ResourceList, ResourceListSize))
    {
        DPRINT("Failed to claim resources\n");
        return FALSE;
    }

    Status = STATUS_SUCCESS;
    IsAddressMmio[0] = FALSE;
    IsAddressMmio[1] = FALSE;

    if (IsNEC_98)
    {
        /* No translation required for the C-Bus I/O space */
        Registers.Data = (PVOID)0x640;
        Registers.Control = (PVOID)0x74C;

        Spare = 2;
    }
    else
    {
        Registers.Data = AtaLegacyTranslateBusAddress(LegacyChannel->IoBase,
                                                      8,
                                                      &IsAddressMmio[0]);
        if (!Registers.Data)
        {
            DPRINT("Failed to map command port\n");

            Status = STATUS_UNSUCCESSFUL;
            goto ReleaseResources;
        }

        Registers.Control = AtaLegacyTranslateBusAddress(LegacyChannel->IoBase + 0x206,
                                                         1,
                                                         &IsAddressMmio[1]);
        if (!Registers.Control)
        {
            DPRINT("Failed to map control port\n");

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
        DPRINT("No IDE devices found\n");
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
        DPRINT("IoReportDetectedDevice() failed with status 0x%lx\n", Status);
        goto Cleanup;
    }

    if (ControllerInferface->Version != PCIIDEX_INTERFACE_VERSION)
    {
        /* ReactOS-specific: Retrieve the interface for legacy device detection */
        Status = PciIdeXInitialize(DriverObject,
                                   NULL,
                                   (PVOID)ControllerInferface,
                                   PCIIDEX_GET_CONTROLLER_INTERFACE_SIGNATURE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to register the legacy channel 0x%lx, status 0x%lx\n",
                LegacyChannel->IoBase, Status);
            goto Cleanup;
        }

        if (ControllerInferface->Version != PCIIDEX_INTERFACE_VERSION)
        {
            DPRINT1("Unknown interface version 0x%lx\n", ControllerInferface->Version);
            Status = STATUS_REVISION_MISMATCH;
            goto Cleanup;
        }
    }

    Status = ControllerInferface->AddDevice(DriverObject, PhysicalDeviceObject, &ControllerContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to add the legacy channel 0x%lx, status 0x%lx\n",
            LegacyChannel->IoBase, Status);
        goto Cleanup;
    }

    AtaLegacyChannelTranslateResources(ResourceList, Registers.Data, Registers.Control);

    Status = ControllerInferface->StartDevice(ControllerContext, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to start the legacy channel 0x%lx, status 0x%lx\n",
            LegacyChannel->IoBase, Status);

        ControllerInferface->RemoveDevice(ControllerContext);
    }

Cleanup:
    if (IsAddressMmio[0] && Registers.Data)
    {
        MmUnmapIoSpace(Registers.Data, 8);
    }

    if (IsAddressMmio[1] && Registers.Control)
    {
        MmUnmapIoSpace(Registers.Control, 1);
    }

    return NT_SUCCESS(Status);
}

static
CODE_SEG("INIT")
NTSTATUS
AtaOpenRegistryKey(
    _Out_ PHANDLE KeyHandle,
    _In_ HANDLE RootKey,
    _In_ PUNICODE_STRING KeyName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootKey,
                               NULL);
    return ZwOpenKey(KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
}

static
CODE_SEG("INIT")
BOOLEAN
AtaLegacyShouldDetectChannels(
    _In_ PUNICODE_STRING RegistryPath)
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
        DPRINT("Skipping legacy detection on a PnP system\n");
        return FALSE;
    }

    /* Open the driver's software key */
    Status = AtaOpenRegistryKey(&SoftKeyHandle, NULL, RegistryPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the '%wZ' key, status 0x%lx\n", RegistryPath, Status);
        return FALSE;
    }

    /* Open the 'Parameters' key */
    Status = AtaOpenRegistryKey(&ParamsKeyHandle, SoftKeyHandle, &ParametersKeyName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the 'Parameters' key, status 0x%lx\n", Status);

        ZwClose(SoftKeyHandle);
        return FALSE;
    }

    /* Check whether it is the first time we detect IDE channels */
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

    /* Do not detect devices again on subsequent boots after driver installation */
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
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    PCIIDEX_LEGACY_CONTROLLER_INTERFACE ControllerInferface = { 0 };
    ATA_LEGACY_CHANNEL LegacyChannel[4 + 1] = { 0 };
    PCM_RESOURCE_LIST ResourceList;
    ULONG i, ListSize, ResourceCount;
    NTSTATUS Status;

    if (!AtaLegacyShouldDetectChannels(RegistryPath))
        return STATUS_CANCELLED;

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
        DPRINT1("Failed to allocate the resource list\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = Isa;
    ResourceList->List[0].BusNumber = 0;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = ResourceCount;

    Status = STATUS_NOT_FOUND;

    for (i = 0; LegacyChannel[i].IoBase != 0; ++i)
    {
        if (AtaLegacyDetectChannel(DriverObject,
                                   &ControllerInferface,
                                   &LegacyChannel[i],
                                   ResourceList,
                                   ListSize))
        {
            Status = STATUS_SUCCESS;
        }
    }

    ExFreePoolWithTag(ResourceList, ATAPORT_TAG);
    return Status;
}
