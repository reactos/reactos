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

MPSTATUS
NTAPI
EHCI_SubmitTransfer(IN PVOID ehciExtension,
                    IN PVOID ehciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ehciTransfer,
                    IN PVOID sgList)
{
    DPRINT1("EHCI_SubmitTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
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
EHCI_SetEndpointState(IN PVOID ehciExtension,
                      IN PVOID ehciEndpoint,
                      IN ULONG EndpointState)
{
    DPRINT1("EHCI_SetEndpointState: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_PollEndpoint(IN PVOID ohciExtension,
                  IN PVOID ohciEndpoint)
{
    DPRINT1("EHCI_PollEndpoint: UNIMPLEMENTED. FIXME\n");
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
EHCI_InterruptNextSOF(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension;

    DPRINT_EHCI("EHCI_InterruptNextSOF: ... \n");

    EhciExtension = (PEHCI_EXTENSION)ehciExtension;
    RegPacket.UsbPortInvalidateController(EhciExtension, 3);
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
