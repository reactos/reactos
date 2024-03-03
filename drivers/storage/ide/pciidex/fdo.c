/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     ATA controller FDO dispatch routines
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "pciidex.h"

/* FUNCTIONS ******************************************************************/

VOID
AtaCtrlSetTransferMode(
    _In_ PATA_CHANNEL_DATA ChanData,
    _In_reads_(MAX_IDE_DEVICE) PCHANNEL_DEVICE_DATA* DeviceList)
{
    PATA_CONTROLLER Controller = ChanData->Controller;
    ULONG i;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PCHANNEL_DEVICE_DATA Device = DeviceList[i];

        if (!Device)
            continue;

        /* Apply the channel limit */
        Device->SupportedModes &= ChanData->TransferModeSupportedBitmap;

        /* Find the fastest supported PIO and DMA mode */
        if (!_BitScanReverse(&Device->DmaMode, Device->SupportedModes & ~PIO_ALL))
            Device->DmaMode = 0;
        NT_VERIFY(_BitScanReverse(&Device->PioMode, Device->SupportedModes & PIO_ALL));
    }

    KeAcquireSpinLockAtDpcLevel(&Controller->Lock);
    ChanData->SetTransferMode(Controller, ChanData->Channel, DeviceList);
    KeReleaseSpinLockFromDpcLevel(&Controller->Lock);
}

CODE_SEG("PAGE")
VOID
AtaCtrlSaveInterruptResource(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc)
{
    PAGED_CODE();

    ASSERT(Desc->Type == CmResourceTypeInterrupt);

    RtlCopyMemory(&Controller->InterruptDesc, Desc, sizeof(*Desc));

    Controller->Flags |= CTRL_FLAG_HAS_INTERRUPT_RES;
}

CODE_SEG("PAGE")
PVOID
AtaCtrlPciGetBar(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Index)
{
    PAGED_CODE();

    if (!(Controller->AccessRange[Index].Flags & RANGE_IS_VALID))
        return NULL;

    if ((Controller->AccessRange[Index].Flags & RANGE_IS_MEMORY) &&
        !(Controller->AccessRange[Index].Flags & RANGE_IS_MAPPED))
    {
        PHYSICAL_ADDRESS PhysicalAddress;

        Controller->AccessRange[Index].Flags |= RANGE_IS_MAPPED;

        PhysicalAddress.QuadPart = (ULONG_PTR)Controller->AccessRange[Index].IoBase;
        Controller->AccessRange[Index].IoBase = MmMapIoSpace(PhysicalAddress,
                                                             Controller->AccessRange[Index].Length,
                                                             MmNonCached);
    }

    return Controller->AccessRange[Index].IoBase;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaCtrlPciGetNextBarIndex(
    _In_ PPCI_COMMON_HEADER PciData,
    _Inout_ PULONG Bar)
{
    PAGED_CODE();

    for (; *Bar < PCI_TYPE0_ADDRESSES; (*Bar)++)
    {
        ULONG Register = PciData->u.type0.BaseAddresses[*Bar];

        if (Register & PCI_ADDRESS_IO_SPACE)
            Register &= ~PCI_ADDRESS_IO_SPACE;
        else
            Register &= ~(PCI_ADDRESS_MEMORY_TYPE_MASK | PCI_ADDRESS_MEMORY_PREFETCHABLE);

        /* Look for a valid BAR */
        if (Register == 0)
            continue;

        return TRUE;
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
VOID
AtaCtrlPciAssignResources(
    _In_ PATA_CONTROLLER Controller,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated,
    _In_ PPCI_COMMON_HEADER PciData)
{
    ULONG i, Bar = 0;

    PAGED_CODE();

    for (i = 0; i < PCI_TYPE0_ADDRESSES; ++i)
    {
        Controller->AccessRange[i].Flags = 0;
    }

    if (!ResourcesTranslated)
        return;

    for (i = 0; i < ResourcesTranslated->List[0].PartialResourceList.Count; ++i)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc;
        ULONG CurrBar, NextBar;

        Desc = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
        if ((Desc->Type != CmResourceTypePort) && (Desc->Type != CmResourceTypeMemory))
        {
            if (Desc->Type == CmResourceTypeInterrupt)
                AtaCtrlSaveInterruptResource(Controller, Desc);

            continue;
        }

        if (!AtaCtrlPciGetNextBarIndex(PciData, &Bar))
            break;

        CurrBar = PciData->u.type0.BaseAddresses[Bar];
        if (Bar < (PCI_TYPE0_ADDRESSES - 1))
            NextBar = PciData->u.type0.BaseAddresses[Bar + 1];
        else
            NextBar = 0;

        if (CurrBar & PCI_ADDRESS_IO_SPACE)
        {
            if (Desc->Type != CmResourceTypePort)
                continue;

            if (Desc->u.Port.Start.QuadPart != (CurrBar & PCI_ADDRESS_IO_ADDRESS_MASK))
                continue;

            INFO("BAR[%lu]: I/O Port %I64X\n", Bar, Desc->u.Port.Start.QuadPart);
            Controller->AccessRange[Bar].Flags |= RANGE_IS_VALID;
            Controller->AccessRange[Bar].IoBase = (PVOID)(ULONG_PTR)Desc->u.Port.Start.QuadPart;
            Controller->AccessRange[Bar].Length = Desc->u.Port.Length;
        }
        else
        {
            if (Desc->Type != CmResourceTypeMemory)
                continue;

            if (Desc->u.Memory.Start.LowPart != (CurrBar & PCI_ADDRESS_MEMORY_ADDRESS_MASK))
                continue;

            if ((CurrBar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
            {
                if (Desc->u.Memory.Start.HighPart != NextBar)
                    continue;
            }
            else
            {
                if (Desc->u.Memory.Start.HighPart != 0)
                    continue;
            }

            INFO("BAR[%lu]: MEM %I64X\n", Bar, Desc->u.Memory.Start.QuadPart);
            Controller->AccessRange[Bar].Flags |= RANGE_IS_VALID | RANGE_IS_MEMORY;
            Controller->AccessRange[Bar].IoBase = (PVOID)(ULONG_PTR)Desc->u.Memory.Start.QuadPart;
            Controller->AccessRange[Bar].Length = Desc->u.Memory.Length;
        }

        ++Bar;
    }
}

static
CODE_SEG("PAGE")
VOID
AtaCtrlPciSaveData(
    _Inout_ ATA_CONTROLLER* __restrict Controller,
    _In_ PCI_COMMON_HEADER* __restrict PciData)
{
    PAGED_CODE();

    Controller->Pci.VendorID = PciData->VendorID;
    Controller->Pci.DeviceID = PciData->DeviceID;
    Controller->Pci.Command = PciData->Command;
    Controller->Pci.RevisionID = PciData->RevisionID;
    Controller->Pci.ProgIf = PciData->ProgIf;
    Controller->Pci.SubClass = PciData->SubClass;
    Controller->Pci.BaseClass = PciData->BaseClass;
    Controller->Pci.CacheLineSize = PciData->CacheLineSize;
    Controller->Pci.SubVendorID = PciData->u.type0.SubVendorID;
    Controller->Pci.SubSystemID = PciData->u.type0.SubSystemID;
}

static
CODE_SEG("PAGE")
NTSTATUS
AtaCtrlPciCollectInformation(
    _In_ PFDO_DEVICE_EXTENSION FdoExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, u.type0.SubSystemID)];
    PPCI_COMMON_HEADER PciData = (PPCI_COMMON_HEADER)Buffer; // Partial PCI header
    ULONG BytesRead;

    PAGED_CODE();

    BytesRead = (*Controller->BusInterface.GetBusData)(Controller->BusInterface.Context,
                                                       PCI_WHICHSPACE_CONFIG,
                                                       Buffer,
                                                       0,
                                                       sizeof(Buffer));
    if (BytesRead != sizeof(Buffer))
        return STATUS_IO_DEVICE_ERROR;

    AtaCtrlPciSaveData(Controller, PciData);
    AtaCtrlPciAssignResources(Controller, ResourcesTranslated, PciData);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStartDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = AtaCtrlPciCollectInformation(FdoExt, ResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to collect PCI information 0x%lx\n", Status);
        return Status;
    }

    Status = PciIdeStartController(&FdoExt->Controller);
    if (!NT_SUCCESS(Status))
    {
        ERR("Miniport initialization failed 0x%lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
VOID
PciIdeXFdoFreeResources(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PATA_CONTROLLER Controller = &FdoExtension->Controller;

    PAGED_CODE();

    if (Controller->ChanDataBlock)
    {
        ExFreePoolWithTag(Controller->ChanDataBlock, TAG_PCIIDEX);
        Controller->ChanDataBlock = NULL;
    }

    if (Controller->HwSyncObject)
    {
        IoDeleteController(Controller->HwSyncObject);
        Controller->HwSyncObject = NULL;
    }

    Controller->Flags = 0;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoStopDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PAGED_CODE();

    PciIdeXFdoFreeResources(FdoExtension);

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoRemoveDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;

    PAGED_CODE();

    IoReleaseRemoveLockAndWait(&FdoExtension->Common.RemoveLock, Irp);

    PciIdeXFdoFreeResources(FdoExtension);

    ExAcquireFastMutex(&FdoExtension->PdoListSyncMutex);

    for (ListEntry = FdoExtension->PdoListHead.Flink;
         ListEntry != &FdoExtension->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);

        PdoExt->Flags |= PDO_FLAG_REPORTED_MISSING;
        PciIdeXPdoRemoveDevice(PdoExt, Irp, TRUE, FALSE);
    }

    ExReleaseFastMutex(&FdoExtension->PdoListSyncMutex);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(FdoExtension->Common.LowerDeviceObject, Irp);

    IoDetachDevice(FdoExtension->Common.LowerDeviceObject);
    IoDeleteDevice(FdoExtension->Common.Self);

    return Status;
}

static
CODE_SEG("PAGE")
PPDO_DEVICE_EXTENSION
PciIdeXPdoCreateDevice(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG ChannelNumber)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\PciIde999Channel99-FFF")];
    PDEVICE_OBJECT Pdo;
    PPDO_DEVICE_EXTENSION PdoExtension;
    ULONG Alignment;
    static ULONG PdoNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\PciIde%luChannel%lu-%lx",
                                FdoExtension->ControllerNumber,
                                ChannelNumber,
                                PdoNumber++);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(FdoExtension->Common.Self->DriverObject,
                            sizeof(*PdoExtension),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create PDO 0x%lx\n", Status);
        return NULL;
    }

    INFO("Created device object %p '%wZ'\n", Pdo, &DeviceName);

    /* DMA buffers alignment */
    Alignment = FdoExtension->Controller.AlignmentRequirement;
    Alignment = max(Alignment, FdoExtension->Common.Self->AlignmentRequirement);
    Alignment = max(Alignment, ATA_MIN_BUFFER_ALIGNMENT);
    Pdo->AlignmentRequirement = Alignment;

    PdoExtension = Pdo->DeviceExtension;

    RtlZeroMemory(PdoExtension, sizeof(*PdoExtension));
    PdoExtension->Common.Self = Pdo;
    PdoExtension->Common.FdoExt = FdoExtension;
    IoInitializeRemoveLock(&PdoExtension->Common.RemoveLock, TAG_PCIIDEX, 0, 0);

    PdoExtension->Channel = ChannelNumber;
    PdoExtension->ChanData = FdoExtension->Controller.Channels[ChannelNumber];

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return PdoExtension;
}

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXGetChannelState(
    _In_ PATA_CONTROLLER Controller,
    _In_ ULONG Channel)
{
    PCONTROLLER_CHANNEL_ENABLED MiniportChannelEnabled;

    PAGED_CODE();

    /*
     * If any PCI IDE channel is enabled, its PCI I/O resources should be enabled as well.
     *
     * [BAR 0] IO:  Start 0:FFC0, Len 8         Channel 0 task file registers
     * [BAR 1] IO:  Start 0:FF8C, Len 4         Channel 0 device control
     * [BAR 2] IO:  Start 0:FF80, Len 8         Channel 1 task file registers
     * [BAR 3] IO:  Start 0:FF88, Len 4         Channel 1 device control
     * [BAR 4] IO:  Start 0:FFA0, Len 10        Bus Mastering registers
     * [BAR 5] MEM: Start 0:FE780000, Len 1000  Aux registers
     */
    if ((Controller->Flags & CTRL_FLAG_PCI_IDE) && !(Controller->Flags & CTRL_FLAG_IN_LEGACY_MOVE))
    {
        const ULONG BarIndex = Channel * 2;

        ASSERT(Channel < 2);

        /* Check task file registers */
        if (!(Controller->AccessRange[BarIndex].Flags & RANGE_IS_VALID))
            return ChannelDisabled;

        /* Check device control */
        if (!(Controller->AccessRange[BarIndex + 1].Flags & RANGE_IS_VALID))
            return ChannelDisabled;
    }

    /* Test PCI bits */
    if (Controller->ChannelEnableBits)
    {
        const ATA_PCI_ENABLE_BITS* EnableBits = &Controller->ChannelEnableBits[Channel];
        UCHAR RegisterValue;

        ASSERT(Controller->ChannelEnableBits);

        PciRead(Controller, &RegisterValue, EnableBits->Register, sizeof(RegisterValue));
        if ((RegisterValue & EnableBits->Mask) == EnableBits->Value)
            return ChannelEnabled;
        else
            return ChannelDisabled;
    }

    /* Device-specific test */
    MiniportChannelEnabled = Controller->ChannelEnabledTest;
    if (MiniportChannelEnabled)
        return MiniportChannelEnabled(Controller, Channel);

    /* This channel is always enabled */
    return ChannelStateUnknown;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryBusRelations(
    _In_ PFDO_DEVICE_EXTENSION FdoExt,
    _In_ PIRP Irp)
{
    PATA_CONTROLLER Controller = &FdoExt->Controller;
    PDEVICE_RELATIONS DeviceRelations;
    PLIST_ENTRY ListEntry;
    ULONG i, Size, ChannelBitmap = 0;

    PAGED_CODE();

    /*
     * Preallocate the device relations structure we need for the QBR IRP,
     * because we don't want to be in a state where allocation fails.
     */
    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects[MAX_CHANNELS]);
    DeviceRelations = ExAllocatePoolUninitialized(PagedPool, Size, TAG_PCIIDEX);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    ExAcquireFastMutex(&FdoExt->PdoListSyncMutex);

    /* Check existing ports */
    for (ListEntry = FdoExt->PdoListHead.Flink;
         ListEntry != &FdoExt->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;
        IDE_CHANNEL_STATE ChannelState;
        ULONG Channel;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);
        Channel = PdoExt->Channel;

        ChannelBitmap |= 1 << Channel;

        ChannelState = PciIdeXGetChannelState(Controller, Channel);
        if (ChannelState == ChannelDisabled)
        {
            TRACE("Channel %lu is disabled\n", Channel);

            PdoExt->Flags |= PDO_FLAG_NOT_PRESENT;
            continue;
        }
    }

    /* Enumerate the remaining ports */
    ChannelBitmap = NUM_TO_BITMAP(Controller->MaxChannels) & ~ChannelBitmap;
    for (i = 0; i < MAX_CHANNELS; ++i)
    {
        PPDO_DEVICE_EXTENSION PdoExt;
        IDE_CHANNEL_STATE ChannelState;

        if (!(ChannelBitmap & (1 << i)))
            continue;

        ChannelState = PciIdeXGetChannelState(Controller, i);
        if (ChannelState == ChannelDisabled)
        {
            TRACE("Channel %lu is disabled\n", i);
            continue;
        }

        /* Need to create a PDO */
        PdoExt = PciIdeXPdoCreateDevice(FdoExt, i);
        if (!PdoExt)
        {
            /* We are out of memory, trying to continue process the QBR IRP anyway */
            continue;
        }

        InsertTailList(&FdoExt->PdoListHead, &PdoExt->ListEntry);
    }

    /* Initialize the device relations structure */
    DeviceRelations->Count = 0;
    for (ListEntry = FdoExt->PdoListHead.Flink;
         ListEntry != &FdoExt->PdoListHead;
         ListEntry = ListEntry->Flink)
    {
        PPDO_DEVICE_EXTENSION PdoExt;

        PdoExt = CONTAINING_RECORD(ListEntry, PDO_DEVICE_EXTENSION, ListEntry);
        if (PdoExt->Flags & PDO_FLAG_NOT_PRESENT)
        {
            PdoExt->Flags |= PDO_FLAG_REPORTED_MISSING;
            continue;
        }

        DeviceRelations->Objects[DeviceRelations->Count++] = PdoExt->Common.Self;
        ObReferenceObject(PdoExt->Common.Self);
    }

    ExReleaseFastMutex(&FdoExt->PdoListSyncMutex);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoQueryInterface(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PAGED_CODE();

    if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                           &GUID_TRANSLATOR_INTERFACE_STANDARD))
    {
        PTRANSLATOR_INTERFACE TranslatorInterface;
        CM_RESOURCE_TYPE ResourceType;
        ULONG BusNumber;

        /* In native mode the IDE controller does not use any legacy interrupt resources */
        if (!(FdoExtension->Controller.Flags & CTRL_FLAG_IN_LEGACY_MOVE))
            return STATUS_NOT_SUPPORTED;

        if (IoStack->Parameters.QueryInterface.Size < sizeof(*TranslatorInterface))
            return STATUS_NOT_SUPPORTED;

        ResourceType = PtrToUlong(IoStack->Parameters.QueryInterface.InterfaceSpecificData);
        if (ResourceType != CmResourceTypeInterrupt)
            return STATUS_NOT_SUPPORTED;

        TranslatorInterface = (PTRANSLATOR_INTERFACE)IoStack->Parameters.QueryInterface.Interface;

        return HalGetInterruptTranslator(PCIBus,
                                         0,
                                         InterfaceTypeUndefined,
                                         sizeof(*TranslatorInterface),
                                         IoStack->Parameters.QueryInterface.Version,
                                         TranslatorInterface,
                                         &BusNumber);
    }

    return STATUS_NOT_SUPPORTED;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoAcquireRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
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
            if (!NT_VERIFY(IoForwardIrpSynchronously(FdoExtension->Common.LowerDeviceObject, Irp)))
            {
                Status = STATUS_UNSUCCESSFUL;
                goto CompleteIrp;
            }
            Status = Irp->IoStatus.Status;
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Status = PciIdeXFdoStartDevice(FdoExtension,
                                           IoStack->Parameters.
                                           StartDevice.AllocatedResourcesTranslated);
            goto CompleteIrp;
        }

        case IRP_MN_STOP_DEVICE:
        {
            Status = PciIdeXFdoStopDevice(FdoExtension);
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
            return PciIdeXFdoRemoveDevice(FdoExtension, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            Status = PciIdeXPnpQueryPnpDeviceState(&FdoExtension->Common, Irp);
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            Status = PciIdeXFdoQueryBusRelations(FdoExtension, Irp);
            if (!NT_SUCCESS(Status))
                goto CompleteIrp;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            Status = PciIdeXPnpQueryDeviceUsageNotification(&FdoExtension->Common, Irp);
            break;
        }

        case IRP_MN_QUERY_INTERFACE:
        {
            Status = PciIdeXFdoQueryInterface(FdoExtension, IoStack);
            if (Status == STATUS_NOT_SUPPORTED)
                break;

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(FdoExtension->Common.LowerDeviceObject, Irp);

    IoReleaseRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
    return Status;

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&FdoExtension->Common.RemoveLock, Irp);
    return Status;
}
