#include "usbehci.h"

//#define NDEBUG
#include <debug.h>

#define NDEBUG_EHCI_TRACE
#include "dbg_ehci.h"

USBPORT_REGISTRATION_PACKET RegPacket;

PEHCI_HCD_TD
NTAPI
EHCI_AllocTd(IN PEHCI_EXTENSION EhciExtension,
             IN PEHCI_ENDPOINT EhciEndpoint)
{
    PEHCI_HCD_TD TD;
    ULONG ix = 0;

    DPRINT_EHCI("EHCI_AllocTd: ... \n");

    if (EhciEndpoint->MaxTDs == 0)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return NULL;
    }

    TD = EhciEndpoint->FirstTD;

    while (TD->TdFlags & EHCI_HCD_TD_FLAG_ALLOCATED)
    {
        ++ix;
        TD += 1;

        if (ix >= EhciEndpoint->MaxTDs)
        {
            RegPacket.UsbPortBugCheck(EhciExtension);
            return NULL;
        }
    }

    TD->TdFlags |= EHCI_HCD_TD_FLAG_ALLOCATED;

    --EhciEndpoint->RemainTDs;

    return TD;
}

PEHCI_HCD_QH
NTAPI
EHCI_InitializeQH(PEHCI_EXTENSION EhciExtension,
                  PEHCI_ENDPOINT EhciEndpoint,
                  PEHCI_HCD_QH QH,
                  PEHCI_HCD_QH QhPA)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG DeviceSpeed;

    DPRINT_EHCI("EHCI_InitializeQH: EhciEndpoint - %p, QH - %p, QhPA - %p\n", 
                EhciEndpoint,
                QH,
                QhPA);

    EndpointProperties = &EhciEndpoint->EndpointProperties;

    RtlZeroMemory(QH, sizeof(EHCI_HCD_QH));

    ASSERT(((ULONG_PTR)QhPA & 0x1F) == 0); // link flags

    QH->PhysicalAddress = QhPA;
    QH->EhciEndpoint = EhciEndpoint;

    QH->HwQH.EndpointParams.DeviceAddress = EndpointProperties->DeviceAddress & 0x7F;
    QH->HwQH.EndpointParams.EndpointNumber = EndpointProperties->EndpointAddress & 0x0F;

    DeviceSpeed = EndpointProperties->DeviceSpeed;

    switch (DeviceSpeed)
    {
        case UsbLowSpeed:
            QH->HwQH.EndpointParams.EndpointSpeed = 1;
            break;

        case UsbFullSpeed:
            QH->HwQH.EndpointParams.EndpointSpeed = 0;
            break;

        case UsbHighSpeed:
            QH->HwQH.EndpointParams.EndpointSpeed = 2;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    QH->HwQH.EndpointParams.MaximumPacketLength = EndpointProperties->MaxPacketSize  & 0x7FF;

    QH->HwQH.EndpointCaps.PipeMultiplier = 1;

    if (DeviceSpeed == UsbHighSpeed)
    {
        QH->HwQH.EndpointCaps.HubAddr = 0;
        QH->HwQH.EndpointCaps.PortNumber = 0;
    }
    else
    {
        QH->HwQH.EndpointCaps.HubAddr = EndpointProperties->HubAddr & 0x7F;
        QH->HwQH.EndpointCaps.PortNumber = EndpointProperties->PortNumber & 0x7F;

        if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
        {
            QH->HwQH.EndpointParams.ControlEndpointFlag = 1;
        }
    }

    QH->HwQH.NextTD = 1;
    QH->HwQH.AlternateNextTD = 1;

    return QH;
}

MPSTATUS
NTAPI
EHCI_OpenBulkOrControlEndpoint(IN PEHCI_EXTENSION EhciExtension,
                               IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                               IN PEHCI_ENDPOINT EhciEndpoint,
                               IN BOOLEAN IsControl)
{
    PEHCI_HCD_QH QH;
    PEHCI_HCD_QH QhPA; 
    PEHCI_HCD_TD TdVA;
    PEHCI_HCD_TD TdPA;
    PEHCI_HCD_TD TD;
    ULONG TdCount;
    ULONG ix;

    DPRINT("EHCI_OpenBulkOrControlEndpoint: EhciEndpoint - %p, IsControl - %x\n",
           EhciEndpoint,
           IsControl);

    InitializeListHead(&EhciEndpoint->ListTDs);

    EhciEndpoint->DummyTdVA = (PEHCI_HCD_TD)EndpointProperties->BufferVA;
    EhciEndpoint->DummyTdPA = (PEHCI_HCD_TD)EndpointProperties->BufferPA;

    RtlZeroMemory(EhciEndpoint->DummyTdVA, sizeof(EHCI_HCD_TD));

    QH = (PEHCI_HCD_QH)(EhciEndpoint->DummyTdVA + 1);
    QhPA = (PEHCI_HCD_QH)(EhciEndpoint->DummyTdPA + 1);

    EhciEndpoint->FirstTD = (PEHCI_HCD_TD)(QH + 1);

    TdCount = (EndpointProperties->BufferLength -
               (sizeof(EHCI_HCD_TD) + sizeof(EHCI_HCD_QH))) /
               sizeof(EHCI_HCD_TD);

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        EhciEndpoint->EndpointStatus |= 4;
    }

    EhciEndpoint->MaxTDs = TdCount;
    EhciEndpoint->RemainTDs = TdCount;

    if (TdCount)
    {
        TdVA = EhciEndpoint->FirstTD;
        TdPA = (PEHCI_HCD_TD)(QhPA + 1);

        ix = 0;

        do
        {
            DPRINT_EHCI("EHCI_OpenBulkOrControlEndpoint: TdVA - %p, TdPA - %p\n",
                        TdVA,
                        TdPA);

            RtlZeroMemory(TdVA, sizeof(EHCI_HCD_TD));

            ASSERT(((ULONG_PTR)TdPA & 0x1F) == 0); // link flags

            TdVA->PhysicalAddress = TdPA;
            TdVA->EhciEndpoint = EhciEndpoint;
            TdVA->EhciTransfer = NULL;

            TdPA += 1;
            TdVA += 1;

            ix++;
        }
        while (ix < TdCount);
    }

    EhciEndpoint->QH = EHCI_InitializeQH(EhciExtension,
                                         EhciEndpoint,
                                         QH,
                                         QhPA);

    if (IsControl)
    {
        QH->HwQH.EndpointParams.DataToggleControl = 1;
        EhciEndpoint->HcdHeadP = NULL;
    }
    else
    {
        QH->HwQH.EndpointParams.DataToggleControl = 0;
    }

    TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!TD)
    {
        return 2;
    }

    TD->TdFlags |= EHCI_HCD_TD_FLAG_DUMMY;
    TD->HwTD.Token.Status &= ~EHCI_TOKEN_STATUS_ACTIVE;

    TD->HwTD.NextTD = 1;
    TD->HwTD.AlternateNextTD = 1;

    TD->NextHcdTD = NULL;
    TD->AltNextHcdTD = NULL;

    EhciEndpoint->HcdTailP = TD;
    EhciEndpoint->HcdHeadP = TD;

    QH->HwQH.CurrentTD = (ULONG_PTR)TD->PhysicalAddress;
    QH->HwQH.NextTD = 1;
    QH->HwQH.AlternateNextTD = 1;

    QH->HwQH.Token.Status &= ~EHCI_TOKEN_STATUS_ACTIVE;
    QH->HwQH.Token.TransferBytes = 0;

    return 0;
}

MPSTATUS
NTAPI
EHCI_OpenInterruptEndpoint(IN PEHCI_EXTENSION EhciExtension,
                           IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                           IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_OpenInterruptEndpoint: UNIMPLEMENTED. FIXME\n");
    return 6;
}

MPSTATUS
NTAPI
EHCI_OpenHsIsoEndpoint(IN PEHCI_EXTENSION EhciExtension,
                       IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                       IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_OpenHsIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return 6;
}

MPSTATUS
NTAPI
EHCI_OpenIsoEndpoint(IN PEHCI_EXTENSION EhciExtension,
                     IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                     IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_OpenIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return 6;
}

MPSTATUS
NTAPI
EHCI_OpenEndpoint(IN PVOID ehciExtension,
                  IN PVOID endpointParameters,
                  IN PVOID ehciEndpoint)
{
    PEHCI_EXTENSION EhciExtension;
    PEHCI_ENDPOINT EhciEndpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;
    ULONG Result;

    DPRINT_EHCI("EHCI_OpenEndpoint: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    EhciEndpoint = (PEHCI_ENDPOINT)ehciEndpoint;
    EndpointProperties = (PUSBPORT_ENDPOINT_PROPERTIES)endpointParameters;

    RtlCopyMemory(&EhciEndpoint->EndpointProperties,
                  endpointParameters,
                  sizeof(USBPORT_ENDPOINT_PROPERTIES));

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
            {
                Result = EHCI_OpenHsIsoEndpoint(EhciExtension,
                                                EndpointProperties,
                                                EhciEndpoint);
            }
            else
            {
                Result = EHCI_OpenIsoEndpoint(EhciExtension,
                                              EndpointProperties,
                                              EhciEndpoint);
            }

            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            Result = EHCI_OpenBulkOrControlEndpoint(EhciExtension,
                                                    EndpointProperties,
                                                    EhciEndpoint,
                                                    TRUE);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            Result = EHCI_OpenBulkOrControlEndpoint(EhciExtension,
                                                    EndpointProperties,
                                                    EhciEndpoint,
                                                    FALSE);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            Result = EHCI_OpenInterruptEndpoint(EhciExtension,
                                                EndpointProperties,
                                                EhciEndpoint);
            break;

        default:
            Result = 6;
            break;
    }

    return Result;
}

MPSTATUS
NTAPI
EHCI_ReopenEndpoint(IN PVOID ehciExtension,
                    IN PVOID endpointParameters,
                    IN PVOID ehciEndpoint)
{
    DPRINT1("EHCI_ReopenEndpoint: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_QueryEndpointRequirements(IN PVOID ehciExtension,
                               IN PVOID endpointParameters,
                               IN PULONG EndpointRequirements)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;

    DPRINT_EHCI("EHCI_QueryEndpointRequirements: ... \n");

    EndpointProperties = (PUSBPORT_ENDPOINT_PROPERTIES)endpointParameters;
    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT("EHCI_QueryEndpointRequirements: IsoTransfer\n");

            if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
            {
                *EndpointRequirements = 0x40000;
                *(PULONG)(EndpointRequirements + 1) = 0x180000;
            }
            else
            {
                *EndpointRequirements = 0x1000;
                *(PULONG)(EndpointRequirements + 1) = 0x40000;
            }
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT("EHCI_QueryEndpointRequirements: ControlTransfer\n");
            *EndpointRequirements = sizeof(EHCI_HCD_QH) + (1 + 6) * sizeof(EHCI_HCD_TD);
            *((PULONG)EndpointRequirements + 1) = 0x10000;
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT("EHCI_QueryEndpointRequirements: BulkTransfer\n");
            *EndpointRequirements = sizeof(EHCI_HCD_QH) + (1 + 209) * sizeof(EHCI_HCD_TD);
            *((PULONG)EndpointRequirements + 1) = 0x400000;
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT("EHCI_QueryEndpointRequirements: InterruptTransfer\n");
            *EndpointRequirements = sizeof(EHCI_HCD_QH) + (1 + 4) * sizeof(EHCI_HCD_TD);
            *((PULONG)EndpointRequirements + 1) = 0x1000;
            break;

        default:
            DPRINT1("EHCI_QueryEndpointRequirements: Unknown TransferType - %x\n",
                    TransferType);
            DbgBreakPoint();
            break;
    }
}

VOID
NTAPI
EHCI_CloseEndpoint(IN PVOID ehciExtension,
                   IN PVOID ehciEndpoint,
                   IN BOOLEAN IsDoDisablePeriodic)
{
    DPRINT1("EHCI_CloseEndpoint: UNIMPLEMENTED. FIXME\n");
}

PEHCI_STATIC_QH
NTAPI
EHCI_GetQhForFrame(IN PEHCI_EXTENSION EhciExtension,
                   IN ULONG FrameIdx)
{
    static UCHAR Balance[32] = {
        0, 16, 8,  24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30, 
        1, 17, 9,  25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31};

    DPRINT_EHCI("EHCI_GetQhForFrame: FrameIdx - %x, Balance[FrameIdx] - %x\n",
                FrameIdx,
                Balance[FrameIdx & 0x1F]);

    return EhciExtension->PeriodicHead[Balance[FrameIdx & 0x1F]];
}

PEHCI_HCD_QH
NTAPI
EHCI_GetDummyQhForFrame(IN PEHCI_EXTENSION EhciExtension,
                        IN ULONG Idx)
{
    return (PEHCI_HCD_QH)(EhciExtension->DummyQHListVA + Idx * sizeof(EHCI_HCD_QH));
}

VOID
NTAPI
EHCI_AlignHwStructure(IN PEHCI_EXTENSION EhciExtension,
                      IN PULONG PhysicalAddress,
                      IN PULONG VirtualAddress,
                      IN ULONG Alignment)
{
    ULONG PAddress;
    PVOID NewPAddress;
    ULONG VAddress;

    DPRINT_EHCI("EHCI_AlignHwStructure: *PhysicalAddress - %p, *VirtualAddress - %p, Alignment - %x\n",
                 *PhysicalAddress,
                 *VirtualAddress,
                 Alignment);

    PAddress = *PhysicalAddress;
    VAddress = *VirtualAddress;

    NewPAddress = PAGE_ALIGN(*PhysicalAddress + Alignment - 1);

    if (NewPAddress != PAGE_ALIGN(*PhysicalAddress))
    {
        VAddress += (ULONG)NewPAddress - PAddress;
        PAddress = (ULONG)PAGE_ALIGN(*PhysicalAddress + Alignment - 1);

        DPRINT("EHCI_AlignHwStructure: VAddress - %p, PAddress - %p\n",
               VAddress,
               PAddress);
    }

    *VirtualAddress = VAddress;
    *PhysicalAddress = PAddress;
}

VOID
NTAPI
EHCI_AddDummyQHs(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_STATIC_QH * FrameHeader;
    PEHCI_STATIC_QH StaticQH;
    PEHCI_HCD_QH DummyQhVA;
    PEHCI_HCD_QH DummyQhPA;
    EHCI_LINK_POINTER PrevPA;
    EHCI_QH_EP_PARAMS EndpointParams;
    ULONG ix = 0;

    DPRINT_EHCI("EHCI_AddDummyQHs: ... \n");

    FrameHeader = &EhciExtension->HcResourcesVA->PeriodicFrameList[0];

    DummyQhVA = (PEHCI_HCD_QH)EhciExtension->DummyQHListVA;
    DummyQhPA = (PEHCI_HCD_QH)EhciExtension->DummyQHListPA;

    while (TRUE)
    {
        ASSERT(EHCI_GetDummyQhForFrame(EhciExtension, ix) == DummyQhVA);

        RtlZeroMemory(DummyQhVA, sizeof(EHCI_HCD_QH));

        DummyQhVA->HwQH.CurrentTD = 0;
        DummyQhVA->HwQH.Token.Status &= ~EHCI_TOKEN_STATUS_ACTIVE;

        PrevPA.AsULONG = (ULONG)DummyQhVA->PhysicalAddress;
        PrevPA.Type = 1;

        DummyQhVA->PhysicalAddress = DummyQhPA;

        DummyQhVA += 1;
        DummyQhPA += 1;

        DummyQhVA->HwQH.HorizontalLink.AsULONG = (ULONG)(*FrameHeader);

        EndpointParams = DummyQhVA->HwQH.EndpointParams;
        EndpointParams.DeviceAddress = 0;
        EndpointParams.EndpointNumber = 0;
        EndpointParams.EndpointSpeed = 0;
        EndpointParams.MaximumPacketLength = 64;

        DummyQhVA->HwQH.EndpointParams = EndpointParams;

        DummyQhVA->HwQH.EndpointCaps.AsULONG = 0;
        DummyQhVA->HwQH.EndpointCaps.InterruptMask = 0;
        DummyQhVA->HwQH.EndpointCaps.SplitCompletionMask = 0;
        DummyQhVA->HwQH.EndpointCaps.PipeMultiplier = 1;

        DummyQhVA->HwQH.AlternateNextTD = 1;
        DummyQhVA->HwQH.NextTD = 1;

        StaticQH = EHCI_GetQhForFrame(EhciExtension, ix);
        DummyQhVA->StaticQH = StaticQH;

        *FrameHeader = (PEHCI_STATIC_QH)PrevPA.AsULONG;

        FrameHeader += 1;
        ++ix;

        if (ix >= 1024)
        {
            break;
        }
    }
}

VOID
NTAPI
EHCI_InitializeInterruptSchedule(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_STATIC_QH StaticQH;
    ULONG ix;
    static UCHAR LinkTable[64] = {
      255, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,  9, 9,
      10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19,
      20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
      30, 30, 0}; 

    DPRINT_EHCI("EHCI_InitializeInterruptSchedule: ... \n");

    for (ix = 0; ix < 63; ix++)
    {
        StaticQH = EhciExtension->PeriodicHead[ix];

        StaticQH->HwQH.EndpointParams.HeadReclamationListFlag = 0;
        StaticQH->HwQH.NextTD |= 1;
        StaticQH->HwQH.Token.Status |= EHCI_TOKEN_STATUS_HALTED;
    }

    for (ix = 1; ix < 63; ix++)
    {
        StaticQH = EhciExtension->PeriodicHead[ix];
 
        StaticQH->PrevHead = NULL;
        StaticQH->NextHead = (PEHCI_HCD_QH)EhciExtension->PeriodicHead[LinkTable[ix]];

        StaticQH->HwQH.HorizontalLink.AsULONG = 
            (ULONG)EhciExtension->PeriodicHead[LinkTable[ix]]->PhysicalAddress;

        StaticQH->HwQH.HorizontalLink.Type = EHCI_LINK_TYPE_QH;
        StaticQH->HwQH.EndpointCaps.AsULONG = -1;

        StaticQH->QhFlags |= EHCI_QH_FLAG_STATIC;

        if ((ix + 1) <= 6)
        {
            StaticQH->QhFlags |= 8;
        }
    }

    EhciExtension->PeriodicHead[0]->HwQH.HorizontalLink.Terminate = 1;
    EhciExtension->PeriodicHead[0]->QhFlags |= (EHCI_QH_FLAG_STATIC | 8);
}

MPSTATUS
NTAPI
EHCI_InitializeSchedule(IN PEHCI_EXTENSION EhciExtension,
                        IN PVOID resourcesStartVA,
                        IN PVOID resourcesStartPA)
{
    PULONG OperationalRegs;
    PEHCI_HC_RESOURCES HcResourcesVA;
    PEHCI_HC_RESOURCES HcResourcesPA;
    PEHCI_STATIC_QH AsyncHead;
    PEHCI_STATIC_QH AsyncHeadPA;
    PEHCI_STATIC_QH PeriodicHead;
    PEHCI_STATIC_QH PeriodicHeadPA;
    PEHCI_STATIC_QH StaticQH;
    EHCI_LINK_POINTER NextLink;
    EHCI_LINK_POINTER StaticHeadPA;
    ULONG Frame;
    ULONG ix;

    DPRINT_EHCI("EHCI_InitializeSchedule: BaseVA - %p, BasePA - %p\n",
                resourcesStartVA,
                resourcesStartPA);

    OperationalRegs = EhciExtension->OperationalRegs;

    HcResourcesVA = (PEHCI_HC_RESOURCES)resourcesStartVA;
    HcResourcesPA = (PEHCI_HC_RESOURCES)resourcesStartPA;

    EhciExtension->HcResourcesVA = HcResourcesVA;
    EhciExtension->HcResourcesPA = HcResourcesPA;

    /* Asynchronous Schedule */

    AsyncHead = &HcResourcesVA->AsyncHead;
    AsyncHeadPA = &HcResourcesPA->AsyncHead;

    RtlZeroMemory(AsyncHead, sizeof(EHCI_STATIC_QH));

    NextLink.AsULONG = (ULONG)AsyncHeadPA;
    NextLink.Type = EHCI_LINK_TYPE_QH;

    AsyncHead->HwQH.HorizontalLink = NextLink;
    AsyncHead->HwQH.EndpointParams.HeadReclamationListFlag = 1;
    AsyncHead->HwQH.EndpointCaps.PipeMultiplier = 1;
    AsyncHead->HwQH.NextTD |= 1;
    AsyncHead->HwQH.Token.Status = EHCI_TOKEN_STATUS_HALTED;

    AsyncHead->PhysicalAddress = AsyncHeadPA;
    AsyncHead->PrevHead = AsyncHead->NextHead = (PEHCI_HCD_QH)AsyncHead;

    EhciExtension->AsyncHead = AsyncHead;

    /* Periodic Schedule */

    PeriodicHead = &HcResourcesVA->PeriodicHead[0];
    PeriodicHeadPA = &HcResourcesPA->PeriodicHead[0];

    ix = 0;

    for (ix = 0; ix < 64; ix++)
    {
        EHCI_AlignHwStructure(EhciExtension,
                              (PULONG)&PeriodicHeadPA,
                              (PULONG)&PeriodicHead,
                              80);

        EhciExtension->PeriodicHead[ix] = PeriodicHead;
        EhciExtension->PeriodicHead[ix]->PhysicalAddress = PeriodicHeadPA;

        PeriodicHead += 1;
        PeriodicHeadPA += 1;
    }

    EHCI_InitializeInterruptSchedule(EhciExtension);

    for (Frame = 0; Frame < 1024; Frame++)
    {
        StaticQH = EHCI_GetQhForFrame(EhciExtension, Frame);

        StaticHeadPA.AsULONG = (ULONG_PTR)StaticQH->PhysicalAddress;
        StaticHeadPA.Type = EHCI_LINK_TYPE_QH;

        DPRINT_EHCI("EHCI_InitializeSchedule: StaticHeadPA[%x] - %p\n",
                    Frame,
                    StaticHeadPA);

        HcResourcesVA->PeriodicFrameList[Frame] = (PEHCI_STATIC_QH)StaticHeadPA.AsULONG;
    }

    EhciExtension->DummyQHListVA = (ULONG_PTR)&HcResourcesVA->DummyQH;
    EhciExtension->DummyQHListPA = (ULONG_PTR)&HcResourcesPA->DummyQH;

    EHCI_AddDummyQHs(EhciExtension);

    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_PERIODICLISTBASE,
                         (ULONG_PTR)EhciExtension->HcResourcesPA);

    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_ASYNCLISTBASE,
                         NextLink.AsULONG);

    return 0;
}

MPSTATUS
NTAPI
EHCI_InitializeHardware(IN PEHCI_EXTENSION EhciExtension)
{
    PULONG BaseIoAdress;
    PULONG OperationalRegs;
    EHCI_USB_COMMAND Command;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER LastTime = {{0, 0}};
    EHCI_HC_STRUCTURAL_PARAMS StructuralParams;

    DPRINT_EHCI("EHCI_InitializeHardware: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;
    BaseIoAdress = EhciExtension->BaseIoAdress;

    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    Command.Reset = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, Command.AsULONG);

    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 100 * 10000; // 100 msec

    DPRINT_EHCI("EHCI_InitializeHardware: Start reset ... \n");

    while (TRUE)
    {
        KeQuerySystemTime(&LastTime);
        Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);

        if (Command.Reset != 1)
        {
            break;
        }

        if (LastTime.QuadPart >= CurrentTime.QuadPart)
        {
            if (Command.Reset == 1)
            {
                DPRINT1("EHCI_InitializeHardware: Reset failed!\n");
                return 7;
            }

            break;
        }
    }

    DPRINT("EHCI_InitializeHardware: Reset - OK\n");

    StructuralParams.AsULONG = READ_REGISTER_ULONG(BaseIoAdress + 1); // HCSPARAMS register

    EhciExtension->NumberOfPorts = StructuralParams.PortCount;
    EhciExtension->PortPowerControl = StructuralParams.PortPowerControl;

    DPRINT("EHCI_InitializeHardware: StructuralParams - %p\n", StructuralParams.AsULONG);
    DPRINT("EHCI_InitializeHardware: PortPowerControl - %x\n", EhciExtension->PortPowerControl);
    DPRINT("EHCI_InitializeHardware: N_PORTS          - %x\n", EhciExtension->NumberOfPorts);

    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_PERIODICLISTBASE, 0);
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_ASYNCLISTBASE, 0);

    EhciExtension->InterruptMask.AsULONG = 0;
    EhciExtension->InterruptMask.Interrupt = 1;
    EhciExtension->InterruptMask.ErrorInterrupt = 1;
    EhciExtension->InterruptMask.PortChangeInterrupt = 0;
    EhciExtension->InterruptMask.FrameListRollover = 1;
    EhciExtension->InterruptMask.HostSystemError = 1;
    EhciExtension->InterruptMask.InterruptOnAsyncAdvance = 1;

    return 0;
}

MPSTATUS
NTAPI
EHCI_TakeControlHC(IN PEHCI_EXTENSION EhciExtension)
{
    DPRINT1("EHCI_TakeControlHC: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_GetRegistryParameters(IN PEHCI_EXTENSION EhciExtension)
{
    DPRINT1("EHCI_GetRegistryParameters: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_StartController(IN PVOID ehciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG BaseIoAdress;
    PULONG OperationalRegs;
    MPSTATUS MPStatus;
    EHCI_USB_COMMAND Command;
    UCHAR CapabilityRegLength;
    UCHAR Fladj;

    DPRINT_EHCI("EHCI_StartController: ... \n");

    if ((Resources->TypesResources & 6) != 6) // (Interrupt | Memory)
    {
        DPRINT1("EHCI_StartController: Resources->TypesResources - %x\n",
                Resources->TypesResources);

        return 4;
    }

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    BaseIoAdress = (PULONG)Resources->ResourceBase;
    EhciExtension->BaseIoAdress = BaseIoAdress;

    CapabilityRegLength = (UCHAR)READ_REGISTER_ULONG(BaseIoAdress);
    OperationalRegs = (PULONG)((ULONG)BaseIoAdress + CapabilityRegLength);
    EhciExtension->OperationalRegs = OperationalRegs;

    DPRINT("EHCI_StartController: BaseIoAdress    - %p\n", BaseIoAdress);
    DPRINT("EHCI_StartController: OperationalRegs - %p\n", OperationalRegs);

    RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                          1,
                                          &Fladj,
                                          0x61,
                                          1);

    EhciExtension->FrameLengthAdjustment = Fladj;

    EHCI_GetRegistryParameters(EhciExtension);

    MPStatus = EHCI_TakeControlHC(EhciExtension);

    if (MPStatus)
    {
        DPRINT1("EHCI_StartController: Unsuccessful TakeControlHC()\n");
        return MPStatus;
    }

    MPStatus = EHCI_InitializeHardware(EhciExtension);

    if (MPStatus)
    {
        DPRINT1("EHCI_StartController: Unsuccessful InitializeHardware()\n");
        return MPStatus;
    }

    MPStatus = EHCI_InitializeSchedule(EhciExtension,
                                       Resources->StartVA,
                                       Resources->StartPA);

    if (MPStatus)
    {
        DPRINT1("EHCI_StartController: Unsuccessful InitializeSchedule()\n");
        return MPStatus;
    }

    RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                          1,
                                          &Fladj,
                                          0x61,
                                          1);

    if (Fladj != EhciExtension->FrameLengthAdjustment)
    {
        Fladj = EhciExtension->FrameLengthAdjustment;

        RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                              0, // write
                                              &Fladj,
                                              0x61,
                                              1);
    }

    /* Port routing control logic default-routes all ports to this HC */
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_CONFIGFLAG, 1);
    EhciExtension->PortRoutingControl = 1;

    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    Command.InterruptThreshold = 1; // one micro-frame
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, Command.AsULONG);

    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    Command.Run = 1; // execution of the schedule
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, Command.AsULONG);

    EhciExtension->IsStarted = 1;

    if (Resources->Reserved1)
    {
        DPRINT1("EHCI_StartController: FIXME\n");
        DbgBreakPoint();
    }

    return MPStatus;
}

VOID
NTAPI
EHCI_StopController(IN PVOID ehciExtension,
                    IN BOOLEAN IsDoDisableInterrupts)
{
    DPRINT1("EHCI_StopController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_SuspendController(IN PVOID ehciExtension)
{
    DPRINT1("EHCI_SuspendController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_ResumeController(IN PVOID ehciExtension)
{
    DPRINT1("EHCI_ResumeController: UNIMPLEMENTED. FIXME\n");
    return 0;
}

BOOLEAN
NTAPI
EHCI_HardwarePresent(IN PEHCI_EXTENSION EhciExtension,
                     IN BOOLEAN IsInvalidateController)
{
    if (READ_REGISTER_ULONG(EhciExtension->OperationalRegs) != -1)
    {
        return TRUE;
    }

    DPRINT1("EHCI_HardwarePresent: IsInvalidateController - %x\n",
            IsInvalidateController);

    if (IsInvalidateController)
    {
        RegPacket.UsbPortInvalidateController(EhciExtension, 2);
    }

    return FALSE;
}

BOOLEAN
NTAPI
EHCI_InterruptService(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG OperationalRegs;
    BOOLEAN Result = FALSE;
    ULONG IntrSts;
    ULONG IntrEn;
    EHCI_INTERRUPT_ENABLE iStatus;
    ULONG FrameIndex;
    ULONG Command;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    OperationalRegs = EhciExtension->OperationalRegs;

    DPRINT_EHCI("EHCI_InterruptService: ... \n");

    Result = EHCI_HardwarePresent(EhciExtension, FALSE);

    if (!Result)
    {
        return FALSE;
    }

    IntrEn = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBINTR);
    IntrSts = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS);

    iStatus.AsULONG = (IntrEn & IntrSts) & 0x3F;

    if (!iStatus.AsULONG)
    {
        return FALSE;
    }

    EhciExtension->InterruptStatus = iStatus;

    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS,
                         (IntrEn & IntrSts) & 0x3F);

    if (iStatus.HostSystemError)
    {
        ++EhciExtension->HcSystemErrors;

        if (EhciExtension->HcSystemErrors < 0x100)
        {
            //attempting reset
            Command = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
            WRITE_REGISTER_ULONG((PULONG)OperationalRegs, Command | 1);
        }
    }

    FrameIndex = (READ_REGISTER_ULONG(OperationalRegs + EHCI_FRINDEX) >> 3) & 0x7FF;

    if ((FrameIndex ^ EhciExtension->FrameIndex) & 0x400)
    {
        EhciExtension->FrameHighPart = EhciExtension->FrameHighPart + 0x800 - 
                                       ((FrameIndex ^ EhciExtension->FrameHighPart) & 0x400);
    }

    EhciExtension->FrameIndex = FrameIndex;

    return TRUE;
}

VOID
NTAPI
EHCI_InterruptDpc(IN PVOID ehciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG OperationalRegs;
    EHCI_INTERRUPT_ENABLE iStatus;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    OperationalRegs = EhciExtension->OperationalRegs;

    DPRINT_EHCI("EHCI_InterruptDpc: ... \n");

    iStatus = EhciExtension->InterruptStatus;
    EhciExtension->InterruptStatus.AsULONG = 0;

    if ((UCHAR)iStatus.AsULONG &
        (UCHAR)EhciExtension->InterruptMask.AsULONG &
        0x23)
    {
        RegPacket.UsbPortInvalidateEndpoint(EhciExtension, 0);
    }

    if ((UCHAR)iStatus.AsULONG &
        (UCHAR)EhciExtension->InterruptMask.AsULONG &
        0x04)
    {
        RegPacket.UsbPortInvalidateRootHub(EhciExtension);
    }

    if (IsDoEnableInterrupts)
    {
        WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBINTR,
                             EhciExtension->InterruptMask.AsULONG);
    }
}

ULONG
NTAPI
EHCI_MapAsyncTransferToTd(IN PEHCI_EXTENSION EhciExtension,
                          IN ULONG MaxPacketSize,
                          IN ULONG TransferedLen,
                          IN PULONG DataToggle,
                          IN PEHCI_TRANSFER EhciTransfer,
                          IN PEHCI_HCD_TD TD,
                          IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    PUSBPORT_SCATTER_GATHER_ELEMENT SgElement;
    ULONG SgIdx;
    ULONG LengthThisTD;
    ULONG ix;
    ULONG SgRemain;
    ULONG DiffLength;
    ULONG NumPackets;

    DPRINT_EHCI("EHCI_MapAsyncTransferToTd: EhciTransfer - %p, TD - %p, TransferedLen - %x, MaxPacketSize - %x, DataToggle - %x\n",
           EhciTransfer,
           TD,
           TransferedLen,
           MaxPacketSize,
           DataToggle);

    TransferParameters = EhciTransfer->TransferParameters;

    SgIdx = 0;

    if (SgList->SgElementCount)
    {
        SgElement = &SgList->SgElement[0];

        do
        {
            if (TransferedLen >= SgElement->SgOffset &&
                TransferedLen < SgElement->SgOffset + SgElement->SgTransferLength)
            {
                break;
            }

            ++SgIdx;
            SgElement += 1;
        }
        while (SgIdx < SgList->SgElementCount);
    }

    SgRemain = SgList->SgElementCount - SgIdx;

    if (SgRemain > 5)
    {
        TD->HwTD.Buffer[0] = SgList->SgElement[SgIdx].SgPhysicalAddress.LowPart - 
                             SgList->SgElement[SgIdx].SgOffset +
                             TransferedLen;

        LengthThisTD = 5 * 0x1000 - (TD->HwTD.Buffer[0] & 0xFFF);

        for (ix = 1; ix <= 4; ix++)
        {
            TD->HwTD.Buffer[ix] = SgList->SgElement[SgIdx + ix].SgPhysicalAddress.LowPart;
        }

        NumPackets = LengthThisTD / MaxPacketSize;
        DiffLength = LengthThisTD - MaxPacketSize * (LengthThisTD / MaxPacketSize);

        if (LengthThisTD != MaxPacketSize * (LengthThisTD / MaxPacketSize))
        {
            LengthThisTD -= DiffLength;
        }

        if (DataToggle && (NumPackets & 1))
        {
            *DataToggle = *DataToggle == 0;
        }
    }
    else
    {
        LengthThisTD = TransferParameters->TransferBufferLength - TransferedLen;

        TD->HwTD.Buffer[0] = TransferedLen +
                             SgList->SgElement[SgIdx].SgPhysicalAddress.LowPart - 
                             SgList->SgElement[SgIdx].SgOffset;

        for (ix = 1; ix < 5; ix++)
        {
            if ((SgIdx + ix) >= SgList->SgElementCount)
            {
                break;
            }

            TD->HwTD.Buffer[ix] = SgList->SgElement[SgIdx + ix].SgPhysicalAddress.LowPart;
        }
    }

    TD->HwTD.Token.TransferBytes = LengthThisTD;
    TD->LengthThisTD = LengthThisTD;

    return LengthThisTD + TransferedLen;
}

VOID
NTAPI
EHCI_EnableAsyncList(IN PEHCI_EXTENSION EhciExtension)
{
    PULONG OperationalRegs;
    EHCI_USB_COMMAND UsbCmd;

    DPRINT_EHCI("EHCI_EnableAsyncList: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    UsbCmd.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    UsbCmd.AsynchronousEnable = 1;
    WRITE_REGISTER_ULONG((OperationalRegs + EHCI_USBCMD), UsbCmd.AsULONG);
}

VOID
NTAPI
EHCI_DisableAsyncList(IN PEHCI_EXTENSION EhciExtension)
{
    PULONG OperationalRegs;
    EHCI_USB_COMMAND UsbCmd;

    DPRINT_EHCI("EHCI_DisableAsyncList: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    UsbCmd.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    UsbCmd.AsynchronousEnable = 0;
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, UsbCmd.AsULONG);
}

VOID
NTAPI
EHCI_EnablePeriodicList(IN PEHCI_EXTENSION EhciExtension)
{
    PULONG OperationalRegs;
    EHCI_USB_COMMAND Command;

    DPRINT_EHCI("EHCI_EnablePeriodicList: ... \n");

    OperationalRegs = (PULONG)EhciExtension->OperationalRegs;

    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    Command.PeriodicEnable = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, Command.AsULONG);
}

VOID
NTAPI
EHCI_FlushAsyncCache(IN PEHCI_EXTENSION EhciExtension)
{
    PULONG OperationalRegs;
    EHCI_USB_COMMAND Command;
    EHCI_USB_STATUS Status;
    LARGE_INTEGER CurrentTime = {{0, 0}};
    LARGE_INTEGER FirstTime = {{0, 0}};
    EHCI_USB_COMMAND Cmd;

    DPRINT_EHCI("EHCI_FlushAsyncCache: EhciExtension - %p\n", EhciExtension);

    OperationalRegs = EhciExtension->OperationalRegs;
    Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
    Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS);

    if (!Status.AsynchronousStatus && !Command.AsynchronousEnable)
    {
        return;
    }

    if (Status.AsynchronousStatus && !Command.AsynchronousEnable)
    {
        KeQuerySystemTime(&FirstTime);
        FirstTime.QuadPart += 100 * 10000;  //100 ms

        do
        {
            Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS);
            Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
            KeQuerySystemTime(&CurrentTime);

            if (CurrentTime.QuadPart > FirstTime.QuadPart)
            {
                RegPacket.UsbPortBugCheck(EhciExtension);
            }
        }
        while (Status.AsynchronousStatus && Command.AsULONG != -1 && Command.Run);

        return;
    }    

    if (!Status.AsynchronousStatus && Command.AsynchronousEnable)
    {
        KeQuerySystemTime(&FirstTime);
        FirstTime.QuadPart += 100 * 10000;  //100 ms

        do
        {
            Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS);
            Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
            KeQuerySystemTime(&CurrentTime);
        }
        while (!Status.AsynchronousStatus && Command.AsULONG != -1 && Command.Run);
    }

    Command.InterruptAdvanceDoorbell = 1;
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD, Command.AsULONG);

    KeQuerySystemTime(&FirstTime);
    FirstTime.QuadPart += 100 * 10000;  //100 ms

    Cmd.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);

    if (Cmd.InterruptAdvanceDoorbell)
    {
        while (Cmd.Run)
        {
            if (Cmd.AsULONG == -1)
            {
                break;
            }

            KeStallExecutionProcessor(1);
            Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
            KeQuerySystemTime(&CurrentTime);

            if (!Command.InterruptAdvanceDoorbell)
            {
                break;
            }

            Cmd = Command;
        }
    }

    /* InterruptOnAsyncAdvance */
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS, 0x20);
}

VOID
NTAPI
EHCI_LockQH(IN PEHCI_EXTENSION EhciExtension,
            IN PEHCI_HCD_QH QueueHead,
            IN ULONG TransferType)
{
    PEHCI_HCD_QH PrevQH;
    ULONG QhPA;
    ULONG FrameIndexReg;
    PULONG OperationalRegs;
    ULONG Command;

    DPRINT_EHCI("EHCI_LockQH: QueueHead - %p, TransferType - %x\n",
                QueueHead,
                TransferType);

    OperationalRegs = EhciExtension->OperationalRegs;

    ASSERT((QueueHead->QhFlags & EHCI_QH_FLAG_UPDATING) == 0);
    ASSERT(EhciExtension->LockQH == NULL);

    PrevQH = QueueHead->PrevHead;
    QueueHead->QhFlags |= EHCI_QH_FLAG_UPDATING;

    ASSERT(PrevQH);

    EhciExtension->PrevQH = PrevQH;
    EhciExtension->NextQH = QueueHead->NextHead;
    EhciExtension->LockQH = QueueHead;

    if (QueueHead->NextHead)
    {
        QhPA = ((ULONG_PTR)QueueHead->NextHead->PhysicalAddress & ~0x1C) | 2;
    }
    else
    {
        QhPA = 1;
    }

    PrevQH->HwQH.HorizontalLink.AsULONG = QhPA;

    FrameIndexReg = READ_REGISTER_ULONG(OperationalRegs + EHCI_FRINDEX);

    if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        do
        {
            Command = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);
        }
        while (READ_REGISTER_ULONG(OperationalRegs + EHCI_FRINDEX) == 
               FrameIndexReg && (Command != -1) && (Command & 1));
    }
    else
    {
        EHCI_FlushAsyncCache(EhciExtension);
    }
}

VOID
NTAPI
EHCI_UnlockQH(IN PEHCI_EXTENSION EhciExtension,
              IN PEHCI_HCD_QH QueueHead)
{
    DPRINT_EHCI("EHCI_UnlockQH: QueueHead - %p\n", QueueHead);

    ASSERT((QueueHead->QhFlags & EHCI_QH_FLAG_UPDATING) != 0);
    ASSERT(EhciExtension->LockQH != 0);
    ASSERT(EhciExtension->LockQH == QueueHead);

    QueueHead->QhFlags &= ~EHCI_QH_FLAG_UPDATING;

    EhciExtension->LockQH = NULL;

    EhciExtension->PrevQH->HwQH.HorizontalLink.AsULONG =
        ((ULONG_PTR)QueueHead->PhysicalAddress & ~0x1C) | 2;
}

VOID
NTAPI
EHCI_LinkTransferToQueue(PEHCI_EXTENSION EhciExtension,
                         PEHCI_ENDPOINT EhciEndpoint,
                         PEHCI_HCD_TD NextTD)
{
    PEHCI_HCD_QH QH;
    PEHCI_HCD_TD HcdHeadP;
    PEHCI_HCD_TD TD;
    PEHCI_TRANSFER Transfer;
    PEHCI_HCD_TD LinkTD;
    BOOLEAN IsPresent;
    ULONG ix;

    DPRINT_EHCI("EHCI_LinkTransferToQueue: EhciEndpoint - %p, NextTD - %p\n",
                EhciEndpoint,
                NextTD);

    ASSERT(EhciEndpoint->HcdHeadP != NULL);
    IsPresent = EHCI_HardwarePresent(EhciExtension, 0);

    QH = EhciEndpoint->QH;
    HcdHeadP = EhciEndpoint->HcdHeadP;

    if (HcdHeadP == EhciEndpoint->HcdTailP)
    {
        if (IsPresent)
        {
            EHCI_LockQH(EhciExtension,
                        QH,
                        EhciEndpoint->EndpointProperties.TransferType);
        }

        QH->HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DummyTdPA;
        QH->HwQH.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
        QH->HwQH.AlternateNextTD = NextTD->HwTD.AlternateNextTD;

        QH->HwQH.Token.Status = (UCHAR)~(EHCI_TOKEN_STATUS_ACTIVE |
                                         EHCI_TOKEN_STATUS_HALTED);

        QH->HwQH.Token.TransferBytes = 0;

        if (IsPresent)
        {
            EHCI_UnlockQH(EhciExtension, QH);
        }

        EhciEndpoint->HcdHeadP = NextTD;
    }
    else
    {
        DbgBreakPoint();

        DPRINT("EHCI_LinkTransferToQueue: HcdHeadP - %p, DummyTd - %p\n",
               EhciEndpoint->HcdHeadP,
               EhciEndpoint->HcdTailP);

        LinkTD = EhciEndpoint->HcdHeadP;

        if (HcdHeadP != EhciEndpoint->HcdTailP)
        {
            while (TRUE)
            {
                LinkTD = HcdHeadP;
                TD = HcdHeadP->NextHcdTD;

                if (TD == EhciEndpoint->HcdTailP)
                {
                    break;
                }

                HcdHeadP = TD;
            }
        }

        ASSERT(LinkTD != EhciEndpoint->HcdTailP);

        Transfer = LinkTD->EhciTransfer;

        ix = 0;

        if (EhciEndpoint->MaxTDs)
        {
            TD = EhciEndpoint->FirstTD;

            do
            {
                if (TD->EhciTransfer == Transfer)
                {
                    TD->HwTD.AlternateNextTD = (ULONG_PTR)NextTD->PhysicalAddress;
                    TD->AltNextHcdTD = NextTD;

                }

                ++ix;
                TD += 1;
            }
            while (ix < EhciEndpoint->MaxTDs);
        }

        LinkTD->HwTD.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
        LinkTD->NextHcdTD = NextTD;

        if (QH->HwQH.CurrentTD == (ULONG_PTR)LinkTD->PhysicalAddress)
        {
            QH->HwQH.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
            QH->HwQH.AlternateNextTD = 1;
        }
    }
}

MPSTATUS
NTAPI
EHCI_ControlTransfer(IN PEHCI_EXTENSION EhciExtension,
                     IN PEHCI_ENDPOINT EhciEndpoint,
                     IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                     IN PEHCI_TRANSFER EhciTransfer,
                     IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PEHCI_HCD_TD FirstTD;
    PEHCI_HCD_TD LastTD;
    PEHCI_HCD_TD TD;
    PEHCI_HCD_TD PrevTD;
    PEHCI_HCD_TD LinkTD;
    ULONG TransferedLen = 0;
    EHCI_TD_TOKEN Token;
    ULONG DataToggle = 1;

    DPRINT_EHCI("EHCI_ControlTransfer: EhciEndpoint - %p, EhciTransfer - %p\n",
                EhciEndpoint,
                EhciTransfer);

    if (EhciEndpoint->RemainTDs < 6)
    {
        return 1;
    }

    ++EhciExtension->PendingTransfers;
    ++EhciEndpoint->PendingTDs;

    EhciTransfer->TransferOnAsyncList = 1;

    FirstTD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!FirstTD)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return 1;
    }

    ++EhciTransfer->PendingTDs;

    FirstTD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
    FirstTD->EhciTransfer = EhciTransfer;

    FirstTD->HwTD.Buffer[0] = 0;
    FirstTD->HwTD.Buffer[1] = 0;
    FirstTD->HwTD.Buffer[2] = 0;
    FirstTD->HwTD.Buffer[3] = 0;
    FirstTD->HwTD.Buffer[4] = 0;

    FirstTD->NextHcdTD = NULL;

    FirstTD->HwTD.NextTD = 1;
    FirstTD->HwTD.AlternateNextTD = 1;
    FirstTD->HwTD.Buffer[0] = (ULONG_PTR)&FirstTD->PhysicalAddress->SetupPacket;

    FirstTD->HwTD.Token.ErrorCounter = 3;
    FirstTD->HwTD.Token.PIDCode = 2;
    FirstTD->HwTD.Token.Status = EHCI_TOKEN_STATUS_ACTIVE;
    FirstTD->HwTD.Token.TransferBytes = 0x8;

    RtlCopyMemory(&FirstTD->SetupPacket,
                  &TransferParameters->SetupPacket,
                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    LastTD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!LastTD)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return 1;
    }

    ++EhciTransfer->PendingTDs;

    LastTD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
    LastTD->EhciTransfer = EhciTransfer;

    LastTD->HwTD.Buffer[0] = 0;
    LastTD->HwTD.Buffer[1] = 0;
    LastTD->HwTD.Buffer[2] = 0;
    LastTD->HwTD.Buffer[3] = 0;
    LastTD->HwTD.Buffer[4] = 0;

    LastTD->NextHcdTD = NULL;
    LastTD->HwTD.NextTD = 1;
    LastTD->HwTD.AlternateNextTD = 1;
    LastTD->HwTD.Token.ErrorCounter = 3;

    FirstTD->HwTD.AlternateNextTD = (ULONG_PTR)LastTD->PhysicalAddress;
    FirstTD->AltNextHcdTD = LastTD;

    PrevTD = FirstTD;
    LinkTD = FirstTD;

    if (TransferParameters->TransferBufferLength > 0)
    {
        while (TRUE)
        {
            TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

            if (!TD)
            {
                break;
            }

            LinkTD = TD;

            ++EhciTransfer->PendingTDs;

            TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
            TD->EhciTransfer = EhciTransfer;

            TD->HwTD.Buffer[0] = 0;
            TD->HwTD.Buffer[1] = 0;
            TD->HwTD.Buffer[2] = 0;
            TD->HwTD.Buffer[3] = 0;
            TD->HwTD.Buffer[4] = 0;

            TD->NextHcdTD = NULL;

            TD->HwTD.NextTD = 1;
            TD->HwTD.AlternateNextTD = 1;
            TD->HwTD.Token.ErrorCounter = 3;

            PrevTD->HwTD.NextTD = (ULONG_PTR)TD->PhysicalAddress;
            PrevTD->NextHcdTD = TD;

            if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            {
                TD->HwTD.Token.PIDCode = 1;
            }
            else
            {
                TD->HwTD.Token.PIDCode = 0;
            }

            TD->HwTD.Token.DataToggle = DataToggle;
            TD->HwTD.Token.Status = EHCI_TOKEN_STATUS_ACTIVE;

            if (DataToggle)
            {
                TD->HwTD.Token.DataToggle = 1;
            }
            else
            {
                TD->HwTD.Token.DataToggle = 0;
            }

            TD->HwTD.AlternateNextTD = (ULONG_PTR)LastTD->PhysicalAddress;
            TD->AltNextHcdTD = LastTD;

            TransferedLen = EHCI_MapAsyncTransferToTd(EhciExtension,
                                                      EhciEndpoint->EndpointProperties.MaxPacketSize,
                                                      TransferedLen,
                                                      &DataToggle,
                                                      EhciTransfer,
                                                      TD,
                                                      SgList);

            PrevTD = TD;

            if (TransferedLen >= TransferParameters->TransferBufferLength)
            {
                goto End;
            }
        }

        RegPacket.UsbPortBugCheck(EhciExtension);
        return 1;
    }

End:

    LinkTD->HwTD.NextTD = (ULONG_PTR)LastTD->PhysicalAddress;
    LinkTD->NextHcdTD = LastTD;

    LastTD->HwTD.Buffer[0] = 0;
    LastTD->LengthThisTD = 0;

    Token = LastTD->HwTD.Token;
    Token.Status = EHCI_TOKEN_STATUS_ACTIVE;
    Token.InterruptOnComplete = 1;
    Token.DataToggle = 1;

    if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        Token.PIDCode = 0;
    }
    else
    {
        Token.PIDCode = 1;
    }

    LastTD->HwTD.Token = Token;

    LastTD->HwTD.NextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    LastTD->NextHcdTD = EhciEndpoint->HcdTailP;

    EHCI_EnableAsyncList(EhciExtension);
    EHCI_LinkTransferToQueue(EhciExtension, EhciEndpoint, FirstTD);

    ASSERT(EhciEndpoint->HcdTailP->NextHcdTD == NULL); // dummyTD - > NextHcdTD
    ASSERT(EhciEndpoint->HcdTailP->AltNextHcdTD == NULL); // dummyTD - > AltNextHcdTD

    return 0;
}

MPSTATUS
NTAPI
EHCI_BulkTransfer(IN PEHCI_EXTENSION EhciExtension,
                  IN PEHCI_ENDPOINT EhciEndpoint,
                  IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                  IN PEHCI_TRANSFER EhciTransfer,
                  IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PEHCI_HCD_TD PrevTD;
    PEHCI_HCD_TD FirstTD;
    PEHCI_HCD_TD TD;
    ULONG TransferedLen;

    DPRINT_EHCI("EHCI_BulkTransfer: EhciEndpoint - %p, EhciTransfer - %p\n",
                EhciEndpoint,
                EhciTransfer);

    if (((TransferParameters->TransferBufferLength >> 14) + 1) > EhciEndpoint->RemainTDs)
    {
        DPRINT1("EHCI_BulkTransfer: return 1\n");
        return 1;
    }

    ++EhciExtension->PendingTransfers;
    ++EhciEndpoint->PendingTDs;

    EhciTransfer->TransferOnAsyncList = 1;

    TransferedLen = 0;
    PrevTD = NULL;

    if (TransferParameters->TransferBufferLength)
    {
        do
        {
            TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

            if (!TD)
            {
                RegPacket.UsbPortBugCheck(EhciExtension);
                return 1;
            }

            ++EhciTransfer->PendingTDs;

            TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
            TD->EhciTransfer = EhciTransfer;

            TD->HwTD.Buffer[0] = 0;
            TD->HwTD.Buffer[1] = 0;
            TD->HwTD.Buffer[2] = 0;
            TD->HwTD.Buffer[3] = 0;
            TD->HwTD.Buffer[4] = 0;

            TD->NextHcdTD = NULL;
            TD->HwTD.NextTD = 1;
            TD->HwTD.AlternateNextTD = 1;
            TD->HwTD.Token.ErrorCounter = 3;

            if (EhciTransfer->PendingTDs == 1)
            {
                FirstTD = TD;
            }
            else
            {
                PrevTD->HwTD.NextTD = (ULONG_PTR)TD->PhysicalAddress;
                PrevTD->NextHcdTD = TD;
            }

            TD->HwTD.AlternateNextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
            TD->AltNextHcdTD = EhciEndpoint->HcdTailP;

            TD->HwTD.Token.InterruptOnComplete = 1;

            if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            {
                TD->HwTD.Token.PIDCode = 1;
            }
            else
            {
                TD->HwTD.Token.PIDCode = 0;
            }

            TD->HwTD.Token.Status = EHCI_TOKEN_STATUS_ACTIVE;
            TD->HwTD.Token.DataToggle = 1;

            TransferedLen = EHCI_MapAsyncTransferToTd(EhciExtension,
                                                      EhciEndpoint->EndpointProperties.MaxPacketSize,
                                                      TransferedLen,
                                                      0,
                                                      EhciTransfer,
                                                      TD,
                                                      SgList);

            PrevTD = TD;
        }
        while (TransferedLen < TransferParameters->TransferBufferLength);
    }
    else
    {
        TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

        if (!TD)
        {
            RegPacket.UsbPortBugCheck(EhciExtension);
            return 1;
        }

        ++EhciTransfer->PendingTDs;

        TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
        TD->EhciTransfer = EhciTransfer;

        TD->HwTD.Buffer[0] = 0;
        TD->HwTD.Buffer[1] = 0;
        TD->HwTD.Buffer[2] = 0;
        TD->HwTD.Buffer[3] = 0;
        TD->HwTD.Buffer[4] = 0;

        TD->HwTD.NextTD = 1;
        TD->HwTD.AlternateNextTD = 1;
        TD->HwTD.Token.ErrorCounter = 3;
        TD->NextHcdTD = NULL;

        ASSERT(EhciTransfer->PendingTDs == 1);

        FirstTD = TD;

        TD->HwTD.AlternateNextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
        TD->AltNextHcdTD = EhciEndpoint->HcdTailP;

        TD->HwTD.Token.InterruptOnComplete = 1;

        if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
        {
            TD->HwTD.Token.PIDCode = 1;
        }
        else
        {
            TD->HwTD.Token.PIDCode = 0;
        }

        TD->HwTD.Buffer[0] = (ULONG_PTR)TD->PhysicalAddress;

        TD->HwTD.Token.Status = EHCI_TOKEN_STATUS_ACTIVE;
        TD->HwTD.Token.DataToggle = 1;

        TD->LengthThisTD = 0;
    }

    TD->HwTD.NextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    TD->NextHcdTD = EhciEndpoint->HcdTailP;

    EHCI_EnableAsyncList(EhciExtension);
    EHCI_LinkTransferToQueue(EhciExtension, EhciEndpoint, FirstTD);

    ASSERT(EhciEndpoint->HcdTailP->NextHcdTD == 0);
    ASSERT(EhciEndpoint->HcdTailP->AltNextHcdTD == 0);

    return 0;
}

MPSTATUS
NTAPI
EHCI_InterruptTransfer(IN PEHCI_EXTENSION EhciExtension,
                       IN PEHCI_ENDPOINT EhciEndpoint,
                       IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                       IN PEHCI_TRANSFER EhciTransfer,
                       IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    DPRINT1("EHCI_InterruptTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_SubmitTransfer(IN PVOID ehciExtension,
                    IN PVOID ehciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ehciTransfer,
                    IN PVOID sgList)
{
    PEHCI_EXTENSION EhciExtension;
    PEHCI_ENDPOINT EhciEndpoint;
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    PEHCI_TRANSFER EhciTransfer;
    PUSBPORT_SCATTER_GATHER_LIST SgList;
    MPSTATUS MPStatus;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    EhciEndpoint = (PEHCI_ENDPOINT)ehciEndpoint;
    TransferParameters = (PUSBPORT_TRANSFER_PARAMETERS)transferParameters;
    EhciTransfer = (PEHCI_TRANSFER)ehciTransfer;
    SgList = (PUSBPORT_SCATTER_GATHER_LIST)sgList;

    DPRINT_EHCI("EHCI_SubmitTransfer: EhciEndpoint - %p, EhciTransfer - %p\n",
                EhciEndpoint,
                EhciTransfer);

    RtlZeroMemory(EhciTransfer, sizeof(EHCI_TRANSFER));

    EhciTransfer->TransferParameters = TransferParameters;
    EhciTransfer->USBDStatus = USBD_STATUS_SUCCESS;
    EhciTransfer->EhciEndpoint = EhciEndpoint;

    switch (EhciEndpoint->EndpointProperties.TransferType)
    {
        case USBPORT_TRANSFER_TYPE_CONTROL:
            MPStatus = EHCI_ControlTransfer(EhciExtension,
                                            EhciEndpoint,
                                            TransferParameters,
                                            EhciTransfer,
                                            SgList);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            MPStatus = EHCI_BulkTransfer(EhciExtension,
                                         EhciEndpoint,
                                         TransferParameters,
                                         EhciTransfer,
                                         SgList);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            MPStatus = EHCI_InterruptTransfer(EhciExtension,
                                              EhciEndpoint,
                                              TransferParameters,
                                              EhciTransfer,
                                              SgList);
            break;

        default:
            DbgBreakPoint();
            MPStatus = 2;//?or?
            break;
    }

    return MPStatus;
}

MPSTATUS
NTAPI
EHCI_SubmitIsoTransfer(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint,
                       IN PVOID transferParameters,
                       IN PVOID ehciTransfer,
                       IN PVOID isoParameters)
{
    DPRINT1("EHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_AbortTransfer(IN PVOID ehciExtension,
                   IN PVOID ehciEndpoint,
                   IN PVOID ehciTransfer,
                   IN PULONG CompletedLength)
{
    DPRINT1("EHCI_AbortTransfer: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
EHCI_GetEndpointState(IN PVOID ehciExtension,
                      IN PVOID ehciEndpoint)
{
    DPRINT1("EHCI_GetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_RemoveQhFromPeriodicList(IN PEHCI_EXTENSION EhciExtension,
                              IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_RemoveQhFromPeriodicList: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_RemoveQhFromAsyncList(IN PEHCI_EXTENSION EhciExtension,
                           IN PEHCI_HCD_QH QH)
{
    PEHCI_HCD_QH NextHead;
    PEHCI_HCD_QH PrevHead;
    PEHCI_STATIC_QH AsyncHead;
    ULONG AsyncHeadPA;

    DPRINT_EHCI("EHCI_RemoveQhFromAsyncList: QH - %p\n", QH);

    if (QH->QhFlags & EHCI_QH_FLAG_IN_SCHEDULE)
    {
        NextHead = QH->NextHead;
        PrevHead = QH->PrevHead;

        AsyncHead = EhciExtension->AsyncHead;
        AsyncHeadPA = ((ULONG_PTR)AsyncHead->PhysicalAddress & ~0x1C) | 2;

        PrevHead->HwQH.HorizontalLink.AsULONG = 
            ((ULONG_PTR)NextHead->PhysicalAddress & ~0x1C) | 2;

        PrevHead->NextHead = NextHead;
        NextHead->PrevHead = PrevHead;

        EHCI_FlushAsyncCache(EhciExtension);

        if (READ_REGISTER_ULONG(EhciExtension->OperationalRegs + EHCI_ASYNCLISTBASE) ==
            (ULONG_PTR)QH->PhysicalAddress)
        {
            WRITE_REGISTER_ULONG((EhciExtension->OperationalRegs + EHCI_ASYNCLISTBASE),
                                 AsyncHeadPA);
        }

        QH->QhFlags &= ~EHCI_QH_FLAG_IN_SCHEDULE;
    }
}

VOID
NTAPI
EHCI_InsertQhInPeriodicList(IN PEHCI_EXTENSION EhciExtension,
                            IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_InsertQhInPeriodicList: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_InsertQhInAsyncList(IN PEHCI_EXTENSION EhciExtension,
                         IN PEHCI_HCD_QH QH)
{
    PEHCI_STATIC_QH AsyncHead;
    PEHCI_HCD_QH NextHead;

    DPRINT_EHCI("EHCI_InsertQhInAsyncList: QH - %p\n", QH);

    ASSERT((QH->QhFlags & EHCI_QH_FLAG_IN_SCHEDULE) == 0);
    ASSERT((QH->QhFlags & EHCI_QH_FLAG_NUKED) == 0);

    AsyncHead = EhciExtension->AsyncHead;
    NextHead = AsyncHead->NextHead;

    QH->HwQH.HorizontalLink = AsyncHead->HwQH.HorizontalLink;
    QH->QhFlags |= EHCI_QH_FLAG_IN_SCHEDULE;
    QH->NextHead = NextHead;
    QH->PrevHead = (PEHCI_HCD_QH)AsyncHead;

    NextHead->PrevHead = QH;

    AsyncHead->HwQH.HorizontalLink.AsULONG = ((ULONG_PTR)QH->PhysicalAddress & ~0x1C) | 2;
    AsyncHead->NextHead = QH;
}

VOID
NTAPI
EHCI_SetIsoEndpointState(IN PEHCI_EXTENSION EhciExtension,
                         IN PEHCI_ENDPOINT EhciEndpoint,
                         IN ULONG EndpointState)
{
    DPRINT1("EHCI_SetIsoEndpointState: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_SetAsyncEndpointState(IN PEHCI_EXTENSION EhciExtension,
                           IN PEHCI_ENDPOINT EhciEndpoint,
                           IN ULONG EndpointState)
{
    PEHCI_HCD_QH QH;
    ULONG TransferType;

    DPRINT_EHCI("EHCI_SetAsyncEndpointState: EhciEndpoint - %p, EndpointState - %p\n",
                EhciEndpoint,
                EndpointState);

    QH = EhciEndpoint->QH;

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    switch (EndpointState)
    {
        case USBPORT_ENDPOINT_PAUSED:
            if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
            {
                EHCI_RemoveQhFromPeriodicList(EhciExtension, EhciEndpoint);
            }
            else
            {
                EHCI_RemoveQhFromAsyncList(EhciExtension, EhciEndpoint->QH);
            }

            break;

        case USBPORT_ENDPOINT_ACTIVE:
            if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
            {
                EHCI_InsertQhInPeriodicList(EhciExtension, EhciEndpoint);
            }
            else
            {
                EHCI_InsertQhInAsyncList(EhciExtension, EhciEndpoint->QH);
            }

            break;

        case USBPORT_ENDPOINT_CLOSED:
            QH->QhFlags |= 2;

            if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
            {
                EHCI_RemoveQhFromPeriodicList(EhciExtension, EhciEndpoint);
            }
            else
            {
                EHCI_RemoveQhFromAsyncList(EhciExtension, EhciEndpoint->QH);
            }

            break;

        default:
            DbgBreakPoint();
            break;
    }

    EhciEndpoint->EndpointState = EndpointState;
}

VOID
NTAPI
EHCI_SetEndpointState(IN PVOID ehciExtension,
                      IN PVOID ehciEndpoint,
                      IN ULONG EndpointState)
{
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG TransferType;

    DPRINT_EHCI("EHCI_SetEndpointState: ... \n");

    EhciEndpoint = (PEHCI_ENDPOINT)ehciEndpoint;
    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
         EHCI_SetAsyncEndpointState((PEHCI_EXTENSION)ehciExtension,
                                    EhciEndpoint,
                                    EndpointState);
    }
    else if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        EHCI_SetIsoEndpointState((PEHCI_EXTENSION)ehciExtension,
                                 EhciEndpoint,
                                 EndpointState);
    }
    else
    {
        RegPacket.UsbPortBugCheck(ehciExtension);
    }
}

VOID
NTAPI
EHCI_InterruptNextSOF(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;

    DPRINT_EHCI("EHCI_InterruptNextSOF: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    RegPacket.UsbPortInvalidateController(EhciExtension, 3);
}

USBD_STATUS
NTAPI
EHCI_GetErrorFromTD(IN PEHCI_HCD_TD TD)
{
    EHCI_TD_TOKEN Token;

    DPRINT("EHCI_GetErrorFromTD: TD - %p\n", TD);

    ASSERT(TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_HALTED);

    Token = TD->HwTD.Token;

    if (Token.Status & EHCI_TOKEN_STATUS_TRANSACTION_ERROR)
    {
        return USBD_STATUS_XACT_ERROR;
    }

    if ( Token.Status & EHCI_TOKEN_STATUS_BABBLE_DETECTED)
    {
        return USBD_STATUS_BABBLE_DETECTED;
    }

    if ( Token.Status & EHCI_TOKEN_STATUS_DATA_BUFFER_ERROR)
    {
        return USBD_STATUS_DATA_BUFFER_ERROR;
    }

    if ( Token.Status & EHCI_TOKEN_STATUS_MISSED_MICROFRAME)
    {
        return USBD_STATUS_XACT_ERROR;
    }

    return USBD_STATUS_STALL_PID;
}

VOID
NTAPI
EHCI_ProcessDoneAsyncTd(IN PEHCI_EXTENSION EhciExtension,
                        IN PEHCI_HCD_TD TD)
{
    PEHCI_TRANSFER EhciTransfer;
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    ULONG TransferType;
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG LengthTransfered;
    USBD_STATUS USBDStatus;
    PULONG OperationalRegs;
    EHCI_USB_COMMAND Command;

    DPRINT_EHCI("EHCI_ProcessDoneAsyncTd: TD - %p\n", TD);

    EhciTransfer = TD->EhciTransfer;

    TransferParameters = EhciTransfer->TransferParameters;
    --EhciTransfer->PendingTDs;

    EhciEndpoint = EhciTransfer->EhciEndpoint;

    if (TD->TdFlags & 0x10)
    {
        goto Next;
    }

    if (TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_HALTED)
    {
       USBDStatus = EHCI_GetErrorFromTD(TD);
    }
    else
    {
        USBDStatus = USBD_STATUS_SUCCESS;
    }

    LengthTransfered = TD->LengthThisTD - TD->HwTD.Token.TransferBytes;

    if (TD->HwTD.Token.PIDCode != 2)
    {
        EhciTransfer->TransferLen += LengthTransfered;
    }

    if (USBDStatus != USBD_STATUS_SUCCESS)
    {
        EhciTransfer->USBDStatus = USBDStatus;
        goto Next;
    }

Next:

    TD->HwTD.NextTD = 0;
    TD->HwTD.AlternateNextTD = 0;

    TD->TdFlags = 0;
    TD->EhciTransfer = NULL;

    ++EhciEndpoint->RemainTDs;

    if (EhciTransfer->PendingTDs == 0)
    {
        --EhciEndpoint->PendingTDs;

        TransferType = EhciEndpoint->EndpointProperties.TransferType;

        if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
            TransferType == USBPORT_TRANSFER_TYPE_BULK)
        {
            --EhciExtension->PendingTransfers;

            if (EhciExtension->PendingTransfers == 0)
            {
                OperationalRegs = EhciExtension->OperationalRegs;
                Command.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBCMD);

                if (!Command.InterruptAdvanceDoorbell &&
                   (EhciExtension->Flags & 20))
                {
                    EHCI_DisableAsyncList(EhciExtension);
                }
            }
        }

        RegPacket.UsbPortCompleteTransfer(EhciExtension,
                                          EhciEndpoint,
                                          TransferParameters,
                                          EhciTransfer->USBDStatus,
                                          EhciTransfer->TransferLen);
    }
}

VOID
NTAPI
EHCI_PollActiveAsyncEndpoint(IN PEHCI_EXTENSION EhciExtension,
                             IN PEHCI_ENDPOINT EhciEndpoint)
{
    PEHCI_HCD_QH QH;
    PEHCI_HCD_TD TD;
    PEHCI_HCD_TD CurrentTD;
    ULONG CurrentTDPhys; 
    BOOLEAN IsSheduled;

    DPRINT_EHCI("EHCI_PollActiveAsyncEndpoint: ... \n");

    QH = EhciEndpoint->QH;

    CurrentTDPhys = QH->HwQH.CurrentTD & ~0x1F;
    ASSERT(CurrentTDPhys != 0);

    CurrentTD = (PEHCI_HCD_TD)RegPacket.UsbPortGetMappedVirtualAddress((PVOID)CurrentTDPhys,
                                                                       EhciExtension,
                                                                       EhciEndpoint);

    if (CurrentTD == EhciEndpoint->DummyTdVA)
    {
        return;
    }

    IsSheduled = QH->QhFlags & EHCI_QH_FLAG_IN_SCHEDULE;

    if (!EHCI_HardwarePresent(EhciExtension, 0))
    {
        IsSheduled = 0;
    }

    TD = EhciEndpoint->HcdHeadP;

    if (TD == CurrentTD)
    {
        if (TD == EhciEndpoint->HcdTailP ||
            TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE)
        {
            goto Next;
        }

        if (TD->NextHcdTD &&
            TD->HwTD.NextTD != (ULONG_PTR)TD->NextHcdTD->PhysicalAddress)
        {
            TD->HwTD.NextTD = (ULONG_PTR)TD->NextHcdTD->PhysicalAddress;
        }

        if (TD->AltNextHcdTD &&
            TD->HwTD.AlternateNextTD != (ULONG_PTR)TD->AltNextHcdTD->PhysicalAddress)
        {
            TD->HwTD.AlternateNextTD = (ULONG_PTR)TD->AltNextHcdTD->PhysicalAddress;
        }

        if (QH->HwQH.CurrentTD == (ULONG_PTR)TD->PhysicalAddress &&
            !(TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE) && 
            (QH->HwQH.NextTD != TD->HwTD.NextTD ||
             QH->HwQH.AlternateNextTD != TD->HwTD.AlternateNextTD))
        {
            QH->HwQH.NextTD = TD->HwTD.NextTD;
            QH->HwQH.AlternateNextTD = TD->HwTD.AlternateNextTD;
        }

        EHCI_InterruptNextSOF((PVOID)EhciExtension);
    }
    else
    {
        do
        {
            ASSERT((TD->TdFlags & EHCI_HCD_TD_FLAG_DUMMY) == 0);

            TD->TdFlags |= EHCI_HCD_TD_FLAG_DONE;

            if (TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE)
            {
                TD->TdFlags |= 0x10;
            }

            InsertTailList(&EhciEndpoint->ListTDs, &TD->DoneLink);
            TD = TD->NextHcdTD;
        }
        while (TD != CurrentTD);
    }

Next:

    if (CurrentTD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE)
    {
        ASSERT(TD != NULL);
        EhciEndpoint->HcdHeadP = TD;
        return;
    }

    if ((CurrentTD->NextHcdTD != EhciEndpoint->HcdTailP) &&
         (CurrentTD->AltNextHcdTD != EhciEndpoint->HcdTailP ||
          CurrentTD->HwTD.Token.TransferBytes == 0))
    {
        ASSERT(TD != NULL);
        EhciEndpoint->HcdHeadP = TD;
        return;
    }

    if (IsSheduled)
    {
        EHCI_LockQH(EhciExtension,
                    QH,
                    EhciEndpoint->EndpointProperties.TransferType);
    }

    QH->HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DummyTdPA;

    CurrentTD->TdFlags |= EHCI_HCD_TD_FLAG_DONE;
    InsertTailList(&EhciEndpoint->ListTDs, &CurrentTD->DoneLink);

    if (CurrentTD->HwTD.Token.TransferBytes &&
        CurrentTD->AltNextHcdTD == EhciEndpoint->HcdTailP)
    {
        TD = CurrentTD->NextHcdTD;
  
        if (TD != EhciEndpoint->HcdTailP)
        {
            do
            {
                TD->TdFlags |= 0x10;
                InsertTailList(&EhciEndpoint->ListTDs, &TD->DoneLink);
                TD = TD->NextHcdTD;
            }
            while (TD != EhciEndpoint->HcdTailP);
        }
    }

    QH->HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    QH->HwQH.NextTD = 1;
    QH->HwQH.AlternateNextTD = 1;
    QH->HwQH.Token.TransferBytes = 0;

    EhciEndpoint->HcdHeadP = EhciEndpoint->HcdTailP;

    if (IsSheduled)
    {
        EHCI_UnlockQH(EhciExtension, QH);
    }
}

VOID
NTAPI
EHCI_PollHaltedAsyncEndpoint(IN PEHCI_EXTENSION EhciExtension,
                             IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_PollHaltedAsyncEndpoint: ... \n");
}

VOID
NTAPI
EHCI_PollAsyncEndpoint(IN PEHCI_EXTENSION EhciExtension,
                       IN PEHCI_ENDPOINT EhciEndpoint)
{
    PEHCI_HCD_QH QH;
    PLIST_ENTRY DoneList;
    PEHCI_HCD_TD TD;

    //DPRINT_EHCI("EHCI_PollAsyncEndpoint: EhciEndpoint - %p\n", EhciEndpoint);

    if (!EhciEndpoint->PendingTDs)
    {
        return;
    }

    QH = EhciEndpoint->QH;

    if (QH->QhFlags & 2)
    {
        return;
    }

    if (QH->HwQH.Token.Status & EHCI_TOKEN_STATUS_ACTIVE ||
       !(QH->HwQH.Token.Status & EHCI_TOKEN_STATUS_HALTED))
    {
        EHCI_PollActiveAsyncEndpoint(EhciExtension, EhciEndpoint);
    }
    else
    {
        EhciEndpoint->EndpointStatus |= 1;
        EHCI_PollHaltedAsyncEndpoint(EhciExtension, EhciEndpoint);
    }

    DoneList = &EhciEndpoint->ListTDs;

    while (!IsListEmpty(DoneList))
    {
        TD = CONTAINING_RECORD(DoneList->Flink,
                               EHCI_HCD_TD,
                               DoneLink);

        RemoveHeadList(DoneList);

        ASSERT((TD->TdFlags & (EHCI_HCD_TD_FLAG_PROCESSED |
                               EHCI_HCD_TD_FLAG_DONE)) != 0);

        EHCI_ProcessDoneAsyncTd(EhciExtension, TD);
    }
}

VOID
NTAPI
EHCI_PollIsoEndpoint(IN PEHCI_EXTENSION EhciExtension,
                     IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_PollIsoEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_PollEndpoint(IN PVOID ehciExtension,
                  IN PVOID ehciEndpoint)
{
    PEHCI_EXTENSION EhciExtension;
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG TransferType;

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    EhciEndpoint = (PEHCI_ENDPOINT)ehciEndpoint;

    //DPRINT_EHCI("EHCI_PollEndpoint: EhciEndpoint - %p\n", EhciEndpoint);

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        EHCI_PollIsoEndpoint(EhciExtension, EhciEndpoint);
    }
    else
    {
        EHCI_PollAsyncEndpoint(EhciExtension, EhciEndpoint);
    }
}

VOID
NTAPI
EHCI_CheckController(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    //DPRINT_EHCI("EHCI_CheckController: ... \n");

    if (EhciExtension->IsStarted)
    {
        EHCI_HardwarePresent(EhciExtension, TRUE);
    }
}

ULONG
NTAPI
EHCI_Get32BitFrameNumber(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;
    ULONG FrameIdx; 
    ULONG FrameIndex;
    ULONG FrameNumber;

    //DPRINT_EHCI("EHCI_Get32BitFrameNumber: EhciExtension - %p\n", EhciExtension);

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    FrameIdx = EhciExtension->FrameIndex;
    FrameIndex = READ_REGISTER_ULONG(EhciExtension->OperationalRegs + EHCI_FRINDEX);

    FrameNumber = (((USHORT)FrameIdx ^ ((FrameIndex >> 3) & 0x7FF)) & 0x400) +
                           (FrameIndex | ((FrameIndex >> 3) & 0x3FF));

    return FrameNumber;
}

VOID
NTAPI
EHCI_EnableInterrupts(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    DPRINT("EHCI_EnableInterrupts: EhciExtension->InterruptMask - %x\n",
           EhciExtension->InterruptMask.AsULONG);

    WRITE_REGISTER_ULONG(EhciExtension->OperationalRegs + EHCI_USBINTR,
                         EhciExtension->InterruptMask.AsULONG);
}

VOID
NTAPI
EHCI_DisableInterrupts(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = (PEHCI_EXTENSION)ehciExtension;

    DPRINT("EHCI_DisableInterrupts: UNIMPLEMENTED. FIXME\n");

    WRITE_REGISTER_ULONG(EhciExtension->OperationalRegs + EHCI_USBINTR,
                         0);
}

VOID
NTAPI
EHCI_PollController(IN PVOID ehciExtension)
{
    DPRINT1("EHCI_PollController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_SetEndpointDataToggle(IN PVOID ehciExtension,
                           IN PVOID ehciEndpoint,
                           IN ULONG DataToggle)
{
    DPRINT1("EHCI_SetEndpointDataToggle: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
EHCI_GetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint)
{
    DPRINT1("EHCI_GetEndpointStatus: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_SetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint,
                       IN ULONG EndpointStatus)
{
    DPRINT1("EHCI_SetEndpointStatus: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_ResetController(IN PVOID ehciExtension)
{
    DPRINT1("EHCI_ResetController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
EHCI_StartSendOnePacket(IN PVOID ehciExtension,
                        IN PVOID PacketParameters,
                        IN PVOID Data,
                        IN PULONG pDataLength,
                        IN PVOID BufferVA,
                        IN PVOID BufferPA,
                        IN ULONG BufferLength,
                        IN USBD_STATUS * pUSBDStatus)
{
    DPRINT1("EHCI_StartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_EndSendOnePacket(IN PVOID ehciExtension,
                      IN PVOID PacketParameters,
                      IN PVOID Data,
                      IN PULONG pDataLength,
                      IN PVOID BufferVA,
                      IN PVOID BufferPA,
                      IN ULONG BufferLength,
                      IN USBD_STATUS * pUSBDStatus)
{
    DPRINT1("EHCI_EndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
EHCI_PassThru(IN PVOID ehciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT1("EHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_RebalanceEndpoint(IN PVOID ohciExtension,
                       IN PVOID endpointParameters,
                       IN PVOID ohciEndpoint)
{
    DPRINT1("EHCI_RebalanceEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_FlushInterrupts(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;
    PULONG OperationalRegs;
    EHCI_USB_STATUS Status;

    DPRINT("EHCI_FlushInterrupts: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    OperationalRegs = EhciExtension->OperationalRegs;

    Status.AsULONG = READ_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS);
    WRITE_REGISTER_ULONG(OperationalRegs + EHCI_USBSTS, Status.AsULONG);
}

MPSTATUS
NTAPI
EHCI_RH_ChirpRootPort(IN PVOID ehciExtension,
                      IN USHORT Port)
{
    DPRINT1("EHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
EHCI_TakePortControl(IN PVOID ohciExtension)
{
    DPRINT1("EHCI_TakePortControl: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_Unload()
{
    DPRINT1("EHCI_Unload: UNIMPLEMENTED. FIXME\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    DPRINT("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n",
           DriverObject,
           RegistryPath);

    if (USBPORT_GetHciMn() != 0x10000001)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));

    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_EHCI;

    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_INTERRUPT |
                              USB_MINIPORT_FLAGS_MEMORY_IO |
                              USB_MINIPORT_FLAGS_USB2 |
                              USB_MINIPORT_FLAGS_POLLING |
                              USB_MINIPORT_FLAGS_WAKE_SUPPORT;

    RegPacket.MiniPortBusBandwidth = 400000;

    RegPacket.MiniPortExtensionSize = sizeof(EHCI_EXTENSION);
    RegPacket.MiniPortEndpointSize = sizeof(EHCI_ENDPOINT);
    RegPacket.MiniPortTransferSize = sizeof(EHCI_TRANSFER);
    RegPacket.MiniPortResourcesSize = sizeof(EHCI_HC_RESOURCES);

    RegPacket.OpenEndpoint = EHCI_OpenEndpoint;
    RegPacket.ReopenEndpoint = EHCI_ReopenEndpoint;
    RegPacket.QueryEndpointRequirements = EHCI_QueryEndpointRequirements;
    RegPacket.CloseEndpoint = EHCI_CloseEndpoint;
    RegPacket.StartController = EHCI_StartController;
    RegPacket.StopController = EHCI_StopController;
    RegPacket.SuspendController = EHCI_SuspendController;
    RegPacket.ResumeController = EHCI_ResumeController;
    RegPacket.InterruptService = EHCI_InterruptService;
    RegPacket.InterruptDpc = EHCI_InterruptDpc;
    RegPacket.SubmitTransfer = EHCI_SubmitTransfer;
    RegPacket.SubmitIsoTransfer = EHCI_SubmitIsoTransfer;
    RegPacket.AbortTransfer = EHCI_AbortTransfer;
    RegPacket.GetEndpointState = EHCI_GetEndpointState;
    RegPacket.SetEndpointState = EHCI_SetEndpointState;
    RegPacket.PollEndpoint = EHCI_PollEndpoint;
    RegPacket.CheckController = EHCI_CheckController;
    RegPacket.Get32BitFrameNumber = EHCI_Get32BitFrameNumber;
    RegPacket.InterruptNextSOF = EHCI_InterruptNextSOF;
    RegPacket.EnableInterrupts = EHCI_EnableInterrupts;
    RegPacket.DisableInterrupts = EHCI_DisableInterrupts;
    RegPacket.PollController = EHCI_PollController;
    RegPacket.SetEndpointDataToggle = EHCI_SetEndpointDataToggle;
    RegPacket.GetEndpointStatus = EHCI_GetEndpointStatus;
    RegPacket.SetEndpointStatus = EHCI_SetEndpointStatus;
    RegPacket.RH_GetRootHubData = EHCI_RH_GetRootHubData;
    RegPacket.RH_GetStatus = EHCI_RH_GetStatus;
    RegPacket.RH_GetPortStatus = EHCI_RH_GetPortStatus;
    RegPacket.RH_GetHubStatus = EHCI_RH_GetHubStatus;
    RegPacket.RH_SetFeaturePortReset = EHCI_RH_SetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = EHCI_RH_SetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = EHCI_RH_SetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = EHCI_RH_SetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = EHCI_RH_ClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = EHCI_RH_ClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = EHCI_RH_ClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = EHCI_RH_ClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = EHCI_RH_ClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = EHCI_RH_ClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = EHCI_RH_ClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = EHCI_RH_ClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = EHCI_RH_DisableIrq;
    RegPacket.RH_EnableIrq = EHCI_RH_EnableIrq;
    RegPacket.StartSendOnePacket = EHCI_StartSendOnePacket;
    RegPacket.EndSendOnePacket = EHCI_EndSendOnePacket;
    RegPacket.PassThru = EHCI_PassThru;
    RegPacket.RebalanceEndpoint = EHCI_RebalanceEndpoint;
    RegPacket.FlushInterrupts = EHCI_FlushInterrupts;
    RegPacket.RH_ChirpRootPort = EHCI_RH_ChirpRootPort;
    RegPacket.TakePortControl = EHCI_TakePortControl;

    DriverObject->DriverUnload = (PDRIVER_UNLOAD)EHCI_Unload;

    return USBPORT_RegisterUSBPortDriver(DriverObject, 200, &RegPacket);
}
