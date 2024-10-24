/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA channel device object (FDO) dispatch routines
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
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

    ASSERT(!IS_AHCI(ChanExt));

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
    static const WCHAR IdeChannelFormat[] = L"\\Device\\Ide\\IdePort%lu";

    PAGED_CODE();

    Status = RtlStringCbPrintfW(ChannelNameBuffer,
                                sizeof(ChannelNameBuffer),
                                IdeChannelFormat,
                                ChanExt->ChannelNumber);
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
        if (NT_SUCCESS(Status))
        {
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

            ChanExt->PortNumber = ScsiAdapter;
            ChanExt->Flags |= CHANNEL_SYMLINK_CREATED;
            break;
        }
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
                                ChanExt->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Delete the '\Device\\ScsiPortX' symbolic link */
    Status = RtlStringCbPrintfW(SymLinkNameBuffer,
                                sizeof(SymLinkNameBuffer),
                                AtapDosSymLinkFormat,
                                ChanExt->PortNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&SymLinkName, SymLinkNameBuffer);
    (VOID)IoDeleteSymbolicLink(&SymLinkName);

    /* Unregister the SCSI port adapter */
    --IoGetConfigurationInformation()->ScsiPortCount;
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
CODE_SEG("PAGE")
NTSTATUS
AtaFdoParsePataResources(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PIDE_REGISTERS Registers = &PortData->Pata.Registers;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CommandPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ControlPortDescriptor = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDescriptor = NULL;
    PVOID IoBase;
    ULONG i, Spare;

    PAGED_CODE();

    ASSERT(!IS_AHCI(ChanExt));

    if (!ResourcesTranslated)
        return STATUS_INSUFFICIENT_RESOURCES;

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

    if (!CommandPortDescriptor || !ControlPortDescriptor || !InterruptDescriptor)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    /* Command port base */
    if ((CommandPortDescriptor->Type == CmResourceTypePort) &&
        (CommandPortDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)CommandPortDescriptor->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(PortData->PortFlags & PORT_FLAG_CBUS_IDE));

        IoBase = MmMapIoSpace(CommandPortDescriptor->u.Memory.Start, 8, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChanExt->Flags |= CHANNEL_COMMAND_PORT_BASE_MAPPED;
    }
    PortData->Pata.CommandPortBase = (PVOID)(ULONG_PTR)CommandPortDescriptor->u.Port.Start.QuadPart;

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
    Registers->Data        = IoBase;
    Registers->Error       = (PVOID)((ULONG_PTR)IoBase + 1 * Spare);
    Registers->SectorCount = (PVOID)((ULONG_PTR)IoBase + 2 * Spare);
    Registers->LbaLow      = (PVOID)((ULONG_PTR)IoBase + 3 * Spare);
    Registers->LbaMid      = (PVOID)((ULONG_PTR)IoBase + 4 * Spare);
    Registers->LbaHigh     = (PVOID)((ULONG_PTR)IoBase + 5 * Spare);
    Registers->Device      = (PVOID)((ULONG_PTR)IoBase + 6 * Spare);
    Registers->Status      = (PVOID)((ULONG_PTR)IoBase + 7 * Spare);

    /* Control port base */
    if ((ControlPortDescriptor->Type == CmResourceTypePort) &&
        (ControlPortDescriptor->Flags & CM_RESOURCE_PORT_IO))
    {
        IoBase = (PVOID)(ULONG_PTR)ControlPortDescriptor->u.Port.Start.QuadPart;
    }
    else
    {
        ASSERT(!(PortData->PortFlags & PORT_FLAG_CBUS_IDE));

        IoBase = MmMapIoSpace(ControlPortDescriptor->u.Memory.Start, 1, MmNonCached);
        if (!IoBase)
            return STATUS_INSUFFICIENT_RESOURCES;

        ChanExt->Flags |= CHANNEL_CONTROL_PORT_BASE_MAPPED;
    }
    Registers->Control = (PVOID)IoBase;
    PortData->Pata.ControlPortBase = (PVOID)(ULONG_PTR)ControlPortDescriptor->u.Port.Start.QuadPart;

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

    if (IS_AHCI(ChanExt))
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

        Command = Context->Enable ? IDE_DC_REENABLE_CONTROLLER : IDE_DC_DISABLE_INTERRUPTS;

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

    TRACE("Initializing port #%u\n", PortNumber);

    InitializeListHead(&PortData->PortQueueList);
    KeInitializeSpinLock(&PortData->PortLock);

    PortData->PortFlags = PORT_FLAG_ACTIVE;

    /* We need the slot numbers to start from zero */
    PortData->LastUsedSlot = AHCI_MAX_COMMAND_SLOTS - 1;

    if (IS_AHCI(ChanExt))
    {
        ULONG QueueDepth;

        QueueDepth = ((ChanExt->AhciCapabilities & AHCI_CAP_NCS) >> 8) + 1;
        PortData->FreeSlotsBitmap = 0xFFFFFFFF >> (RTL_BITS_OF(ULONG) - QueueDepth);

        PortData->Ahci.IoBase = AHCI_PORT_BASE(ChanExt->IoBase, PortNumber);

        KeInitializeDpc(&PortData->Ahci.Dpc, AtaAhciPortDpc, PortData);

        return AtaAhciPortInit(ChanExt, PortData, PortInfo);
    }
    else
    {
        PortData->FreeSlotsBitmap = 1;

        KeInitializeDpc(&PortData->Pata.Dpc, AtaPataChannelDpc, PortData);

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

    if (IS_AHCI(ChanExt))
        Size += sizeof(*PortInfo);
    else
        Size += ATA_LOCAL_BUFFER_SIZE;

    Size *= ChanExt->NumberOfPorts;

    ChanExt->PortData = ExAllocatePoolZero(NonPagedPoolCacheAligned, Size, ATAPORT_TAG);
    if (!ChanExt->PortData)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (IS_AHCI(ChanExt))
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

    INFO("Starting channel %lu\n", ChanExt->ChannelNumber);

    if (IS_AHCI(ChanExt))
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
    }
    else
    {
        PATAPORT_PORT_DATA PortData;

        ChanExt->NumberOfPorts = 1;
        ChanExt->PortBitmap = 1;

        Status = AtaFdoCreatePortData(ChanExt);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create port data 0x%lx\n", Status);
            return Status;
        }
        PortData = ChanExt->PortData;

        /* Maximum for PIO or DMA data transfers */
        ChanExt->MaximumTransferLength = ATA_MAX_TRANSFER_LENGTH;

        /* Get the interface of the PCI IDE controller */
        if (!IS_LEGACY_IDE(ChanExt))
        {
            Status = AtaFdoQueryInterface(ChanExt,
                                          &GUID_PCIIDE_INTERFACE,
                                          &PortData->Pata.PciIdeInterface,
                                          sizeof(PortData->Pata.PciIdeInterface));
            if (NT_SUCCESS(Status))
            {
                PPCIIDE_INTERFACE PciIdeInterface = &PortData->Pata.PciIdeInterface;

                /* The hardware can use DMA */
                ChanExt->Common.Flags |= DO_IS_PCI_IDE;

                ChanExt->MaximumTransferLength =
                    min(ChanExt->MaximumTransferLength, PciIdeInterface->MaximumTransferLength);

                ChanExt->AdapterObject = PciIdeInterface->AdapterObject;
                ChanExt->AdapterDeviceObject = PciIdeInterface->DeviceObject;
            }
        }

        Status = AtaFdoParsePataResources(ChanExt, PortData, ResourcesTranslated);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to parse resources 0x%lx\n", Status);
            return Status;
        }

        /* Reserve memory resources early. Storage drivers should not fail paging I/O operations */
        if (!ChanExt->ReservedVaSpace)
        {
            ChanExt->ReservedVaSpace =
                MmAllocateMappingAddress(ATA_RESERVED_PAGES * PAGE_SIZE, ATAPORT_TAG);
        }

        AtaFdoClaimLegacyAddressRanges(ChanExt, PortData);
    }

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

    if (IS_AHCI(ChanExt))
    {
        IsrHandler = AtaHbaIsr;
        IsrContext = ChanExt;
    }
    else if (IS_PCIIDE(ChanExt))
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

    if (IS_AHCI(ChanExt) && ChanExt->AdapterObject)
    {
        PDMA_OPERATIONS DmaOperations = ChanExt->AdapterObject->DmaOperations;

        DmaOperations->PutDmaAdapter(ChanExt->AdapterObject);
        ChanExt->AdapterObject = NULL;
    }

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
                static const WCHAR IdeGenericId[] = L"*PNP0600\0";
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

    INFO("(%p, %p) Ch.%lu\n",
         ChanExt->Common.Self,
         Irp,
         ChanExt->ChannelNumber);

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

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress)
{
    PATAPORT_DEVICE_EXTENSION DevExt, Result = NULL;
    PSINGLE_LIST_ENTRY Entry;

    PAGED_CODE();

    for (Entry = ChanExt->DeviceList.Next;
         Entry != NULL;
         Entry = Entry->Next)
    {
        DevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (DevExt->AtaScsiAddress.AsULONG == AtaScsiAddress.AsULONG)
        {
            Result = DevExt;
            break;
        }
    }

    return Result;
}

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListRemove(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PSINGLE_LIST_ENTRY Entry, PrevEntry;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = ChanExt->DeviceList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        PATAPORT_DEVICE_EXTENSION CurrentDevExt;

        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (CurrentDevExt->AtaScsiAddress.AsULONG == DevExt->AtaScsiAddress.AsULONG)
            break;

        PrevEntry = Entry;
    }

    if (PrevEntry)
    {
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = DevExt->ListEntry.Next;
    }
    else
    {
        PopEntryList(&DevExt->ListEntry);
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);
}

CODE_SEG("PAGE")
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_DEVICE_EXTENSION CurrentDevExt;
    PSINGLE_LIST_ENTRY Entry, PrevEntry;
    ATA_SCSI_ADDRESS Address = DevExt->AtaScsiAddress;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    for (Entry = ChanExt->DeviceList.Next, PrevEntry = NULL;
         Entry != NULL;
         Entry = Entry->Next)
    {
        CurrentDevExt = CONTAINING_RECORD(Entry, ATAPORT_DEVICE_EXTENSION, ListEntry);

        if (CurrentDevExt->AtaScsiAddress.AsULONG > Address.AsULONG)
            break;

        PrevEntry = Entry;
    }

    /* The device list is ordered by SCSI address, smallest first */
    if (PrevEntry)
    {
        /* Insert before the current entry */
        DevExt->ListEntry.Next = PrevEntry->Next;
        PrevEntry->Next = &DevExt->ListEntry;
    }
    else
    {
        /* Insert in the beginning */
        PushEntryList(&ChanExt->DeviceList, &DevExt->ListEntry);
    }

    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);
}
