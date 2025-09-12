/*
 * PROJECT:     ReactOS USB UHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBUHCI main driver functions
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbuhci.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_UHCI_TRACE
#define NDEBUG_UHCI_IMPLEMENT
#include "dbg_uhci.h"

USBPORT_REGISTRATION_PACKET RegPacket;

VOID
NTAPI
UhciDumpHcdQH(PUHCI_HCD_QH QH)
{
    DPRINT("QH              - %p\n", QH);
    DPRINT("NextQH          - %p\n", QH->HwQH.NextQH);
    DPRINT("NextElement     - %p\n", QH->HwQH.NextElement);

    DPRINT("PhysicalAddress - %p\n", QH->PhysicalAddress);
    DPRINT("QhFlags         - %X\n", QH->QhFlags);
    DPRINT("NextHcdQH       - %X\n", QH->NextHcdQH);
    DPRINT("PrevHcdQH       - %X\n", QH->PrevHcdQH);
    DPRINT("UhciEndpoint    - %X\n", QH->UhciEndpoint);
}

VOID
NTAPI
UhciDumpHcdTD(PUHCI_HCD_TD TD)
{
    DPRINT("TD              - %p\n", TD);
    DPRINT("NextElement     - %p\n", TD->HwTD.NextElement);
    DPRINT("ControlStatus   - %08X\n", TD->HwTD.ControlStatus.AsULONG);
    DPRINT("Token           - %p\n", TD->HwTD.Token.AsULONG);
    if (TD->HwTD.Buffer)
        DPRINT("Buffer          - %p\n", TD->HwTD.Buffer);

    if (TD->SetupPacket.bmRequestType.B)
        DPRINT("bmRequestType   - %02X\n", TD->SetupPacket.bmRequestType.B);
    if (TD->SetupPacket.bRequest)
        DPRINT("bRequest        - %02X\n", TD->SetupPacket.bRequest);
    if (TD->SetupPacket.wValue.W)
        DPRINT("wValue          - %04X\n", TD->SetupPacket.wValue.W);
    if (TD->SetupPacket.wIndex.W)
        DPRINT("wIndex          - %04X\n", TD->SetupPacket.wIndex.W);
    if (TD->SetupPacket.wLength)
        DPRINT("wLength         - %04X\n", TD->SetupPacket.wLength);

    DPRINT("PhysicalAddress - %p\n", TD->PhysicalAddress);
    DPRINT("Flags           - %X\n", TD->Flags);
    DPRINT("NextHcdTD       - %p\n", TD->NextHcdTD);
    DPRINT("UhciTransfer    - %p\n", TD->UhciTransfer);
}

VOID
NTAPI
UhciFixDataToggle(IN PUHCI_EXTENSION UhciExtension,
                  IN PUHCI_ENDPOINT UhciEndpoint,
                  IN PUHCI_HCD_TD TD,
                  IN BOOL DataToggle)
{
    DPRINT_UHCI("UhciFixDataToggle: UhciExtension - %p, UhciEndpoint - %p, DataToggle - %X\n",
                UhciExtension,
                UhciEndpoint,
                DataToggle);

    while (TD)
    {
        TD->HwTD.Token.DataToggle = !TD->HwTD.Token.DataToggle;
        DataToggle = !DataToggle;

        TD = TD->NextHcdTD;
    }

    UhciEndpoint->DataToggle = DataToggle;
}

VOID
NTAPI
UhciCleanupFrameListEntry(IN PUHCI_EXTENSION UhciExtension,
                          IN ULONG Index)
{
    PUHCI_HC_RESOURCES UhciResources;
    ULONG PhysicalAddress;
    ULONG HeadIdx;

    UhciResources = UhciExtension->HcResourcesVA;

    if (Index == 0)
    {
        PhysicalAddress = UhciExtension->StaticTD->PhysicalAddress;

        UhciResources->FrameList[0] = PhysicalAddress |
                                      UHCI_FRAME_LIST_POINTER_TD;
    }
    else
    {
        HeadIdx = (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms) +
                  (Index & (ENDPOINT_INTERRUPT_32ms - 1));

        PhysicalAddress = UhciExtension->IntQH[HeadIdx]->PhysicalAddress;

        UhciResources->FrameList[Index] = PhysicalAddress |
                                          UHCI_FRAME_LIST_POINTER_QH;
    }
}

VOID
NTAPI
UhciCleanupFrameList(IN PUHCI_EXTENSION UhciExtension,
                     IN BOOLEAN IsAllEntries)
{
    ULONG NewFrameNumber;
    ULONG OldFrameNumber;
    ULONG ix;

    DPRINT_UHCI("UhciCleanupFrameList: [%p] All - %x\n",
                UhciExtension, IsAllEntries);

    // FIXME: using UhciExtension->LockFrameList after supporting ISOs.

    NewFrameNumber = UhciGet32BitFrameNumber(UhciExtension);
    OldFrameNumber = UhciExtension->FrameNumber;

    if ((NewFrameNumber - OldFrameNumber) < UHCI_FRAME_LIST_MAX_ENTRIES &&
        IsAllEntries == FALSE)
    {
        for (ix = OldFrameNumber & UHCI_FRAME_LIST_INDEX_MASK;
             ix != (NewFrameNumber & UHCI_FRAME_LIST_INDEX_MASK);
             ix = (ix + 1) & UHCI_FRAME_LIST_INDEX_MASK)
        {
            UhciCleanupFrameListEntry(UhciExtension, ix);
        }
    }
    else
    {
        for (ix = 0; ix < UHCI_FRAME_LIST_MAX_ENTRIES; ++ix)
        {
            UhciCleanupFrameListEntry(UhciExtension, ix);
        }
    }

    UhciExtension->FrameNumber = NewFrameNumber;
}

VOID
NTAPI
UhciUpdateCounter(IN PUHCI_EXTENSION UhciExtension)
{
    ULONG FrameNumber;

    FrameNumber = READ_PORT_USHORT(&UhciExtension->BaseRegister->FrameNumber);
    FrameNumber &= UHCI_FRNUM_FRAME_MASK;

    if ((FrameNumber ^ UhciExtension->FrameHighPart) & UHCI_FRNUM_OVERFLOW_LIST)
    {
        UhciExtension->FrameHighPart += UHCI_FRAME_LIST_MAX_ENTRIES;

        DPRINT_UHCI("UhciUpdateCounter: UhciExtension->FrameHighPart - %lX\n",
                    UhciExtension->FrameHighPart);
    }
}

VOID
NTAPI
UhciSetNextQH(IN PUHCI_HCD_QH QH,
              IN PUHCI_HCD_QH NextQH)
{
    DPRINT_UHCI("UhciSetNextQH: QH - %p, NextQH - %p\n", QH, NextQH);

    QH->HwQH.NextQH = NextQH->PhysicalAddress | UHCI_QH_HEAD_LINK_PTR_QH;
    QH->NextHcdQH = NextQH;

    NextQH->PrevHcdQH = QH;
    NextQH->QhFlags |= UHCI_HCD_QH_FLAG_ACTIVE;
}

MPSTATUS
NTAPI
UhciOpenEndpoint(IN PVOID uhciExtension,
                 IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                 IN PVOID uhciEndpoint)
{
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG TransferType;
    ULONG_PTR BufferVA;
    ULONG BufferPA;
    ULONG ix;
    ULONG TdCount;
    PUHCI_HCD_TD TD;
    SIZE_T BufferLength;
    PUHCI_HCD_QH QH;

    RtlCopyMemory(&UhciEndpoint->EndpointProperties,
                  EndpointProperties,
                  sizeof(UhciEndpoint->EndpointProperties));

    InitializeListHead(&UhciEndpoint->ListTDs);

    UhciEndpoint->EndpointLock = 0;
    UhciEndpoint->DataToggle = UHCI_TD_PID_DATA0;
    UhciEndpoint->Flags = 0;

    TransferType = EndpointProperties->TransferType;

    DPRINT("UhciOpenEndpoint: UhciEndpoint - %p, TransferType - %x\n",
           UhciEndpoint,
           TransferType);

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        UhciEndpoint->Flags |= UHCI_ENDPOINT_FLAG_CONTROL_OR_ISO;
    }

    BufferVA = EndpointProperties->BufferVA;
    BufferPA = EndpointProperties->BufferPA;

    BufferLength = EndpointProperties->BufferLength;

    /* For Isochronous transfers not used QHs (only TDs) */
    if (EndpointProperties->TransferType != USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        /* Initialize HCD Queue Head */
        UhciEndpoint->QH = (PUHCI_HCD_QH)BufferVA;

        QH = UhciEndpoint->QH;

        QH->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        QH->PhysicalAddress = BufferPA;

        QH->NextHcdQH = QH;
        QH->PrevHcdQH = QH;
        QH->UhciEndpoint = UhciEndpoint;

        BufferVA += sizeof(UHCI_HCD_QH);
        BufferPA += sizeof(UHCI_HCD_QH);

        BufferLength -= sizeof(UHCI_HCD_QH);
    }

    /* Initialize HCD Transfer Descriptors */
    TdCount = BufferLength / sizeof(UHCI_HCD_TD);
    UhciEndpoint->MaxTDs = TdCount;

    UhciEndpoint->FirstTD = (PUHCI_HCD_TD)BufferVA;
    UhciEndpoint->AllocatedTDs = 0;

    RtlZeroMemory(UhciEndpoint->FirstTD, TdCount * sizeof(UHCI_HCD_TD));

    for (ix = 0; ix < UhciEndpoint->MaxTDs; ix++)
    {
        TD = &UhciEndpoint->FirstTD[ix];
        TD->PhysicalAddress = BufferPA;
        BufferPA += sizeof(UHCI_HCD_TD);
    }

    UhciEndpoint->TailTD = NULL;
    UhciEndpoint->HeadTD = NULL;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciReopenEndpoint(IN PVOID uhciExtension,
                   IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                   IN PVOID uhciEndpoint)
{
    DPRINT_IMPL("Uhci: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciQueryEndpointRequirements(IN PVOID uhciExtension,
                              IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                              IN PUSBPORT_ENDPOINT_REQUIREMENTS EndpointRequirements)
{
    ULONG TransferType;
    ULONG TdCount;

    DPRINT("UhciQueryEndpointRequirements: ... \n");

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT("UhciQueryEndpointRequirements: IsoTransfer\n");
            TdCount = 2 * UHCI_MAX_ISO_TD_COUNT;

            EndpointRequirements->HeaderBufferSize = 0 + // Iso queue is have not Queue Heads
                                                     TdCount * sizeof(UHCI_HCD_TD);

            EndpointRequirements->MaxTransferSize = UHCI_MAX_ISO_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT("UhciQueryEndpointRequirements: ControlTransfer\n");
            TdCount = EndpointProperties->MaxTransferSize /
                      EndpointProperties->TotalMaxPacketSize;

            TdCount += 2; // First + Last TDs

            EndpointRequirements->HeaderBufferSize = sizeof(UHCI_HCD_QH) +
                                                     TdCount * sizeof(UHCI_HCD_TD);

            EndpointRequirements->MaxTransferSize = EndpointProperties->MaxTransferSize;
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT("UhciQueryEndpointRequirements: BulkTransfer\n");
            TdCount = 2 * UHCI_MAX_BULK_TRANSFER_SIZE /
                      EndpointProperties->TotalMaxPacketSize;

            EndpointRequirements->HeaderBufferSize = sizeof(UHCI_HCD_QH) +
                                                     TdCount * sizeof(UHCI_HCD_TD);

            EndpointRequirements->MaxTransferSize = UHCI_MAX_BULK_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT("UhciQueryEndpointRequirements: InterruptTransfer\n");
            TdCount = 2 * UHCI_MAX_INTERRUPT_TD_COUNT;

            EndpointRequirements->HeaderBufferSize = sizeof(UHCI_HCD_QH) +
                                                     TdCount * sizeof(UHCI_HCD_TD);

            EndpointRequirements->MaxTransferSize = UHCI_MAX_INTERRUPT_TD_COUNT *
                                                    EndpointProperties->TotalMaxPacketSize;
            break;

        default:
            DPRINT1("UhciQueryEndpointRequirements: Unknown TransferType - %x\n",
                    TransferType);
            DbgBreakPoint();
            break;
    }
}

VOID
NTAPI
UhciCloseEndpoint(IN PVOID uhciExtension,
                  IN PVOID uhciEndpoint,
                  IN BOOLEAN IsDoDisablePeriodic)
{
    DPRINT_IMPL("UhciCloseEndpoint: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciTakeControlHC(IN PUHCI_EXTENSION UhciExtension,
                  IN PUSBPORT_RESOURCES Resources)
{
    LARGE_INTEGER EndTime;
    LARGE_INTEGER CurrentTime;
    ULONG ResourcesTypes;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT StatusRegister;
    UHCI_PCI_LEGSUP LegacySupport;
    UHCI_PCI_LEGSUP LegacyMask;
    UHCI_USB_COMMAND Command;
    UHCI_USB_STATUS HcStatus;
    MPSTATUS MpStatus = MP_STATUS_SUCCESS;

    DPRINT("UhciTakeControlHC: Resources->ResourcesTypes - %x\n",
           Resources->ResourcesTypes);

    ResourcesTypes = Resources->ResourcesTypes;

    if ((ResourcesTypes & (USBPORT_RESOURCES_PORT | USBPORT_RESOURCES_INTERRUPT)) !=
                          (USBPORT_RESOURCES_PORT | USBPORT_RESOURCES_INTERRUPT))
    {
        DPRINT1("UhciTakeControlHC: MP_STATUS_ERROR\n");
        MpStatus = MP_STATUS_ERROR;
    }

    BaseRegister = UhciExtension->BaseRegister;
    StatusRegister = &BaseRegister->HcStatus.AsUSHORT;

    RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                          TRUE,
                                          &LegacySupport.AsUSHORT,
                                          PCI_LEGSUP,
                                          sizeof(USHORT));

    UhciDisableInterrupts(UhciExtension);

    Command.AsUSHORT = READ_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT);

    Command.Run = 0;
    Command.GlobalReset = 0;
    Command.ConfigureFlag = 0;

    WRITE_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT, Command.AsUSHORT);

    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += 100 * 10000; // 100 ms

    HcStatus.AsUSHORT = READ_PORT_USHORT(StatusRegister);
    DPRINT("UhciTakeControlHC: HcStatus.AsUSHORT - %04X\n", HcStatus.AsUSHORT);

    while (HcStatus.HcHalted == 0)
    {
        HcStatus.AsUSHORT = READ_PORT_USHORT(StatusRegister);
        KeQuerySystemTime(&CurrentTime);

        if (CurrentTime.QuadPart >= EndTime.QuadPart)
            break;
    }

    WRITE_PORT_USHORT(StatusRegister, UHCI_USB_STATUS_MASK);

    LegacyMask.AsUSHORT = 0;
    LegacyMask.Smi60Read = 1;
    LegacyMask.Smi60Write = 1;
    LegacyMask.Smi64Read = 1;
    LegacyMask.Smi64Write = 1;
    LegacyMask.SmiIrq = 1;
    LegacyMask.A20Gate = 1;
    LegacyMask.SmiEndPassThrough = 1;

    if (LegacySupport.AsUSHORT & LegacyMask.AsUSHORT)
    {
        DPRINT("UhciTakeControlHC: LegacySupport.AsUSHORT - %04X\n",
               LegacySupport.AsUSHORT);

        Resources->LegacySupport = 1;

        RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                              TRUE,
                                              &LegacySupport.AsUSHORT,
                                              PCI_LEGSUP,
                                              sizeof(USHORT));
        LegacySupport.AsUSHORT = 0;

        RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                              FALSE,
                                              &LegacySupport.AsUSHORT,
                                              PCI_LEGSUP,
                                              sizeof(USHORT));
    }

    return MpStatus;
}

MPSTATUS
NTAPI
UhciInitializeHardware(IN PUHCI_EXTENSION UhciExtension)
{
    PUHCI_HW_REGISTERS BaseRegister;
    UHCI_USB_COMMAND Command;
    UHCI_USB_STATUS StatusMask;

    DPRINT("UhciInitializeHardware: UhciExtension - %p\n", UhciExtension);
    DPRINT("UhciInitializeHardware: VIA HW FIXME\n"); // after supporting HcFlavor in usbport

    BaseRegister = UhciExtension->BaseRegister;

    /* Save SOF Timing Value */
    UhciExtension->SOF_Modify = READ_PORT_UCHAR(&BaseRegister->SOF_Modify);

    RegPacket.UsbPortWait(UhciExtension, 20);

    Command.AsUSHORT = READ_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT);

    /* Global Reset */
    Command.AsUSHORT = 0;
    Command.GlobalReset = 1;
    WRITE_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT, Command.AsUSHORT);

    RegPacket.UsbPortWait(UhciExtension, 20);

    Command.AsUSHORT = 0;
    WRITE_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT, Command.AsUSHORT);

    /* Set MaxPacket for full speed bandwidth reclamation */
    Command.AsUSHORT = 0;
    Command.MaxPacket = 1; // 64 bytes
    WRITE_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT, Command.AsUSHORT);

    /* Restore SOF Timing Value */
    WRITE_PORT_UCHAR(&BaseRegister->SOF_Modify, UhciExtension->SOF_Modify);

    StatusMask = UhciExtension->StatusMask;

    StatusMask.Interrupt = 1;
    StatusMask.ErrorInterrupt = 1;
    StatusMask.ResumeDetect = 1;
    StatusMask.HostSystemError = 1;

    UhciExtension->StatusMask = StatusMask;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciInitializeSchedule(IN PUHCI_EXTENSION UhciExtension,
                       IN PUHCI_HC_RESOURCES HcResourcesVA,
                       IN ULONG hcResourcesPA)
{
    PUHCI_HCD_QH IntQH;
    ULONG IntQhPA;
    PUHCI_HCD_QH StaticControlHead;
    ULONG StaticControlHeadPA;
    PUHCI_HCD_QH StaticBulkHead;
    ULONG StaticBulkHeadPA;
    PUHCI_HCD_TD StaticBulkTD;
    ULONG StaticBulkTdPA;
    PUHCI_HCD_TD StaticTD;
    ULONG StaticTdPA;
    PUHCI_HCD_TD StaticSofTD;
    ULONG StaticSofTdPA;
    ULONG PhysicalAddress;
    ULONG Idx;
    ULONG HeadIdx;
    UCHAR FrameIdx;

    DPRINT("UhciInitializeSchedule: Ext[%p], VA - %p, PA - %lx\n",
           UhciExtension,
           HcResourcesVA,
           hcResourcesPA);

    /* Build structure (tree) of static QHs
       for interrupt and isochronous transfers */
    for (FrameIdx = 0; FrameIdx < INTERRUPT_ENDPOINTs; FrameIdx++)
    {
        IntQH = &HcResourcesVA->StaticIntHead[FrameIdx];
        IntQhPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticIntHead[FrameIdx]);

        RtlZeroMemory(IntQH, sizeof(UHCI_HCD_QH));

        IntQH->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        IntQH->PhysicalAddress = IntQhPA;

        UhciExtension->IntQH[FrameIdx] = IntQH;

        if (FrameIdx == 0)
            UhciSetNextQH(IntQH, UhciExtension->IntQH[0]);
        else
            UhciSetNextQH(IntQH, UhciExtension->IntQH[(FrameIdx - 1) / 2]);
    }

    /* Initialize static QH for control transfers */
    StaticControlHead = &HcResourcesVA->StaticControlHead;
    StaticControlHeadPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticControlHead);

    RtlZeroMemory(StaticControlHead, sizeof(UHCI_HCD_QH));

    StaticControlHead->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    StaticControlHead->PhysicalAddress = StaticControlHeadPA;

    UhciSetNextQH(UhciExtension->IntQH[0],StaticControlHead);

    UhciExtension->ControlQH = StaticControlHead;

    /* Initialize static QH for bulk transfers */
    StaticBulkHead = &HcResourcesVA->StaticBulkHead;
    StaticBulkHeadPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticBulkHead);

    RtlZeroMemory(StaticBulkHead, sizeof(UHCI_HCD_QH));

    StaticBulkHead->PhysicalAddress = StaticBulkHeadPA;
    PhysicalAddress = StaticBulkHeadPA | UHCI_QH_ELEMENT_LINK_PTR_QH;
    PhysicalAddress |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    StaticBulkHead->HwQH.NextQH = PhysicalAddress;

    UhciSetNextQH(StaticControlHead, StaticBulkHead);

    UhciExtension->BulkQH = StaticBulkHead;
    UhciExtension->BulkTailQH = StaticBulkHead;

    /* Initialize static TD for bulk transfers */
    StaticBulkTD = &HcResourcesVA->StaticBulkTD;
    StaticBulkTdPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticBulkTD);

    StaticBulkTD->HwTD.NextElement = StaticBulkTdPA | UHCI_TD_LINK_PTR_TD;

    StaticBulkTD->HwTD.ControlStatus.AsULONG = 0;
    StaticBulkTD->HwTD.ControlStatus.IsochronousType = 1;

    StaticBulkTD->HwTD.Token.AsULONG = 0;
    StaticBulkTD->HwTD.Token.Endpoint = 1;
    StaticBulkTD->HwTD.Token.MaximumLength = UHCI_TD_LENGTH_NULL;
    StaticBulkTD->HwTD.Token.PIDCode = UHCI_TD_PID_OUT;

    StaticBulkTD->HwTD.Buffer = 0;

    StaticBulkTD->PhysicalAddress = StaticBulkTdPA;
    StaticBulkTD->NextHcdTD = NULL;
    StaticBulkTD->Flags = UHCI_HCD_TD_FLAG_PROCESSED;

    PhysicalAddress = StaticBulkTdPA | UHCI_QH_ELEMENT_LINK_PTR_TD;
    UhciExtension->BulkQH->HwQH.NextElement = PhysicalAddress;

    /* Set Frame List pointers */
    for (Idx = 0; Idx < UHCI_FRAME_LIST_MAX_ENTRIES; Idx++)
    {
        HeadIdx = (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms) +
                  (Idx & (ENDPOINT_INTERRUPT_32ms - 1));

        PhysicalAddress = UhciExtension->IntQH[HeadIdx]->PhysicalAddress;
        PhysicalAddress |= UHCI_FRAME_LIST_POINTER_QH;
        HcResourcesVA->FrameList[Idx] = PhysicalAddress;
    }

    /* Initialize static TD for first frame */
    StaticTD = &HcResourcesVA->StaticTD;
    StaticTdPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticTD);

    RtlZeroMemory(StaticTD, sizeof(UHCI_HCD_TD));

    StaticTD->PhysicalAddress = StaticTdPA;

    HeadIdx = (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms);
    PhysicalAddress = UhciExtension->IntQH[HeadIdx]->PhysicalAddress;
    StaticTD->HwTD.NextElement = PhysicalAddress | UHCI_TD_LINK_PTR_QH;

    StaticTD->HwTD.ControlStatus.InterruptOnComplete = 1;
    StaticTD->HwTD.Token.PIDCode = UHCI_TD_PID_IN;

    UhciExtension->StaticTD = StaticTD;

    /* Initialize StaticSofTDs for UhciInterruptNextSOF() */
    UhciExtension->SOF_HcdTDs = &HcResourcesVA->StaticSofTD[0];
    StaticSofTdPA = hcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, StaticSofTD[0]);

    for (Idx = 0; Idx < UHCI_MAX_STATIC_SOF_TDS; Idx++)
    {
        StaticSofTD = UhciExtension->SOF_HcdTDs + Idx;

        RtlZeroMemory(StaticSofTD, sizeof(UHCI_HCD_TD));

        PhysicalAddress = UhciExtension->IntQH[HeadIdx]->PhysicalAddress;
        StaticSofTD->HwTD.NextElement = PhysicalAddress | UHCI_TD_LINK_PTR_QH;

        StaticSofTD->HwTD.ControlStatus.InterruptOnComplete = 1;
        StaticSofTD->PhysicalAddress = StaticSofTdPA;

        StaticSofTdPA += sizeof(UHCI_HCD_TD);
    }

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciStartController(IN PVOID uhciExtension,
                    IN PUSBPORT_RESOURCES Resources)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    MPSTATUS MpStatus;
    PUSHORT PortControlRegister;
    UHCI_PORT_STATUS_CONTROL PortControl;
    UHCI_USB_COMMAND Command;
    USHORT Port;

    UhciExtension->Flags &= ~UHCI_EXTENSION_FLAG_SUSPENDED;
    UhciExtension->BaseRegister = Resources->ResourceBase;
    BaseRegister = UhciExtension->BaseRegister;
    DPRINT("UhciStartController: UhciExtension - %p, BaseRegister - %p\n", UhciExtension, BaseRegister);

    UhciExtension->HcFlavor = Resources->HcFlavor;

    MpStatus = UhciTakeControlHC(UhciExtension, Resources);

    if (MpStatus == MP_STATUS_SUCCESS)
    {
        MpStatus = UhciInitializeHardware(UhciExtension);

        if (MpStatus == MP_STATUS_SUCCESS)
        {
            UhciExtension->HcResourcesVA = (PUHCI_HC_RESOURCES)Resources->StartVA;
            UhciExtension->HcResourcesPA = Resources->StartPA;

            MpStatus = UhciInitializeSchedule(UhciExtension,
                                              UhciExtension->HcResourcesVA,
                                              UhciExtension->HcResourcesPA);

            UhciExtension->LockFrameList = 0;
        }
    }

    WRITE_PORT_ULONG(&BaseRegister->FrameAddress,
                     UhciExtension->HcResourcesPA + FIELD_OFFSET(UHCI_HC_RESOURCES, FrameList));

    if (MpStatus == MP_STATUS_SUCCESS)
    {
        Command.AsUSHORT = READ_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT);
        Command.Run = 1;
        WRITE_PORT_USHORT(&BaseRegister->HcCommand.AsUSHORT, Command.AsUSHORT);

        for (Port = 0; Port < UHCI_NUM_ROOT_HUB_PORTS; Port++)
        {
            PortControlRegister = &BaseRegister->PortControl[Port].AsUSHORT;
            PortControl.AsUSHORT = READ_PORT_USHORT(PortControlRegister);

            PortControl.ConnectStatusChange = 0;
            PortControl.Suspend = 0;

            WRITE_PORT_USHORT(PortControlRegister, PortControl.AsUSHORT);
        }

        UhciExtension->HcResourcesVA->FrameList[0] =
            UhciExtension->StaticTD->PhysicalAddress;
    }

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciStopController(IN PVOID uhciExtension,
                   IN BOOLEAN IsDoDisableInterrupts)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT CommandReg;
    UHCI_USB_COMMAND Command;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER CurrentTime;

    DPRINT("UhciStopController: UhciExtension - %p\n", UhciExtension);

    BaseRegister = UhciExtension->BaseRegister;
    CommandReg = &BaseRegister->HcCommand.AsUSHORT;

    Command.AsUSHORT = READ_PORT_USHORT(CommandReg);

    if (Command.AsUSHORT == 0xFFFF)
    {
        DPRINT("UhciStopController: Command == -1\n");
        return;
    }

    DPRINT("UhciStopController: Command.AsUSHORT - %p\n", Command.AsUSHORT);

    if (Command.GlobalReset)
    {
        Command.GlobalReset = 0;
        WRITE_PORT_USHORT(CommandReg, Command.AsUSHORT);
    }

    Command.HcReset = 1;

    WRITE_PORT_USHORT(CommandReg, Command.AsUSHORT);

    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += 100 * 1000;

    while (Command.AsUSHORT = READ_PORT_USHORT(CommandReg),
           Command.HcReset == 1)
    {
        KeQuerySystemTime(&CurrentTime);

        if (CurrentTime.QuadPart >= EndTime.QuadPart)
        {
            DPRINT1("UhciStopController: Failed to reset\n");
            DbgBreakPoint();
            break;
        }
    }
}

VOID
NTAPI
UhciSuspendController(IN PVOID uhciExtension)
{
    DPRINT_IMPL("UhciSuspendController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciResumeController(IN PVOID uhciExtension)
{
    DPRINT_IMPL("UhciResumeController: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
UhciHardwarePresent(IN PUHCI_EXTENSION UhciExtension)
{
    UHCI_USB_STATUS UhciStatus;
    PUSHORT StatusReg;

    StatusReg = &UhciExtension->BaseRegister->HcStatus.AsUSHORT;
    UhciStatus.AsUSHORT = READ_PORT_USHORT(StatusReg);

    if (UhciStatus.AsUSHORT == MAXUSHORT)
        DPRINT_UHCI("UhciHardwarePresent: HW not present\n");

    return UhciStatus.AsUSHORT != MAXUSHORT;
}

BOOLEAN
NTAPI
UhciInterruptService(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT CommandReg;
    UHCI_USB_COMMAND Command;
    PUSHORT StatusReg;
    UHCI_USB_STATUS HcStatus;
    PUSHORT InterruptEnableReg;
    PUHCI_HCD_QH QH;
    PUHCI_HCD_QH BulkTailQH;
    ULONG ScheduleError;
    BOOLEAN Result = FALSE;

    BaseRegister = UhciExtension->BaseRegister;
    StatusReg = &BaseRegister->HcStatus.AsUSHORT;
    InterruptEnableReg = &BaseRegister->HcInterruptEnable.AsUSHORT;
    CommandReg = &BaseRegister->HcCommand.AsUSHORT;

    if (!UhciHardwarePresent(UhciExtension))
    {
        DPRINT1("UhciInterruptService: return FALSE\n");
        return FALSE;
    }

    HcStatus.AsUSHORT = READ_PORT_USHORT(StatusReg) & UHCI_USB_STATUS_MASK;

    if (HcStatus.HostSystemError || HcStatus.HcProcessError)
    {
        DPRINT1("UhciInterruptService: Error [%p] HcStatus %X\n",
                UhciExtension,
                HcStatus.AsUSHORT);
    }
    else if (HcStatus.AsUSHORT)
    {
        UhciExtension->HcScheduleError = 0;
    }

    if (HcStatus.HcProcessError)
    {
        USHORT fn = READ_PORT_USHORT(&BaseRegister->FrameNumber);
        USHORT intr = READ_PORT_USHORT(InterruptEnableReg);
        USHORT cmd = READ_PORT_USHORT(CommandReg);

        DPRINT1("UhciInterruptService: HC ProcessError!\n");
        DPRINT1("UhciExtension %p, frame %X\n", UhciExtension, fn);
        DPRINT1("HcCommand %X\n", cmd);
        DPRINT1("HcStatus %X\n", HcStatus.AsUSHORT);
        DPRINT1("HcInterruptEnable %X\n", intr);

        DbgBreakPoint();
    }

    if (HcStatus.HcHalted)
    {
        DPRINT_UHCI("UhciInterruptService: Hc Halted [%p] HcStatus %X\n",
                    UhciExtension,
                    HcStatus.AsUSHORT);
    }
    else if (HcStatus.AsUSHORT)
    {
        UhciExtension->HcStatus.AsUSHORT = HcStatus.AsUSHORT;

        WRITE_PORT_USHORT(StatusReg, HcStatus.AsUSHORT);
        WRITE_PORT_USHORT(InterruptEnableReg, 0);

        if (HcStatus.HostSystemError)
        {
            DPRINT1("UhciInterruptService: HostSystemError! HcStatus - %X\n",
                    HcStatus.AsUSHORT);

            DbgBreakPoint();
        }

        Result = TRUE;
    }

    if (!HcStatus.Interrupt)
        goto NextProcess;

    UhciUpdateCounter(UhciExtension);

    BulkTailQH = UhciExtension->BulkTailQH;

    if (BulkTailQH->HwQH.NextQH & UHCI_QH_HEAD_LINK_PTR_TERMINATE)
        goto NextProcess;

    QH = UhciExtension->BulkQH;

    do
    {
        QH = QH->NextHcdQH;

        if (!QH)
        {
            BulkTailQH->HwQH.NextQH |= UHCI_QH_HEAD_LINK_PTR_TERMINATE;
            goto NextProcess;
        }
    }
    while (QH->HwQH.NextElement & UHCI_QH_ELEMENT_LINK_PTR_TERMINATE);

NextProcess:

    if (HcStatus.HcProcessError)
    {
        UhciCleanupFrameList(UhciExtension, TRUE);

        ScheduleError = UhciExtension->HcScheduleError;
        UhciExtension->HcScheduleError = ScheduleError + 1;

        DPRINT1("UhciInterruptService: [%p] ScheduleError %X\n",
                UhciExtension,
                ScheduleError);

        if (ScheduleError < UHCI_MAX_HC_SCHEDULE_ERRORS)
        {
            Command.AsUSHORT = READ_PORT_USHORT(CommandReg);
            Command.Run = 1;
            WRITE_PORT_USHORT(CommandReg, Command.AsUSHORT);
        }
    }
    else if (HcStatus.Interrupt && UhciExtension->ExtensionLock)
    {
        DPRINT1("UhciInterruptService: [%p] HcStatus %X\n",
                UhciExtension,
                HcStatus.AsUSHORT);

        UhciCleanupFrameList(UhciExtension, FALSE);
    }

    return Result;
}

VOID
NTAPI
UhciInterruptDpc(IN PVOID uhciExtension,
                 IN BOOLEAN IsDoEnableInterrupts)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    UHCI_USB_STATUS HcStatus;

    DPRINT_UHCI("UhciInterruptDpc: [%p] EnableInt %x, HcStatus %X\n",
                uhciExtension, IsDoEnableInterrupts, UhciExtension->HcStatus);

    BaseRegister = UhciExtension->BaseRegister;

    HcStatus = UhciExtension->HcStatus;
    UhciExtension->HcStatus.AsUSHORT = 0;

    if ((HcStatus.Interrupt | HcStatus.ErrorInterrupt) != 0)
        RegPacket.UsbPortInvalidateEndpoint(UhciExtension, 0);

    if (IsDoEnableInterrupts)
    {
        WRITE_PORT_USHORT(&BaseRegister->HcInterruptEnable.AsUSHORT,
                          UhciExtension->StatusMask.AsUSHORT);
    }
}

VOID
NTAPI
UhciQueueTransfer(IN PUHCI_EXTENSION UhciExtension,
                  IN PUHCI_ENDPOINT UhciEndpoint,
                  IN PUHCI_HCD_TD FirstTD,
                  IN PUHCI_HCD_TD LastTD)
{
    PUHCI_HCD_QH QH;
    PUHCI_HCD_QH BulkTailQH;
    PUHCI_HCD_TD TailTD;
    ULONG PhysicalAddress;

    DPRINT("UhciQueueTransfer: FirstTD - %p, LastTD - %p\n", FirstTD, LastTD);

    TailTD = UhciEndpoint->TailTD;
    QH = UhciEndpoint->QH;

    if (UhciEndpoint->HeadTD)
    {
        TailTD->NextHcdTD = FirstTD;

        TailTD->HwTD.NextElement = FirstTD->PhysicalAddress;
        TailTD->HwTD.NextElement |= UHCI_TD_LINK_PTR_TD;

        PhysicalAddress = QH->HwQH.NextElement;

        PhysicalAddress &= ~(UHCI_TD_LINK_PTR_TERMINATE |
                             UHCI_TD_LINK_PTR_QH |
                             UHCI_TD_LINK_PTR_DEPTH_FIRST);

        if (FirstTD->HwTD.ControlStatus.Status & UHCI_TD_STS_ACTIVE)
        {
            if (PhysicalAddress == TailTD->PhysicalAddress &&
                !(TailTD->HwTD.ControlStatus.Status & UHCI_TD_STS_ACTIVE))
            {
                QH->HwQH.NextElement = FirstTD->PhysicalAddress;

                QH->HwQH.NextElement &= ~(UHCI_QH_ELEMENT_LINK_PTR_TERMINATE |
                                          UHCI_QH_ELEMENT_LINK_PTR_QH);
            }
        }
    }
    else
    {
        if (FirstTD)
        {
            UhciEndpoint->HeadTD = FirstTD;
        }
        else
        {
            UhciEndpoint->TailTD = NULL;
            UhciEndpoint->HeadTD = NULL;
        }

        if (FirstTD == NULL || UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
        {
            PhysicalAddress = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        }
        else
        {
            PhysicalAddress = FirstTD->PhysicalAddress;
            PhysicalAddress &= ~UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        }

        QH->HwQH.NextElement = PhysicalAddress & ~UHCI_QH_ELEMENT_LINK_PTR_QH;
    }

    if (UhciEndpoint->EndpointProperties.TransferType == USBPORT_TRANSFER_TYPE_BULK)
    {
        BulkTailQH = UhciExtension->BulkTailQH;
        BulkTailQH->HwQH.NextQH &= ~UHCI_QH_HEAD_LINK_PTR_TERMINATE;
    }

    UhciEndpoint->TailTD = LastTD;
}

PUHCI_HCD_TD
NTAPI
UhciAllocateTD(IN PUHCI_EXTENSION UhciExtension,
               IN PUHCI_ENDPOINT UhciEndpoint)
{
    PUHCI_HCD_TD TD;
    ULONG AllocTdCounter;
    ULONG ix;

    DPRINT_UHCI("UhciAllocateTD: ...\n");

    AllocTdCounter = UhciEndpoint->AllocTdCounter;

    for (ix = 0; ix < UhciEndpoint->MaxTDs; ++ix)
    {
        TD = &UhciEndpoint->FirstTD[AllocTdCounter];

        if (!(TD->Flags & UHCI_HCD_TD_FLAG_ALLOCATED))
        {
            TD->Flags |= UHCI_HCD_TD_FLAG_ALLOCATED;

            UhciEndpoint->AllocatedTDs++;
            UhciEndpoint->AllocTdCounter = AllocTdCounter;

            return TD;
        }

        if (AllocTdCounter < UhciEndpoint->MaxTDs - 1)
            AllocTdCounter++;
        else
            AllocTdCounter = 0;
    }

    return NULL;
}

VOID
NTAPI
UhciMapAsyncTransferToTDs(IN PUHCI_EXTENSION UhciExtension,
                          IN PUHCI_ENDPOINT UhciEndpoint,
                          IN PUHCI_TRANSFER UhciTransfer,
                          OUT PUHCI_HCD_TD * OutFirstTD,
                          OUT PUHCI_HCD_TD * OutLastTD,
                          IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PUHCI_HCD_TD TD;
    PUHCI_HCD_TD LastTD = NULL;
    ULONG PhysicalAddress;
    USHORT TotalMaxPacketSize;
    USHORT DeviceSpeed;
    USHORT EndpointAddress;
    USHORT DeviceAddress;
    ULONG TransferType;
    SIZE_T TransferLength = 0;
    SIZE_T LengthMapped = 0;
    SIZE_T BytesRemaining;
    SIZE_T LengthThisTD;
    ULONG ix;
    BOOL DataToggle;
    UCHAR PIDCode;
    BOOLEAN IsLastTd = TRUE;
    BOOLEAN ZeroLengthTransfer = TRUE;

    DPRINT_UHCI("UhciMapAsyncTransferToTDs: ...\n");

    TotalMaxPacketSize = UhciEndpoint->EndpointProperties.TotalMaxPacketSize;
    DeviceSpeed = UhciEndpoint->EndpointProperties.DeviceSpeed;
    EndpointAddress = UhciEndpoint->EndpointProperties.EndpointAddress;
    DeviceAddress = UhciEndpoint->EndpointProperties.DeviceAddress;
    TransferType = UhciEndpoint->EndpointProperties.TransferType;

    if (SgList->SgElementCount || TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
        ZeroLengthTransfer = FALSE;

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        if (UhciTransfer->TransferParameters->TransferFlags &
            USBD_TRANSFER_DIRECTION_IN)
        {
            PIDCode = UHCI_TD_PID_IN;
        }
        else
        {
            PIDCode = UHCI_TD_PID_OUT;
        }

        DataToggle = UHCI_TD_PID_DATA1;
    }
    else
    {
        if (USB_ENDPOINT_DIRECTION_OUT(EndpointAddress))
            PIDCode = UHCI_TD_PID_OUT;
        else
            PIDCode = UHCI_TD_PID_IN;

        DataToggle = UhciEndpoint->DataToggle;
    }

    for (ix = 0; ix < SgList->SgElementCount || ZeroLengthTransfer; ix++)
    {
        BytesRemaining = SgList->SgElement[ix].SgTransferLength;
        PhysicalAddress = SgList->SgElement[ix].SgPhysicalAddress.LowPart;

        if (!IsLastTd)
        {
            PhysicalAddress += TransferLength;
            BytesRemaining -= TransferLength;
        }

        IsLastTd = TRUE;
        TransferLength = 0;

        while (BytesRemaining || ZeroLengthTransfer)
        {
            ZeroLengthTransfer = FALSE;

            if (BytesRemaining >= TotalMaxPacketSize)
            {
                LengthThisTD = TotalMaxPacketSize;
                BytesRemaining -= TotalMaxPacketSize;
            }
            else
            {
                if (ix >= SgList->SgElementCount - 1)
                {
                    LengthThisTD = BytesRemaining;
                }
                else
                {
                    IsLastTd = FALSE;

                    DPRINT1("UhciMapAsyncTransferToTds: IsLastTd = FALSE. FIXME\n");
                    ASSERT(FALSE);
                }

                BytesRemaining = 0;
            }

            UhciTransfer->PendingTds++;
            TD = UhciAllocateTD(UhciExtension, UhciEndpoint);
            TD->Flags |= UHCI_HCD_TD_FLAG_PROCESSED;

            TD->HwTD.NextElement = 0;
            TD->HwTD.Buffer = PhysicalAddress;

            TD->HwTD.ControlStatus.AsULONG = 0;
            TD->HwTD.ControlStatus.LowSpeedDevice = (DeviceSpeed == UsbLowSpeed);
            TD->HwTD.ControlStatus.Status = UHCI_TD_STS_ACTIVE;
            TD->HwTD.ControlStatus.ErrorCounter = 3;
            TD->HwTD.ControlStatus.ActualLength = UHCI_TD_LENGTH_NULL;
            TD->HwTD.ControlStatus.ShortPacketDetect = 1;

            TD->HwTD.Token.AsULONG = 0;
            TD->HwTD.Token.Endpoint = EndpointAddress;
            TD->HwTD.Token.DeviceAddress = DeviceAddress;
            TD->HwTD.Token.PIDCode = PIDCode;

            if (LengthThisTD == 0)
                TD->HwTD.Token.MaximumLength = UHCI_TD_LENGTH_NULL;
            else
                TD->HwTD.Token.MaximumLength = LengthThisTD - 1;

            TD->HwTD.Token.DataToggle = (DataToggle == UHCI_TD_PID_DATA1);

            TD->NextHcdTD = 0;
            TD->UhciTransfer = UhciTransfer;

            if (!IsLastTd)
                ASSERT(FALSE);

            PhysicalAddress += LengthThisTD;
            LengthMapped += LengthThisTD;

            if (LastTD)
            {
                LastTD->HwTD.NextElement = TD->PhysicalAddress &
                                           UHCI_TD_LINK_POINTER_MASK;
                LastTD->NextHcdTD = TD;
            }
            else
            {
                *OutFirstTD = TD;
            }

            LastTD = TD;
            DataToggle = DataToggle == UHCI_TD_PID_DATA0;
        }
    }

    UhciEndpoint->DataToggle = DataToggle;

    *OutLastTD = LastTD;
}

MPSTATUS
NTAPI
UhciControlTransfer(IN PUHCI_EXTENSION UhciExtension,
                    IN PUHCI_ENDPOINT UhciEndpoint,
                    IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                    IN PUHCI_TRANSFER UhciTransfer,
                    IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PUHCI_HCD_TD FirstTD;
    PUHCI_HCD_TD LastTD;
    PUHCI_HCD_TD DataFirstTD;
    PUHCI_HCD_TD DataLastTD;
    UHCI_CONTROL_STATUS ControlStatus;
    USB_DEVICE_SPEED DeviceSpeed;
    USHORT EndpointAddress;
    USHORT DeviceAddress;
    ULONG PhysicalAddress;

    DPRINT_UHCI("UhciControlTransfer: UhciTransfer - %p\n", UhciTransfer);

    if (UhciEndpoint->EndpointLock > 1)
    {
        InterlockedDecrement(&UhciEndpoint->EndpointLock);

        if (UhciEndpoint->EndpointProperties.TransferType ==
            USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            InterlockedDecrement(&UhciExtension->ExtensionLock);
        }

        DPRINT("UhciControlTransfer: end MP_STATUS_FAILURE\n");
        return MP_STATUS_FAILURE;
    }

    DeviceSpeed = UhciEndpoint->EndpointProperties.DeviceSpeed;
    EndpointAddress = UhciEndpoint->EndpointProperties.EndpointAddress;
    DeviceAddress = UhciEndpoint->EndpointProperties.DeviceAddress;

    /* Allocate and setup first TD */
    UhciTransfer->PendingTds++;
    FirstTD = UhciAllocateTD(UhciExtension, UhciEndpoint);
    FirstTD->Flags |= UHCI_HCD_TD_FLAG_PROCESSED;
    DPRINT_UHCI("UhciControlTransfer: FirstTD - %p\n", FirstTD);

    FirstTD->HwTD.NextElement = 0;

    ControlStatus.AsULONG = 0;
    ControlStatus.LowSpeedDevice = (DeviceSpeed == UsbLowSpeed);
    ControlStatus.Status |= UHCI_TD_STS_ACTIVE;
    ControlStatus.ErrorCounter = 3;
    FirstTD->HwTD.ControlStatus = ControlStatus;

    FirstTD->HwTD.Token.AsULONG = 0;
    FirstTD->HwTD.Token.Endpoint = EndpointAddress;
    FirstTD->HwTD.Token.DeviceAddress = DeviceAddress;

    FirstTD->HwTD.Token.MaximumLength = sizeof(USB_DEFAULT_PIPE_SETUP_PACKET);
    FirstTD->HwTD.Token.MaximumLength--;
    FirstTD->HwTD.Token.PIDCode = UHCI_TD_PID_SETUP;
    FirstTD->HwTD.Token.DataToggle = UHCI_TD_PID_DATA0;

    RtlCopyMemory(&FirstTD->SetupPacket,
                  &TransferParameters->SetupPacket,
                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    FirstTD->HwTD.Buffer = FirstTD->PhysicalAddress + FIELD_OFFSET(UHCI_HCD_TD, SetupPacket);

    FirstTD->NextHcdTD = NULL;
    FirstTD->UhciTransfer = UhciTransfer;

    /* Allocate and setup last TD */
    UhciTransfer->PendingTds++;
    LastTD = UhciAllocateTD(UhciExtension, UhciEndpoint);
    LastTD->Flags |= UHCI_HCD_TD_FLAG_PROCESSED;
    DPRINT_UHCI("UhciControlTransfer: LastTD - %p\n", LastTD);

    LastTD->HwTD.NextElement = 0;

    LastTD->HwTD.ControlStatus.AsULONG = 0;
    LastTD->HwTD.ControlStatus.LowSpeedDevice = (DeviceSpeed == UsbLowSpeed);
    LastTD->HwTD.ControlStatus.Status |= UHCI_TD_STS_ACTIVE;
    LastTD->HwTD.ControlStatus.ErrorCounter = 3;

    LastTD->HwTD.Token.AsULONG = 0;
    LastTD->HwTD.Token.Endpoint = EndpointAddress;
    LastTD->HwTD.Token.DeviceAddress = DeviceAddress;

    LastTD->UhciTransfer = UhciTransfer;
    LastTD->NextHcdTD = NULL;

    /* Allocate and setup TDs for data */
    DataFirstTD = NULL;
    DataLastTD = NULL;

    UhciMapAsyncTransferToTDs(UhciExtension,
                              UhciEndpoint,
                              UhciTransfer,
                              &DataFirstTD,
                              &DataLastTD,
                              SgList);

    if (DataFirstTD)
    {
        PhysicalAddress = DataFirstTD->PhysicalAddress;
        PhysicalAddress &= UHCI_TD_LINK_POINTER_MASK;
        FirstTD->HwTD.NextElement = PhysicalAddress;
        FirstTD->NextHcdTD = DataFirstTD;

        PhysicalAddress = LastTD->PhysicalAddress;
        PhysicalAddress &= UHCI_TD_LINK_POINTER_MASK;
        DataLastTD->HwTD.NextElement = PhysicalAddress;
        DataLastTD->NextHcdTD = LastTD;
    }
    else
    {
        PhysicalAddress = LastTD->PhysicalAddress;
        PhysicalAddress &= UHCI_TD_LINK_POINTER_MASK;
        FirstTD->HwTD.NextElement = PhysicalAddress;
        FirstTD->NextHcdTD = LastTD;
    }

    LastTD->HwTD.Buffer = 0;
    LastTD->HwTD.ControlStatus.InterruptOnComplete = 1;

    LastTD->HwTD.Token.DataToggle = UHCI_TD_PID_DATA1;
    LastTD->HwTD.Token.MaximumLength = UHCI_TD_LENGTH_NULL;

    if (UhciTransfer->TransferParameters->TransferFlags &
        USBD_TRANSFER_DIRECTION_IN)
    {
        LastTD->HwTD.Token.PIDCode = UHCI_TD_PID_OUT;
    }
    else
    {
        LastTD->HwTD.Token.PIDCode = UHCI_TD_PID_IN;
    }

    LastTD->HwTD.NextElement = UHCI_TD_LINK_PTR_TERMINATE;

    LastTD->Flags |= UHCI_HCD_TD_FLAG_CONTROLL;
    LastTD->NextHcdTD = NULL;

    /* Link this transfer to queue */
    UhciQueueTransfer(UhciExtension, UhciEndpoint, FirstTD, LastTD);

    DPRINT_UHCI("UhciControlTransfer: end MP_STATUS_SUCCESS\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciBulkOrInterruptTransfer(IN PUHCI_EXTENSION UhciExtension,
                            IN PUHCI_ENDPOINT UhciEndpoint,
                            IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                            IN PUHCI_TRANSFER UhciTransfer,
                            IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PUHCI_HCD_TD DataFirstTD;
    PUHCI_HCD_TD DataLastTD;
    ULONG TotalMaxPacketSize;
    ULONG SgCount;
    ULONG TransferLength;
    ULONG TDs;
    ULONG ix;

    DPRINT_UHCI("UhciBulkOrInterruptTransfer: ...\n");

    TotalMaxPacketSize = UhciEndpoint->EndpointProperties.TotalMaxPacketSize;

    SgCount = SgList->SgElementCount;

    if (SgCount == 0)
    {
        DPRINT("UhciBulkOrInterruptTransfer: SgCount == 0 \n");
        TDs = 1;
    }
    else
    {
        TransferLength = 0;

        for (ix = 0; ix < SgCount; ++ix)
        {
            TransferLength += SgList->SgElement[ix].SgTransferLength;
        }

        DPRINT("UhciBulkOrInterruptTransfer: SgCount - %X, TransferLength - %X\n",
               SgList->SgElementCount,
               TransferLength);

        if (TransferLength)
        {
            TDs = TransferLength + (TotalMaxPacketSize - 1);
            TDs /= TotalMaxPacketSize;
        }
        else
        {
            TDs = 1;
        }
    }

    if ((UhciEndpoint->MaxTDs - UhciEndpoint->AllocatedTDs) < TDs)
    {
        DPRINT1("UhciBulkOrInterruptTransfer: Not enough TDs \n");
        return MP_STATUS_FAILURE;
    }

    DataFirstTD = NULL;
    DataLastTD = NULL;

    UhciMapAsyncTransferToTDs(UhciExtension,
                              UhciEndpoint,
                              UhciTransfer,
                              &DataFirstTD,
                              &DataLastTD,
                              SgList);

    if (DataLastTD == NULL || DataFirstTD == NULL)
    {
        DPRINT1("UhciBulkOrInterruptTransfer: !DataLastTD || !DataFirstTD\n");
        return MP_STATUS_FAILURE;
    }

    DataLastTD->HwTD.NextElement = UHCI_TD_LINK_PTR_TERMINATE;
    DataLastTD->HwTD.ControlStatus.InterruptOnComplete = 1;
    DataLastTD->NextHcdTD = NULL;

    UhciQueueTransfer(UhciExtension,
                      UhciEndpoint,
                      DataFirstTD,
                      DataLastTD);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciSubmitTransfer(IN PVOID uhciExtension,
                   IN PVOID uhciEndpoint,
                   IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                   IN PVOID uhciTransfer,
                   IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    PUHCI_TRANSFER UhciTransfer = uhciTransfer;
    ULONG TransferType;

    DPRINT_UHCI("UhciSubmitTransfer: ...\n");

    InterlockedIncrement(&UhciEndpoint->EndpointLock);

    TransferType = UhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS &&
        InterlockedIncrement(&UhciExtension->ExtensionLock) == 1)
    {
        UhciExtension->FrameNumber = UhciGet32BitFrameNumber(UhciExtension);
    }

    RtlZeroMemory(UhciTransfer, sizeof(UHCI_TRANSFER));

    UhciTransfer->TransferParameters = TransferParameters;
    UhciTransfer->UhciEndpoint = UhciEndpoint;
    UhciTransfer->USBDStatus = USBD_STATUS_SUCCESS;

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        return UhciControlTransfer(UhciExtension,
                                   UhciEndpoint,
                                   TransferParameters,
                                   UhciTransfer,
                                   SgList);
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        return UhciBulkOrInterruptTransfer(UhciExtension,
                                           UhciEndpoint,
                                           TransferParameters,
                                           UhciTransfer,
                                           SgList);
    }

    DPRINT1("UhciSubmitTransfer: Error TransferType - %x\n", TransferType);

    return MP_STATUS_SUCCESS;
}

USBD_STATUS
NTAPI
UhciGetErrorFromTD(IN PUHCI_EXTENSION UhciExtension,
                   IN PUHCI_HCD_TD TD)
{
    USBD_STATUS USBDStatus;
    UCHAR TdStatus;

    //DPRINT("UhciGetErrorFromTD: ...\n");

    TdStatus = TD->HwTD.ControlStatus.Status;

    if (TdStatus == UHCI_TD_STS_ACTIVE)
    {
        if (TD->HwTD.Token.MaximumLength == UHCI_TD_LENGTH_NULL)
            return USBD_STATUS_SUCCESS;

        if (TD->HwTD.ControlStatus.ActualLength + 1 >=
            TD->HwTD.Token.MaximumLength + 1)
        {
            return USBD_STATUS_SUCCESS;
        }

        if (TD->HwTD.ControlStatus.InterruptOnComplete == 1)
            return USBD_STATUS_SUCCESS;

        return USBD_STATUS_ERROR_SHORT_TRANSFER;
    }

    if (TdStatus & UHCI_TD_STS_BABBLE_DETECTED &&
        TdStatus & UHCI_TD_STS_STALLED)
    {
        DPRINT1("UhciGetErrorFromTD: USBD_STATUS_BUFFER_OVERRUN, TD - %p\n", TD);
        return USBD_STATUS_BUFFER_OVERRUN;
    }

    if (TdStatus & UHCI_TD_STS_TIMEOUT_CRC_ERROR &&
        TdStatus & UHCI_TD_STS_STALLED)
    {
        DPRINT1("UhciGetErrorFromTD: USBD_STATUS_DEV_NOT_RESPONDING, TD - %p\n", TD);
        return USBD_STATUS_DEV_NOT_RESPONDING;
    }

    if (TdStatus & UHCI_TD_STS_TIMEOUT_CRC_ERROR)
    {
        if (TD->HwTD.ControlStatus.ActualLength == UHCI_TD_LENGTH_NULL)
        {
            DPRINT1("UhciGetErrorFromTD: USBD_STATUS_DEV_NOT_RESPONDING, TD - %p\n", TD);
            return USBD_STATUS_DEV_NOT_RESPONDING;
        }
        else
        {
            DPRINT1("UhciGetErrorFromTD: USBD_STATUS_CRC, TD - %p\n", TD);
            return USBD_STATUS_CRC;
        }
    }
    else if (TdStatus & UHCI_TD_STS_DATA_BUFFER_ERROR)
    {
        DPRINT1("UhciGetErrorFromTD: USBD_STATUS_DATA_OVERRUN, TD - %p\n", TD);
        USBDStatus = USBD_STATUS_DATA_OVERRUN;
    }
    else if (TdStatus & UHCI_TD_STS_STALLED)
    {
        DPRINT1("UhciGetErrorFromTD: USBD_STATUS_STALL_PID, TD - %p\n", TD);
        USBDStatus = USBD_STATUS_STALL_PID;
    }
    else
    {
        DPRINT1("UhciGetErrorFromTD: USBD_STATUS_INTERNAL_HC_ERROR, TD - %p\n", TD);
        USBDStatus = USBD_STATUS_INTERNAL_HC_ERROR;
    }

    return USBDStatus;
}

VOID
NTAPI
UhciProcessDoneNonIsoTD(IN PUHCI_EXTENSION UhciExtension,
                        IN PUHCI_HCD_TD TD)
{
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    PUHCI_ENDPOINT UhciEndpoint;
    PUHCI_TRANSFER UhciTransfer;
    USBD_STATUS USBDStatus = USBD_STATUS_SUCCESS;
    SIZE_T TransferedLen;

    DPRINT_UHCI("UhciProcessDoneNonIsoTD: TD - %p\n", TD);

    UhciTransfer = TD->UhciTransfer;
    UhciTransfer->PendingTds--;

    TransferParameters = UhciTransfer->TransferParameters;
    UhciEndpoint = UhciTransfer->UhciEndpoint;

    if (!(TD->Flags & UHCI_HCD_TD_FLAG_NOT_ACCESSED))
    {
        if (UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
            USBDStatus = UhciGetErrorFromTD(UhciExtension, TD);

        if (USBDStatus != USBD_STATUS_SUCCESS ||
            (TD->HwTD.ControlStatus.ActualLength == UHCI_TD_LENGTH_NULL))
        {
            TransferedLen = 0;
        }
        else
        {
            TransferedLen = TD->HwTD.ControlStatus.ActualLength + 1;
        }

        if (TD->HwTD.Token.PIDCode != UHCI_TD_PID_SETUP)
            UhciTransfer->TransferLen += TransferedLen;

        if (TD->HwTD.Token.PIDCode == UHCI_TD_PID_IN &&
            TD->Flags & UHCI_HCD_TD_FLAG_DATA_BUFFER)
        {
            DPRINT_IMPL("UhciProcessDoneNonIsoTD: UNIMPLEMENTED. FIXME\n");
        }

        if (USBDStatus != USBD_STATUS_SUCCESS)
            UhciTransfer->USBDStatus = USBDStatus;
    }

    if (TD->Flags & UHCI_HCD_TD_FLAG_DATA_BUFFER)
        DPRINT_IMPL("UhciProcessDoneNonIsoTD: UNIMPLEMENTED. FIXME\n");

    UhciEndpoint->AllocatedTDs--;

    TD->HwTD.NextElement = 0;
    TD->UhciTransfer = NULL;
    TD->Flags = 0;

    if (UhciTransfer->PendingTds == 0)
    {
        InterlockedDecrement(&UhciEndpoint->EndpointLock);

        if (UhciEndpoint->EndpointProperties.TransferType ==
            USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            InterlockedDecrement(&UhciExtension->ExtensionLock);
        }

        RegPacket.UsbPortCompleteTransfer(UhciExtension,
                                          UhciEndpoint,
                                          TransferParameters,
                                          UhciTransfer->USBDStatus,
                                          UhciTransfer->TransferLen);
    }
}

MPSTATUS
NTAPI
UhciIsochTransfer(IN PVOID ehciExtension,
                  IN PVOID ehciEndpoint,
                  IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                  IN PVOID ehciTransfer,
                  IN PVOID isoParameters)
{
    DPRINT_IMPL("UhciIsochTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciAbortIsoTransfer(IN PUHCI_EXTENSION UhciExtension,
                     IN PUHCI_ENDPOINT UhciEndpoint,
                     IN PUHCI_TRANSFER UhciTransfer)
{
    DPRINT_IMPL("UhciAbortIsoTransfer: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciAbortNonIsoTransfer(IN PUHCI_EXTENSION UhciExtension,
                        IN PUHCI_ENDPOINT UhciEndpoint,
                        IN PUHCI_TRANSFER UhciTransfer,
                        IN PULONG CompletedLength)
{
    PUHCI_HCD_TD TD;
    PUHCI_HCD_TD PrevTD = NULL;
    ULONG PhysicalAddress;
    BOOL DataToggle;
    BOOLEAN IsHeadTD = FALSE;

    DPRINT("UhciAbortNonIsoTransfer: UhciExtension - %p, QH - %p, UhciTransfer - %p\n",
           UhciExtension,
           UhciEndpoint->QH,
           UhciTransfer);

    for (TD = UhciEndpoint->HeadTD;
         TD && TD->UhciTransfer != UhciTransfer;
         TD = TD->NextHcdTD)
    {
        PrevTD = TD;
    }

    DataToggle = TD->HwTD.Token.DataToggle;

    if (TD == UhciEndpoint->HeadTD)
        IsHeadTD = TRUE;

    while (TD && TD->UhciTransfer == UhciTransfer)
    {
        DPRINT_UHCI("UhciAbortNonIsoTransfer: TD - %p\n", TD);

        if (TD->HwTD.ControlStatus.Status & UHCI_TD_STS_ACTIVE)
        {
            if (TD->Flags & UHCI_HCD_TD_FLAG_DATA_BUFFER)
                DPRINT_IMPL("UhciAbortNonIsoTransfer: UNIMPLEMENTED. FIXME\n");

            UhciEndpoint->AllocatedTDs--;

            DPRINT_UHCI("UhciAbortNonIsoTransfer: Active TD - %p\n", TD);

            TD->HwTD.NextElement = 0;
            TD->Flags = 0;
            TD->UhciTransfer = NULL;
        }
        else
        {
            UhciProcessDoneNonIsoTD(UhciExtension, TD);
        }

        TD = TD->NextHcdTD;
    }

    UhciFixDataToggle(UhciExtension,
                      UhciEndpoint,
                      TD,
                      DataToggle);

    if (IsHeadTD)
    {
        if (TD)
        {
            UhciEndpoint->HeadTD = TD;
        }
        else
        {
            UhciEndpoint->HeadTD = NULL;
            UhciEndpoint->TailTD = NULL;
        }

        if (TD == NULL || UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
        {
            PhysicalAddress = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        }
        else
        {
            PhysicalAddress = TD->PhysicalAddress;
            PhysicalAddress &= ~UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        }

        DPRINT_UHCI("UhciAbortNonIsoTransfer: TD - %p\n", TD);

        UhciEndpoint->QH->HwQH.NextElement = PhysicalAddress;
        UhciEndpoint->QH->HwQH.NextElement &= ~UHCI_QH_ELEMENT_LINK_PTR_QH;
    }
    else if (TD)
    {
        PrevTD->HwTD.NextElement = TD->PhysicalAddress & UHCI_TD_LINK_POINTER_MASK;
        PrevTD->NextHcdTD = TD;
    }
    else
    {
        PrevTD->NextHcdTD = NULL;
        PrevTD->HwTD.NextElement = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;

        UhciEndpoint->TailTD = PrevTD;
    }

    *CompletedLength = UhciTransfer->TransferLen;
}

VOID
NTAPI
UhciAbortTransfer(IN PVOID uhciExtension,
                  IN PVOID uhciEndpoint,
                  IN PVOID uhciTransfer,
                  IN PULONG CompletedLength)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    PUHCI_TRANSFER UhciTransfer = uhciTransfer;
    ULONG TransferType;

    DPRINT("UhciAbortTransfer: ...\n");

    InterlockedDecrement(&UhciEndpoint->EndpointLock);

    TransferType = UhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        InterlockedDecrement(&UhciExtension->ExtensionLock);

        UhciAbortIsoTransfer(UhciExtension,
                             UhciEndpoint,
                             UhciTransfer);
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        UhciAbortNonIsoTransfer(UhciExtension,
                                UhciEndpoint,
                                UhciTransfer,
                                CompletedLength);
    }
}

ULONG
NTAPI
UhciGetEndpointState(IN PVOID uhciExtension,
                     IN PVOID uhciEndpoint)
{
    DPRINT_IMPL("UhciGetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
UhciInsertQH(IN PUHCI_EXTENSION UhciExtension,
             IN PUHCI_HCD_QH StaticQH,
             IN PUHCI_HCD_QH QH)
{
    PUHCI_HCD_QH NextHcdQH;

    DPRINT("UhciInsertQH: UhciExtension - %p, StaticQH - %p, QH - %p\n", UhciExtension, StaticQH, QH);

    QH->HwQH.NextQH = StaticQH->HwQH.NextQH;
    NextHcdQH = StaticQH->NextHcdQH;

    QH->PrevHcdQH = StaticQH;
    QH->NextHcdQH = NextHcdQH;

    if (NextHcdQH)
        NextHcdQH->PrevHcdQH = QH;
    else
        UhciExtension->BulkTailQH = QH;

    StaticQH->HwQH.NextQH = QH->PhysicalAddress | UHCI_QH_HEAD_LINK_PTR_QH;
    StaticQH->NextHcdQH = QH;

    QH->QhFlags |= UHCI_HCD_QH_FLAG_ACTIVE;
}

VOID
NTAPI
UhciUnlinkQH(IN PUHCI_EXTENSION UhciExtension,
             IN PUHCI_HCD_QH QH)
{
    PUHCI_HCD_QH NextHcdQH;
    PUHCI_HCD_QH PrevHcdQH;
    PUHCI_HCD_QH BulkQH;

    DPRINT("UhciUnlinkQH: ... \n");

    NextHcdQH = QH->NextHcdQH;
    PrevHcdQH = QH->PrevHcdQH;

    if (UhciExtension->BulkTailQH == QH)
        UhciExtension->BulkTailQH = PrevHcdQH;

    PrevHcdQH->HwQH.NextQH = QH->HwQH.NextQH;
    PrevHcdQH->NextHcdQH = NextHcdQH;

    if (NextHcdQH)
        NextHcdQH->PrevHcdQH = PrevHcdQH;

    QH->PrevHcdQH = QH;
    QH->NextHcdQH = QH;

    if (!(QH->UhciEndpoint->EndpointProperties.TransferType ==
          USBPORT_TRANSFER_TYPE_BULK))
    {
        QH->QhFlags &= ~UHCI_HCD_QH_FLAG_ACTIVE;
        return;
    }

    if ((UhciExtension->BulkTailQH->HwQH.NextQH & UHCI_QH_HEAD_LINK_PTR_TERMINATE)
                                               == UHCI_QH_HEAD_LINK_PTR_TERMINATE)
    {
        QH->QhFlags &= ~UHCI_HCD_QH_FLAG_ACTIVE;
        return;
    }

    BulkQH = UhciExtension->BulkQH;

    while (TRUE)
    {
        BulkQH = BulkQH->NextHcdQH;

        if (!BulkQH)
            break;

        if (!(BulkQH->HwQH.NextElement & UHCI_QH_ELEMENT_LINK_PTR_TERMINATE))
        {
            QH->QhFlags &= ~UHCI_HCD_QH_FLAG_ACTIVE;
            return;
        }
    }

    UhciExtension->BulkTailQH->HwQH.NextQH |= UHCI_QH_HEAD_LINK_PTR_TERMINATE;

    QH->QhFlags &= ~UHCI_HCD_QH_FLAG_ACTIVE;
}

VOID
NTAPI
UhciSetEndpointState(IN PVOID uhciExtension,
                     IN PVOID uhciEndpoint,
                     IN ULONG EndpointState)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG TransferType;
    PUHCI_HCD_QH QH;
    ULONG Idx;

    TransferType = UhciEndpoint->EndpointProperties.TransferType;
    QH = UhciEndpoint->QH;

    DPRINT("UhciSetEndpointState: EndpointState - %x, TransferType - %x\n",
           EndpointState,
           TransferType);

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        return;

    if (TransferType != USBPORT_TRANSFER_TYPE_CONTROL &&
        TransferType != USBPORT_TRANSFER_TYPE_BULK &&
        TransferType != USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        DPRINT("UhciSetEndpointState: Unknown TransferType - %x\n",
               TransferType);
    }

    switch (EndpointState)
    {
        case USBPORT_ENDPOINT_PAUSED:
            UhciUnlinkQH(UhciExtension, QH);
            return;

        case USBPORT_ENDPOINT_ACTIVE:
            switch (TransferType)
            {
                case USBPORT_TRANSFER_TYPE_CONTROL:
                    UhciInsertQH(UhciExtension,
                                 UhciExtension->ControlQH,
                                 UhciEndpoint->QH);
                    break;

                case USBPORT_TRANSFER_TYPE_BULK:
                    UhciInsertQH(UhciExtension,
                                 UhciExtension->BulkQH,
                                 UhciEndpoint->QH);
                    break;

                case USBPORT_TRANSFER_TYPE_INTERRUPT:
                    Idx = UhciEndpoint->EndpointProperties.Period +
                          UhciEndpoint->EndpointProperties.ScheduleOffset;

                    UhciInsertQH(UhciExtension,
                                 UhciExtension->IntQH[Idx - 1],
                                 UhciEndpoint->QH);
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }

            break;

        case USBPORT_ENDPOINT_REMOVE:
            QH->QhFlags |= UHCI_HCD_QH_FLAG_REMOVE;
            UhciUnlinkQH(UhciExtension, QH);
            break;

        default:
            ASSERT(FALSE);
            break;
    }
}

ULONG
NTAPI
UhciGetEndpointStatus(IN PVOID uhciExtension,
                      IN PVOID uhciEndpoint)
{
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG EndpointStatus;

    DPRINT_UHCI("UhciGetEndpointStatus: ...\n");

    if (UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
        EndpointStatus = USBPORT_ENDPOINT_HALT;
    else
        EndpointStatus = USBPORT_ENDPOINT_RUN;

    return EndpointStatus;
}

VOID
NTAPI
UhciSetEndpointStatus(IN PVOID uhciExtension,
                      IN PVOID uhciEndpoint,
                      IN ULONG EndpointStatus)
{
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG PhysicalAddress;

    DPRINT("UhciSetEndpointStatus: uhciEndpoint - %p, EndpointStatus - %X\n",
           uhciEndpoint,
           EndpointStatus);

    if (EndpointStatus != USBPORT_ENDPOINT_RUN)
        return;

    if (!(UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED))
        return;

    UhciEndpoint->Flags &= ~UHCI_ENDPOINT_FLAG_HALTED;

    if (UhciEndpoint->HeadTD == NULL)
        UhciEndpoint->TailTD = NULL;

    if (UhciEndpoint->HeadTD)
    {
        PhysicalAddress = UhciEndpoint->HeadTD->PhysicalAddress;
        PhysicalAddress &= ~UHCI_TD_LINK_PTR_TERMINATE;
        UhciEndpoint->QH->HwQH.NextElement = PhysicalAddress;
        UhciEndpoint->QH->HwQH.NextElement &= ~UHCI_QH_ELEMENT_LINK_PTR_QH;
    }
    else
    {
        UhciEndpoint->QH->HwQH.NextElement = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    }
}

VOID
NTAPI
UhciPollIsoEndpoint(IN PUHCI_EXTENSION UhciExtension,
                    IN PUHCI_ENDPOINT UhciEndpoint)
{
    DPRINT_IMPL("UhciPollIsoEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciPollNonIsoEndpoint(IN PUHCI_EXTENSION UhciExtension,
                       IN PUHCI_ENDPOINT UhciEndpoint)
{
    PUHCI_HCD_QH QH;
    PUHCI_HCD_TD NextTD;
    PUHCI_HCD_TD TD;
    ULONG NextTdPA;
    ULONG PhysicalAddress;
    SIZE_T TransferedLen;
    PLIST_ENTRY ListTDs;
    UCHAR TdStatus;

    DPRINT_UHCI("UhciPollNonIsoEndpoint: UhciExtension - %p, UhciEndpoint - %p\n",
                UhciExtension,
                UhciEndpoint);

    if (UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
    {
        DPRINT("UhciPollNonIsoEndpoint: Ep->Flags & UHCI_ENDPOINT_FLAG_HALTED \n");
        return;
    }

    QH = UhciEndpoint->QH;

    NextTdPA = QH->HwQH.NextElement & UHCI_QH_ELEMENT_LINK_POINTER_MASK;

    if (NextTdPA)
    {
        NextTD = RegPacket.UsbPortGetMappedVirtualAddress(NextTdPA,
                                                          UhciExtension,
                                                          UhciEndpoint);
    }
    else
    {
        NextTD = NULL;
    }

    DPRINT_UHCI("UhciPollNonIsoEndpoint: NextTD - %p, NextTdPA - %p\n",
                NextTD,
                NextTdPA);

    for (TD = UhciEndpoint->HeadTD; TD != NextTD && TD != NULL; TD = TD->NextHcdTD)
    {
        DPRINT_UHCI("UhciPollNonIsoEndpoint: TD - %p, TD->NextHcdTD - %p\n",
                    TD,
                    TD->NextHcdTD);

        TD->Flags |= UHCI_HCD_TD_FLAG_DONE;
        InsertTailList(&UhciEndpoint->ListTDs, &TD->TdLink);

        if (TD->NextHcdTD &&
            TD->NextHcdTD->HwTD.ControlStatus.Status & UHCI_TD_STS_ACTIVE)
        {
            if (NextTdPA == 0)
            {
                TD = TD->NextHcdTD;
                goto EnqueueTD;
            }

            if (NextTdPA != TD->NextHcdTD->PhysicalAddress)
            {
                DPRINT("UhciPollNonIsoEndpoint: TD->NextHcdTD->PhysicalAddress - %p\n",
                       TD->NextHcdTD->PhysicalAddress);
                ASSERT(FALSE);
            }
        }
        else
        {
            if (TD->NextHcdTD == NULL)
            {
                DPRINT_UHCI("UhciPollNonIsoEndpoint: TD->NextHcdTD == NULL\n");
            }
            else
            {
                DPRINT_UHCI("UhciPollNonIsoEndpoint: ControlStatus - %X\n",
                            TD->NextHcdTD->HwTD.ControlStatus.AsULONG);
            }
        }
    }

    UhciEndpoint->HeadTD = NextTD;

    if (NextTD == NULL)
    {
        DPRINT_UHCI("UhciPollNonIsoEndpoint: NextTD == NULL\n");

        UhciEndpoint->HeadTD = NULL;
        UhciEndpoint->TailTD = NULL;

        QH->HwQH.NextElement = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;

        goto ProcessListTDs;
    }

    DPRINT_UHCI("UhciPollNonIsoEndpoint: NextTD - %p, NextTdPA - %p\n",
                NextTD,
                NextTdPA);

    TdStatus = NextTD->HwTD.ControlStatus.Status;

    if (TdStatus & UHCI_TD_STS_ACTIVE)
    {
        DPRINT_UHCI("UhciPollNonIsoEndpoint: UHCI_TD_STS_ACTIVE \n");
        goto ProcessListTDs;
    }

    if (NextTD->HwTD.Token.PIDCode == UHCI_TD_PID_IN &&
        TdStatus & UHCI_TD_STS_STALLED &&
        TdStatus & UHCI_TD_STS_TIMEOUT_CRC_ERROR &&
        !(TdStatus & UHCI_TD_STS_NAK_RECEIVED) &&
        !(TdStatus & UHCI_TD_STS_BABBLE_DETECTED) &&
        !(TdStatus & UHCI_TD_STS_BITSTUFF_ERROR))
    {
        DPRINT("UhciPollNonIsoEndpoint: USBD_STATUS_DEV_NOT_RESPONDING\n");

        UhciDumpHcdTD(NextTD);

        if (!(NextTD->Flags & UHCI_HCD_TD_FLAG_STALLED_SETUP))
        {
            NextTD->HwTD.ControlStatus.ErrorCounter = 3;

            NextTD->HwTD.ControlStatus.Status &= ~(UHCI_TD_STS_STALLED |
                                                   UHCI_TD_STS_TIMEOUT_CRC_ERROR);

            NextTD->HwTD.ControlStatus.Status |= UHCI_TD_STS_ACTIVE;

            NextTD->Flags = NextTD->Flags | UHCI_HCD_TD_FLAG_STALLED_SETUP;

            goto ProcessListTDs;
        }
    }

    if (TdStatus & (UHCI_TD_STS_STALLED |
                    UHCI_TD_STS_DATA_BUFFER_ERROR |
                    UHCI_TD_STS_BABBLE_DETECTED |
                    UHCI_TD_STS_TIMEOUT_CRC_ERROR |
                    UHCI_TD_STS_BITSTUFF_ERROR))
    {
        DPRINT("UhciPollNonIsoEndpoint: NextTD UHCI_TD_STS_ - %02X, PIDCode - %02X\n",
               NextTD->HwTD.ControlStatus.Status,
               NextTD->HwTD.Token.PIDCode);

        UhciDumpHcdTD(NextTD);

        UhciEndpoint->Flags |= UHCI_ENDPOINT_FLAG_HALTED;
        NextTD->Flags |= UHCI_HCD_TD_FLAG_DONE;

        InsertTailList(&UhciEndpoint->ListTDs, &NextTD->TdLink);

        if (TD->UhciTransfer != NextTD->UhciTransfer)
            ASSERT(TD->UhciTransfer == NextTD->UhciTransfer);

        while (TD &&
               TD->UhciTransfer->TransferParameters->TransferCounter ==
               NextTD->UhciTransfer->TransferParameters->TransferCounter)
        {
            DPRINT("UhciPollNonIsoEndpoint: Bad TD - %p\n", TD);

            if (!(TD->Flags & UHCI_HCD_TD_FLAG_DONE))
            {
                TD->Flags |= UHCI_HCD_TD_FLAG_DONE;
                TD->Flags |= UHCI_HCD_TD_FLAG_NOT_ACCESSED;

                InsertTailList(&UhciEndpoint->ListTDs, &TD->TdLink);
            }

            TD = TD->NextHcdTD;
        }

        if (UhciEndpoint->EndpointProperties.TransferType !=
            USBPORT_TRANSFER_TYPE_CONTROL)
        {
            UhciFixDataToggle(UhciExtension,
                              UhciEndpoint,
                              TD,
                              NextTD->HwTD.Token.DataToggle);
        }
    }
    else
    {
        TransferedLen = NextTD->HwTD.ControlStatus.ActualLength;

        if (TransferedLen == UHCI_TD_LENGTH_NULL)
            TransferedLen = 0;
        else
            TransferedLen += 1;

        if (NextTD->HwTD.Token.MaximumLength == UHCI_TD_LENGTH_NULL ||
            TransferedLen >= (NextTD->HwTD.Token.MaximumLength + 1))
        {
            DPRINT_UHCI("UhciPollNonIsoEndpoint: NextTD - %p, TransferedLen - %X\n",
                        NextTD,
                        TransferedLen);

            if (NextTdPA ==
                (QH->HwQH.NextElement & UHCI_QH_ELEMENT_LINK_POINTER_MASK))
            {
                NextTD->Flags |= UHCI_HCD_TD_FLAG_DONE;
                InsertTailList(&UhciEndpoint->ListTDs, &NextTD->TdLink);

                UhciEndpoint->HeadTD = NextTD->NextHcdTD;

                QH->HwQH.NextElement = NextTD->HwTD.NextElement;
                QH->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TD;

                DPRINT_UHCI("UhciPollNonIsoEndpoint: NextTD - %p, TD - %p\n",
                            NextTD,
                            TD);
            }

            goto ProcessListTDs;
        }

        DPRINT_UHCI("UhciPollNonIsoEndpoint: ShortPacket. ControlStatus - %X\n",
                    NextTD->HwTD.ControlStatus.AsULONG);

        NextTD->Flags |= UHCI_HCD_TD_FLAG_DONE;
        InsertTailList(&UhciEndpoint->ListTDs, &NextTD->TdLink);

        while (TD &&
               TD->UhciTransfer->TransferParameters->TransferCounter ==
               NextTD->UhciTransfer->TransferParameters->TransferCounter)
        {
            if (TD->Flags & UHCI_HCD_TD_FLAG_CONTROLL &&
                NextTD->UhciTransfer->TransferParameters->TransferFlags &
                USBD_SHORT_TRANSFER_OK)
            {
                break;
            }

            if (!(TD->Flags & UHCI_HCD_TD_FLAG_DONE))
            {
                DPRINT_UHCI("UhciPollNonIsoEndpoint: TD - %p\n", TD);

                TD->Flags |= (UHCI_HCD_TD_FLAG_DONE |
                              UHCI_HCD_TD_FLAG_NOT_ACCESSED);

                InsertTailList(&UhciEndpoint->ListTDs, &TD->TdLink);
            }

            TD = TD->NextHcdTD;
        }

        if (NextTD->NextHcdTD &&
            (UhciEndpoint->EndpointProperties.TransferType !=
             USBPORT_TRANSFER_TYPE_CONTROL))
        {
            UhciFixDataToggle(UhciExtension,
                              UhciEndpoint,
                              TD,
                              NextTD->NextHcdTD->HwTD.Token.DataToggle);
        }

        if (!(NextTD->UhciTransfer->TransferParameters->TransferFlags &
              USBD_SHORT_TRANSFER_OK))
        {
            UhciEndpoint->Flags |= UHCI_ENDPOINT_FLAG_HALTED;
        }
    }

EnqueueTD:

    if (TD)
    {
        UhciEndpoint->HeadTD = TD;
    }
    else
    {
        UhciEndpoint->HeadTD = NULL;
        UhciEndpoint->TailTD = NULL;
    }

    if (TD == NULL || UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
    {
        PhysicalAddress = UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    }
    else
    {
        PhysicalAddress = TD->PhysicalAddress;
        PhysicalAddress &= ~UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    }

    DPRINT_UHCI("UhciPollNonIsoEndpoint: TD - %p\n", TD);

    QH->HwQH.NextElement = PhysicalAddress;
    QH->HwQH.NextElement &= ~UHCI_QH_ELEMENT_LINK_PTR_QH;

ProcessListTDs:

    ListTDs = &UhciEndpoint->ListTDs;

    while (!IsListEmpty(ListTDs))
    {
        TD = CONTAINING_RECORD(ListTDs->Flink,
                               UHCI_HCD_TD,
                               TdLink.Flink);

        RemoveHeadList(ListTDs);

        if ((TD->Flags & (UHCI_HCD_TD_FLAG_PROCESSED | UHCI_HCD_TD_FLAG_DONE)) ==
                         (UHCI_HCD_TD_FLAG_PROCESSED | UHCI_HCD_TD_FLAG_DONE))
        {
            UhciProcessDoneNonIsoTD(UhciExtension, TD);
        }
    }

    if (UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_CONTROL_OR_ISO &&
        UhciEndpoint->Flags & UHCI_ENDPOINT_FLAG_HALTED)
    {
        DPRINT_UHCI("UhciPollNonIsoEndpoint: Halted periodic EP - %p\n",
                    UhciEndpoint);

        UhciSetEndpointStatus(UhciExtension,
                              UhciEndpoint,
                              USBPORT_ENDPOINT_RUN);
    }
}

VOID
NTAPI
UhciPollEndpoint(IN PVOID uhciExtension,
                 IN PVOID uhciEndpoint)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG TransferType;

    DPRINT_UHCI("UhciPollEndpoint: ...\n");

    TransferType = UhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        UhciPollIsoEndpoint(UhciExtension, UhciEndpoint);
        return;
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        UhciPollNonIsoEndpoint(UhciExtension, UhciEndpoint);
    }
}

VOID
NTAPI
UhciCheckController(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;

    if (!UhciHardwarePresent(UhciExtension) ||
        UhciExtension->HcScheduleError >= UHCI_MAX_HC_SCHEDULE_ERRORS)
    {
       DPRINT1("UhciCheckController: INVALIDATE_CONTROLLER_SURPRISE_REMOVE !!!\n");

       RegPacket.UsbPortInvalidateController(UhciExtension,
                                             USBPORT_INVALIDATE_CONTROLLER_SURPRISE_REMOVE);
    }
}

ULONG
NTAPI
UhciGet32BitFrameNumber(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    ULONG Uhci32BitFrame;
    USHORT Fn; // FrameNumber
    ULONG Hp; // FrameHighPart

    Fn = READ_PORT_USHORT(&UhciExtension->BaseRegister->FrameNumber);
    Fn &= UHCI_FRNUM_FRAME_MASK;
    Hp = UhciExtension->FrameHighPart;

    Uhci32BitFrame = Hp;
    Uhci32BitFrame += ((USHORT)Hp ^ Fn) & UHCI_FRNUM_OVERFLOW_LIST;
    Uhci32BitFrame |= Fn;

    DPRINT_UHCI("UhciGet32BitFrameNumber: Uhci32BitFrame - %lX\n",
                Uhci32BitFrame);

    return Uhci32BitFrame;
}

VOID
NTAPI
UhciInterruptNextSOF(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HC_RESOURCES UhciResources;
    ULONG CurrentFrame;
    PUHCI_HCD_TD SOF_HcdTDs;
    ULONG ix;
    ULONG NextFrame;
    ULONG SofFrame;
    ULONG Idx;

    DPRINT_UHCI("UhciInterruptNextSOF: ...\n");

    CurrentFrame = UhciGet32BitFrameNumber(UhciExtension);

    SOF_HcdTDs = UhciExtension->SOF_HcdTDs;
    NextFrame = CurrentFrame + 2;

    for (ix = 0; ix < UHCI_MAX_STATIC_SOF_TDS; ++ix)
    {
        SofFrame = SOF_HcdTDs->Frame;

        if (SofFrame == NextFrame)
            break;

        if (SofFrame < CurrentFrame)
        {
            SOF_HcdTDs->Frame = NextFrame;
            SOF_HcdTDs->Flags |= UHCI_HCD_TD_FLAG_GOOD_FRAME;

            /* Insert SOF_HcdTD (InterruptOnComplete = TRUE) in Frame List */
            UhciResources = UhciExtension->HcResourcesVA;
            Idx = SOF_HcdTDs->Frame & UHCI_FRAME_LIST_INDEX_MASK;

            InterlockedExchange((PLONG)&SOF_HcdTDs->HwTD.NextElement,
                                UhciResources->FrameList[Idx]);

            UhciResources->FrameList[Idx] = SOF_HcdTDs->PhysicalAddress;
            break;
        }

        /* Go to next SOF_HcdTD */
        SOF_HcdTDs += 1;
    }

    for (ix = 0; ix < UHCI_MAX_STATIC_SOF_TDS; ++ix)
    {
        SOF_HcdTDs = &UhciExtension->SOF_HcdTDs[ix];

        if (SOF_HcdTDs->Frame &&
            (SOF_HcdTDs->Frame < CurrentFrame ||
             (SOF_HcdTDs->Frame - CurrentFrame) > UHCI_FRAME_LIST_MAX_ENTRIES))
        {
            SOF_HcdTDs->Frame = 0;
        }
    }
}

VOID
NTAPI
UhciEnableInterrupts(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    UHCI_PCI_LEGSUP LegacySupport;

    DPRINT("UhciEnableInterrupts: UhciExtension - %p\n", UhciExtension);

    BaseRegister = UhciExtension->BaseRegister;

    RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                          TRUE,
                                          &LegacySupport.AsUSHORT,
                                          PCI_LEGSUP,
                                          sizeof(USHORT));

    LegacySupport.UsbPIRQ = 1;

    RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                          FALSE,
                                          &LegacySupport.AsUSHORT,
                                          PCI_LEGSUP,
                                          sizeof(USHORT));

    WRITE_PORT_USHORT(&BaseRegister->HcInterruptEnable.AsUSHORT,
                      UhciExtension->StatusMask.AsUSHORT);
}

VOID
NTAPI
UhciDisableInterrupts(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    USB_CONTROLLER_FLAVOR HcFlavor;
    UHCI_PCI_LEGSUP LegacySupport;

    DPRINT("UhciDisableInterrupts: UhciExtension - %p\n", UhciExtension);

    BaseRegister = UhciExtension->BaseRegister;
    WRITE_PORT_USHORT(&BaseRegister->HcInterruptEnable.AsUSHORT, 0);

    HcFlavor = UhciExtension->HcFlavor;
    DPRINT("UhciDisableInterrupts: FIXME HcFlavor - %lx\n", HcFlavor);

    RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                          TRUE,
                                          &LegacySupport.AsUSHORT,
                                          PCI_LEGSUP,
                                          sizeof(USHORT));

    LegacySupport.UsbPIRQ = 0;

    RegPacket.UsbPortReadWriteConfigSpace(UhciExtension,
                                          FALSE,
                                          &LegacySupport.AsUSHORT,
                                          PCI_LEGSUP,
                                          sizeof(USHORT));
}

VOID
NTAPI
UhciPollController(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT PortRegister;
    UHCI_PORT_STATUS_CONTROL PortControl;
    USHORT Port;

    DPRINT_UHCI("UhciPollController: UhciExtension - %p\n", UhciExtension);

    BaseRegister = UhciExtension->BaseRegister;

    if (!(UhciExtension->Flags & UHCI_EXTENSION_FLAG_SUSPENDED))
    {
        UhciCleanupFrameList(UhciExtension, FALSE);
        UhciUpdateCounter(UhciExtension);
        RegPacket.UsbPortInvalidateRootHub(UhciExtension);
        return;
    }

    for (Port = 0; Port < UHCI_NUM_ROOT_HUB_PORTS; Port++)
    {
        PortRegister = (PUSHORT)&BaseRegister->PortControl[Port];
        PortControl.AsUSHORT = READ_PORT_USHORT(PortRegister);

        if (PortControl.ConnectStatusChange == 1)
            RegPacket.UsbPortInvalidateRootHub(UhciExtension);
    }
}

VOID
NTAPI
UhciSetEndpointDataToggle(IN PVOID uhciExtension,
                          IN PVOID uhciEndpoint,
                          IN ULONG DataToggle)
{
    DPRINT_IMPL("UhciSetEndpointDataToggle: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciResetController(IN PVOID uhciExtension)
{
    DPRINT_IMPL("UhciResetController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciStartSendOnePacket(IN PVOID uhciExtension,
                       IN PVOID PacketParameters,
                       IN PVOID Data,
                       IN PULONG pDataLength,
                       IN PVOID BufferVA,
                       IN PVOID BufferPA,
                       IN ULONG BufferLength,
                       IN USBD_STATUS * pUSBDStatus)
{
    DPRINT_IMPL("UhciStartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciEndSendOnePacket(IN PVOID uhciExtension,
                     IN PVOID PacketParameters,
                     IN PVOID Data,
                     IN PULONG pDataLength,
                     IN PVOID BufferVA,
                     IN PVOID BufferPA,
                     IN ULONG BufferLength,
                     IN USBD_STATUS * pUSBDStatus)
{
    DPRINT_IMPL("UhciEndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciPassThru(IN PVOID uhciExtension,
             IN PVOID passThruParameters,
             IN ULONG ParameterLength,
             IN PVOID pParameters)
{
    DPRINT_IMPL("UhciPassThru: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciFlushInterrupts(IN PVOID uhciExtension)
{
    DPRINT_IMPL("UhciFlushInterrupts: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciUnload(IN PVOID uhciExtension)
{
    DPRINT_IMPL("UhciUnload: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("DriverEntry: DriverObject - %p, RegistryPath - %p\n", DriverObject, RegistryPath);

    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));

    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_UHCI;

    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_INTERRUPT |
                              USB_MINIPORT_FLAGS_PORT_IO |
                              USB_MINIPORT_FLAGS_NOT_LOCK_INT |
                              USB_MINIPORT_FLAGS_POLLING |
                              USB_MINIPORT_FLAGS_WAKE_SUPPORT;

    RegPacket.MiniPortBusBandwidth = TOTAL_USB11_BUS_BANDWIDTH;

    RegPacket.MiniPortExtensionSize = sizeof(UHCI_EXTENSION);
    RegPacket.MiniPortEndpointSize = sizeof(UHCI_ENDPOINT);
    RegPacket.MiniPortTransferSize = sizeof(UHCI_TRANSFER);
    RegPacket.MiniPortResourcesSize = sizeof(UHCI_HC_RESOURCES);

    RegPacket.OpenEndpoint = UhciOpenEndpoint;
    RegPacket.ReopenEndpoint = UhciReopenEndpoint;
    RegPacket.QueryEndpointRequirements = UhciQueryEndpointRequirements;
    RegPacket.CloseEndpoint = UhciCloseEndpoint;
    RegPacket.StartController = UhciStartController;
    RegPacket.StopController = UhciStopController;
    RegPacket.SuspendController = UhciSuspendController;
    RegPacket.ResumeController = UhciResumeController;
    RegPacket.InterruptService = UhciInterruptService;
    RegPacket.InterruptDpc = UhciInterruptDpc;
    RegPacket.SubmitTransfer = UhciSubmitTransfer;
    RegPacket.SubmitIsoTransfer = UhciIsochTransfer;
    RegPacket.AbortTransfer = UhciAbortTransfer;
    RegPacket.GetEndpointState = UhciGetEndpointState;
    RegPacket.SetEndpointState = UhciSetEndpointState;
    RegPacket.PollEndpoint = UhciPollEndpoint;
    RegPacket.CheckController = UhciCheckController;
    RegPacket.Get32BitFrameNumber = UhciGet32BitFrameNumber;
    RegPacket.InterruptNextSOF = UhciInterruptNextSOF;
    RegPacket.EnableInterrupts = UhciEnableInterrupts;
    RegPacket.DisableInterrupts = UhciDisableInterrupts;
    RegPacket.PollController = UhciPollController;
    RegPacket.SetEndpointDataToggle = UhciSetEndpointDataToggle;
    RegPacket.GetEndpointStatus = UhciGetEndpointStatus;
    RegPacket.SetEndpointStatus = UhciSetEndpointStatus;
    RegPacket.RH_GetRootHubData = UhciRHGetRootHubData;
    RegPacket.RH_GetStatus = UhciRHGetStatus;
    RegPacket.RH_GetPortStatus = UhciRHGetPortStatus;
    RegPacket.RH_GetHubStatus = UhciRHGetHubStatus;
    RegPacket.RH_SetFeaturePortReset = UhciRHSetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = UhciRHSetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = UhciRHSetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = UhciRHSetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = UhciRHClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = UhciRHClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = UhciRHClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = UhciRHClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = UhciRHClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = UhciRHClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = UhciRHClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = UhciRHClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = UhciRHDisableIrq;
    RegPacket.RH_EnableIrq = UhciRHEnableIrq;
    RegPacket.StartSendOnePacket = UhciStartSendOnePacket;
    RegPacket.EndSendOnePacket = UhciEndSendOnePacket;
    RegPacket.PassThru = UhciPassThru;
    RegPacket.FlushInterrupts = UhciFlushInterrupts;

    DriverObject->DriverUnload = NULL;

    return USBPORT_RegisterUSBPortDriver(DriverObject,
                                         USB10_MINIPORT_INTERFACE_VERSION,
                                         &RegPacket);
}
