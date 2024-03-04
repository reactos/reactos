/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA channel device object (FDO) dispatch routines
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const WCHAR AtapDevSymLinkFormat[] = L"\\Device\\ScsiPort%lu";
static const WCHAR AtapDosSymLinkFormat[] = L"\\DosDevices\\Scsi%lu:";

static KSYNCHRONIZE_ROUTINE AtaFdoEnableInterruptsSync;

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
AtaFdoClaimLegacyAddressRanges(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

    ASSERT(!IS_AHCI_EXT(ChanExt));

#if defined(_M_IX86)
    if (PortData->PortFlags & PORT_FLAG_CBUS_IDE)
    {
        /* On NEC PC-98 systems we have at least four PDOs in use for the legacy IDE interface */
        ConfigInfo->AtDiskPrimaryAddressClaimed =
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

        ChanExt->Flags |= CHANNEL_PRIMARY_ADDRESS_CLAIMED |
                          CHANNEL_SECONDARY_ADDRESS_CLAIMED;
    }
    else
#endif
    if (PortData->Pata.CommandPortBase == (PVOID)0x1F0)
    {
        ConfigInfo->AtDiskPrimaryAddressClaimed = TRUE;

        ChanExt->Flags |= CHANNEL_PRIMARY_ADDRESS_CLAIMED;
    }
    else if (PortData->Pata.CommandPortBase == (PVOID)0x170)
    {
        ConfigInfo->AtDiskSecondaryAddressClaimed = TRUE;

        ChanExt->Flags |= CHANNEL_SECONDARY_ADDRESS_CLAIMED;
    }
}

static
CODE_SEG("PAGE")
VOID
AtaFdoReleaseLegacyAddressRanges(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PCONFIGURATION_INFORMATION ConfigInfo = IoGetConfigurationInformation();

    PAGED_CODE();

    if (ChanExt->Flags & CHANNEL_PRIMARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskPrimaryAddressClaimed = FALSE;
    if (ChanExt->Flags & CHANNEL_SECONDARY_ADDRESS_CLAIMED)
        ConfigInfo->AtDiskSecondaryAddressClaimed = FALSE;

    ChanExt->Flags &= ~(CHANNEL_PRIMARY_ADDRESS_CLAIMED | CHANNEL_SECONDARY_ADDRESS_CLAIMED);
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoCreateSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    UNICODE_STRING ChannelName;
    ULONG ScsiAdapter, ScsiPortCount;
    WCHAR ChannelNameBuffer[sizeof("\\Device\\Ide\\IdePort999")];
    NTSTATUS Status;
    static const WCHAR FdoFormat[] = L"\\Device\\Ide\\IdePort%lu";

    PAGED_CODE();

    Status = RtlStringCbPrintfW(ChannelNameBuffer,
                                sizeof(ChannelNameBuffer),
                                FdoFormat,
                                ChanExt->DeviceObjectNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&ChannelName, ChannelNameBuffer);

    ScsiPortCount = IoGetConfigurationInformation()->ScsiPortCount;

    /* Search for a free SCSI port adapter in the system */
    for (ScsiAdapter = 0; ScsiAdapter <= ScsiPortCount; ++ScsiAdapter)
    {
        WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];
        UNICODE_STRING SymLinkName;

        Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                    sizeof(SymLinkNameBuffer),
                                    AtapDevSymLinkFormat,
                                    ScsiAdapter);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

        /* Create a symbolic link '\Device\ScsiPortX' -> '\Device\Ide\IdePortN' */
        Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
        if (!NT_SUCCESS(Status))
            continue;

        INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);

        Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                    sizeof(SymLinkNameBuffer),
                                    AtapDosSymLinkFormat,
                                    ScsiAdapter);
        ASSERT(NT_SUCCESS(Status));
        RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);

        /* Create a symbolic link '\DosDevices\ScsiX:' -> '\Device\Ide\IdePortN' */
        Status = IoCreateSymbolicLink(&SymLinkName, &ChannelName);
        if (NT_SUCCESS(Status))
        {
            INFO("Symlink created '%wZ' -> '%wZ'\n", &SymLinkName, &ChannelName);
        }

        /* Register ourselves (ATA channel) as a SCSI port adapter */
        ++IoGetConfigurationInformation()->ScsiPortCount;

        ChanExt->ScsiPortNumber = ScsiAdapter;
        ChanExt->Flags |= CHANNEL_SYMLINK_CREATED;
        break;
    }

    return Status;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoRemoveSymLinks(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    NTSTATUS Status;
    UNICODE_STRING SymLinkName;
    WCHAR SymLinkNameBuffer[sizeof("\\DosDevices\\Scsi999:")];

    PAGED_CODE();

    /* Delete the '\DosDevices\\ScsiX:' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDevSymLinkFormat,
                                ChanExt->ScsiPortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Delete the '\Device\\ScsiPortX' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDosSymLinkFormat,
                                ChanExt->ScsiPortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Unregister the SCSI port adapter */
    --IoGetConfigurationInformation()->ScsiPortCount;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoParsePataChannelResources(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* CommandPortDesc,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* ControlPortDesc,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* InterruptDesc)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    ULONG i;

    PAGED_CODE();

    if (!ResourcesTranslated)
        goto Exit;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                switch (Descriptor->u.Port.Length)
                {
                    case 1:
                    {
                        /*
                         * Note that on PC-98 systems we still need to support PCI IDE controllers
                         * with a different register layout. The NEC PC-98 legacy IDE interface
                         * can coexist with a normal PCI IDE controller.
                         */
                        if (IsNEC_98 &&
                            ResourcesTranslated->List[0].PartialResourceList.Count >= 12)
                        {
                            if ((Descriptor->u.Port.Start.QuadPart == (ULONG64)0x640))
                            {
                                CommandPortDescriptor = Descriptor;
                                PortData->PortFlags |= PORT_FLAG_CBUS_IDE;
                            }
                            else if ((Descriptor->u.Port.Start.QuadPart == (ULONG64)0x74C))
                            {
                                ControlPortDescriptor = Descriptor;
                                PortData->PortFlags |= PORT_FLAG_CBUS_IDE;
                            }
                        }
                        else if (!ControlPortDescriptor)
                        {
                            ControlPortDescriptor = Descriptor;
                        }
                        break;
                    }

                    case 8:
                    {
                        if (!CommandPortDescriptor)
                            CommandPortDescriptor = Descriptor;
                        break;
                    }

                    default:
                        break;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

Exit:
    *CommandPortDesc = CommandPortDescriptor;
    *ControlPortDesc = ControlPortDescriptor;
    *InterruptDesc = InterruptDescriptor;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoParsePataControllerResources(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* CommandPortDesc,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* ControlPortDesc,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR* InterruptDesc)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    ULONG i, CommandPortNumber = 0, ControlPortNumber = 0;

    PAGED_CODE();

    if (!ResourcesTranslated)
        goto Exit;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
            {
                switch (Descriptor->u.Port.Length)
                {
                    case 4:
                    {
                        /* Primary or Secondary channel */
                        if (ControlPortNumber++ == PortData->PortNumber)
                        {
                            if (!ControlPortDescriptor)
                                ControlPortDescriptor = Descriptor;
                        }
                        break;
                    }

                    case 8:
                    {
                        if (CommandPortNumber++ == PortData->PortNumber)
                        {
                            if (!CommandPortDescriptor)
                                CommandPortDescriptor = Descriptor;
                        }
                        break;
                    }

                    default:
                        break;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

Exit:
    *CommandPortDesc = CommandPortDescriptor;
    *ControlPortDesc = ControlPortDescriptor;
    *InterruptDesc = InterruptDescriptor;
}

/*
 * PCI IDE Notes:
 *
 * When the PCI IDE controller operates in compatibility mode,
 * the pciidex driver assigns a list of boot resources to each IDE channel.
 *
 *                                        _GTM, _STM                            _GTF
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFA0, Len 10      |   IO:  Start 0:1F0, Len 8        |
 *                                       |   IO:  Start 0:3F6, Len 1        \-->DRV1
 *                                       |   INT: Lev A Vec A Aff FFFFFFFF
 *                                       |
 *                                       \-->CHN1---------------------------+-->DRV0
 *                                           IO:  Start 0:170, Len 8        |
 *                                           IO:  Start 0:376, Len 1        \-->DRV1
 *                                           INT: Lvl F Vec F Aff FFFFFFFF
 *
 * In native-PCI mode, the PCI IDE controller acts as a true PCI device
 * and no resources are assigned to the channel device object at all,
 * so here we have to call a private interface between the pciidex and our port driver
 * to retrieve the actual hardware resources.
 *
 * PCI0-->IDE0---------------------------+-->CHN0---------------------------+-->DRV0
 *        IO:  Start 0:FFC0, Len 8       |                                  |
 *        IO:  Start 0:FF8C, Len 4       |                                  \-->DRV1
 *        IO:  Start 0:FF80, Len 8       |
 *        IO:  Start 0:FF88, Len 4       |
 *        IO:  Start 0:FFA0, Len 10      \-->CHN1---------------------------+-->DRV0
 *        INT: Lvl B Vec B Aff FFFFFFFF                                     |
 *                                                                          \-->DRV1
 *
 * Note that the NT architecture does not support switching only one IDE channel to native mode.
 */
static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoParsePataResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PIDE_REGISTERS Registers = &PortData->Pata.Registers;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDesc, ControlPortDesc, InterruptDesc;
    PVOID IoBase;
    ULONG Spare;

    PAGED_CODE();

    ASSERT(!IS_AHCI_EXT(ChanExt));

    /* Try to parse the channel resources and fall back to controller resources if that fails */
    AtaFdoParsePataChannelResources(PortData,
                                    ResourcesTranslated,
                                    &CommandPortDesc,
                                    &ControlPortDesc,
                                    &InterruptDesc);
    if ((!CommandPortDesc || !ControlPortDesc || !InterruptDesc) && IS_PCIIDE_EXT(ChanExt))
    {
        INFO("Trying to parse controller resources\n");

        AtaFdoParsePataControllerResources(PortData,
                                           PortData->Pata.PciIdeInterface.ControllerResources,
                                           &CommandPortDesc,
                                           &ControlPortDesc,
                                           &InterruptDesc);
    }

    if (!CommandPortDesc || !ControlPortDesc || !InterruptDesc)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    /* Command port base */
    if ((CommandPortDesc->Type == CmResourceTypePort) &&
        (CommandPortDesc->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)CommandPortDesc->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(PortData->PortFlags & PORT_FLAG_CBUS_IDE));

        IoBase = MmMapIoSpace(CommandPortDesc->u.Memory.Start, 8, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChanExt->Flags |= CHANNEL_COMMAND_PORT_BASE_MAPPED;
    }
    PortData->Pata.CommandPortBase = (PVOID)(ULONG_PTR)CommandPortDesc->u.Port.Start.QuadPart;

#if defined(_M_IX86)
    if (PortData->PortFlags & PORT_FLAG_CBUS_IDE)
    {
        Spare = 2;
    }
    else
#endif
    {
        Spare = 1;
    }
    Registers->Data        = (PVOID)((ULONG_PTR)IoBase + 0 * Spare);
    Registers->Error       = (PVOID)((ULONG_PTR)IoBase + 1 * Spare);
    Registers->SectorCount = (PVOID)((ULONG_PTR)IoBase + 2 * Spare);
    Registers->LbaLow      = (PVOID)((ULONG_PTR)IoBase + 3 * Spare);
    Registers->LbaMid      = (PVOID)((ULONG_PTR)IoBase + 4 * Spare);
    Registers->LbaHigh     = (PVOID)((ULONG_PTR)IoBase + 5 * Spare);
    Registers->Device      = (PVOID)((ULONG_PTR)IoBase + 6 * Spare);
    Registers->Status      = (PVOID)((ULONG_PTR)IoBase + 7 * Spare);

    /* Control port base */
    if ((ControlPortDesc->Type == CmResourceTypePort) &&
        (ControlPortDesc->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)ControlPortDesc->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(PortData->PortFlags & PORT_FLAG_CBUS_IDE));

        IoBase = MmMapIoSpace(ControlPortDesc->u.Memory.Start, 1, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChanExt->Flags |= CHANNEL_CONTROL_PORT_BASE_MAPPED;
    }
    Registers->Control = (PVOID)IoBase;
    PortData->Pata.ControlPortBase = (PVOID)(ULONG_PTR)ControlPortDesc->u.Port.Start.QuadPart;

    /* Interrupt */
    ChanExt->InterruptVector = InterruptDesc->u.Interrupt.Vector;
    ChanExt->InterruptLevel = InterruptDesc->u.Interrupt.Level;
    ChanExt->InterruptAffinity = InterruptDesc->u.Interrupt.Affinity;

    if (InterruptDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
        ChanExt->InterruptMode = Latched;
    else
        ChanExt->InterruptMode = LevelSensitive;

    if (InterruptDesc->ShareDisposition == CmResourceShareShared)
        ChanExt->Flags |= CHANNEL_INTERRUPT_SHARED;
    else
        ChanExt->Flags &= ~CHANNEL_INTERRUPT_SHARED;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoParseAhciResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR AbarDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

        Descriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        switch (Descriptor->Type)
        {
            case CmResourceTypeMemory:
            {
                /* We save the last memory descriptor as ABAR */
                AbarDescriptor = Descriptor;
                break;
            }

            case CmResourceTypeInterrupt:
            {
                if (!InterruptDescriptor)
                    InterruptDescriptor = Descriptor;
                break;
            }

            default:
                break;
        }
    }

    if (!AbarDescriptor || !InterruptDescriptor)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    ChanExt->IoBase = MmMapIoSpace(AbarDescriptor->u.Memory.Start,
                                   AbarDescriptor->u.Memory.Length,
                                   MmNonCached);
    if (!ChanExt->IoBase)
        return STATUS_INSUFFICIENT_RESOURCES;

    ChanExt->IoLength = AbarDescriptor->u.Memory.Length;

    /* Interrupt */
    ChanExt->InterruptVector = InterruptDescriptor->u.Interrupt.Vector;
    ChanExt->InterruptLevel = InterruptDescriptor->u.Interrupt.Level;
    ChanExt->InterruptAffinity = InterruptDescriptor->u.Interrupt.Affinity;

    if (InterruptDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
        ChanExt->InterruptMode = Latched;
    else
        ChanExt->InterruptMode = LevelSensitive;

    if (InterruptDescriptor->ShareDisposition == CmResourceShareShared)
        ChanExt->Flags |= CHANNEL_INTERRUPT_SHARED;
    else
        ChanExt->Flags &= ~CHANNEL_INTERRUPT_SHARED;

    return STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
AtaFdoEnableInterruptsSync(
    _In_ PVOID SynchronizeContext)
{
    PATA_ENABLE_INTERRUPTS_CONTEXT Context = SynchronizeContext;
    PATAPORT_CHANNEL_EXTENSION ChanExt = Context->ChanExt;

    if (IS_AHCI_EXT(ChanExt))
    {
        ULONG GlobalControl;

        GlobalControl = AHCI_HBA_READ(ChanExt->IoBase, HbaGlobalControl);

        if (Context->Enable)
            GlobalControl |= AHCI_GHC_IE;
        else
            GlobalControl &= ~AHCI_GHC_IE;

        AHCI_HBA_WRITE(ChanExt->IoBase, HbaGlobalControl, GlobalControl);
    }
    else
    {
        PATAPORT_PORT_DATA PortData = ChanExt->PortData;
        UCHAR Command;

        if (Context->Enable)
            Command = IDE_DC_REENABLE_CONTROLLER;
        else
            Command = IDE_DC_DISABLE_INTERRUPTS;
        Command |= IDE_DC_ALWAYS;

#if defined(_M_IX86)
        if (PortData->PortFlags & PORT_FLAG_CBUS_IDE)
        {
            ATA_SELECT_BANK(1);
            ATA_WRITE(PortData->Pata.Registers.Control, Command);

            ATA_SELECT_BANK(0);
        }
#endif
        ATA_WRITE(PortData->Pata.Registers.Control, Command);
    }

    return TRUE;
}

static
CODE_SEG("PAGE")
VOID
AtaFdoEnableInterrupts(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ BOOLEAN Enable)
{
    ATA_ENABLE_INTERRUPTS_CONTEXT SynchronizeContext;

    PAGED_CODE();

    SynchronizeContext.ChanExt = ChanExt;
    SynchronizeContext.Enable = Enable;

    KeSynchronizeExecution(ChanExt->InterruptObject,
                           AtaFdoEnableInterruptsSync,
                           &SynchronizeContext);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoFreeIoResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    PAGED_CODE();

    if (ChanExt->InterruptObject)
    {
        AtaFdoEnableInterrupts(ChanExt, FALSE);

        IoDisconnectInterrupt(ChanExt->InterruptObject);
        ChanExt->InterruptObject = NULL;
    }

    if (ChanExt->IoBase)
    {
        MmUnmapIoSpace(ChanExt->IoBase, ChanExt->IoLength);
        ChanExt->IoBase = NULL;
    }
    else
    {
        PATAPORT_PORT_DATA PortData = ChanExt->PortData;

        if (ChanExt->Flags & CHANNEL_COMMAND_PORT_BASE_MAPPED)
        {
            ChanExt->Flags &= ~CHANNEL_COMMAND_PORT_BASE_MAPPED;

            MmUnmapIoSpace(PortData->Pata.Registers.Data, 8);
        }

        if (ChanExt->Flags & CHANNEL_CONTROL_PORT_BASE_MAPPED)
        {
            ChanExt->Flags &= ~CHANNEL_CONTROL_PORT_BASE_MAPPED;

            MmUnmapIoSpace(PortData->Pata.Registers.Control, 1);
        }
    }

    // TODO
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoGetDmaAdapter(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    DEVICE_DESCRIPTION DeviceDescription = { 0 };

    PAGED_CODE();

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = !(ChanExt->AhciCapabilities & AHCI_CAP_S64A);
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = ATA_MAX_TRANSFER_LENGTH;

    ChanExt->AdapterObject = IoGetDmaAdapter(ChanExt->Ldo,
                                             &DeviceDescription,
                                             &ChanExt->MapRegisterCount);
    if (!ChanExt->AdapterObject)
        return STATUS_INSUFFICIENT_RESOURCES;

    ChanExt->AdapterDeviceObject = ChanExt->Ldo;
    ChanExt->MaximumTransferLength = ChanExt->MapRegisterCount << PAGE_SHIFT;

    ChanExt->MaximumTransferLength = min(ChanExt->MaximumTransferLength,
                                         ATA_MAX_TRANSFER_LENGTH);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoInitPortData(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ULONG PortNumber,
    _Inout_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATAPORT_PORT_INFO PortInfo)
{
    PAGED_CODE();

    TRACE("Initializing port #%lu\n", PortNumber);

    PortData->Worker.InternalRequest = ExAllocatePoolZero(NonPagedPool,
                                                          sizeof(*PortData->Worker.InternalRequest),
                                                          ATAPORT_TAG);
    if (!PortData->Worker.InternalRequest)
        return STATUS_INSUFFICIENT_RESOURCES;

    PortData->Worker.InternalRequest->Complete = AtaPortCompleteInternalRequest;
#if DBG
    PortData->Worker.InternalRequest->Signature = ATA_DEVICE_REQUEST_SIGNATURE;
#endif

    KeInitializeEvent(&PortData->Worker.EnumerationEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&PortData->Worker.SetTimingsEvent, NotificationEvent, FALSE);
    KeInitializeTimer(&PortData->Worker.Timer);

    KeInitializeDpc(&PortData->Worker.Dpc, AtaPortWorkerDpc, PortData);
    KeInitializeSpinLock(&PortData->Worker.Lock);

    InitializeListHead(&PortData->PortQueueList);
    KeInitializeDpc(&PortData->PollingTimerDpc, AtaReqPollingTimerDpc, PortData);


    PortData->ChanExt = ChanExt;
    PortData->InterruptFlags = PORT_INT_FLAG_IS_IO_ACTIVE;
    PortData->SendRequest = AtaReqSendRequest;
    PortData->PortNumber = PortNumber;
    PortData->QueueFlags = 0xFF; // PORT_QUEUE_FLAG_LAST_TARGET_MASK

    PortData->Worker.InternalDevice.SectorSize = ATA_MIN_SECTOR_SIZE;
    PortData->Worker.InternalDevice.DeviceFlags |= DEVICE_UNINITIALIZED | DEVICE_PIO_ONLY;
    PortData->Worker.InternalDevice.PortData = PortData;
    PortData->Worker.InternalDevice.ChanExt = ChanExt;
    PortData->Worker.Flags = WORKER_FLAG_DISABLE_FSM;

    /* We need the slot numbers to start from zero */
    PortData->LastUsedSlot = AHCI_MAX_COMMAND_SLOTS - 1;

    if (IS_AHCI_EXT(ChanExt))
    {
        ULONG QueueDepth;

        QueueDepth = ((ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;

        PortData->PrepareIo = AtaAhciPrepareIo;
        PortData->PreparePrdTable = AtaAhciPreparePrdTable;
        PortData->StartIo = AtaAhciStartIo;
        PortData->FreeSlotsBitmap =
        PortData->MaxSlotsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - QueueDepth);

        PortData->Ahci.IoBase = AHCI_PORT_BASE(ChanExt->IoBase, PortNumber);

        PortData->Worker.InternalDevice.DeviceFlags |= DEVICE_IS_AHCI;

        return AtaAhciPortInit(ChanExt, PortData, PortInfo);
    }
    else
    {
        PortData->PreparePrdTable = AtaPciIdePreparePrdTable;
        PortData->PrepareIo = AtaPataPrepareIo;
        PortData->StartIo = AtaPataStartIo;
        PortData->FreeSlotsBitmap =
        PortData->MaxSlotsBitmap = PATA_CHANNEL_QUEUE_DEPTH;
        PortData->Pata.LastTargetId = 0xFF;

        return STATUS_SUCCESS;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoCreatePortData(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    ULONG i, Size;
    NTSTATUS Status;
    PATAPORT_PORT_DATA PortData;
    PATAPORT_PORT_INFO PortInfo;

    PAGED_CODE();

    Size = sizeof(*PortData);

    if (IS_AHCI_EXT(ChanExt))
        Size += sizeof(*PortInfo);
    else
        Size += ATA_LOCAL_BUFFER_SIZE;

    Size *= ChanExt->NumberOfPorts;

    ChanExt->PortData = ExAllocatePoolZero(NonPagedPoolCacheAligned, Size, ATAPORT_TAG);
    if (!ChanExt->PortData)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (IS_AHCI_EXT(ChanExt))
        ChanExt->PortInfo = (PVOID)(ChanExt->PortData + ChanExt->NumberOfPorts);
    else
        ChanExt->PortData->Pata.LocalBuffer = (PVOID)(ChanExt->PortData + 1);

    PortData = ChanExt->PortData;
    PortInfo = ChanExt->PortInfo;

    /* Initialize each port on the HBA */
    for (i = 0; i < AHCI_MAX_PORTS; ++i)
    {
        if (!(ChanExt->PortBitmap & (1 << i)))
            continue;

        Status = AtaFdoInitPortData(ChanExt, i, PortData, PortInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to initialize port %lu with status %lx\n", i, Status);
            return Status;
        }

        ++PortData;
        ++PortInfo;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
AtaPciIdeDmaInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData)
{
    PPCIIDE_INTERFACE PciIdeInterface = &PortData->Pata.PciIdeInterface;
    NTSTATUS Status;

    PAGED_CODE();

    if (IS_LEGACY_IDE_EXT(ChanExt))
        goto FallbackToPio;

    /* Get the interface of the PCI IDE controller */
    Status = AtaFdoQueryInterface(ChanExt,
                                  &GUID_PCIIDE_INTERFACE_ROS,
                                  PciIdeInterface,
                                  sizeof(*PciIdeInterface));
    if (!NT_SUCCESS(Status))
        goto FallbackToPio;

    /* The hardware is a PCI IDE controller */
    ChanExt->Common.Flags |= DO_IS_PCIIDE;
    PortData->PortNumber = PciIdeInterface->ChannelNumber;

    if (!PciIdeInterface->PrdTable)
        goto FallbackToPio;

    if (PciIdeInterface->ControllerObject)
    {
        INFO("Sync access for DMA hardware is required\n");
        PortData->PortFlags |= PORT_FLAG_SIMPLEX_DMA;
    }

    ChanExt->MaximumTransferLength = min(ChanExt->MaximumTransferLength,
                                         PciIdeInterface->MaximumTransferLength);

    /* The hardware can use DMA */
    ChanExt->AdapterObject = PciIdeInterface->AdapterObject;
    ChanExt->AdapterDeviceObject = PciIdeInterface->DeviceObject;
    ChanExt->MapRegisterCount = PciIdeInterface->MapRegisterCount;
    return;

FallbackToPio:
    INFO("No DMA support\n");
    ChanExt->Flags |= CHANNEL_PIO_ONLY;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartPataChannel(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PATAPORT_PORT_DATA PortData;
    NTSTATUS Status;

    PAGED_CODE();

    ChanExt->NumberOfPorts = 1;
    ChanExt->PortBitmap = 1;

    /* Maximum for PIO or DMA data transfers */
    ChanExt->MaximumTransferLength = ATA_MAX_TRANSFER_LENGTH;

    Status = AtaFdoCreatePortData(ChanExt);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create port data 0x%lx\n", Status);
        return Status;
    }
    PortData = ChanExt->PortData;

    AtaPciIdeDmaInit(ChanExt, PortData);

    Status = AtaFdoParsePataResources(ChanExt, PortData, ResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to parse resources 0x%lx\n", Status);
        return Status;
    }

    AtaFdoClaimLegacyAddressRanges(ChanExt, PortData);

    /* Reserve memory resources early. Storage drivers should not fail paging I/O operations */
    if (!ChanExt->ReservedVaSpace)
    {
        ChanExt->ReservedVaSpace = // TODO pata only?
            MmAllocateMappingAddress(ATA_RESERVED_PAGES * PAGE_SIZE, ATAPORT_TAG);
    }

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    NTSTATUS Status;
    PKSERVICE_ROUTINE IsrHandler;
    PVOID IsrContext;

    PAGED_CODE();

    INFO("Starting channel %lu\n", ChanExt->DeviceObjectNumber);

    if (IS_AHCI_EXT(ChanExt))
    {
        Status = AtaFdoParseAhciResources(ChanExt, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to parse resources 0x%lx\n", Status);
            return Status;
        }

        Status = AtaFdoGetDmaAdapter(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to initialize DMA 0x%lx\n", Status);
            return Status;
        }

        Status = AtaAhciInitHba(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to init the HBA 0x%lx\n", Status);
            return Status;
        }

        Status = AtaFdoCreatePortData(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create port data 0x%lx\n", Status);
            return Status;
        }

        if (ChanExt->AhciCapabilities & AHCI_CAP_SPM)
        {
            ChanExt->MaxTargetId = AHCI_MAX_PMP_DEVICES;
        }
        else
        {
            ChanExt->MaxTargetId = ChanExt->NumberOfPorts;
        }
    }
    else
    {
        Status = AtaFdoStartPataChannel(ChanExt, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to start channel 0x%lx\n", Status);
            return Status;
        }

#if defined(_M_IX86)
        if (ChanExt->PortData->PortFlags & PORT_FLAG_CBUS_IDE)
            ChanExt->MaxTargetId = CHANNEL_PC98_MAX_DEVICES;
        else
#endif
            ChanExt->MaxTargetId = CHANNEL_PCAT_MAX_DEVICES;
    }

    AtaSetPortRegistryKey(ChanExt, L"MaxTargetId", ChanExt->MaxTargetId);

    if (!(ChanExt->Flags & CHANNEL_SYMLINK_CREATED))
    {
        Status = AtaFdoCreateSymLinks(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create symbolic links 0x%lx\n", Status);
            return Status;
        }
    }

    Status = IoRegisterDeviceInterface(ChanExt->Common.Self,
                                       &GUID_DEVINTERFACE_STORAGEPORT,
                                       NULL,
                                       &ChanExt->StorageInterfaceName);
    if (NT_SUCCESS(Status))
    {
        INFO("InterfaceName: '%wZ'\n", &ChanExt->StorageInterfaceName);

        Status = IoSetDeviceInterfaceState(&ChanExt->StorageInterfaceName, TRUE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString(&ChanExt->StorageInterfaceName);
            ChanExt->StorageInterfaceName.Buffer = NULL;
        }
    }

    Status = IoInitializeTimer(ChanExt->Common.Self, AtaReqPortIoTimer, ChanExt);
    if (!NT_SUCCESS(Status))
        return Status;

    if (IS_AHCI_EXT(ChanExt))
    {
        IsrHandler = AtaHbaIsr;
        IsrContext = ChanExt;
    }
    else if (IS_PCIIDE_EXT(ChanExt))
    {
        IsrHandler = AtaPciIdeChannelIsr;
        IsrContext = ChanExt->PortData;
    }
    else
    {
        IsrHandler = AtaPataChannelIsr;
        IsrContext = ChanExt->PortData;
    }
    Status = IoConnectInterrupt(&ChanExt->InterruptObject,
                                IsrHandler,
                                IsrContext,
                                NULL,
                                ChanExt->InterruptVector,
                                ChanExt->InterruptLevel,
                                ChanExt->InterruptLevel,
                                ChanExt->InterruptMode,
                                !!(ChanExt->Flags & CHANNEL_INTERRUPT_SHARED),
                                ChanExt->InterruptAffinity,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("Could not connect to interrupt %lu, status 0x%lx\n",
            ChanExt->InterruptVector, Status);

        return Status;
    }

    AtaFdoEnableInterrupts(ChanExt, TRUE);

    IoStartTimer(ChanExt->Common.Self);
    ChanExt->Flags |= CHANNEL_IO_TIMER_ACTIVE;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoStopDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    AtaFdoFreeIoResources(ChanExt);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (ChanExt->Flags & CHANNEL_IO_TIMER_ACTIVE)
    {
        ChanExt->Flags &= ~CHANNEL_IO_TIMER_ACTIVE;

        IoStopTimer(ChanExt->Common.Self);
    }

    AtaFdoFreeIoResources(ChanExt);
    AtaFdoFreePortMemory(ChanExt);

    if (ChanExt->Flags & CHANNEL_SYMLINK_CREATED)
    {
        ChanExt->Flags &= ~CHANNEL_SYMLINK_CREATED;

        AtaFdoRemoveSymLinks(ChanExt);
    }

    AtaFdoReleaseLegacyAddressRanges(ChanExt);

    if (ChanExt->StorageInterfaceName.Buffer)
    {
        IoSetDeviceInterfaceState(&ChanExt->StorageInterfaceName, FALSE);

        RtlFreeUnicodeString(&ChanExt->StorageInterfaceName);
        ChanExt->StorageInterfaceName.Buffer = NULL;
    }

    if (ChanExt->ReservedVaSpace)
    {
        MmFreeMappingAddress(ChanExt->ReservedVaSpace, ATAPORT_TAG);
        ChanExt->ReservedVaSpace = NULL;
    }

    if (IS_AHCI_EXT(ChanExt) && ChanExt->AdapterObject)
    {
        PDMA_OPERATIONS DmaOperations = ChanExt->AdapterObject->DmaOperations;

        DmaOperations->PutDmaAdapter(ChanExt->AdapterObject);
        ChanExt->AdapterObject = NULL;
    }

    /* Called from the non-PnP enumerator code */
    if (!Irp)
        return STATUS_SUCCESS;

    if (FinalRemove)
    {
        IoReleaseRemoveLockAndWait(&ChanExt->Common.RemoveLock, Irp);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Ldo, Irp);

        IoDetachDevice(ChanExt->Ldo);
        IoDeleteDevice(ChanExt->Common.Self);
    }
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(ChanExt->Ldo, Irp);

        IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryPnpDeviceState(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (ChanExt->Common.PageFiles ||
        ChanExt->Common.HibernateFiles ||
        ChanExt->Common.DumpFiles)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryDeviceUsageNotification(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
    {
        return STATUS_UNSUCCESSFUL;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &ChanExt->Common.PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &ChanExt->Common.HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &ChanExt->Common.DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryId(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;

    PAGED_CODE();

    switch (IoStack->Parameters.QueryId.IdType)
    {
        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            Status = Irp->IoStatus.Status;

            /*
             * We provide a generic identifier necessary to install the device
             * in case our FDO is root-enumerated device.
             */
            if (Status == STATUS_NOT_SUPPORTED)
            {
                static const WCHAR IdeGenericId[] = L"*PNP0600\0"; // multi-string
                PWCHAR Buffer;

                Buffer = ExAllocatePoolUninitialized(PagedPool, sizeof(IdeGenericId), ATAPORT_TAG);
                if (!Buffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory(Buffer, IdeGenericId, sizeof(IdeGenericId));

                Irp->IoStatus.Information = (ULONG_PTR)Buffer;
                Status = STATUS_SUCCESS;
            }

            break;
        }

        default:
        {
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(ChanExt->Ldo, Irp);

            IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

            return Status;
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;

    PAGED_CODE();

    INFO("(%p, %p) Ch.%lu %s\n",
         ChanExt->Common.Self,
         Irp,
         ChanExt->DeviceObjectNumber,
         GetIRPMinorFunctionString(IoGetCurrentIrpStackLocation(Irp)->MinorFunction));

    Status = IoAcquireRemoveLock(&ChanExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            if (!NT_VERIFY(IoForwardIrpSynchronously(ChanExt->Ldo, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto CompleteIrp;
            }
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Status = AtaFdoStartDevice(ChanExt,
                                       IoStack->Parameters.
                                       StartDevice.AllocatedResourcesTranslated);
            goto CompleteIrp;
        }

        case IRP_MN_STOP_DEVICE:
            Status = AtaFdoStopDevice(ChanExt, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            return AtaFdoRemoveDevice(ChanExt,
                                      Irp,
                                      (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE));

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Status = AtaFdoQueryPnpDeviceState(ChanExt, Irp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = AtaFdoQueryBusRelations(ChanExt, Irp);
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            Status = AtaFdoQueryDeviceUsageNotification(ChanExt, Irp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            return AtaFdoQueryId(ChanExt, Irp, IoStack);

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(ChanExt->Ldo, Irp);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&ChanExt->Common.RemoveLock, Irp);

    return Status;
}

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ PVOID ReferenceTag)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldLevel;
    NTSTATUS Status;

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next; Entry != NULL; Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DevExt->Device.AtaScsiAddress.AsULONG != AtaScsiAddress.AsULONG)
            continue;

        if (DevExt->ReportedMissing | DevExt->RemovalPending)
            continue;

        if (ReferenceTag)
        {
            Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, ReferenceTag);
            if (!NT_SUCCESS(Status))
                break;
        }

        Result = DevExt;
        break;
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);

    return Result;
}

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindNextDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ ULONG MatchFlags,
    _In_ PVOID ReferenceTag)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;
    KIRQL OldLevel;
    NTSTATUS Status;

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next; Entry != NULL; Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (!(MatchFlags & SEARCH_FDO_DEV) &&
             (DevExt->Device.AtaScsiAddress.PathId != AtaScsiAddress->PathId))
            continue;

        if (DevExt->Device.AtaScsiAddress.AsULONG <= AtaScsiAddress->AsULONG)
            continue;

        if (DevExt->ReportedMissing)
            continue;

        if (!(MatchFlags & SEARCH_REMOVED_DEV) && DevExt->RemovalPending)
            continue;

        *AtaScsiAddress = DevExt->Device.AtaScsiAddress;

        if (ReferenceTag)
        {
            Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, ReferenceTag);
            if (!NT_SUCCESS(Status))
                continue;
        }
        Result = DevExt;
        break;
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);

    return Result;
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN DoInsert)
{
    PSINGLE_LIST_ENTRY Entry, PrevEntry;
    KIRQL OldLevel;
    ULONG Address = DevExt->Device.AtaScsiAddress.AsULONG;

    PAGED_CODE();

    KeAcquireSpinLock(&ChanExt->PdoListLock, &OldLevel);

    for (Entry = ChanExt->PdoList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        PATAPORT_DEVICE_EXTENSION CurrentDevExt;

        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DoInsert)
        {
            if (CurrentDevExt->Device.AtaScsiAddress.AsULONG > Address)
                break;
        }
        else
        {
            if (CurrentDevExt->Device.AtaScsiAddress.AsULONG == Address)
                break;
        }

        PrevEntry = Entry;
    }

    /* The device list is ordered by SCSI address (Path:Target:Lun), smallest first */
    if (PrevEntry)
    {
        /* Before the current entry */
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = DoInsert ? &DevExt->ListEntry : DevExt->ListEntry.Next;
    }
    else
    {
        /* In the beginning */
        if (DoInsert)
            PushEntryList(&ChanExt->PdoList, &DevExt->ListEntry);
        else
            PopEntryList(&DevExt->ListEntry);
    }

    KeReleaseSpinLock(&ChanExt->PdoListLock, OldLevel);
}
