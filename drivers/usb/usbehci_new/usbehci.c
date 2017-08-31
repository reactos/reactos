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
    ULONG ix;

    DPRINT_EHCI("EHCI_AllocTd: ... \n");

    if (EhciEndpoint->MaxTDs == 0)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return NULL;
    }

    TD = EhciEndpoint->FirstTD;

    for (ix = 1; TD->TdFlags & EHCI_HCD_TD_FLAG_ALLOCATED; ix++)
    {
        TD += 1;

        if (ix >= EhciEndpoint->MaxTDs)
        {
            RegPacket.UsbPortBugCheck(EhciExtension);
            return NULL;
        }
    }

    TD->TdFlags |= EHCI_HCD_TD_FLAG_ALLOCATED;

    EhciEndpoint->RemainTDs--;

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

    ASSERT(((ULONG_PTR)QhPA & ~LINK_POINTER_MASK) == 0); // link flags

    QH->sqh.PhysicalAddress = QhPA;
    //QH->EhciEndpoint = EhciEndpoint;

    QH->sqh.HwQH.EndpointParams.DeviceAddress = EndpointProperties->DeviceAddress & USBPORT_MAX_DEVICE_ADDRESS;
    QH->sqh.HwQH.EndpointParams.EndpointNumber = EndpointProperties->EndpointAddress & 0xF;//USB_ENDPOINT_ADDRESS_MASK

    DeviceSpeed = EndpointProperties->DeviceSpeed;

    switch (DeviceSpeed)
    {
        case UsbLowSpeed:
            QH->sqh.HwQH.EndpointParams.EndpointSpeed = 1;
            break;

        case UsbFullSpeed:
            QH->sqh.HwQH.EndpointParams.EndpointSpeed = 0;
            break;

        case UsbHighSpeed:
            QH->sqh.HwQH.EndpointParams.EndpointSpeed = 2;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    QH->sqh.HwQH.EndpointParams.MaximumPacketLength = EndpointProperties->MaxPacketSize;

    QH->sqh.HwQH.EndpointCaps.PipeMultiplier = 1;

    if (DeviceSpeed == UsbHighSpeed)
    {
        QH->sqh.HwQH.EndpointCaps.HubAddr = 0;
        QH->sqh.HwQH.EndpointCaps.PortNumber = 0;
    }
    else
    {
        QH->sqh.HwQH.EndpointCaps.HubAddr = EndpointProperties->HubAddr & 0x7F;
        QH->sqh.HwQH.EndpointCaps.PortNumber = EndpointProperties->PortNumber & 0x7F;

        if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
        {
            QH->sqh.HwQH.EndpointParams.ControlEndpointFlag = 1;
        }
    }

    QH->sqh.HwQH.NextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;

    QH->sqh.HwQH.Token.Status &= (UCHAR)~(EHCI_TOKEN_STATUS_ACTIVE |
                                          EHCI_TOKEN_STATUS_HALTED);

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

    EhciEndpoint->DmaBufferVA = (PVOID)EndpointProperties->BufferVA;
    EhciEndpoint->DmaBufferPA = (PVOID)EndpointProperties->BufferPA;

    RtlZeroMemory(EhciEndpoint->DmaBufferVA, sizeof(EHCI_HCD_TD));

    QH = (PEHCI_HCD_QH)EhciEndpoint->DmaBufferVA + 1;
    QhPA = (PEHCI_HCD_QH)EhciEndpoint->DmaBufferPA + 1;

    EhciEndpoint->FirstTD = (PEHCI_HCD_TD)(QH + 1);

    TdCount = (EndpointProperties->BufferLength -
               (sizeof(EHCI_HCD_TD) + sizeof(EHCI_HCD_QH))) /
               sizeof(EHCI_HCD_TD);

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        EhciEndpoint->EndpointStatus |= USBPORT_ENDPOINT_CONTROL;
    }

    EhciEndpoint->MaxTDs = TdCount;
    EhciEndpoint->RemainTDs = TdCount;

    TdVA = EhciEndpoint->FirstTD;
    TdPA = (PEHCI_HCD_TD)(QhPA + 1);

    for (ix = 0; ix < TdCount; ix++)
    {
        DPRINT_EHCI("EHCI_OpenBulkOrControlEndpoint: TdVA - %p, TdPA - %p\n",
                    TdVA,
                    TdPA);

        RtlZeroMemory(TdVA, sizeof(EHCI_HCD_TD));

        ASSERT(((ULONG_PTR)TdPA & ~LINK_POINTER_MASK) == 0); // link flags

        TdVA->PhysicalAddress = TdPA;
        TdVA->EhciEndpoint = EhciEndpoint;
        TdVA->EhciTransfer = NULL;

        TdPA += 1;
        TdVA += 1;
    }

    EhciEndpoint->QH = EHCI_InitializeQH(EhciExtension,
                                         EhciEndpoint,
                                         QH,
                                         QhPA);

    if (IsControl)
    {
        QH->sqh.HwQH.EndpointParams.DataToggleControl = TRUE;
        EhciEndpoint->HcdHeadP = NULL;
    }
    else
    {
        QH->sqh.HwQH.EndpointParams.DataToggleControl = FALSE;
    }

    TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!TD)
    {
        return MP_STATUS_NO_RESOURCES;
    }

    TD->TdFlags |= EHCI_HCD_TD_FLAG_DUMMY;
    TD->HwTD.Token.Status &= (UCHAR)~EHCI_TOKEN_STATUS_ACTIVE;

    TD->HwTD.NextTD = TERMINATE_POINTER;
    TD->HwTD.AlternateNextTD = TERMINATE_POINTER;

    TD->NextHcdTD = NULL;
    TD->AltNextHcdTD = NULL;

    EhciEndpoint->HcdTailP = TD;
    EhciEndpoint->HcdHeadP = TD;

    QH->sqh.HwQH.CurrentTD = (ULONG_PTR)TD->PhysicalAddress;
    QH->sqh.HwQH.NextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;

    QH->sqh.HwQH.Token.Status &= (UCHAR)~EHCI_TOKEN_STATUS_ACTIVE;
    QH->sqh.HwQH.Token.TransferBytes = 0;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_OpenInterruptEndpoint(IN PEHCI_EXTENSION EhciExtension,
                           IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                           IN PEHCI_ENDPOINT EhciEndpoint)
{
    PEHCI_HCD_QH QH;
    ULONG_PTR QhPA;
    PEHCI_HCD_TD FirstTD;
    ULONG_PTR FirstTdPA;
    PEHCI_HCD_TD TD;
    PEHCI_HCD_TD DummyTD;
    ULONG TdCount;
    ULONG ix;
    PEHCI_PERIOD PeriodTable = NULL;
    ULONG ScheduleOffset;
    ULONG Idx = 0;
    UCHAR Period;
    static UCHAR ClassicPeriod[8] = {
        ENDPOINT_INTERRUPT_1ms - 1,
        ENDPOINT_INTERRUPT_2ms - 1,
        ENDPOINT_INTERRUPT_4ms - 1,
        ENDPOINT_INTERRUPT_8ms - 1,
        ENDPOINT_INTERRUPT_16ms - 1,
        ENDPOINT_INTERRUPT_32ms - 1,
        ENDPOINT_INTERRUPT_32ms - 1,
        ENDPOINT_INTERRUPT_32ms - 1
    };
    static EHCI_PERIOD pTable[INTERRUPT_ENDPOINTs + 1] = {
        { ENDPOINT_INTERRUPT_1ms, 0x00, 0xFF },
        { ENDPOINT_INTERRUPT_2ms, 0x00, 0x55 },
        { ENDPOINT_INTERRUPT_2ms, 0x00, 0xAA },
        { ENDPOINT_INTERRUPT_4ms, 0x00, 0x11 },
        { ENDPOINT_INTERRUPT_4ms, 0x00, 0x44 },
        { ENDPOINT_INTERRUPT_4ms, 0x00, 0x22 },
        { ENDPOINT_INTERRUPT_4ms, 0x00, 0x88 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x01 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x10 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x04 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x40 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x02 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x20 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x08 },
        { ENDPOINT_INTERRUPT_8ms, 0x00, 0x80 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x01 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x01 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x10 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x10 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x04 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x04 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x40 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x40 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x02 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x02 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x20 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x20 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x08 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x08 },
        { ENDPOINT_INTERRUPT_16ms, 0x01, 0x80 },
        { ENDPOINT_INTERRUPT_16ms, 0x02, 0x80 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x01 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x01 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x01 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x01 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x10 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x10 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x10 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x10 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x04 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x04 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x04 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x04 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x40 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x40 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x40 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x40 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x02 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x02 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x02 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x02 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x20 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x20 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x20 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x20 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x08 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x08 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x08 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x08 },
        { ENDPOINT_INTERRUPT_32ms, 0x03, 0x80 },
        { ENDPOINT_INTERRUPT_32ms, 0x05, 0x80 },
        { ENDPOINT_INTERRUPT_32ms, 0x04, 0x80 },
        { ENDPOINT_INTERRUPT_32ms, 0x06, 0x80 },
        { 0x00, 0x00, 0x00 }
    };

    DPRINT("EHCI_OpenInterruptEndpoint: EhciExtension - %p, EndpointProperties - %p, EhciEndpoint - %p\n",
           EhciExtension,
           EndpointProperties,
           EhciEndpoint);

    Period = EndpointProperties->Period;
    ScheduleOffset = EndpointProperties->ScheduleOffset;

    ASSERT(Period < (INTERRUPT_ENDPOINTs + 1));

    while (!(Period & 1))
    {
        Idx++;
        Period >>= 1;
    }

    ASSERT(Idx < 8);

    InitializeListHead(&EhciEndpoint->ListTDs);

    if (EhciEndpoint->EndpointProperties.DeviceSpeed == UsbHighSpeed)
    {
        PeriodTable = &pTable[ClassicPeriod[Idx] + ScheduleOffset];
        EhciEndpoint->PeriodTable = PeriodTable;

        DPRINT("EHCI_OpenInterruptEndpoint: EhciEndpoint - %p, ScheduleMask - %X, Index - %X\n",
               EhciEndpoint,
               PeriodTable->ScheduleMask,
               ClassicPeriod[Idx]);

        EhciEndpoint->StaticQH = EhciExtension->PeriodicHead[PeriodTable->PeriodIdx];
    }
    else
    {
        EhciEndpoint->PeriodTable = NULL;

        DPRINT("EHCI_OpenInterruptEndpoint: EhciEndpoint - %p, Index - %X\n",
               EhciEndpoint,
               ClassicPeriod[Idx]);

        EhciEndpoint->StaticQH = EhciExtension->PeriodicHead[ClassicPeriod[Idx] +
                                                             ScheduleOffset];
    }

    EhciEndpoint->DmaBufferVA = (PVOID)EndpointProperties->BufferVA;
    EhciEndpoint->DmaBufferPA = (PVOID)EndpointProperties->BufferPA;

    RtlZeroMemory((PVOID)EndpointProperties->BufferVA, sizeof(EHCI_HCD_TD));

    QH = (PEHCI_HCD_QH)(EndpointProperties->BufferVA + sizeof(EHCI_HCD_TD));
    QhPA = EndpointProperties->BufferPA + sizeof(EHCI_HCD_TD);

    FirstTD = (PEHCI_HCD_TD)(EndpointProperties->BufferVA +
                             sizeof(EHCI_HCD_TD) +
                             sizeof(EHCI_HCD_QH));

    FirstTdPA = EndpointProperties->BufferPA +
                sizeof(EHCI_HCD_TD) +
                sizeof(EHCI_HCD_QH);

    TdCount = (EndpointProperties->BufferLength -
               (sizeof(EHCI_HCD_TD) + sizeof(EHCI_HCD_QH))) /
               sizeof(EHCI_HCD_TD);

    ASSERT(TdCount >= EHCI_MAX_INTERRUPT_TD_COUNT + 1);

    EhciEndpoint->FirstTD = FirstTD;
    EhciEndpoint->MaxTDs = TdCount;

    for (ix = 0; ix < TdCount; ix++)
    {
        TD = EhciEndpoint->FirstTD + ix;

        RtlZeroMemory(TD, sizeof(EHCI_HCD_TD));

        ASSERT((FirstTdPA & ~LINK_POINTER_MASK) == 0);

        TD->PhysicalAddress = (PEHCI_HCD_TD)FirstTdPA;
        TD->EhciEndpoint = EhciEndpoint;
        TD->EhciTransfer = NULL;

        FirstTdPA += sizeof(EHCI_HCD_TD);
    }

    EhciEndpoint->RemainTDs = TdCount;

    EhciEndpoint->QH = EHCI_InitializeQH(EhciExtension,
                                         EhciEndpoint,
                                         QH,
                                         (PEHCI_HCD_QH)QhPA);

    if (EhciEndpoint->EndpointProperties.DeviceSpeed == UsbHighSpeed)
    {
        QH->sqh.HwQH.EndpointCaps.InterruptMask = PeriodTable->ScheduleMask;
    }
    else
    {
        QH->sqh.HwQH.EndpointCaps.InterruptMask =
        EndpointProperties->InterruptScheduleMask;

        QH->sqh.HwQH.EndpointCaps.SplitCompletionMask =
        EndpointProperties->SplitCompletionMask;
    }

    DummyTD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    DummyTD->TdFlags |= EHCI_HCD_TD_FLAG_DUMMY;
    DummyTD->NextHcdTD = NULL;
    DummyTD->AltNextHcdTD = NULL;

    DummyTD->HwTD.Token.Status &= ~EHCI_TOKEN_STATUS_ACTIVE;

    DummyTD->HwTD.NextTD = TERMINATE_POINTER;
    DummyTD->HwTD.AlternateNextTD = TERMINATE_POINTER;

    EhciEndpoint->HcdTailP = DummyTD;
    EhciEndpoint->HcdHeadP = DummyTD;

    QH->sqh.HwQH.CurrentTD = (ULONG_PTR)DummyTD->PhysicalAddress;
    QH->sqh.HwQH.NextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;

    QH->sqh.HwQH.Token.Status &= ~EHCI_TOKEN_STATUS_ACTIVE;
    QH->sqh.HwQH.Token.TransferBytes = 0;

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_OpenHsIsoEndpoint(IN PEHCI_EXTENSION EhciExtension,
                       IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                       IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_OpenHsIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_NOT_SUPPORTED;
}

MPSTATUS
NTAPI
EHCI_OpenIsoEndpoint(IN PEHCI_EXTENSION EhciExtension,
                     IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                     IN PEHCI_ENDPOINT EhciEndpoint)
{
    DPRINT1("EHCI_OpenIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_NOT_SUPPORTED;
}

MPSTATUS
NTAPI
EHCI_OpenEndpoint(IN PVOID ehciExtension,
                  IN PVOID endpointParameters,
                  IN PVOID ehciEndpoint)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_ENDPOINT EhciEndpoint = ehciEndpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties = endpointParameters;
    ULONG TransferType;
    MPSTATUS MPStatus;

    DPRINT_EHCI("EHCI_OpenEndpoint: ... \n");

    RtlCopyMemory(&EhciEndpoint->EndpointProperties,
                  endpointParameters,
                  sizeof(USBPORT_ENDPOINT_PROPERTIES));

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
            {
                MPStatus = EHCI_OpenHsIsoEndpoint(EhciExtension,
                                                  EndpointProperties,
                                                  EhciEndpoint);
            }
            else
            {
                MPStatus = EHCI_OpenIsoEndpoint(EhciExtension,
                                                EndpointProperties,
                                                EhciEndpoint);
            }

            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            MPStatus = EHCI_OpenBulkOrControlEndpoint(EhciExtension,
                                                      EndpointProperties,
                                                      EhciEndpoint,
                                                      TRUE);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            MPStatus = EHCI_OpenBulkOrControlEndpoint(EhciExtension,
                                                      EndpointProperties,
                                                      EhciEndpoint,
                                                      FALSE);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            MPStatus = EHCI_OpenInterruptEndpoint(EhciExtension,
                                                  EndpointProperties,
                                                  EhciEndpoint);
            break;

        default:
            MPStatus = MP_STATUS_NOT_SUPPORTED;
            break;
    }

    return MPStatus;
}

MPSTATUS
NTAPI
EHCI_ReopenEndpoint(IN PVOID ehciExtension,
                    IN PVOID endpointParameters,
                    IN PVOID ehciEndpoint)
{
    PEHCI_ENDPOINT EhciEndpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;
    PEHCI_HCD_QH QH;
    MPSTATUS MPStatus;

    EhciEndpoint = ehciEndpoint;
    EndpointProperties = endpointParameters;

    TransferType = EndpointProperties->TransferType;

    DPRINT("EHCI_ReopenEndpoint: EhciEndpoint - %p, TransferType - %x\n",
           EhciEndpoint,
           TransferType);

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
            {
                DPRINT1("EHCI_ReopenEndpoint: HS Iso. UNIMPLEMENTED. FIXME\n");
                MPStatus = MP_STATUS_NOT_SUPPORTED;
            }
            else
            {
                DPRINT1("EHCI_ReopenEndpoint: Iso. UNIMPLEMENTED. FIXME\n");
                MPStatus = MP_STATUS_NOT_SUPPORTED;
            }

            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
        case USBPORT_TRANSFER_TYPE_BULK:
        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            RtlCopyMemory(&EhciEndpoint->EndpointProperties,
                          endpointParameters,
                          sizeof(USBPORT_ENDPOINT_PROPERTIES));

            QH = EhciEndpoint->QH;

            QH->sqh.HwQH.EndpointParams.DeviceAddress = EndpointProperties->DeviceAddress & USBPORT_MAX_DEVICE_ADDRESS;
            QH->sqh.HwQH.EndpointParams.MaximumPacketLength = EndpointProperties->MaxPacketSize;

            QH->sqh.HwQH.EndpointCaps.HubAddr = EndpointProperties->HubAddr & USBPORT_MAX_DEVICE_ADDRESS;

            break;

        default:
            DPRINT1("EHCI_ReopenEndpoint: Unknown TransferType\n");
            MPStatus = MP_STATUS_SUCCESS;
            break;
    }

    return MPStatus;
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

    EndpointProperties = endpointParameters;
    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT("EHCI_QueryEndpointRequirements: IsoTransfer\n");

            if (EndpointProperties->DeviceSpeed == UsbHighSpeed)
            {
                EndpointRequirements[0] = 0x40000; // FIXME Isoch
                EndpointRequirements[1] = EHCI_MAX_HS_ISO_TRANSFER_SIZE;
            }
            else
            {
                EndpointRequirements[0] = 0x1000; // FIXME Isoch
                EndpointRequirements[1] = EHCI_MAX_FS_ISO_TRANSFER_SIZE;
            }
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT("EHCI_QueryEndpointRequirements: ControlTransfer\n");
            EndpointRequirements[0] = sizeof(EHCI_HCD_TD) +
                                      sizeof(EHCI_HCD_QH) +
                                      EHCI_MAX_CONTROL_TD_COUNT * sizeof(EHCI_HCD_TD);

            EndpointRequirements[1] = EHCI_MAX_CONTROL_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT("EHCI_QueryEndpointRequirements: BulkTransfer\n");
            EndpointRequirements[0] = sizeof(EHCI_HCD_TD) +
                                      sizeof(EHCI_HCD_QH) +
                                      EHCI_MAX_BULK_TD_COUNT * sizeof(EHCI_HCD_TD);

            EndpointRequirements[1] = EHCI_MAX_BULK_TRANSFER_SIZE;
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT("EHCI_QueryEndpointRequirements: InterruptTransfer\n");
            EndpointRequirements[0] = sizeof(EHCI_HCD_TD) +
                                      sizeof(EHCI_HCD_QH) +
                                      EHCI_MAX_INTERRUPT_TD_COUNT * sizeof(EHCI_HCD_TD);

            EndpointRequirements[1] = EHCI_MAX_INTERRUPT_TRANSFER_SIZE;
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

    //DPRINT_EHCI("EHCI_GetQhForFrame: FrameIdx - %x, Balance[FrameIdx] - %x\n",
    //            FrameIdx,
    //            Balance[FrameIdx & 0x1F]);

    return EhciExtension->PeriodicHead[Balance[FrameIdx & 0x1F]];
}

PEHCI_HCD_QH
NTAPI
EHCI_GetDummyQhForFrame(IN PEHCI_EXTENSION EhciExtension,
                        IN ULONG Idx)
{
    return (PEHCI_HCD_QH)(EhciExtension->IsoDummyQHListVA + Idx * sizeof(EHCI_HCD_QH));
}

VOID
NTAPI
EHCI_AlignHwStructure(IN PEHCI_EXTENSION EhciExtension,
                      IN PULONG PhysicalAddress,
                      IN PULONG VirtualAddress,
                      IN ULONG Alignment)
{
    ULONG_PTR PAddress;
    PVOID NewPAddress;
    ULONG_PTR VAddress;

    //DPRINT_EHCI("EHCI_AlignHwStructure: *PhysicalAddress - %p, *VirtualAddress - %p, Alignment - %x\n",
    //             *PhysicalAddress,
    //             *VirtualAddress,
    //             Alignment);

    PAddress = *PhysicalAddress;
    VAddress = *VirtualAddress;

    NewPAddress = PAGE_ALIGN(*PhysicalAddress + Alignment - 1);

    if (NewPAddress != PAGE_ALIGN(*PhysicalAddress))
    {
        VAddress += (ULONG_PTR)NewPAddress - PAddress;
        PAddress = (ULONG_PTR)PAGE_ALIGN(*PhysicalAddress + Alignment - 1);

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
    ULONG ix;

    DPRINT_EHCI("EHCI_AddDummyQHs: ... \n");

    FrameHeader = &EhciExtension->HcResourcesVA->PeriodicFrameList[0];

    DummyQhVA = (PEHCI_HCD_QH)EhciExtension->IsoDummyQHListVA;
    DummyQhPA = (PEHCI_HCD_QH)EhciExtension->IsoDummyQHListPA;

    for (ix = 0; ix < EHCI_FRAME_LIST_MAX_ENTRIES; ix++)
    {
        ASSERT(EHCI_GetDummyQhForFrame(EhciExtension, ix) == DummyQhVA);

        RtlZeroMemory(DummyQhVA, sizeof(EHCI_HCD_QH));

        DummyQhVA->sqh.HwQH.CurrentTD = 0;
        DummyQhVA->sqh.HwQH.Token.Status &= (UCHAR)~EHCI_TOKEN_STATUS_ACTIVE;

        PrevPA.AsULONG = (ULONG)DummyQhVA->sqh.PhysicalAddress;
        PrevPA.Type = 1;

        DummyQhVA->sqh.PhysicalAddress = DummyQhPA;

        DummyQhVA += 1;
        DummyQhPA += 1;

        DummyQhVA->sqh.HwQH.HorizontalLink.AsULONG = (ULONG_PTR)(*FrameHeader);

        EndpointParams = DummyQhVA->sqh.HwQH.EndpointParams;
        EndpointParams.DeviceAddress = 0;
        EndpointParams.EndpointNumber = 0;
        EndpointParams.EndpointSpeed = 0;
        EndpointParams.MaximumPacketLength = 64;

        DummyQhVA->sqh.HwQH.EndpointParams = EndpointParams;

        DummyQhVA->sqh.HwQH.EndpointCaps.AsULONG = 0;
        DummyQhVA->sqh.HwQH.EndpointCaps.InterruptMask = 0;
        DummyQhVA->sqh.HwQH.EndpointCaps.SplitCompletionMask = 0;
        DummyQhVA->sqh.HwQH.EndpointCaps.PipeMultiplier = 1;

        DummyQhVA->sqh.HwQH.NextTD = TERMINATE_POINTER;
        DummyQhVA->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;

        StaticQH = EHCI_GetQhForFrame(EhciExtension, ix);
        DummyQhVA->sqh.StaticQH = StaticQH;

        *FrameHeader = (PEHCI_STATIC_QH)PrevPA.AsULONG;

        FrameHeader += 1;
    }
}

VOID
NTAPI
EHCI_InitializeInterruptSchedule(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_STATIC_QH StaticQH;
    ULONG ix;
    static UCHAR LinkTable[INTERRUPT_ENDPOINTs + 1] = {
      255, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,  9, 9,
      10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19,
      20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
      30, 30, 0}; 

    DPRINT_EHCI("EHCI_InitializeInterruptSchedule: ... \n");

    for (ix = 0; ix < INTERRUPT_ENDPOINTs; ix++)
    {
        StaticQH = EhciExtension->PeriodicHead[ix];

        StaticQH->HwQH.EndpointParams.HeadReclamationListFlag = 0;
        StaticQH->HwQH.NextTD |= TERMINATE_POINTER;
        StaticQH->HwQH.Token.Status |= (UCHAR)EHCI_TOKEN_STATUS_HALTED;
    }

    for (ix = 1; ix < INTERRUPT_ENDPOINTs; ix++)
    {
        StaticQH = EhciExtension->PeriodicHead[ix];
 
        StaticQH->PrevHead = NULL;
        StaticQH->NextHead = (PEHCI_HCD_QH)EhciExtension->PeriodicHead[LinkTable[ix]];

        StaticQH->HwQH.HorizontalLink.AsULONG = 
            (ULONG)EhciExtension->PeriodicHead[LinkTable[ix]]->PhysicalAddress;

        StaticQH->HwQH.HorizontalLink.Type = EHCI_LINK_TYPE_QH;
        StaticQH->HwQH.EndpointCaps.InterruptMask = 0xFF;

        StaticQH->QhFlags |= EHCI_QH_FLAG_STATIC;
        StaticQH->QhFlags |= ix > 6 ? 0 : EHCI_QH_FLAG_STATIC_FAST;
    }

    EhciExtension->PeriodicHead[0]->HwQH.HorizontalLink.Terminate = 1;

    EhciExtension->PeriodicHead[0]->QhFlags |= (EHCI_QH_FLAG_STATIC |
                                                EHCI_QH_FLAG_STATIC_FAST);
}

MPSTATUS
NTAPI
EHCI_InitializeSchedule(IN PEHCI_EXTENSION EhciExtension,
                        IN PVOID resourcesStartVA,
                        IN PVOID resourcesStartPA)
{
    PEHCI_HW_REGISTERS OperationalRegs;
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

    NextLink.AsULONG = (ULONG_PTR)AsyncHeadPA;
    NextLink.Type = EHCI_LINK_TYPE_QH;

    AsyncHead->HwQH.HorizontalLink = NextLink;
    AsyncHead->HwQH.EndpointParams.HeadReclamationListFlag = 1;
    AsyncHead->HwQH.EndpointCaps.PipeMultiplier = 1;
    AsyncHead->HwQH.NextTD |= TERMINATE_POINTER;
    AsyncHead->HwQH.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_HALTED;

    AsyncHead->PhysicalAddress = (PEHCI_HCD_QH)AsyncHeadPA;
    AsyncHead->PrevHead = AsyncHead->NextHead = (PEHCI_HCD_QH)AsyncHead;

    EhciExtension->AsyncHead = AsyncHead;

    /* Periodic Schedule */

    PeriodicHead = &HcResourcesVA->PeriodicHead[0];
    PeriodicHeadPA = &HcResourcesPA->PeriodicHead[0];

    for (ix = 0; ix < (INTERRUPT_ENDPOINTs + 1); ix++)
    {
        EHCI_AlignHwStructure(EhciExtension,
                              (PULONG)&PeriodicHeadPA,
                              (PULONG)&PeriodicHead,
                              80);

        EhciExtension->PeriodicHead[ix] = PeriodicHead;
        EhciExtension->PeriodicHead[ix]->PhysicalAddress = (PEHCI_HCD_QH)PeriodicHeadPA;

        PeriodicHead += 1;
        PeriodicHeadPA += 1;
    }

    EHCI_InitializeInterruptSchedule(EhciExtension);

    for (Frame = 0; Frame < EHCI_FRAME_LIST_MAX_ENTRIES; Frame++)
    {
        StaticQH = EHCI_GetQhForFrame(EhciExtension, Frame);

        StaticHeadPA.AsULONG = (ULONG_PTR)StaticQH->PhysicalAddress;
        StaticHeadPA.Type = EHCI_LINK_TYPE_QH;

        //DPRINT_EHCI("EHCI_InitializeSchedule: StaticHeadPA[%x] - %p\n",
        //            Frame,
        //            StaticHeadPA);

        HcResourcesVA->PeriodicFrameList[Frame] = (PEHCI_STATIC_QH)StaticHeadPA.AsULONG;
    }

    EhciExtension->IsoDummyQHListVA = (ULONG_PTR)&HcResourcesVA->IsoDummyQH[0];
    EhciExtension->IsoDummyQHListPA = (ULONG_PTR)&HcResourcesPA->IsoDummyQH[0];

    EHCI_AddDummyQHs(EhciExtension);

    WRITE_REGISTER_ULONG(&OperationalRegs->PeriodicListBase,
                         (ULONG_PTR)EhciExtension->HcResourcesPA);

    WRITE_REGISTER_ULONG(&OperationalRegs->AsyncListBase,
                         NextLink.AsULONG);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_InitializeHardware(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_HC_CAPABILITY_REGISTERS CapabilityRegisters;
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER CurrentTime;
    EHCI_HC_STRUCTURAL_PARAMS StructuralParams;

    DPRINT_EHCI("EHCI_InitializeHardware: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;
    CapabilityRegisters = EhciExtension->CapabilityRegisters;

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.Reset = 1;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += 100 * 10000; // 100 msec

    DPRINT_EHCI("EHCI_InitializeHardware: Start reset ... \n");

    while (TRUE)
    {
        KeQuerySystemTime(&CurrentTime);
        Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);

        if (Command.Reset != 1)
        {
            break;
        }

        if (CurrentTime.QuadPart >= EndTime.QuadPart)
        {
            if (Command.Reset == 1)
            {
                DPRINT1("EHCI_InitializeHardware: Reset failed!\n");
                return MP_STATUS_HW_ERROR;
            }

            break;
        }
    }

    DPRINT("EHCI_InitializeHardware: Reset - OK\n");

    StructuralParams.AsULONG = READ_REGISTER_ULONG(&CapabilityRegisters->StructParameters.AsULONG);

    EhciExtension->NumberOfPorts = StructuralParams.PortCount;
    EhciExtension->PortPowerControl = StructuralParams.PortPowerControl;

    DPRINT("EHCI_InitializeHardware: StructuralParams - %p\n", StructuralParams.AsULONG);
    DPRINT("EHCI_InitializeHardware: PortPowerControl - %x\n", EhciExtension->PortPowerControl);
    DPRINT("EHCI_InitializeHardware: N_PORTS          - %x\n", EhciExtension->NumberOfPorts);

    WRITE_REGISTER_ULONG(&OperationalRegs->PeriodicListBase, 0);
    WRITE_REGISTER_ULONG(&OperationalRegs->AsyncListBase, 0);

    EhciExtension->InterruptMask.AsULONG = 0;
    EhciExtension->InterruptMask.Interrupt = 1;
    EhciExtension->InterruptMask.ErrorInterrupt = 1;
    EhciExtension->InterruptMask.PortChangeInterrupt = 0;
    EhciExtension->InterruptMask.FrameListRollover = 1;
    EhciExtension->InterruptMask.HostSystemError = 1;
    EhciExtension->InterruptMask.InterruptOnAsyncAdvance = 1;

    return MP_STATUS_SUCCESS;
}

UCHAR
NTAPI
EHCI_GetOffsetEECP(IN PEHCI_EXTENSION EhciExtension,
                   IN UCHAR CapabilityID)
{
    EHCI_LEGACY_EXTENDED_CAPABILITY LegacyCapability;
    EHCI_HC_CAPABILITY_PARAMS CapParameters;
    UCHAR OffsetEECP;

    DPRINT("EHCI_GetOffsetEECP: CapabilityID - %x\n", CapabilityID);

    CapParameters = EhciExtension->CapabilityRegisters->CapParameters;

    OffsetEECP = CapParameters.ExtCapabilitiesPointer;

    if (!OffsetEECP)
    {
        return 0;
    }

    while (TRUE)
    {
        RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                              TRUE,
                                              &LegacyCapability.AsULONG,
                                              OffsetEECP,
                                              sizeof(LegacyCapability));

        DPRINT("EHCI_GetOffsetEECP: OffsetEECP - %x\n", OffsetEECP);

        if (LegacyCapability.CapabilityID == CapabilityID)
        {
            break;
        }

        OffsetEECP = LegacyCapability.NextCapabilityPointer;

        if (!OffsetEECP)
        {
            return 0;
        }
    }

    return OffsetEECP;
}

MPSTATUS
NTAPI
EHCI_TakeControlHC(IN PEHCI_EXTENSION EhciExtension)
{
    LARGE_INTEGER EndTime;
    LARGE_INTEGER CurrentTime;
    EHCI_LEGACY_EXTENDED_CAPABILITY LegacyCapability;
    UCHAR OffsetEECP;

    DPRINT("EHCI_TakeControlHC: EhciExtension - %p\n", EhciExtension);

    OffsetEECP = EHCI_GetOffsetEECP(EhciExtension, 1);

    if (OffsetEECP == 0)
    {
        return MP_STATUS_SUCCESS;
    }

    DPRINT("EHCI_TakeControlHC: OffsetEECP - %X\n", OffsetEECP);

    RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                          TRUE,
                                          &LegacyCapability.AsULONG,
                                          OffsetEECP,
                                          sizeof(LegacyCapability));

    if (LegacyCapability.BiosOwnedSemaphore == FALSE)
    {
        return MP_STATUS_SUCCESS;
    }

    LegacyCapability.OsOwnedSemaphore = TRUE;

    RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                          FALSE,
                                          &LegacyCapability.AsULONG,
                                          OffsetEECP,
                                          sizeof(LegacyCapability));

    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += 100 * 10000;

    do
    {
        RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                              TRUE,
                                              &LegacyCapability.AsULONG,
                                              OffsetEECP,
                                              sizeof(LegacyCapability));
        KeQuerySystemTime(&CurrentTime);

        if (LegacyCapability.BiosOwnedSemaphore)
        {
            DPRINT("EHCI_TakeControlHC: Ownership is ok\n");
            break;
        }
    }
    while (CurrentTime.QuadPart <= EndTime.QuadPart);

    return MP_STATUS_SUCCESS;
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
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HC_CAPABILITY_REGISTERS CapabilityRegisters;
    PEHCI_HW_REGISTERS OperationalRegs;
    MPSTATUS MPStatus;
    EHCI_USB_COMMAND Command;
    UCHAR CapabilityRegLength;
    UCHAR Fladj;

    DPRINT_EHCI("EHCI_StartController: ... \n");

    if ((Resources->ResourcesTypes & (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT)) !=
                                     (USBPORT_RESOURCES_MEMORY | USBPORT_RESOURCES_INTERRUPT))
    {
        DPRINT1("EHCI_StartController: Resources->ResourcesTypes - %x\n",
                Resources->ResourcesTypes);

        return MP_STATUS_ERROR;
    }

    CapabilityRegisters = (PEHCI_HC_CAPABILITY_REGISTERS)Resources->ResourceBase;
    EhciExtension->CapabilityRegisters = CapabilityRegisters;

    CapabilityRegLength = READ_REGISTER_UCHAR(&CapabilityRegisters->RegistersLength);

    OperationalRegs = (PEHCI_HW_REGISTERS)((ULONG)CapabilityRegisters +
                                                  CapabilityRegLength);

    EhciExtension->OperationalRegs = OperationalRegs;

    DPRINT("EHCI_StartController: CapabilityRegisters - %p\n", CapabilityRegisters);
    DPRINT("EHCI_StartController: OperationalRegs     - %p\n", OperationalRegs);

    RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                          TRUE,
                                          &Fladj,
                                          EHCI_FLADJ_PCI_CONFIG_OFFSET,
                                          sizeof(Fladj));

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
                                          TRUE,
                                          &Fladj,
                                          EHCI_FLADJ_PCI_CONFIG_OFFSET,
                                          sizeof(Fladj));

    if (Fladj != EhciExtension->FrameLengthAdjustment)
    {
        Fladj = EhciExtension->FrameLengthAdjustment;

        RegPacket.UsbPortReadWriteConfigSpace(EhciExtension,
                                              FALSE, // write
                                              &Fladj,
                                              EHCI_FLADJ_PCI_CONFIG_OFFSET,
                                              sizeof(Fladj));
    }

    /* Port routing control logic default-routes all ports to this HC */
    WRITE_REGISTER_ULONG(&OperationalRegs->ConfiFlag, 1);
    EhciExtension->PortRoutingControl = 1;

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.InterruptThreshold = 1; // one micro-frame
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.Run = 1; // execution of the schedule
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    EhciExtension->IsStarted = TRUE;

    if (Resources->IsChirpHandled)
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
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;
    EHCI_USB_STATUS Status;
    EHCI_INTERRUPT_ENABLE IntrEn;
    ULONG ix;

    DPRINT("EHCI_SuspendController: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    EhciExtension->BakupPeriodiclistbase = READ_REGISTER_ULONG(&OperationalRegs->PeriodicListBase);
    EhciExtension->BakupAsynclistaddr = READ_REGISTER_ULONG(&OperationalRegs->AsyncListBase);
    EhciExtension->BakupCtrlDSSegment = READ_REGISTER_ULONG(&OperationalRegs->SegmentSelector);
    EhciExtension->BakupUSBCmd = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.InterruptAdvanceDoorbell = 0;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.Run = 0;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    KeStallExecutionProcessor(125);

    Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);

    Status.HCHalted = 0;
    Status.Reclamation = 0;
    Status.PeriodicStatus = 0;
    Status.AsynchronousStatus = 0;

    if (Status.AsULONG)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG, Status.AsULONG);
    }

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG, 0);

    for (ix = 0; ix < 10; ix++)
    {
        Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);

        if (Status.HCHalted)
        {
            break;
        }

        RegPacket.UsbPortWait(EhciExtension, 1);
    }

    if (!Status.HCHalted)
    {
        DbgBreakPoint();
    }

    IntrEn.AsULONG = READ_REGISTER_ULONG((PULONG)(&OperationalRegs->HcInterruptEnable.AsULONG));
    IntrEn.PortChangeInterrupt = 1;
    WRITE_REGISTER_ULONG((PULONG)(&OperationalRegs->HcInterruptEnable.AsULONG), IntrEn.AsULONG);

    EhciExtension->Flags |= EHCI_FLAGS_CONTROLLER_SUSPEND;
}

MPSTATUS
NTAPI
EHCI_ResumeController(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    ULONG RoutingControl;
    EHCI_USB_COMMAND Command;

    DPRINT("EHCI_ResumeController: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    RoutingControl = EhciExtension->PortRoutingControl;

    if (!(RoutingControl & 1))
    {
        EhciExtension->PortRoutingControl = RoutingControl | 1;
        WRITE_REGISTER_ULONG(&OperationalRegs->ConfiFlag, RoutingControl | 1);

        return MP_STATUS_HW_ERROR;
    }

    WRITE_REGISTER_ULONG(&OperationalRegs->SegmentSelector,
                         EhciExtension->BakupCtrlDSSegment);

    WRITE_REGISTER_ULONG(&OperationalRegs->PeriodicListBase,
                         EhciExtension->BakupPeriodiclistbase);

    WRITE_REGISTER_ULONG(&OperationalRegs->AsyncListBase,
                         EhciExtension->BakupAsynclistaddr);

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);

    Command.AsULONG = Command.AsULONG ^ EhciExtension->BakupUSBCmd;

    Command.Reset = 0;
    Command.FrameListSize = 0;
    Command.InterruptAdvanceDoorbell = 0;
    Command.LightResetHC = 0;
    Command.AsynchronousParkModeCount = 0;
    Command.AsynchronousParkModeEnable = 0;

    Command.Run = 1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG,
                         Command.AsULONG);

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                         EhciExtension->InterruptMask.AsULONG);

    EhciExtension->Flags &= ~EHCI_FLAGS_CONTROLLER_SUSPEND;

    return MP_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
EHCI_HardwarePresent(IN PEHCI_EXTENSION EhciExtension,
                     IN BOOLEAN IsInvalidateController)
{
    if (READ_REGISTER_ULONG((PULONG)EhciExtension->OperationalRegs) != -1)
    {
        return TRUE;
    }

    DPRINT1("EHCI_HardwarePresent: IsInvalidateController - %x\n",
            IsInvalidateController);

    if (!IsInvalidateController)
    {
        return FALSE;
    }

    RegPacket.UsbPortInvalidateController(EhciExtension,
                                          USBPORT_INVALIDATE_CONTROLLER_SURPRISE_REMOVE);
    return FALSE;
}

BOOLEAN
NTAPI
EHCI_InterruptService(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    BOOLEAN Result = FALSE;
    ULONG IntrSts;
    ULONG IntrEn;
    EHCI_INTERRUPT_ENABLE iStatus;
    ULONG FrameIndex;
    ULONG Command;

    OperationalRegs = EhciExtension->OperationalRegs;

    DPRINT_EHCI("EHCI_InterruptService: ... \n");

    Result = EHCI_HardwarePresent(EhciExtension, FALSE);

    if (!Result)
    {
        return FALSE;
    }

    IntrEn = READ_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG);
    IntrSts = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);

    iStatus.AsULONG = (IntrEn & IntrSts) & 0x3F;

    if (!iStatus.AsULONG)
    {
        return FALSE;
    }

    EhciExtension->InterruptStatus = iStatus;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG,
                         (IntrEn & IntrSts) & 0x3F);

    if (iStatus.HostSystemError)
    {
        EhciExtension->HcSystemErrors++;

        if (EhciExtension->HcSystemErrors < 0x100)
        {
            //attempting reset
            Command = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
            WRITE_REGISTER_ULONG((PULONG)OperationalRegs, Command | 1);
        }
    }

    FrameIndex = READ_REGISTER_ULONG(&OperationalRegs->FrameIndex) / 8;
    FrameIndex &= EHCI_FRINDEX_FRAME_MASK;

    if ((FrameIndex ^ EhciExtension->FrameIndex) & EHCI_FRAME_LIST_MAX_ENTRIES)
    {
        EhciExtension->FrameHighPart += 2 * EHCI_FRAME_LIST_MAX_ENTRIES;

        EhciExtension->FrameHighPart -= (FrameIndex ^ EhciExtension->FrameHighPart) &
                                        EHCI_FRAME_LIST_MAX_ENTRIES;
    }

    EhciExtension->FrameIndex = FrameIndex;

    return TRUE;
}

VOID
NTAPI
EHCI_InterruptDpc(IN PVOID ehciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_INTERRUPT_ENABLE iStatus;

    OperationalRegs = EhciExtension->OperationalRegs;

    DPRINT_EHCI("EHCI_InterruptDpc: ... \n");

    iStatus = EhciExtension->InterruptStatus;
    EhciExtension->InterruptStatus.AsULONG = 0;

    if ((UCHAR)iStatus.AsULONG &
        (UCHAR)EhciExtension->InterruptMask.AsULONG & 0x23)
    {
        RegPacket.UsbPortInvalidateEndpoint(EhciExtension, NULL);
    }

    if ((UCHAR)iStatus.AsULONG &
        (UCHAR)EhciExtension->InterruptMask.AsULONG & 0x04)
    {
        RegPacket.UsbPortInvalidateRootHub(EhciExtension);
    }

    if (IsDoEnableInterrupts)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
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

    SgElement = &SgList->SgElement[0];

    for (SgIdx = 0; SgIdx < SgList->SgElementCount; SgIdx++)
    {
        if (TransferedLen >= SgElement->SgOffset &&
            TransferedLen < SgElement->SgOffset + SgElement->SgTransferLength)
        {
            break;
        }

        SgElement += 1;
    }

    SgRemain = SgList->SgElementCount - SgIdx;

    if (SgRemain > 5)
    {
        TD->HwTD.Buffer[0] = SgList->SgElement[SgIdx].SgPhysicalAddress.LowPart - 
                             SgList->SgElement[SgIdx].SgOffset +
                             TransferedLen;

        LengthThisTD = 5 * PAGE_SIZE - (TD->HwTD.Buffer[0] & (PAGE_SIZE - 1));

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
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND UsbCmd;

    DPRINT_EHCI("EHCI_EnableAsyncList: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    UsbCmd.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    UsbCmd.AsynchronousEnable = 1;
    WRITE_REGISTER_ULONG((&OperationalRegs->HcCommand.AsULONG), UsbCmd.AsULONG);
}

VOID
NTAPI
EHCI_DisableAsyncList(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND UsbCmd;

    DPRINT_EHCI("EHCI_DisableAsyncList: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    UsbCmd.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    UsbCmd.AsynchronousEnable = 0;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, UsbCmd.AsULONG);
}

VOID
NTAPI
EHCI_EnablePeriodicList(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;

    DPRINT_EHCI("EHCI_EnablePeriodicList: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Command.PeriodicEnable = 1;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);
}

VOID
NTAPI
EHCI_DisablePeriodicList(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;

    DPRINT_EHCI("EHCI_DisablePeriodicList: ... \n");

    if (EhciExtension->Flags & EHCI_FLAGS_IDLE_SUPPORT)
    {
        OperationalRegs = EhciExtension->OperationalRegs;

        Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
        Command.PeriodicEnable = 0;
        WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);
    }
}

VOID
NTAPI
EHCI_FlushAsyncCache(IN PEHCI_EXTENSION EhciExtension)
{
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;
    EHCI_USB_STATUS Status;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER EndTime;
    EHCI_USB_COMMAND Cmd;

    DPRINT_EHCI("EHCI_FlushAsyncCache: EhciExtension - %p\n", EhciExtension);

    OperationalRegs = EhciExtension->OperationalRegs;
    Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
    Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);

    if (!Status.AsynchronousStatus && !Command.AsynchronousEnable)
    {
        return;
    }

    if (Status.AsynchronousStatus && !Command.AsynchronousEnable)
    {
        KeQuerySystemTime(&EndTime);
        EndTime.QuadPart += 100 * 10000;  //100 ms

        do
        {
            Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);
            Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
            KeQuerySystemTime(&CurrentTime);

            if (CurrentTime.QuadPart > EndTime.QuadPart)
            {
                RegPacket.UsbPortBugCheck(EhciExtension);
            }
        }
        while (Status.AsynchronousStatus && Command.AsULONG != -1 && Command.Run);

        return;
    }    

    if (!Status.AsynchronousStatus && Command.AsynchronousEnable)
    {
        KeQuerySystemTime(&EndTime);
        EndTime.QuadPart += 100 * 10000;  //100 ms

        do
        {
            Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);
            Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
            KeQuerySystemTime(&CurrentTime);
        }
        while (!Status.AsynchronousStatus && Command.AsULONG != -1 && Command.Run);
    }

    Command.InterruptAdvanceDoorbell = 1;
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG, Command.AsULONG);

    KeQuerySystemTime(&EndTime);
    EndTime.QuadPart += 100 * 10000;  //100 ms

    Cmd.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);

    if (Cmd.InterruptAdvanceDoorbell)
    {
        while (Cmd.Run)
        {
            if (Cmd.AsULONG == -1)
            {
                break;
            }

            KeStallExecutionProcessor(1);
            Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
            KeQuerySystemTime(&CurrentTime);

            if (!Command.InterruptAdvanceDoorbell)
            {
                break;
            }

            Cmd = Command;
        }
    }

    /* InterruptOnAsyncAdvance */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG, 0x20);
}

VOID
NTAPI
EHCI_LockQH(IN PEHCI_EXTENSION EhciExtension,
            IN PEHCI_HCD_QH QueueHead,
            IN ULONG TransferType)
{
    PEHCI_HCD_QH PrevQH;
    PEHCI_HCD_QH NextQH;
    ULONG_PTR QhPA;
    ULONG FrameIndexReg;
    PEHCI_HW_REGISTERS OperationalRegs;
    ULONG Command;

    DPRINT_EHCI("EHCI_LockQH: QueueHead - %p, TransferType - %x\n",
                QueueHead,
                TransferType);

    OperationalRegs = EhciExtension->OperationalRegs;

    ASSERT((QueueHead->sqh.QhFlags & EHCI_QH_FLAG_UPDATING) == 0);
    ASSERT(EhciExtension->LockQH == NULL);

    PrevQH = QueueHead->sqh.PrevHead;
    QueueHead->sqh.QhFlags |= EHCI_QH_FLAG_UPDATING;

    ASSERT(PrevQH);

    NextQH = QueueHead->sqh.NextHead;

    EhciExtension->PrevQH = PrevQH;
    EhciExtension->NextQH = NextQH;
    EhciExtension->LockQH = QueueHead;

    if (NextQH)
    {
        QhPA = (ULONG_PTR)NextQH->sqh.PhysicalAddress;
        QhPA &= LINK_POINTER_MASK + TERMINATE_POINTER;
        QhPA |= (EHCI_LINK_TYPE_QH << 1);
    }
    else
    {
        QhPA = TERMINATE_POINTER;
    }

    PrevQH->sqh.HwQH.HorizontalLink.AsULONG = QhPA;

    FrameIndexReg = READ_REGISTER_ULONG(&OperationalRegs->FrameIndex);

    if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        do
        {
            Command = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);
        }
        while (READ_REGISTER_ULONG(&OperationalRegs->FrameIndex) == 
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
    ULONG_PTR QhPA;

    DPRINT_EHCI("EHCI_UnlockQH: QueueHead - %p\n", QueueHead);

    ASSERT((QueueHead->sqh.QhFlags & EHCI_QH_FLAG_UPDATING) != 0);
    ASSERT(EhciExtension->LockQH != 0);
    ASSERT(EhciExtension->LockQH == QueueHead);

    QueueHead->sqh.QhFlags &= ~EHCI_QH_FLAG_UPDATING;

    EhciExtension->LockQH = NULL;

    QhPA = (ULONG_PTR)QueueHead->sqh.PhysicalAddress;
    QhPA &= LINK_POINTER_MASK + TERMINATE_POINTER;
    QhPA |= (EHCI_LINK_TYPE_QH << 1);

    EhciExtension->PrevQH->sqh.HwQH.HorizontalLink.AsULONG = QhPA;
}

VOID
NTAPI
EHCI_LinkTransferToQueue(PEHCI_EXTENSION EhciExtension,
                         PEHCI_ENDPOINT EhciEndpoint,
                         PEHCI_HCD_TD NextTD)
{
    PEHCI_HCD_QH QH;
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
    TD = EhciEndpoint->HcdHeadP;

    if (TD == EhciEndpoint->HcdTailP)
    {
        if (IsPresent)
        {
            EHCI_LockQH(EhciExtension,
                        QH,
                        EhciEndpoint->EndpointProperties.TransferType);
        }

        QH->sqh.HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DmaBufferPA;
        QH->sqh.HwQH.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
        QH->sqh.HwQH.AlternateNextTD = NextTD->HwTD.AlternateNextTD;

        QH->sqh.HwQH.Token.Status = (UCHAR)~(EHCI_TOKEN_STATUS_ACTIVE |
                                             EHCI_TOKEN_STATUS_HALTED);

        QH->sqh.HwQH.Token.TransferBytes = 0;

        if (IsPresent)
        {
            EHCI_UnlockQH(EhciExtension, QH);
        }

        EhciEndpoint->HcdHeadP = NextTD;
    }
    else
    {
        DbgBreakPoint();

        DPRINT("EHCI_LinkTransferToQueue: TD - %p, HcdTailP - %p\n",
               EhciEndpoint->HcdHeadP,
               EhciEndpoint->HcdTailP);

        LinkTD = EhciEndpoint->HcdHeadP;

        if (TD != EhciEndpoint->HcdTailP)
        {
            while (TRUE)
            {
                LinkTD = TD;
                TD = TD->NextHcdTD;

                if (TD == EhciEndpoint->HcdTailP)
                {
                    break;
                }
            }
        }

        ASSERT(LinkTD != EhciEndpoint->HcdTailP);

        Transfer = LinkTD->EhciTransfer;

        TD = EhciEndpoint->FirstTD;

        for (ix = 0; ix < EhciEndpoint->MaxTDs; ix++)
        {
            if (TD->EhciTransfer == Transfer)
            {
                TD->AltNextHcdTD = NextTD;
                TD->HwTD.AlternateNextTD = (ULONG_PTR)NextTD->PhysicalAddress;
            }

            TD += 1;
        }

        LinkTD->HwTD.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
        LinkTD->NextHcdTD = NextTD;

        if (QH->sqh.HwQH.CurrentTD == (ULONG_PTR)LinkTD->PhysicalAddress)
        {
            QH->sqh.HwQH.NextTD = (ULONG_PTR)NextTD->PhysicalAddress;
            QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;
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
        return MP_STATUS_FAILURE;
    }

    EhciExtension->PendingTransfers++;
    EhciEndpoint->PendingTDs++;

    EhciTransfer->TransferOnAsyncList = 1;

    FirstTD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!FirstTD)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return MP_STATUS_FAILURE;
    }

    EhciTransfer->PendingTDs++;

    FirstTD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
    FirstTD->EhciTransfer = EhciTransfer;

    FirstTD->HwTD.Buffer[0] = (ULONG_PTR)&FirstTD->PhysicalAddress->SetupPacket;
    FirstTD->HwTD.Buffer[1] = 0;
    FirstTD->HwTD.Buffer[2] = 0;
    FirstTD->HwTD.Buffer[3] = 0;
    FirstTD->HwTD.Buffer[4] = 0;

    FirstTD->NextHcdTD = NULL;

    FirstTD->HwTD.NextTD = TERMINATE_POINTER;
    FirstTD->HwTD.AlternateNextTD = TERMINATE_POINTER;

    FirstTD->HwTD.Token.AsULONG = 0;
    FirstTD->HwTD.Token.ErrorCounter = 3;
    FirstTD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_SETUP;
    FirstTD->HwTD.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;
    FirstTD->HwTD.Token.TransferBytes = 0x8;

    RtlCopyMemory(&FirstTD->SetupPacket,
                  &TransferParameters->SetupPacket,
                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    LastTD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

    if (!LastTD)
    {
        RegPacket.UsbPortBugCheck(EhciExtension);
        return MP_STATUS_FAILURE;
    }

    EhciTransfer->PendingTDs++;

    LastTD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
    LastTD->EhciTransfer = EhciTransfer;

    LastTD->HwTD.Buffer[0] = 0;
    LastTD->HwTD.Buffer[1] = 0;
    LastTD->HwTD.Buffer[2] = 0;
    LastTD->HwTD.Buffer[3] = 0;
    LastTD->HwTD.Buffer[4] = 0;

    LastTD->NextHcdTD = NULL;
    LastTD->HwTD.NextTD = TERMINATE_POINTER;
    LastTD->HwTD.AlternateNextTD = TERMINATE_POINTER;

    LastTD->HwTD.Token.AsULONG = 0;
    LastTD->HwTD.Token.ErrorCounter = 3;

    FirstTD->AltNextHcdTD = LastTD;
    FirstTD->HwTD.AlternateNextTD = (ULONG_PTR)LastTD->PhysicalAddress;

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

            EhciTransfer->PendingTDs++;

            TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
            TD->EhciTransfer = EhciTransfer;

            TD->HwTD.Buffer[0] = 0;
            TD->HwTD.Buffer[1] = 0;
            TD->HwTD.Buffer[2] = 0;
            TD->HwTD.Buffer[3] = 0;
            TD->HwTD.Buffer[4] = 0;

            TD->NextHcdTD = NULL;

            TD->HwTD.NextTD = TERMINATE_POINTER;
            TD->HwTD.AlternateNextTD = TERMINATE_POINTER;

            TD->HwTD.Token.AsULONG = 0;
            TD->HwTD.Token.ErrorCounter = 3;

            PrevTD->NextHcdTD = TD;
            PrevTD->HwTD.NextTD = (ULONG_PTR)TD->PhysicalAddress;

            if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            {
                TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_IN;
            }
            else
            {
                TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_OUT;
            }

            TD->HwTD.Token.DataToggle = DataToggle;
            TD->HwTD.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;

            if (DataToggle)
            {
                TD->HwTD.Token.DataToggle = TRUE;
            }
            else
            {
                TD->HwTD.Token.DataToggle = FALSE;
            }

            TD->AltNextHcdTD = LastTD;
            TD->HwTD.AlternateNextTD = (ULONG_PTR)LastTD->PhysicalAddress;

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
        return MP_STATUS_FAILURE;
    }

End:

    LinkTD->NextHcdTD = LastTD;
    LinkTD->HwTD.NextTD = (ULONG_PTR)LastTD->PhysicalAddress;

    LastTD->HwTD.Buffer[0] = 0;
    LastTD->LengthThisTD = 0;

    Token.AsULONG = 0;
    Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;
    Token.InterruptOnComplete = TRUE;
    Token.DataToggle = TRUE;

    if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        Token.PIDCode = EHCI_TD_TOKEN_PID_OUT;
    }
    else
    {
        Token.PIDCode = EHCI_TD_TOKEN_PID_IN;
    }

    LastTD->HwTD.Token = Token;

    LastTD->NextHcdTD = EhciEndpoint->HcdTailP;
    LastTD->HwTD.NextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;

    EHCI_EnableAsyncList(EhciExtension);
    EHCI_LinkTransferToQueue(EhciExtension, EhciEndpoint, FirstTD);

    ASSERT(EhciEndpoint->HcdTailP->NextHcdTD == NULL);
    ASSERT(EhciEndpoint->HcdTailP->AltNextHcdTD == NULL);

    //EHCI_DPrintHwTD(FirstTD);
    //EHCI_DPrintHwTD(FirstTD->NextHcdTD);
    //EHCI_DPrintHwTD(LastTD);

    return MP_STATUS_SUCCESS;
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
        DPRINT1("EHCI_BulkTransfer: return MP_STATUS_FAILURE\n");
        return MP_STATUS_FAILURE;
    }

    EhciExtension->PendingTransfers++;
    EhciEndpoint->PendingTDs++;

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
                return MP_STATUS_FAILURE;
            }

            EhciTransfer->PendingTDs++;

            TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
            TD->EhciTransfer = EhciTransfer;

            TD->HwTD.Buffer[0] = 0;
            TD->HwTD.Buffer[1] = 0;
            TD->HwTD.Buffer[2] = 0;
            TD->HwTD.Buffer[3] = 0;
            TD->HwTD.Buffer[4] = 0;

            TD->NextHcdTD = NULL;
            TD->HwTD.NextTD = TERMINATE_POINTER;
            TD->HwTD.AlternateNextTD = TERMINATE_POINTER;

            TD->HwTD.Token.AsULONG = 0;
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
                TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_IN;
            }
            else
            {
                TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_OUT;
            }

            TD->HwTD.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;
            TD->HwTD.Token.DataToggle = TRUE;

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
            return MP_STATUS_FAILURE;
        }

        EhciTransfer->PendingTDs++;

        TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
        TD->EhciTransfer = EhciTransfer;

        TD->HwTD.Buffer[0] = 0;
        TD->HwTD.Buffer[1] = 0;
        TD->HwTD.Buffer[2] = 0;
        TD->HwTD.Buffer[3] = 0;
        TD->HwTD.Buffer[4] = 0;

        TD->HwTD.NextTD = TERMINATE_POINTER;
        TD->HwTD.AlternateNextTD = TERMINATE_POINTER;

        TD->HwTD.Token.AsULONG = 0;
        TD->HwTD.Token.ErrorCounter = 3;

        TD->NextHcdTD = NULL;

        ASSERT(EhciTransfer->PendingTDs == 1);

        FirstTD = TD;

        TD->HwTD.AlternateNextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
        TD->AltNextHcdTD = EhciEndpoint->HcdTailP;

        TD->HwTD.Token.InterruptOnComplete = TRUE;

        if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
        {
            TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_IN;
        }
        else
        {
            TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_OUT;
        }

        TD->HwTD.Buffer[0] = (ULONG_PTR)TD->PhysicalAddress;

        TD->HwTD.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;
        TD->HwTD.Token.DataToggle = TRUE;

        TD->LengthThisTD = 0;
    }

    TD->HwTD.NextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    TD->NextHcdTD = EhciEndpoint->HcdTailP;

    EHCI_EnableAsyncList(EhciExtension);
    EHCI_LinkTransferToQueue(EhciExtension, EhciEndpoint, FirstTD);

    ASSERT(EhciEndpoint->HcdTailP->NextHcdTD == 0);
    ASSERT(EhciEndpoint->HcdTailP->AltNextHcdTD == 0);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_InterruptTransfer(IN PEHCI_EXTENSION EhciExtension,
                       IN PEHCI_ENDPOINT EhciEndpoint,
                       IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                       IN PEHCI_TRANSFER EhciTransfer,
                       IN PUSBPORT_SCATTER_GATHER_LIST SgList)
{
    PEHCI_HCD_TD TD;
    PEHCI_HCD_TD FirstTD;
    PEHCI_HCD_TD PrevTD = NULL;
    ULONG TransferedLen = 0;

    DPRINT_EHCI("EHCI_InterruptTransfer: EhciEndpoint - %p, EhciTransfer - %p\n",
                EhciEndpoint,
                EhciTransfer);

    if (!EhciEndpoint->RemainTDs)
    {
        DPRINT1("EHCI_InterruptTransfer: EhciEndpoint - %p\n", EhciEndpoint);
        DbgBreakPoint();
        return MP_STATUS_FAILURE;
    }

    EhciEndpoint->PendingTDs++;

    if (!TransferParameters->TransferBufferLength)
    {
        DPRINT1("EHCI_InterruptTransfer: EhciEndpoint - %p\n", EhciEndpoint);
        DbgBreakPoint();
        return MP_STATUS_FAILURE;
    }

    do
    {
        TD = EHCI_AllocTd(EhciExtension, EhciEndpoint);

        if (!TD)
        {
            DPRINT1("EHCI_InterruptTransfer: EhciEndpoint - %p\n", EhciEndpoint);
            RegPacket.UsbPortBugCheck(EhciExtension);
            return MP_STATUS_FAILURE;
        }

        EhciTransfer->PendingTDs++;

        TD->TdFlags |= EHCI_HCD_TD_FLAG_PROCESSED;
        TD->EhciTransfer = EhciTransfer;

        TD->HwTD.Buffer[0] = 0;
        TD->HwTD.Buffer[1] = 0;
        TD->HwTD.Buffer[2] = 0;
        TD->HwTD.Buffer[3] = 0;
        TD->HwTD.Buffer[4] = 0;

        TD->HwTD.NextTD = TERMINATE_POINTER;
        TD->HwTD.AlternateNextTD = TERMINATE_POINTER;

        TD->HwTD.Token.AsULONG = 0;
        TD->HwTD.Token.ErrorCounter = 3;

        TD->NextHcdTD = NULL;

        if (EhciTransfer->PendingTDs == 1)
        {
            FirstTD = TD;
        }
        else if (PrevTD)
        {
            PrevTD->HwTD.NextTD = (ULONG_PTR)TD->PhysicalAddress;
            PrevTD->NextHcdTD = TD;
        }

        if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
        {
            TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_IN;
        }
        else
        {
            TD->HwTD.Token.PIDCode = EHCI_TD_TOKEN_PID_OUT;
        }

        TD->HwTD.Token.Status = (UCHAR)EHCI_TOKEN_STATUS_ACTIVE;
        TD->HwTD.Token.DataToggle = TRUE;

        TransferedLen = EHCI_MapAsyncTransferToTd(EhciExtension,
                                                  EhciEndpoint->EndpointProperties.MaxPacketSize,
                                                  TransferedLen,
                                                  NULL,
                                                  EhciTransfer,
                                                  TD,
                                                  SgList);

        PrevTD = TD;
    }
    while (TransferedLen < TransferParameters->TransferBufferLength);

    TD->HwTD.Token.InterruptOnComplete = 1;

    DPRINT_EHCI("EHCI_InterruptTransfer: PendingTDs - %p, TD->PhysicalAddress - %p, FirstTD - %p\n",
                EhciTransfer->PendingTDs,
                TD->PhysicalAddress,
                FirstTD);

    TD->HwTD.NextTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    TD->NextHcdTD = EhciEndpoint->HcdTailP;

    EHCI_LinkTransferToQueue(EhciExtension, EhciEndpoint, FirstTD);

    ASSERT(EhciEndpoint->HcdTailP->NextHcdTD == NULL);
    ASSERT(EhciEndpoint->HcdTailP->AltNextHcdTD == NULL);

    EHCI_EnablePeriodicList(EhciExtension);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_SubmitTransfer(IN PVOID ehciExtension,
                    IN PVOID ehciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ehciTransfer,
                    IN PVOID sgList)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_ENDPOINT EhciEndpoint = ehciEndpoint;
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters = transferParameters;
    PEHCI_TRANSFER EhciTransfer = ehciTransfer;
    PUSBPORT_SCATTER_GATHER_LIST SgList = sgList;
    MPSTATUS MPStatus;

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
            MPStatus = MP_STATUS_NOT_SUPPORTED;
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
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_AbortIsoTransfer(IN PEHCI_EXTENSION EhciExtension,
                      IN PEHCI_ENDPOINT EhciEndpoint,
                      IN PEHCI_TRANSFER EhciTransfer)
{
    DPRINT1("EHCI_AbortIsoTransfer: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_AbortAsyncTransfer(IN PEHCI_EXTENSION EhciExtension,
                        IN PEHCI_ENDPOINT EhciEndpoint,
                        IN PEHCI_TRANSFER EhciTransfer)
{
    PEHCI_HCD_QH QH;
    PEHCI_HCD_TD TD;
    ULONG TransferLength;
    PEHCI_HCD_TD CurrentTD;
    PEHCI_TRANSFER CurrentTransfer;
    PEHCI_HCD_TD FirstTD;
    PEHCI_HCD_TD LastTD;
    PEHCI_HCD_TD NextTD;

    DPRINT("EHCI_AbortAsyncTransfer: ... \n");

    QH = EhciEndpoint->QH;
    TD = EhciEndpoint->HcdHeadP;

    EhciEndpoint->PendingTDs--;

    if (TD->EhciTransfer == EhciTransfer)
    {
        TransferLength = 0;

        while (TD != EhciEndpoint->HcdTailP &&
               TD->EhciTransfer == EhciTransfer)
        {
            TransferLength += TD->LengthThisTD - TD->HwTD.Token.TransferBytes;

            TD = TD->NextHcdTD;

            TD->TdFlags = 0;
            TD->HwTD.NextTD = 0;
            TD->HwTD.AlternateNextTD = 0;

            EhciEndpoint->RemainTDs++;

            TD->EhciTransfer = NULL;
        }

        if (TransferLength)
        {
            EhciTransfer->TransferLen += TransferLength;
        }

        QH->sqh.HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DmaBufferPA;
        QH->sqh.HwQH.NextTD = (ULONG)TD->PhysicalAddress;
        QH->sqh.HwQH.AlternateNextTD = TD->HwTD.AlternateNextTD;

        QH->sqh.HwQH.Token.TransferBytes = 0;
        QH->sqh.HwQH.Token.Status = (UCHAR)~(EHCI_TOKEN_STATUS_ACTIVE |
                                             EHCI_TOKEN_STATUS_HALTED);

        EhciEndpoint->HcdHeadP = TD;
    }
    else
    {
        CurrentTD = RegPacket.UsbPortGetMappedVirtualAddress((PVOID)QH->sqh.HwQH.CurrentTD,
                                                             EhciExtension,
                                                             EhciEndpoint);

        CurrentTransfer = CurrentTD->EhciTransfer;

        TD = EhciEndpoint->HcdHeadP;

        while (TD && TD->EhciTransfer != EhciTransfer)
        {
            TD = TD->NextHcdTD;
        }

        FirstTD = TD;

        do
        {
            if (TD->EhciTransfer != EhciTransfer)
            {
                break;
            }

            TD->TdFlags = 0;
            TD->HwTD.NextTD = 0;
            TD->HwTD.AlternateNextTD = 0;

            EhciEndpoint->RemainTDs++;

            TD->EhciTransfer = NULL;

            TD = TD->NextHcdTD;
        }
        while (TD);

        LastTD = TD;
        NextTD = (PEHCI_HCD_TD)LastTD->PhysicalAddress->HwTD.NextTD;

        FirstTD->HwTD.NextTD = (ULONG)LastTD->PhysicalAddress;
        FirstTD->HwTD.AlternateNextTD = (ULONG)LastTD->PhysicalAddress;

        FirstTD->NextHcdTD = LastTD;
        FirstTD->AltNextHcdTD = LastTD;

        if (CurrentTransfer == EhciTransfer)
        {
            QH->sqh.HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DmaBufferPA;

            QH->sqh.HwQH.Token.Status = (UCHAR)~EHCI_TOKEN_STATUS_ACTIVE;
            QH->sqh.HwQH.Token.TransferBytes = 0;

            QH->sqh.HwQH.NextTD = (ULONG)NextTD;
            QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;

            return;
        }

        if (FirstTD->EhciTransfer == CurrentTransfer)
        {
            if (QH->sqh.HwQH.NextTD == (ULONG)FirstTD->PhysicalAddress)
            {
                QH->sqh.HwQH.NextTD = (ULONG)NextTD;
            }

            if (QH->sqh.HwQH.AlternateNextTD == (ULONG)FirstTD->PhysicalAddress)
            {
                QH->sqh.HwQH.AlternateNextTD = (ULONG)NextTD;
            }

            for (TD = EhciEndpoint->HcdHeadP;
                 TD;
                 TD = TD->NextHcdTD)
            {
                if (TD->EhciTransfer == CurrentTransfer)
                {
                    TD->HwTD.AlternateNextTD = (ULONG)NextTD;
                    TD->AltNextHcdTD = LastTD;
                }
            }
        }
    }
}

VOID
NTAPI
EHCI_AbortTransfer(IN PVOID ehciExtension,
                   IN PVOID ehciEndpoint,
                   IN PVOID ehciTransfer,
                   IN PULONG CompletedLength)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_ENDPOINT EhciEndpoint = ehciEndpoint;
    PEHCI_TRANSFER EhciTransfer = ehciTransfer;
    ULONG TransferType;

    DPRINT_EHCI("EHCI_AbortTransfer: EhciTransfer - %p, CompletedLength - %x\n",
                EhciTransfer,
                CompletedLength);

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        EHCI_AbortIsoTransfer(EhciExtension, EhciEndpoint, EhciTransfer);
    }
    else
    {
        EHCI_AbortAsyncTransfer(EhciExtension, EhciEndpoint, EhciTransfer);
    }
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
    ULONG_PTR NextHeadPA;
    PEHCI_HCD_QH PrevHead;
    PEHCI_STATIC_QH AsyncHead;
    ULONG_PTR AsyncHeadPA;

    DPRINT_EHCI("EHCI_RemoveQhFromAsyncList: QH - %p\n", QH);

    if (QH->sqh.QhFlags & EHCI_QH_FLAG_IN_SCHEDULE)
    {
        NextHead = QH->sqh.NextHead;
        PrevHead = QH->sqh.PrevHead;

        AsyncHead = EhciExtension->AsyncHead;

        AsyncHeadPA = (ULONG_PTR)AsyncHead->PhysicalAddress;
        AsyncHeadPA &= LINK_POINTER_MASK + TERMINATE_POINTER;
        AsyncHeadPA |= (EHCI_LINK_TYPE_QH << 1);

        NextHeadPA = (ULONG_PTR)NextHead->sqh.PhysicalAddress;
        NextHeadPA &= LINK_POINTER_MASK + TERMINATE_POINTER;
        NextHeadPA |= (EHCI_LINK_TYPE_QH << 1);

        PrevHead->sqh.HwQH.HorizontalLink.AsULONG = NextHeadPA;

        PrevHead->sqh.NextHead = NextHead;
        NextHead->sqh.PrevHead = PrevHead;

        EHCI_FlushAsyncCache(EhciExtension);

        if (READ_REGISTER_ULONG(&EhciExtension->OperationalRegs->AsyncListBase) ==
            (ULONG_PTR)QH->sqh.PhysicalAddress)
        {
            WRITE_REGISTER_ULONG(&EhciExtension->OperationalRegs->AsyncListBase,
                                 AsyncHeadPA);
        }

        QH->sqh.QhFlags &= ~EHCI_QH_FLAG_IN_SCHEDULE;
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
    ULONG_PTR QhPA;
    PEHCI_HCD_QH NextHead;

    DPRINT_EHCI("EHCI_InsertQhInAsyncList: QH - %p\n", QH);

    ASSERT((QH->sqh.QhFlags & EHCI_QH_FLAG_IN_SCHEDULE) == 0);
    ASSERT((QH->sqh.QhFlags & EHCI_QH_FLAG_NUKED) == 0);

    AsyncHead = EhciExtension->AsyncHead;
    NextHead = AsyncHead->NextHead;

    QH->sqh.HwQH.HorizontalLink = AsyncHead->HwQH.HorizontalLink;
    QH->sqh.QhFlags |= EHCI_QH_FLAG_IN_SCHEDULE;
    QH->sqh.NextHead = NextHead;
    QH->sqh.PrevHead = (PEHCI_HCD_QH)AsyncHead;

    NextHead->sqh.PrevHead = QH;

    QhPA = (ULONG_PTR)QH->sqh.PhysicalAddress;
    QhPA &= LINK_POINTER_MASK + TERMINATE_POINTER;
    QhPA |= (EHCI_LINK_TYPE_QH << 1);

    AsyncHead->HwQH.HorizontalLink.AsULONG = QhPA;

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

        case USBPORT_ENDPOINT_REMOVE:
            QH->sqh.QhFlags |= EHCI_QH_FLAG_CLOSED;

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

    EhciEndpoint = ehciEndpoint;
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
    PEHCI_EXTENSION EhciExtension = ehciExtension;

    DPRINT_EHCI("EHCI_InterruptNextSOF: ... \n");

    RegPacket.UsbPortInvalidateController(EhciExtension,
                                          USBPORT_INVALIDATE_CONTROLLER_SOFT_INTERRUPT);
}

USBD_STATUS
NTAPI
EHCI_GetErrorFromTD(IN PEHCI_HCD_TD TD)
{
    EHCI_TD_TOKEN Token;

    DPRINT_EHCI("EHCI_GetErrorFromTD: ... \n");

    ASSERT(TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_HALTED);

    Token = TD->HwTD.Token;

    if (Token.Status & EHCI_TOKEN_STATUS_TRANSACTION_ERROR)
    {
        DPRINT("EHCI_GetErrorFromTD: TD - %p, TRANSACTION_ERROR\n", TD);
        return USBD_STATUS_XACT_ERROR;
    }

    if (Token.Status & EHCI_TOKEN_STATUS_BABBLE_DETECTED)
    {
        DPRINT("EHCI_GetErrorFromTD: TD - %p, BABBLE_DETECTED\n", TD);
        return USBD_STATUS_BABBLE_DETECTED;
    }

    if (Token.Status & EHCI_TOKEN_STATUS_DATA_BUFFER_ERROR)
    {
        DPRINT("EHCI_GetErrorFromTD: TD - %p, DATA_BUFFER_ERROR\n", TD);
        return USBD_STATUS_DATA_BUFFER_ERROR;
    }

    if (Token.Status & EHCI_TOKEN_STATUS_MISSED_MICROFRAME)
    {
        DPRINT("EHCI_GetErrorFromTD: TD - %p, MISSED_MICROFRAME\n", TD);
        return USBD_STATUS_XACT_ERROR;
    }

    DPRINT("EHCI_GetErrorFromTD: TD - %p, STALL_PID\n", TD);
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
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_COMMAND Command;

    DPRINT_EHCI("EHCI_ProcessDoneAsyncTd: TD - %p\n", TD);

    EhciTransfer = TD->EhciTransfer;

    TransferParameters = EhciTransfer->TransferParameters;
    EhciTransfer->PendingTDs--;

    EhciEndpoint = EhciTransfer->EhciEndpoint;

    if (TD->TdFlags & EHCI_HCD_TD_FLAG_ACTIVE)
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

    if (TD->HwTD.Token.PIDCode != EHCI_TD_TOKEN_PID_SETUP)
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

    EhciEndpoint->RemainTDs++;

    if (EhciTransfer->PendingTDs == 0)
    {
        EhciEndpoint->PendingTDs--;

        TransferType = EhciEndpoint->EndpointProperties.TransferType;

        if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
            TransferType == USBPORT_TRANSFER_TYPE_BULK)
        {
            EhciExtension->PendingTransfers--;

            if (EhciExtension->PendingTransfers == 0)
            {
                OperationalRegs = EhciExtension->OperationalRegs;
                Command.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcCommand.AsULONG);

                if (!Command.InterruptAdvanceDoorbell &&
                    (EhciExtension->Flags & EHCI_FLAGS_IDLE_SUPPORT))
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
    ULONG_PTR CurrentTDPhys; 
    BOOLEAN IsSheduled;

    DPRINT_EHCI("EHCI_PollActiveAsyncEndpoint: ... \n");

    QH = EhciEndpoint->QH;

    CurrentTDPhys = QH->sqh.HwQH.CurrentTD & LINK_POINTER_MASK;
    ASSERT(CurrentTDPhys != 0);

    CurrentTD = RegPacket.UsbPortGetMappedVirtualAddress((PVOID)CurrentTDPhys,
                                                         EhciExtension,
                                                         EhciEndpoint);

    if (CurrentTD == EhciEndpoint->DmaBufferVA)
    {
        return;
    }

    IsSheduled = QH->sqh.QhFlags & EHCI_QH_FLAG_IN_SCHEDULE;

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

        if (QH->sqh.HwQH.CurrentTD == (ULONG_PTR)TD->PhysicalAddress &&
            !(TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE) && 
            (QH->sqh.HwQH.NextTD != TD->HwTD.NextTD ||
             QH->sqh.HwQH.AlternateNextTD != TD->HwTD.AlternateNextTD))
        {
            QH->sqh.HwQH.NextTD = TD->HwTD.NextTD;
            QH->sqh.HwQH.AlternateNextTD = TD->HwTD.AlternateNextTD;
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
                TD->TdFlags |= EHCI_HCD_TD_FLAG_ACTIVE;
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

    QH->sqh.HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->DmaBufferPA;

    CurrentTD->TdFlags |= EHCI_HCD_TD_FLAG_DONE;
    InsertTailList(&EhciEndpoint->ListTDs, &CurrentTD->DoneLink);

    if (CurrentTD->HwTD.Token.TransferBytes &&
        CurrentTD->AltNextHcdTD == EhciEndpoint->HcdTailP)
    {
        TD = CurrentTD->NextHcdTD;
  
        while (TD != EhciEndpoint->HcdTailP)
        {
            TD->TdFlags |= EHCI_HCD_TD_FLAG_ACTIVE;
            InsertTailList(&EhciEndpoint->ListTDs, &TD->DoneLink);
            TD = TD->NextHcdTD;
        }
    }

    QH->sqh.HwQH.CurrentTD = (ULONG_PTR)EhciEndpoint->HcdTailP->PhysicalAddress;
    QH->sqh.HwQH.NextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.Token.TransferBytes = 0;

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
    PEHCI_HCD_QH QH;
    PEHCI_HCD_TD CurrentTD;
    ULONG_PTR CurrentTdPA;
    PEHCI_HCD_TD TD;
    PEHCI_TRANSFER Transfer;
    BOOLEAN IsSheduled;

    DPRINT("EHCI_PollHaltedAsyncEndpoint: EhciEndpoint - %p\n", EhciEndpoint);

    QH = EhciEndpoint->QH;
    EHCI_DumpHwQH(QH);

    CurrentTdPA = QH->sqh.HwQH.CurrentTD & LINK_POINTER_MASK;
    ASSERT(CurrentTdPA != 0);

    IsSheduled = QH->sqh.QhFlags & EHCI_QH_FLAG_IN_SCHEDULE;

    if (!EHCI_HardwarePresent(EhciExtension, 0))
    {
        IsSheduled = 0;
    }

    CurrentTD = RegPacket.UsbPortGetMappedVirtualAddress((PVOID)CurrentTdPA,
                                                         EhciExtension,
                                                         EhciEndpoint);

    DPRINT("EHCI_PollHaltedAsyncEndpoint: CurrentTD - %p\n", CurrentTD);

    if (CurrentTD == EhciEndpoint->DmaBufferVA)
    {
        return;
    }

    ASSERT(EhciEndpoint->HcdTailP != CurrentTD);

    if (IsSheduled)
    {
        EHCI_LockQH(EhciExtension,
                    QH,
                    EhciEndpoint->EndpointProperties.TransferType);
    }

    TD = EhciEndpoint->HcdHeadP;

    while (TD != CurrentTD)
    {
        DPRINT("EHCI_PollHaltedAsyncEndpoint: TD - %p\n", TD);

        ASSERT((TD->TdFlags & EHCI_HCD_TD_FLAG_DUMMY) == 0);

        if (TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE)
        {
            TD->TdFlags |= EHCI_HCD_TD_FLAG_ACTIVE;
        }

        TD->TdFlags |= EHCI_HCD_TD_FLAG_DONE;

        InsertTailList(&EhciEndpoint->ListTDs, &TD->DoneLink);

        TD = TD->NextHcdTD;
    }

    TD = CurrentTD;

    Transfer = CurrentTD->EhciTransfer;

    do
    {
        DPRINT("EHCI_PollHaltedAsyncEndpoint: TD - %p\n", TD);

        if (TD->HwTD.Token.Status & EHCI_TOKEN_STATUS_ACTIVE)
        {
            TD->TdFlags |= EHCI_HCD_TD_FLAG_ACTIVE;
        }

        TD->TdFlags |= EHCI_HCD_TD_FLAG_DONE;

        InsertTailList(&EhciEndpoint->ListTDs, &TD->DoneLink);

        TD = TD->NextHcdTD;
    }
    while (TD->EhciTransfer == Transfer);

    EhciEndpoint->HcdHeadP = TD;

    QH->sqh.HwQH.CurrentTD = (ULONG)EhciEndpoint->DmaBufferVA;
    QH->sqh.HwQH.NextTD = (ULONG)TD->PhysicalAddress;
    QH->sqh.HwQH.AlternateNextTD = TERMINATE_POINTER;
    QH->sqh.HwQH.Token.TransferBytes = 0;

    if (IsSheduled)
    {
        EHCI_UnlockQH(EhciExtension, QH);
    }

    if (EhciEndpoint->EndpointStatus & USBPORT_ENDPOINT_CONTROL)
    {
        EhciEndpoint->EndpointStatus &= ~USBPORT_ENDPOINT_HALT;
        QH->sqh.HwQH.Token.ErrorCounter = 0;
        QH->sqh.HwQH.Token.Status &= (UCHAR)~(EHCI_TOKEN_STATUS_ACTIVE |
                                              EHCI_TOKEN_STATUS_HALTED);

    }
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

    if (QH->sqh.QhFlags & EHCI_QH_FLAG_CLOSED)
    {
        return;
    }

    if (QH->sqh.HwQH.Token.Status & EHCI_TOKEN_STATUS_ACTIVE ||
       !(QH->sqh.HwQH.Token.Status & EHCI_TOKEN_STATUS_HALTED))
    {
        EHCI_PollActiveAsyncEndpoint(EhciExtension, EhciEndpoint);
    }
    else
    {
        EhciEndpoint->EndpointStatus |= USBPORT_ENDPOINT_HALT;
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
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_ENDPOINT EhciEndpoint = ehciEndpoint;
    ULONG TransferType;

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
    PEHCI_EXTENSION EhciExtension = ehciExtension;

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
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    ULONG FrameIdx; 
    ULONG FrameIndex;
    ULONG FrameNumber;

    //DPRINT_EHCI("EHCI_Get32BitFrameNumber: EhciExtension - %p\n", EhciExtension);

    FrameIdx = EhciExtension->FrameIndex;
    FrameIndex = READ_REGISTER_ULONG((PULONG)EhciExtension->OperationalRegs + EHCI_FRINDEX);

    FrameNumber = (USHORT)FrameIdx ^ ((FrameIndex / 8) & EHCI_FRINDEX_FRAME_MASK);
    FrameNumber &= EHCI_FRAME_LIST_MAX_ENTRIES;
    FrameNumber += FrameIndex | ((FrameIndex / 8) & EHCI_FRINDEX_INDEX_MASK);

    return FrameNumber;
}

VOID
NTAPI
EHCI_EnableInterrupts(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;

    DPRINT("EHCI_EnableInterrupts: EhciExtension->InterruptMask - %x\n",
           EhciExtension->InterruptMask.AsULONG);

    WRITE_REGISTER_ULONG((PULONG)EhciExtension->OperationalRegs + EHCI_USBINTR,
                         EhciExtension->InterruptMask.AsULONG);
}

VOID
NTAPI
EHCI_DisableInterrupts(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;

    DPRINT("EHCI_DisableInterrupts: UNIMPLEMENTED. FIXME\n");

    WRITE_REGISTER_ULONG((PULONG)EhciExtension->OperationalRegs + EHCI_USBINTR,
                         0);
}

VOID
NTAPI
EHCI_PollController(IN PVOID ehciExtension)
{
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    ULONG Port;
    EHCI_PORT_STATUS_CONTROL PortSC;

    DPRINT_EHCI("EHCI_PollController: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    if (!(EhciExtension->Flags & EHCI_FLAGS_CONTROLLER_SUSPEND))
    {
        RegPacket.UsbPortInvalidateRootHub(EhciExtension);
        return;
    }

    if (EhciExtension->NumberOfPorts)
    {
        for (Port = 0; Port < EhciExtension->NumberOfPorts; Port++)
        {
            PortSC.AsULONG = READ_REGISTER_ULONG(((PULONG)OperationalRegs + EHCI_PORTSC) + Port);

            if (PortSC.ConnectStatusChange)
            {
                RegPacket.UsbPortInvalidateRootHub(EhciExtension);
            }
        }
    }
}

VOID
NTAPI
EHCI_SetEndpointDataToggle(IN PVOID ehciExtension,
                           IN PVOID ehciEndpoint,
                           IN ULONG DataToggle)
{
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG TransferType;

    EhciEndpoint = ehciEndpoint;

    DPRINT_EHCI("EHCI_SetEndpointDataToggle: EhciEndpoint - %p, DataToggle - %x\n",
                EhciEndpoint,
                DataToggle);

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        EhciEndpoint->QH->sqh.HwQH.Token.DataToggle = DataToggle;
    }
}

ULONG
NTAPI
EHCI_GetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint)
{
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG TransferType;
    ULONG EndpointStatus = USBPORT_ENDPOINT_RUN;

    EhciEndpoint = ehciEndpoint;

    DPRINT_EHCI("EHCI_GetEndpointStatus: EhciEndpoint - %p\n", EhciEndpoint);

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        return EndpointStatus;
    }

    if (EhciEndpoint->EndpointStatus & USBPORT_ENDPOINT_HALT)
    {
        EndpointStatus = USBPORT_ENDPOINT_HALT;
    }

    return EndpointStatus;
}

VOID
NTAPI
EHCI_SetEndpointStatus(IN PVOID ehciExtension,
                       IN PVOID ehciEndpoint,
                       IN ULONG EndpointStatus)
{
    PEHCI_ENDPOINT EhciEndpoint;
    ULONG TransferType;
    PEHCI_HCD_QH QH;

    EhciEndpoint = ehciEndpoint;

    DPRINT("EHCI_SetEndpointStatus: EhciEndpoint - %p, EndpointStatus - %x\n",
                EhciEndpoint,
                EndpointStatus);

    TransferType = EhciEndpoint->EndpointProperties.TransferType;

    if (TransferType != USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {

        if (EndpointStatus == USBPORT_ENDPOINT_RUN)
        {
            EhciEndpoint->EndpointStatus &= ~USBPORT_ENDPOINT_HALT;

            QH = EhciEndpoint->QH;
            QH->sqh.HwQH.Token.Status &= (UCHAR)~EHCI_TOKEN_STATUS_HALTED;

            return;
        }

        if (EndpointStatus == USBPORT_ENDPOINT_HALT)
        {
            DbgBreakPoint();
        }
    }
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
    return MP_STATUS_SUCCESS;
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
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
EHCI_PassThru(IN PVOID ehciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT1("EHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
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
    PEHCI_EXTENSION EhciExtension = ehciExtension;
    PEHCI_HW_REGISTERS OperationalRegs;
    EHCI_USB_STATUS Status;

    DPRINT("EHCI_FlushInterrupts: ... \n");

    OperationalRegs = EhciExtension->OperationalRegs;

    Status.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG);
    WRITE_REGISTER_ULONG(&OperationalRegs->HcStatus.AsULONG, Status.AsULONG);
}

MPSTATUS
NTAPI
EHCI_RH_ChirpRootPort(IN PVOID ehciExtension,
                      IN USHORT Port)
{
    DPRINT1("EHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
EHCI_TakePortControl(IN PVOID ohciExtension)
{
    DPRINT1("EHCI_TakePortControl: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
EHCI_Unload(IN PDRIVER_OBJECT DriverObject)
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

    if (USBPORT_GetHciMn() != USBPORT_HCI_MN)
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

    RegPacket.MiniPortBusBandwidth = TOTAL_USB20_BUS_BANDWIDTH;

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

    DriverObject->DriverUnload = EHCI_Unload;

    return USBPORT_RegisterUSBPortDriver(DriverObject,
                                         USB20_MINIPORT_INTERFACE_VERSION,
                                         &RegPacket);
}
