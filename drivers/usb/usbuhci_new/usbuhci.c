#include "usbuhci.h"

//#define NDEBUG
#include <debug.h>

USBPORT_REGISTRATION_PACKET RegPacket;

VOID
NTAPI
UhciCleanupFrameListEntry(IN PUHCI_EXTENSION UhciExtension,
                          IN ULONG Index)
{
    PUHCI_HC_RESOURCES UhciResources;
    ULONG_PTR PhysicalAddress;
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

    DPRINT("UhciCleanupFrameList: IsAllEntries - %x\n", IsAllEntries);

    if (InterlockedIncrement(&UhciExtension->LockFrameList) != 1)
    {
        InterlockedDecrement(&UhciExtension->LockFrameList);
        return;
    }

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

    InterlockedDecrement(&UhciExtension->LockFrameList);
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

        DPRINT("UhciUpdateCounter: UhciExtension->FrameHighPart - %lX\n",
                UhciExtension->FrameHighPart);
    }
}

VOID
NTAPI
UhciSetNextQH(IN PUHCI_EXTENSION UhciExtension,
              IN PUHCI_HCD_QH QH,
              IN PUHCI_HCD_QH NextQH)
{
    DPRINT("UhciSetNextQH: QH - %p, NextQH - %p\n", QH, NextQH);

    QH->HwQH.NextQH = NextQH->PhysicalAddress | UHCI_QH_HEAD_LINK_PTR_QH;
    QH->NextHcdQH = NextQH;

    NextQH->PrevHcdQH = QH;
    NextQH->QhFlags |= UHCI_HCD_QH_FLAG_ACTIVE;
}

MPSTATUS
NTAPI
UhciOpenEndpoint(IN PVOID uhciExtension,
                 IN PVOID endpointParameters,
                 IN PVOID uhciEndpoint)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointParameters = endpointParameters;
    PUHCI_ENDPOINT UhciEndpoint = uhciEndpoint;
    ULONG TransferType;
    ULONG_PTR BufferVA;
    ULONG_PTR BufferPA;
    ULONG ix;
    ULONG TdCount;
    PUHCI_HCD_TD TD;
    SIZE_T BufferLength;
    PUHCI_HCD_QH QH;

    RtlCopyMemory(&UhciEndpoint->EndpointProperties,
                  EndpointParameters,
                  sizeof(UhciEndpoint->EndpointProperties));

    InitializeListHead(&UhciEndpoint->ListTDs);

    UhciEndpoint->EndpointLock = 0;
    UhciEndpoint->DataToggle = 0;
    UhciEndpoint->Flags = 0;

    TransferType = EndpointParameters->TransferType;

    DPRINT("UhciOpenEndpoint: UhciEndpoint - %p, TransferType - %x\n",
           UhciEndpoint,
           TransferType);

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        UhciEndpoint->Flags |= UHCI_ENDPOINT_FLAG_CONTROLL_OR_ISO;
    }

    BufferVA = EndpointParameters->BufferVA;
    BufferPA = EndpointParameters->BufferPA;

    BufferLength = EndpointParameters->BufferLength;

    /* For Isochronous transfers not used QHs (only TDs) */
    if (EndpointParameters->TransferType != USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
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

    UhciEndpoint->FirstTD = (PVOID)BufferVA;
    UhciEndpoint->AlloccatedTDs = 0;

    RtlZeroMemory(UhciEndpoint->FirstTD, TdCount * sizeof(UHCI_HCD_TD));

    for (ix = 0; ix < UhciEndpoint->MaxTDs; ++ix)
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
                   IN PVOID endpointParameters,
                   IN PVOID uhciEndpoint)
{
    DPRINT("Uhci: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciQueryEndpointRequirements(IN PVOID uhciExtension,
                              IN PVOID endpointParameters,
                              IN PULONG EndpointRequirements)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties = endpointParameters;
    ULONG TransferType;
    ULONG TdCont;

    DPRINT("UhciQueryEndpointRequirements: ... \n");

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT("UhciQueryEndpointRequirements: IsoTransfer\n");
            TdCont = 2 * UHCI_MAX_ISO_TD_COUNT;

            EndpointRequirements[0] = 0 + // Iso queue is have not Queue Heads
                                      TdCont * sizeof(UHCI_HCD_TD);

            EndpointRequirements[1] = UHCI_MAX_ISO_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT("UhciQueryEndpointRequirements: ControlTransfer\n");
            TdCont = EndpointProperties->MaxTransferSize /
                     EndpointProperties->TotalMaxPacketSize;
            TdCont += 2; // First + Last TDs

            EndpointRequirements[0] = sizeof(UHCI_HCD_QH) +
                                      TdCont * sizeof(UHCI_HCD_TD);

            EndpointRequirements[1] = EndpointProperties->MaxTransferSize;
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT("UhciQueryEndpointRequirements: BulkTransfer\n");
            TdCont = 2 * UHCI_MAX_BULK_TRANSFER_SIZE /
                     EndpointProperties->TotalMaxPacketSize;

            EndpointRequirements[0] = sizeof(UHCI_HCD_QH) +
                                      TdCont * sizeof(UHCI_HCD_TD);

            EndpointRequirements[1] = UHCI_MAX_BULK_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT("UhciQueryEndpointRequirements: InterruptTransfer\n");
            TdCont = 2 * UHCI_MAX_INTERRUPT_TD_COUNT;

            EndpointRequirements[0] = sizeof(UHCI_HCD_QH) +
                                      TdCont * sizeof(UHCI_HCD_TD);

            EndpointRequirements[1] = UHCI_MAX_INTERRUPT_TD_COUNT *
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
    DPRINT("UhciCloseEndpoint: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciTakeControlHC(IN PUHCI_EXTENSION UhciExtension,
                  IN PUSBPORT_RESOURCES Resources)
{
    LARGE_INTEGER FirstTime;
    LARGE_INTEGER CurrentTime;
    ULONG ResourcesTypes;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT StatusRegister;
    UHCI_PCI_LEGSUP LegacySupport;
    UHCI_USB_COMMAND Command;
    UHCI_USB_STATUS HcStatus;
    MPSTATUS MpStatus = MP_STATUS_SUCCESS;

    DPRINT("UhciTakeControlHC: Resources->ResourcesTypes - %x\n",
           Resources->ResourcesTypes);

    ResourcesTypes = Resources->ResourcesTypes;

    if ((ResourcesTypes & (USBPORT_RESOURCES_PORT | USBPORT_RESOURCES_INTERRUPT)) !=
                          (USBPORT_RESOURCES_PORT | USBPORT_RESOURCES_INTERRUPT))
    {
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

    KeQuerySystemTime(&FirstTime);
    FirstTime.QuadPart += 100 * 10000; // 100 ms.

    HcStatus.AsUSHORT = READ_PORT_USHORT(StatusRegister);

    while (HcStatus.HcHalted == 0)
    {
        HcStatus.AsUSHORT = READ_PORT_USHORT(StatusRegister);

        KeQuerySystemTime(&CurrentTime);

        if (CurrentTime.QuadPart < FirstTime.QuadPart)
        {
            break;
        }
    }

    HcStatus.Interrupt = 1;
    HcStatus.ErrorInterrupt = 1;
    HcStatus.ResumeDetect = 1;
    HcStatus.HostSystemError = 1;
    HcStatus.HcProcessError = 1;
    HcStatus.HcHalted = 1;

    WRITE_PORT_USHORT(StatusRegister, HcStatus.AsUSHORT);

    if (LegacySupport.Smi60Read == 1 ||
        LegacySupport.Smi60Write == 1 || 
        LegacySupport.Smi64Read == 1 ||
        LegacySupport.Smi64Write == 1 ||
        LegacySupport.SmiIrq == 1 ||
        LegacySupport.A20Gate == 1 ||
        LegacySupport.SmiEndPassThrough == 1)
    {
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

    DPRINT("UhciInitializeHardware: VIA HW FIXME\n");

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
    WRITE_PORT_UCHAR((PUCHAR)(BaseRegister + UHCI_SOFMOD),
                     UhciExtension->SOF_Modify);

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
                       IN PUHCI_HC_RESOURCES HcResourcesPA)
{
    PUHCI_HCD_QH IntQH;
    ULONG_PTR IntQhPA;
    PUHCI_HCD_QH StaticControlHead;
    ULONG_PTR StaticControlHeadPA;
    PUHCI_HCD_QH StaticBulkHead;
    ULONG_PTR StaticBulkHeadPA;
    PUHCI_HCD_TD StaticBulkTD;
    ULONG_PTR StaticBulkTdPA;
    ULONG Idx;
    ULONG HeadIdx;
    ULONG_PTR PhysicalAddress;
    PUHCI_HCD_TD StaticTD;
    ULONG_PTR StaticTdPA;
    UCHAR FrameIdx;

    DPRINT("UhciInitializeSchedule: ...\n");

    /* Build structure (tree) of static QHs
       for interrupt and isochronous transfers */
    for (FrameIdx = 0; FrameIdx < INTERRUPT_ENDPOINTs; FrameIdx++)
    {
        IntQH = &HcResourcesVA->StaticIntHead[FrameIdx];
        IntQhPA = (ULONG_PTR)&HcResourcesPA->StaticIntHead[FrameIdx];

        RtlZeroMemory(IntQH, sizeof(UHCI_HCD_QH));

        IntQH->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
        IntQH->PhysicalAddress = IntQhPA;

        UhciExtension->IntQH[FrameIdx] = IntQH;

        if (FrameIdx == 0)
        {
            UhciSetNextQH(UhciExtension,
                          IntQH,
                          UhciExtension->IntQH[0]);
        }
        else
        {
            UhciSetNextQH(UhciExtension,
                          IntQH,
                          UhciExtension->IntQH[(FrameIdx - 1) / 2]);
        }
    }

    /* Initialize static QH for control transfers */
    StaticControlHead = &HcResourcesVA->StaticControlHead;
    StaticControlHeadPA = (ULONG_PTR)&HcResourcesPA->StaticControlHead;

    RtlZeroMemory(StaticControlHead, sizeof(UHCI_HCD_QH));

    StaticControlHead->HwQH.NextElement |= UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;
    StaticControlHead->PhysicalAddress = StaticControlHeadPA;

    UhciSetNextQH(UhciExtension,
                  UhciExtension->IntQH[0],
                  StaticControlHead);

    UhciExtension->ControlQH = StaticControlHead;

    /* Initialize static QH for bulk transfers */
    StaticBulkHead = &HcResourcesVA->StaticBulkHead;
    StaticBulkHeadPA = (ULONG_PTR)&HcResourcesPA->StaticBulkHead;

    RtlZeroMemory(StaticBulkHead, sizeof(UHCI_HCD_QH));

    StaticBulkHead->PhysicalAddress = StaticBulkHeadPA;

    StaticBulkHeadPA |= UHCI_QH_ELEMENT_LINK_PTR_QH |
                        UHCI_QH_ELEMENT_LINK_PTR_TERMINATE;

    StaticBulkHead->HwQH.NextElement = StaticBulkHeadPA;

    UhciSetNextQH(UhciExtension,
                  StaticControlHead,
                  StaticBulkHead);

    UhciExtension->BulkQH = StaticBulkHead;
    UhciExtension->BulkTailQH = StaticBulkHead;

    /* Initialize static TD for bulk transfers */
    StaticBulkTD = &HcResourcesVA->StaticBulkTD;
    StaticBulkTdPA = (ULONG_PTR)&HcResourcesPA->StaticBulkTD;

    StaticBulkTD->HwTD.NextElement = StaticBulkTdPA | UHCI_TD_LINK_PTR_TD;

    StaticBulkTD->HwTD.ControlStatus.AsULONG = 0;
    StaticBulkTD->HwTD.ControlStatus.IsochronousType = TRUE;

    StaticBulkTD->HwTD.Token.AsULONG = 0;
    StaticBulkTD->HwTD.Token.Endpoint = 1;
    StaticBulkTD->HwTD.Token.MaximumLength = UHCI_TD_MAX_LENGTH_NULL;
    StaticBulkTD->HwTD.Token.PIDCode = UHCI_TD_PID_OUT;

    StaticBulkTD->HwTD.Buffer = 0;

    StaticBulkTD->PhysicalAddress = StaticBulkTdPA;
    StaticBulkTD->NextHcdTD = NULL;
    StaticBulkTD->Flags = UHCI_HCD_TD_FLAG_PROCESSED;

    StaticBulkTdPA |= UHCI_QH_ELEMENT_LINK_PTR_TD;
    UhciExtension->BulkQH->HwQH.NextElement = StaticBulkTdPA;

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
    StaticTdPA = (ULONG_PTR)&HcResourcesPA->StaticTD;

    RtlZeroMemory(StaticTD, sizeof(UHCI_HCD_TD));

    StaticTD->PhysicalAddress = StaticTdPA;

    StaticTD->HwTD.ControlStatus.InterruptOnComplete = TRUE;

    HeadIdx = (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms);
    PhysicalAddress = UhciExtension->IntQH[HeadIdx]->PhysicalAddress;
    StaticTD->HwTD.NextElement = PhysicalAddress | UHCI_TD_LINK_PTR_QH;

    StaticTD->HwTD.Token.PIDCode = UHCI_TD_PID_IN;

    UhciExtension->StaticTD = StaticTD;

    DPRINT("UhciInitializeSchedule: Initialize StaticSofTD - FIXME\n");
//ASSERT(FALSE);

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

    DPRINT("UhciStartController: uhciExtension - %p\n", uhciExtension);

    UhciExtension->Flags &= ~UHCI_EXTENSION_FLAG_SUSPENDED;
    UhciExtension->BaseRegister = Resources->ResourceBase;
    BaseRegister = UhciExtension->BaseRegister;

    UhciExtension->HcFlavor = Resources->HcFlavor;

    MpStatus = UhciTakeControlHC(UhciExtension, Resources);

    if (MpStatus == MP_STATUS_SUCCESS)
    {
        MpStatus = UhciInitializeHardware(UhciExtension);

        if (MpStatus == MP_STATUS_SUCCESS)
        {
            UhciExtension->HcResourcesVA = Resources->StartVA;
            UhciExtension->HcResourcesPA = Resources->StartPA;

            MpStatus = UhciInitializeSchedule(UhciExtension,
                                              UhciExtension->HcResourcesVA,
                                              UhciExtension->HcResourcesPA);

            UhciExtension->LockFrameList = 0;
        }
    }

    WRITE_PORT_ULONG(&BaseRegister->FrameAddress,
                     (ULONG)UhciExtension->HcResourcesPA->FrameList);

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

        UhciExtension->HcResourcesVA->
                       FrameList[0] = UhciExtension->StaticTD->PhysicalAddress;
    }

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciStopController(IN PVOID uhciExtension,
                   IN BOOLEAN IsDoDisableInterrupts)
{
    DPRINT("UhciStopController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciSuspendController(IN PVOID uhciExtension)
{
    DPRINT("UhciSuspendController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciResumeController(IN PVOID uhciExtension)
{
    DPRINT("UhciResumeController: UNIMPLEMENTED. FIXME\n");
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
    {
        DPRINT("UhciHardwarePresent: HW not present\n");
    }

    return UhciStatus.AsUSHORT != MAXUSHORT;
}

BOOLEAN
NTAPI
UhciInterruptService(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    PUSHORT StatusReg;
    UHCI_USB_STATUS HcStatus;
    PUSHORT InterruptEnableReg;
    UHCI_INTERRUPT_ENABLE IntEnable;
    PUHCI_HCD_QH BulkTailQH;
    PUHCI_HCD_QH QH;
    BOOLEAN Result = FALSE;

    DPRINT("UhciInterruptService: ...\n");

    BaseRegister = UhciExtension->BaseRegister;
    StatusReg = &BaseRegister->HcStatus.AsUSHORT;
    InterruptEnableReg = &BaseRegister->HcInterruptEnable.AsUSHORT;

    Result = UhciHardwarePresent(UhciExtension);

    if (Result == FALSE)
    {
        return Result;
    }

    HcStatus.AsUSHORT = READ_PORT_USHORT(StatusReg);
    HcStatus.AsUSHORT &= UHCI_USB_STATUS_MASK;

    if (HcStatus.ResumeDetect == FALSE &&
        HcStatus.HcProcessError == FALSE &&
        (HcStatus.Interrupt |
         HcStatus.ErrorInterrupt |
         HcStatus.HostSystemError |
         HcStatus.HcHalted) == TRUE)
    {
        UhciExtension->HcScheduleError = 0;
    }

    if (HcStatus.Interrupt == TRUE ||
        HcStatus.ErrorInterrupt == TRUE ||
        HcStatus.ResumeDetect == TRUE ||
        HcStatus.HostSystemError == TRUE ||
        HcStatus.HcProcessError == TRUE)
    {
        UhciExtension->HcStatus.AsUSHORT = HcStatus.AsUSHORT;
        WRITE_PORT_USHORT(StatusReg, HcStatus.AsUSHORT);
        WRITE_PORT_USHORT(InterruptEnableReg, 0);
        Result = TRUE;
    }

    if (HcStatus.Interrupt == FALSE)
    {
        goto NextProcess;
    }

    UhciUpdateCounter(UhciExtension);

    BulkTailQH = UhciExtension->BulkTailQH;

    if (BulkTailQH->HwQH.NextQH & UHCI_QH_HEAD_LINK_PTR_TERMINATE)
    {
        goto NextProcess;
    }

    QH = UhciExtension->BulkQH;

    while (TRUE)
    {
        QH = QH->NextHcdQH;

        if (!QH)
        {
            BulkTailQH->HwQH.NextQH |= UHCI_QH_HEAD_LINK_PTR_TERMINATE;
            break;
        }

        if ((QH->HwQH.NextElement & UHCI_QH_ELEMENT_LINK_PTR_TERMINATE) ==
                                    UHCI_QH_ELEMENT_LINK_PTR_VALID)
        {
            break;
        }
    }

NextProcess:

    if (HcStatus.HcProcessError == TRUE)
    {
        UhciCleanupFrameList(UhciExtension, TRUE);

        if (UhciExtension->HcScheduleError < UHCI_MAX_HC_SCHEDULE_ERRORS)
        {
            IntEnable.AsUSHORT = READ_PORT_USHORT(InterruptEnableReg);
            IntEnable.TimeoutCRC = TRUE;
            WRITE_PORT_USHORT(InterruptEnableReg, IntEnable.AsUSHORT);
        }

        UhciExtension->HcScheduleError++;
    }
    else if (HcStatus.Interrupt == TRUE && UhciExtension->ExtensionLock != 0)
    {
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

    DPRINT("UhciInterruptDpc: ...\n");

    BaseRegister = UhciExtension->BaseRegister;

    HcStatus = UhciExtension->HcStatus;
    UhciExtension->HcStatus.AsUSHORT = 0;

    if ((HcStatus.Interrupt | HcStatus.ErrorInterrupt) == TRUE)
    {
        RegPacket.UsbPortInvalidateEndpoint(UhciExtension, 0);
    }

    if (IsDoEnableInterrupts)
    {
        WRITE_PORT_USHORT(&BaseRegister->HcInterruptEnable.AsUSHORT,
                          UhciExtension->StatusMask.AsUSHORT);
    }
}

MPSTATUS
NTAPI
UhciSubmitTransfer(IN PVOID uhciExtension,
                   IN PVOID uhciEndpoint,
                   IN PVOID transferParameters,
                   IN PVOID uhciTransfer,
                   IN PVOID sgList)
{
    DPRINT("UhciSubmitTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciIsochTransfer(IN PVOID ehciExtension,
                  IN PVOID ehciEndpoint,
                  IN PVOID transferParameters,
                  IN PVOID ehciTransfer,
                  IN PVOID isoParameters)
{
    DPRINT("UhciIsochTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciAbortTransfer(IN PVOID uhciExtension,
                  IN PVOID uhciEndpoint,
                  IN PVOID uhciTransfer,
                  IN PULONG CompletedLength)
{
    DPRINT("UhciAbortTransfer: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
UhciGetEndpointState(IN PVOID uhciExtension,
                     IN PVOID uhciEndpoint)
{
    DPRINT("UhciGetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
UhciInsertQH(IN PUHCI_EXTENSION UhciExtension,
             IN PUHCI_HCD_QH StaticQH,
             IN PUHCI_HCD_QH QH)
{
    PUHCI_HCD_QH NextHcdQH;

    DPRINT("UhciInsertQH: ...\n");

    QH->HwQH.NextQH = StaticQH->HwQH.NextQH;
    NextHcdQH = StaticQH->NextHcdQH;

    QH->PrevHcdQH = StaticQH;
    QH->NextHcdQH = NextHcdQH;

    if (NextHcdQH)
    {
        NextHcdQH->PrevHcdQH = QH;
    }
    else
    {
        UhciExtension->BulkTailQH = QH;
    }

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
    {
        UhciExtension->BulkTailQH = PrevHcdQH;
    }

    PrevHcdQH->HwQH.NextQH = QH->HwQH.NextQH;
    PrevHcdQH->NextHcdQH = NextHcdQH;

    if (NextHcdQH)
    {
        NextHcdQH->PrevHcdQH = PrevHcdQH;
    }

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
        {
            break;
        }

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
    {
        return;
    }

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

VOID
NTAPI
UhciPollEndpoint(IN PVOID uhciExtension,
                 IN PVOID ohciEndpoint)
{
    DPRINT("UhciPollEndpoint: UNIMPLEMENTED. FIXME\n");
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
    PUHCI_EXTENSION UhciExtension = (PUHCI_EXTENSION)uhciExtension;
    ULONG Uhci32BitFrame;
    USHORT Fn; // FrameNumber
    ULONG Hp; // FrameHighPart

    Fn = READ_PORT_USHORT(&UhciExtension->BaseRegister->FrameNumber);
    Fn &= UHCI_FRNUM_FRAME_MASK;
    Hp = UhciExtension->FrameHighPart;

    Uhci32BitFrame = Hp;
    Uhci32BitFrame += ((USHORT)Hp ^ Fn) & UHCI_FRNUM_OVERFLOW_LIST;
    Uhci32BitFrame |= Fn;

    DPRINT("UhciGet32BitFrameNumber: Uhci32BitFrame - %lX\n", Uhci32BitFrame);

    return Uhci32BitFrame;
}

VOID
NTAPI
UhciInterruptNextSOF(IN PVOID uhciExtension)
{
    DPRINT("UhciInterruptNextSOF: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciEnableInterrupts(IN PVOID uhciExtension)
{
    PUHCI_EXTENSION UhciExtension = uhciExtension;
    PUHCI_HW_REGISTERS BaseRegister;
    UHCI_PCI_LEGSUP LegacySupport;

    DPRINT("UhciEnableInterrupts: ...\n");

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

    DPRINT("UhciPollController: ...\n");

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
        {
            RegPacket.UsbPortInvalidateRootHub(UhciExtension);
        }
    }
}

VOID
NTAPI
UhciSetEndpointDataToggle(IN PVOID uhciExtension,
                          IN PVOID uhciEndpoint,
                          IN ULONG DataToggle)
{
    DPRINT("UhciSetEndpointDataToggle: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
UhciGetEndpointStatus(IN PVOID uhciExtension,
                      IN PVOID uhciEndpoint)
{
    DPRINT("UhciGetEndpointStatus: UNIMPLEMENTED. FIXME\n");
    return USBPORT_ENDPOINT_RUN;
}

VOID
NTAPI
UhciSetEndpointStatus(IN PVOID uhciExtension,
                      IN PVOID uhciEndpoint,
                      IN ULONG EndpointStatus)
{
    DPRINT("UhciSetEndpointStatus: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
UhciResetController(IN PVOID uhciExtension)
{
    DPRINT("UhciResetController: UNIMPLEMENTED. FIXME\n");
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
    DPRINT("UhciStartSendOnePacket: UNIMPLEMENTED. FIXME\n");
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
    DPRINT("UhciEndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
UhciPassThru(IN PVOID uhciExtension,
             IN PVOID passThruParameters,
             IN ULONG ParameterLength,
             IN PVOID pParameters)
{
    DPRINT("UhciPassThru: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
UhciFlushInterrupts(IN PVOID uhciExtension)
{
    DPRINT("UhciFlushInterrupts: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
UhciUnload(IN PVOID uhciExtension)
{
    DPRINT("UhciUnload: UNIMPLEMENTED. FIXME\n");
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
