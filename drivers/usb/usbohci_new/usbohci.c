#include "usbohci.h"

//#define NDEBUG
#include <debug.h>

#define NDEBUG_OHCI_TRACE
#include "dbg_ohci.h"

USBPORT_REGISTRATION_PACKET RegPacket;

VOID
NTAPI
OHCI_EnableList(IN POHCI_EXTENSION OhciExtension,
                IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    ULONG TransferType;
    OHCI_REG_COMMAND_STATUS CommandStatus;

    DPRINT_OHCI("OHCI_EnableList: ... \n");

    OperationalRegs = OhciExtension->OperationalRegs;

    CommandStatus.AsULONG = 0;

    if (READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcControlHeadED))
    {
        CommandStatus.ControlListFilled = 1;
    }

    if (READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcBulkHeadED))
    {
        CommandStatus.BulkListFilled = 1;
    }

    TransferType = OhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK)
    {
        CommandStatus.BulkListFilled = 1;
    }
    else if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        CommandStatus.ControlListFilled = 1;
    }

    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommandStatus.AsULONG,
                         CommandStatus.AsULONG);
}

VOID
NTAPI
OHCI_InsertEndpointInSchedule(IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_STATIC_ED HeadED;
    POHCI_HCD_ED ED;
    POHCI_HCD_ED PrevED;
    PLIST_ENTRY HeadLink;

    DPRINT_OHCI("OHCI_InsertEndpointInSchedule: OhciEndpoint - %p\n",
                OhciEndpoint);

    ED = OhciEndpoint->HcdED;

    HeadED = OhciEndpoint->HeadED;
    HeadLink = &HeadED->Link;

    if (IsListEmpty(HeadLink))
    {
        InsertHeadList(HeadLink, &ED->HcdEDLink);

        if (HeadED->Type & 0x20) // ControlTransfer or BulkTransfer
        {
            ED->HwED.NextED = READ_REGISTER_ULONG(HeadED->pNextED);
            WRITE_REGISTER_ULONG(HeadED->pNextED, ED->PhysicalAddress);
        }
        else
        {
            ED->HwED.NextED = *HeadED->pNextED;
            *HeadED->pNextED = ED->PhysicalAddress;
        }
    }
    else
    {
        PrevED = CONTAINING_RECORD(HeadLink->Blink,
                                   OHCI_HCD_ED,
                                   HcdEDLink);

        InsertTailList(HeadLink, &ED->HcdEDLink);

        ED->HwED.NextED = 0;
        PrevED->HwED.NextED = ED->PhysicalAddress;
    }
}

POHCI_HCD_ED
NTAPI
OHCI_InitializeED(IN POHCI_ENDPOINT OhciEndpoint,
                  IN POHCI_HCD_ED ED,
                  IN POHCI_HCD_TD FirstTD,
                  IN ULONG_PTR EdPA)
{
    OHCI_ENDPOINT_CONTROL EndpointControl;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;

    DPRINT_OHCI("OHCI_InitializeED: OhciEndpoint - %p, ED - %p, FirstTD - %p, EdPA - %p\n",
                OhciEndpoint,
                ED,
                FirstTD,
                EdPA);

    RtlZeroMemory(ED, sizeof(OHCI_HCD_ED));

    ED->PhysicalAddress = EdPA;

    EndpointProperties = &OhciEndpoint->EndpointProperties;

    ED->HwED.EndpointControl.FunctionAddress = EndpointProperties->DeviceAddress;
    ED->HwED.EndpointControl.EndpointNumber = EndpointProperties->EndpointAddress;

    EndpointControl = ED->HwED.EndpointControl;

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        EndpointControl.Direction = OHCI_ED_DATA_FLOW_DIRECTION_FROM_TD;
    }
    else if (EndpointProperties->Direction)
    {
        EndpointControl.Direction = OHCI_ED_DATA_FLOW_DIRECTION_OUT;
    }
    else
    {
        EndpointControl.Direction = OHCI_ED_DATA_FLOW_DIRECTION_IN;
    }

    ED->HwED.EndpointControl = EndpointControl;

    if (EndpointProperties->DeviceSpeed == UsbLowSpeed)
    {
        ED->HwED.EndpointControl.Speed = OHCI_ENDPOINT_LOW_SPEED;
    }

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        ED->HwED.EndpointControl.Format = OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;
    }
    else
    {
        ED->HwED.EndpointControl.sKip = 1;
    }

    ED->HwED.EndpointControl.MaximumPacketSize = EndpointProperties->TotalMaxPacketSize;

    ED->HwED.TailPointer = (ULONG_PTR)FirstTD->PhysicalAddress;
    ED->HwED.HeadPointer = (ULONG_PTR)FirstTD->PhysicalAddress;

    FirstTD->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;

    OhciEndpoint->HcdTailP = FirstTD;
    OhciEndpoint->HcdHeadP = FirstTD;

    return ED;
}

MPSTATUS
NTAPI
OHCI_OpenControlEndpoint(IN POHCI_EXTENSION OhciExtension,
                         IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                         IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_TD TdVA;
    POHCI_HCD_TD TdPA;
    POHCI_HCD_ED ED;
    ULONG TdCount;

    DPRINT_OHCI("OHCI_OpenControlEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));
    OhciEndpoint->HeadED = &OhciExtension->ControlStaticED;

    TdCount = (EndpointProperties->BufferLength - sizeof(OHCI_HCD_ED)) / 
              sizeof(OHCI_HCD_TD);

    OhciEndpoint->MaxTransferDescriptors = TdCount;

    if (TdCount > 0)
    {
        TdVA = OhciEndpoint->FirstTD;
        TdPA = (POHCI_HCD_TD)
               ((ULONG_PTR)EndpointProperties->BufferPA +
               sizeof(OHCI_HCD_ED));

        do
        {
            DPRINT_OHCI("OHCI_OpenControlEndpoint: InitTD. TdVA - %p, TdPA - %p\n",
                        TdVA,
                        TdPA);

            RtlZeroMemory(TdVA, sizeof(OHCI_HCD_TD));

            TdVA->PhysicalAddress = (ULONG_PTR)TdPA;
            TdVA->Flags = 0;
            TdVA->OhciTransfer = NULL;

            ++TdVA;
            ++TdPA;
            --TdCount;
        }
        while (TdCount > 0);
    }

    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OhciEndpoint->HcdED->Flags = OHCI_HCD_ED_FLAG_RESET_ON_HALT | 1;

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return 0;
}

MPSTATUS
NTAPI
OHCI_OpenBulkEndpoint(IN POHCI_EXTENSION OhciExtension,
                      IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                      IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;
    POHCI_HCD_TD TdVA;
    POHCI_HCD_TD TdPA;
    ULONG TdCount;

    DPRINT_OHCI("OHCI_OpenBulkEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));
    OhciEndpoint->HeadED = &OhciExtension->BulkStaticED;

    TdCount = (EndpointProperties->BufferLength - sizeof(OHCI_HCD_ED)) /
               sizeof(OHCI_HCD_TD);

    OhciEndpoint->MaxTransferDescriptors = TdCount;

    if (TdCount > 0)
    {
        TdVA = OhciEndpoint->FirstTD;

        TdPA = (POHCI_HCD_TD)((ULONG_PTR)EndpointProperties->BufferPA +
                              sizeof(OHCI_HCD_ED));

        do
        {
            DPRINT_OHCI("OHCI_OpenBulkEndpoint: InitTD. TdVA - %p, TdPA - %p\n",
                        TdVA,
                        TdPA);

            RtlZeroMemory(TdVA, sizeof(OHCI_HCD_TD));

            TdVA->PhysicalAddress = (ULONG_PTR)TdPA;
            TdVA->Flags = 0;
            TdVA->OhciTransfer = NULL;

            ++TdVA;
            ++TdPA;
            --TdCount;
       }
       while (TdCount > 0);
    }


    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return 0;
}

MPSTATUS
NTAPI
OHCI_OpenInterruptEndpoint(IN POHCI_EXTENSION OhciExtension,
                           IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                           IN POHCI_ENDPOINT OhciEndpoint)
{
    UCHAR Index[8];
    UCHAR Period;
    ULONG PeriodIdx = 0;
    POHCI_HCD_ED ED;
    ULONG ScheduleOffset;
    POHCI_HCD_TD TdVA;
    POHCI_HCD_TD TdPA;
    ULONG TdCount;

    DPRINT("OHCI_OpenInterruptEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));

    Index[0] = (1 << 0) - 1;
    Index[1] = (1 << 1) - 1;
    Index[2] = (1 << 2) - 1;
    Index[3] = (1 << 3) - 1;
    Index[4] = (1 << 4) - 1;
    Index[5] = (1 << 5) - 1;
    Index[6] = (1 << 5) - 1;
    Index[7] = (1 << 5) - 1;

    Period = EndpointProperties->Period;

    while (!(Period & 1))
    {
        ++PeriodIdx;
        Period >>= 1;
    }

    ScheduleOffset = EndpointProperties->ScheduleOffset;
    DPRINT_OHCI("OHCI_OpenInterruptEndpoint: InitTD. Index[PeriodIdx] - %x, ScheduleOffset - %x\n",
                Index[PeriodIdx],
                ScheduleOffset);

    OhciEndpoint->HeadED = &OhciExtension->IntStaticED[Index[PeriodIdx] +
                                                       ScheduleOffset];

    //OhciEndpoint->HeadED->UsbBandwidth += EndpointProperties->UsbBandwidth;

    TdCount = (EndpointProperties->BufferLength - sizeof(OHCI_HCD_ED)) /
               sizeof(OHCI_HCD_TD);

    OhciEndpoint->MaxTransferDescriptors = TdCount;

    if (TdCount > 0)
    {
        TdVA = OhciEndpoint->FirstTD;
        TdPA = (POHCI_HCD_TD)((ULONG_PTR)EndpointProperties->BufferPA +
                              sizeof(OHCI_HCD_ED));

        do
        {
            DPRINT_OHCI("OHCI_OpenInterruptEndpoint: InitTD. TdVA - %p, TdPA - %p\n",
                        TdVA,
                        TdPA);

            RtlZeroMemory(TdVA, sizeof(OHCI_HCD_TD));

            TdVA->PhysicalAddress = (ULONG_PTR)TdPA;
            TdVA->Flags = 0;
            TdVA->OhciTransfer = NULL;

            ++TdVA;
            ++TdPA;
            --TdCount;
       }
       while (TdCount > 0);

    }

    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return 0;
}

MPSTATUS
NTAPI
OHCI_OpenIsoEndpoint(IN POHCI_EXTENSION OhciExtension,
                     IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                     IN POHCI_ENDPOINT OhciEndpoint)
{
    DPRINT1("OHCI_OpenIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return 6;
}

MPSTATUS
NTAPI
OHCI_OpenEndpoint(IN PVOID ohciExtension,
                  IN PVOID endpointParameters,
                  IN PVOID ohciEndpoint)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;
    ULONG Result;

    DPRINT_OHCI("OHCI_OpenEndpoint: ... \n");

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;
    EndpointProperties = (PUSBPORT_ENDPOINT_PROPERTIES)endpointParameters;

    RtlCopyMemory(&OhciEndpoint->EndpointProperties,
                  endpointParameters,
                  sizeof(USBPORT_ENDPOINT_PROPERTIES));

    InitializeListHead(&OhciEndpoint->TDList);

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            Result = OHCI_OpenIsoEndpoint(OhciExtension,
                                          EndpointProperties,
                                          OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            Result = OHCI_OpenControlEndpoint(OhciExtension,
                                              EndpointProperties,
                                              OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            Result = OHCI_OpenBulkEndpoint(OhciExtension,
                                           EndpointProperties,
                                           OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            Result = OHCI_OpenInterruptEndpoint(OhciExtension,
                                                EndpointProperties,
                                                OhciEndpoint);
            break;

        default:
            Result = 6;
            break;
    }

    return Result;
}

MPSTATUS
NTAPI
OHCI_ReopenEndpoint(IN PVOID ohciExtension,
                    IN PVOID endpointParameters,
                    IN PVOID ohciEndpoint)
{
    POHCI_ENDPOINT OhciEndpoint;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_ReopenEndpoint: ... \n");

    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;
    EndpointProperties = (PUSBPORT_ENDPOINT_PROPERTIES)endpointParameters;

    ED = OhciEndpoint->HcdED;

    RtlCopyMemory(&OhciEndpoint->EndpointProperties,
                  EndpointProperties,
                  sizeof(USBPORT_ENDPOINT_PROPERTIES));

    ED->HwED.EndpointControl.FunctionAddress =
        OhciEndpoint->EndpointProperties.DeviceAddress;

    ED->HwED.EndpointControl.MaximumPacketSize =
        OhciEndpoint->EndpointProperties.TotalMaxPacketSize;

    return 0;
}

VOID
NTAPI
OHCI_QueryEndpointRequirements(IN PVOID ohciExtension,
                               IN PVOID endpointParameters,
                               IN PULONG EndpointRequirements)
{
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    ULONG TransferType;

    DPRINT("OHCI_QueryEndpointRequirements: ... \n");

    EndpointProperties = (PUSBPORT_ENDPOINT_PROPERTIES)endpointParameters;
    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: IsoTransfer\n");
            *((PULONG)EndpointRequirements + 1) = 0x10000;
            *EndpointRequirements = sizeof(OHCI_HCD_ED) +
                                    0x40 * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: ControlTransfer\n");
            *((PULONG)EndpointRequirements + 1) = 0x10000;
            *EndpointRequirements = sizeof(OHCI_HCD_ED) +
                                    0x26 * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: BulkTransfer\n");
            *((PULONG)EndpointRequirements + 1) = 0x40000;
            *EndpointRequirements = sizeof(OHCI_HCD_ED) +
                                    0x44 * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: InterruptTransfer\n");
            *((PULONG)EndpointRequirements + 1) = 0x1000;
            *EndpointRequirements = sizeof(OHCI_HCD_ED) +
                                    4 * sizeof(OHCI_HCD_TD);
            break;

        default:
            DPRINT1("OHCI_QueryEndpointRequirements: Unknown TransferType - %x\n",
                    TransferType);
            DbgBreakPoint();
            break;
    }
}

VOID
NTAPI
OHCI_CloseEndpoint(IN PVOID ohciExtension,
                   IN PVOID ohciEndpoint,
                   IN BOOLEAN IsDoDisablePeriodic)
{
    DPRINT("OHCI_CloseEndpoint: Not supported\n");
}

MPSTATUS
NTAPI
OHCI_TakeControlHC(IN POHCI_EXTENSION OhciExtension)
{
    DPRINT1("OHCI_TakeControlHC: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
OHCI_StartController(IN PVOID ohciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE Interrupts;
    OHCI_REG_RH_STATUS HcRhStatus;
    OHCI_REG_FRAME_INTERVAL FrameInterval;
    OHCI_REG_CONTROL Control;
    PVOID ScheduleStartVA;
    PVOID ScheduleStartPA;
    UCHAR HeadIndex;
    POHCI_ENDPOINT_DESCRIPTOR StaticED;
    ULONG_PTR SchedulePA;
    POHCI_HCCA OhciHCCA;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER CurrentTime;
    ULONG ix;
    ULONG jx;
    MPSTATUS MPStatus = 0;

    DPRINT_OHCI("OHCI_StartController: ohciExtension - %p, Resources - %p\n",
                ohciExtension,
                Resources);

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    /* HC on-chip operational registers */
    OperationalRegs = (POHCI_OPERATIONAL_REGISTERS)Resources->ResourceBase;
    OhciExtension->OperationalRegs = OperationalRegs;

    MPStatus = OHCI_TakeControlHC(OhciExtension);

    if (MPStatus != 0)
    {
        DPRINT1("OHCI_StartController: OHCI_TakeControlHC return MPStatus - %x\n",
                MPStatus);

        return MPStatus;
    }

    OhciExtension->HcResourcesVA = (ULONG_PTR)Resources->StartVA;
    OhciExtension->HcResourcesPA = (ULONG_PTR)Resources->StartPA;

    DPRINT_OHCI("OHCI_StartController: HcResourcesVA - %p, HcResourcesPA - %p\n",
                OhciExtension->HcResourcesVA,
                OhciExtension->HcResourcesPA);

    ScheduleStartVA = (PVOID)((ULONG_PTR)Resources->StartVA + sizeof(OHCI_HCCA));
    ScheduleStartPA = (PVOID)((ULONG_PTR)Resources->StartPA + sizeof(OHCI_HCCA));

    OhciExtension->ScheduleStartVA = ScheduleStartVA;
    OhciExtension->ScheduleStartPA = ScheduleStartPA;

    StaticED = (POHCI_ENDPOINT_DESCRIPTOR)ScheduleStartVA;
    SchedulePA = (ULONG_PTR)ScheduleStartPA;

    ix = 0;

    for (ix = 0; ix < 63; ix++) // FIXME 63 == 32+16+8+4+2+1 (Endpoint Poll Interval (ms))
    {
        if (ix == 0)
        {
            HeadIndex = ED_EOF;
            StaticED->NextED = 0;
        }
        else
        {
            HeadIndex = ((ix - 1) >> 1);
            ASSERT(HeadIndex >= 0 && HeadIndex < 31);
            StaticED->NextED = OhciExtension->IntStaticED[HeadIndex].PhysicalAddress;
        }
  
        StaticED->EndpointControl.sKip = 1;
        StaticED->TailPointer = 0;
        StaticED->HeadPointer = 0;
  
        OhciExtension->IntStaticED[ix].HwED = StaticED;
        OhciExtension->IntStaticED[ix].PhysicalAddress = SchedulePA;
        OhciExtension->IntStaticED[ix].HeadIndex = HeadIndex;
        OhciExtension->IntStaticED[ix].pNextED = &StaticED->NextED;
  
        InitializeListHead(&OhciExtension->IntStaticED[ix].Link);
  
        StaticED += 1;
        SchedulePA += sizeof(OHCI_ENDPOINT_DESCRIPTOR);
    }

    OhciHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;
    DPRINT_OHCI("OHCI_InitializeSchedule: OhciHCCA - %p\n", OhciHCCA);

    for (ix = 0, jx = 31; ix < 32; ix++, jx++)
    {
        static UCHAR Balance[32] =
        {0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30, 
         1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31};
  
        OhciHCCA->InterrruptTable[Balance[ix]] =
            (POHCI_ENDPOINT_DESCRIPTOR)(OhciExtension->IntStaticED[jx].PhysicalAddress);
  
        OhciExtension->IntStaticED[jx].pNextED =
            (PULONG)&OhciHCCA->InterrruptTable[Balance[ix]];

        OhciExtension->IntStaticED[jx].HccaIndex = Balance[ix];
    }

    DPRINT_OHCI("OHCI_InitializeSchedule: ix - %x\n", ix);

    InitializeListHead(&OhciExtension->ControlStaticED.Link);

    OhciExtension->ControlStaticED.HeadIndex = ED_EOF;
    OhciExtension->ControlStaticED.Type = OHCI_NUMBER_OF_INTERRUPTS + 1;
    OhciExtension->ControlStaticED.pNextED = &OperationalRegs->HcControlHeadED;

    InitializeListHead(&OhciExtension->BulkStaticED.Link);

    OhciExtension->BulkStaticED.HeadIndex = ED_EOF;
    OhciExtension->BulkStaticED.Type = OHCI_NUMBER_OF_INTERRUPTS + 2;
    OhciExtension->BulkStaticED.pNextED = &OperationalRegs->HcBulkHeadED;

    FrameInterval.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcFmInterval.AsULONG);

    if ((FrameInterval.FrameInterval) < ((12000 - 1) - 120) ||
        (FrameInterval.FrameInterval) > ((12000 - 1) + 120)) // FIXME 10%
    {
        FrameInterval.FrameInterval = (12000 - 1);
    }

    FrameInterval.FrameIntervalToggle = 1;
    FrameInterval.FSLargestDataPacket = 
        ((FrameInterval.FrameInterval - MAXIMUM_OVERHEAD) * 6) / 7;

    OhciExtension->FrameInterval = FrameInterval;

    DPRINT_OHCI("OHCI_StartController: FrameInterval - %p\n",
                FrameInterval.AsULONG);

    /* reset */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcCommandStatus.AsULONG,
                         1);

    KeStallExecutionProcessor(25);

    Control.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG);
    Control.HostControllerFunctionalState = OHCI_HC_STATE_RESET;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG,
                         Control.AsULONG);

    KeQuerySystemTime(&CurrentTime);
    CurrentTime.QuadPart += 5000000; // 0.5 sec

    while (TRUE)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcFmInterval.AsULONG,
                             OhciExtension->FrameInterval.AsULONG);

        FrameInterval.AsULONG =
            READ_REGISTER_ULONG(&OperationalRegs->HcFmInterval.AsULONG);

        KeQuerySystemTime(&SystemTime);

        if (SystemTime.QuadPart >= CurrentTime.QuadPart)
        {
            MPStatus = 7;
            break;
        }

        if (FrameInterval.AsULONG == OhciExtension->FrameInterval.AsULONG)
        {
            MPStatus = 0;
            break;
        }
    }

    if (MPStatus != 0)
    {
        DPRINT_OHCI("OHCI_StartController: frame interval not set\n");
        return MPStatus;
    }

    WRITE_REGISTER_ULONG(&OperationalRegs->HcPeriodicStart,
                        (OhciExtension->FrameInterval.FrameInterval * 9) / 10); //90%

    WRITE_REGISTER_ULONG(&OperationalRegs->HcHCCA,
                         OhciExtension->HcResourcesPA);

    Interrupts.AsULONG = 0;

    Interrupts.SchedulingOverrun = 1;
    Interrupts.WritebackDoneHead = 1;
    Interrupts.UnrecoverableError = 1;
    Interrupts.FrameNumberOverflow = 1;
    Interrupts.OwnershipChange = 1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                         Interrupts.AsULONG);

    Control.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG);

    Control.ControlBulkServiceRatio = 0; // FIXME (1 : 1)
    Control.PeriodicListEnable = 1;
    Control.IsochronousEnable = 1;
    Control.ControlListEnable = 1;
    Control.BulkListEnable = 1;
    Control.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG,
                         Control.AsULONG);
  
    HcRhStatus.AsULONG = 0;
    HcRhStatus.SetGlobalPower = 1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcRhStatus.AsULONG,
                         HcRhStatus.AsULONG);

    return 0;
}

VOID
NTAPI
OHCI_StopController(IN PVOID ohciExtension,
                    IN BOOLEAN IsDoDisableInterrupts)
{
    DPRINT("OHCI_StopController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_SuspendController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_CONTROL Control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptReg;

    DPRINT("OHCI_SuspendController: ... \n");

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OperationalRegs = OhciExtension->OperationalRegs;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptDisable.AsULONG,
                         0xFFFFFFFF);

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG,
                         0xFFFFFFFF);

    Control.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG);

    Control.HostControllerFunctionalState = OHCI_HC_STATE_SUSPEND;
    Control.RemoteWakeupEnable =  1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG,
                         Control.AsULONG);

    InterruptReg.AsULONG = 0;
    InterruptReg.ResumeDetected = 1;
    InterruptReg.UnrecoverableError = 1;
    InterruptReg.RootHubStatusChange = 1;
    InterruptReg.MasterInterruptEnable = 1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                         InterruptReg.AsULONG);
}

MPSTATUS
NTAPI
OHCI_ResumeController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    POHCI_HCCA HcHCCA;
    OHCI_REG_CONTROL control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptReg;

    DPRINT("OHCI_ResumeController \n");

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OperationalRegs = OhciExtension->OperationalRegs;

    control.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG);

    if (control.HostControllerFunctionalState != OHCI_HC_STATE_SUSPEND)
    {
        return 7;
    }

    HcHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;
    HcHCCA->Pad1 = 0;

    control.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG,
                         control.AsULONG);

    InterruptReg.AsULONG = 0;

    InterruptReg.SchedulingOverrun = 1;
    InterruptReg.WritebackDoneHead = 1;
    InterruptReg.UnrecoverableError = 1;
    InterruptReg.FrameNumberOverflow = 1;
    InterruptReg.OwnershipChange = 1;

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                         InterruptReg.AsULONG);

    WRITE_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG,
                         control.AsULONG);

    return 0;
}

BOOLEAN
NTAPI
OHCI_HardwarePresent(IN POHCI_EXTENSION OhciExtension,
                     IN BOOLEAN IsInvalidateController)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    BOOLEAN Result = FALSE;

    //DPRINT_OHCI("OHCI_HardwarePresent: IsInvalidateController - %x\n",
    //            IsInvalidateController);

    OperationalRegs = OhciExtension->OperationalRegs;

    if (READ_REGISTER_ULONG(&OperationalRegs->HcCommandStatus.AsULONG) == -1)
    {
        if (IsInvalidateController)
        {
            RegPacket.UsbPortInvalidateController(OhciExtension, 2);
        }

        Result = FALSE;
    }
    else
    {
        Result = TRUE;
    }

    return Result;
}

BOOLEAN
NTAPI
OHCI_InterruptService(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptReg;
    OHCI_REG_INTERRUPT_STATUS IntStatus; 
    POHCI_HCCA HcHCCA;
    BOOLEAN Result = FALSE;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;

    DPRINT_OHCI("OHCI_InterruptService: OhciExtension - %p\n",
                OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;

    Result = OHCI_HardwarePresent(OhciExtension, FALSE);

    if (!Result)
    {
        return FALSE;
    }

    InterruptReg.AsULONG = 
        READ_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG);

    IntStatus.AsULONG = InterruptReg.AsULONG &
        READ_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG);

    if (IntStatus.AsULONG && InterruptReg.MasterInterruptEnable)
    {
        if (IntStatus.UnrecoverableError)
        {
            DbgBreakPoint();
        }

        if (IntStatus.FrameNumberOverflow)
        {
            HcHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;

            DPRINT("OHCI_InterruptService: FrameNumberOverflow. HcHCCA->FrameNumber - %p\n",
                        HcHCCA->FrameNumber);

            OhciExtension->FrameHighPart += 0x10000 -
                ((HcHCCA->FrameNumber ^ OhciExtension->FrameHighPart) & 0x8000);
        }

        Result = TRUE;

        WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptDisable.AsULONG,
                             0x80000000);
    }

    return Result;
}

VOID
NTAPI
OHCI_InterruptDpc(IN PVOID ohciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_INTERRUPT_STATUS IntStatus;
    POHCI_HCCA HcHCCA;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OperationalRegs = OhciExtension->OperationalRegs;

    DPRINT_OHCI("OHCI_InterruptDpc: OhciExtension - %p, IsDoEnableInterrupts - %p\n",
                OhciExtension,
                IsDoEnableInterrupts);

    IntStatus.AsULONG = 
        READ_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG);

    if (IntStatus.RootHubStatusChange)
    {
        DPRINT_OHCI("OHCI_InterruptDpc: RootHubStatusChange\n");
        RegPacket.UsbPortInvalidateRootHub(OhciExtension);
    }

    if (IntStatus.WritebackDoneHead)
    {
        DPRINT_OHCI("OHCI_InterruptDpc: WritebackDoneHead\n");

        HcHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;
        HcHCCA->DoneHead = 0;

        RegPacket.UsbPortInvalidateEndpoint(OhciExtension, 0);
    }

    if (IntStatus.StartofFrame)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptDisable.AsULONG,
                             4);
    }

    if (IntStatus.ResumeDetected)
    {
        ASSERT(FALSE);
    }

    if (IntStatus.UnrecoverableError)
    {
        ASSERT(FALSE);
    }

    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG,
                         IntStatus.AsULONG);

    if (IsDoEnableInterrupts)
    {
        WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                             0x80000000);
    }
}

MPSTATUS
NTAPI
OHCI_SubmitTransfer(IN PVOID ohciExtension,
                    IN PVOID ohciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ohciTransfer,
                    IN PVOID sgList)
{
    DPRINT("OHCI_SubmitTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
OHCI_SubmitIsoTransfer(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint,
                       IN PVOID transferParameters,
                       IN PVOID ohciTransfer,
                       IN PVOID isoParameters)
{
    DPRINT("OHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
OHCI_AbortTransfer(IN PVOID ohciExtension,
                   IN PVOID ohciEndpoint,
                   IN PVOID ohciTransfer,
                   IN PULONG CompletedLength)
{
    DPRINT("OHCI_AbortTransfer: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
OHCI_GetEndpointState(IN PVOID ohciExtension,
                      IN PVOID ohciEndpoint)
{
    DPRINT("OHCI_GetEndpointState: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
OHCI_SetEndpointState(IN PVOID ohciExtension,
                      IN PVOID ohciEndpoint,
                      IN ULONG EndpointState)
{
    DPRINT("OHCI_SetEndpointState: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_PollEndpoint(IN PVOID ohciExtension,
                  IN PVOID ohciEndpoint)
{
    DPRINT("OHCI_PollEndpoint: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_CheckController(IN PVOID ohciExtension)
{
    DPRINT("OHCI_CheckController: UNIMPLEMENTED. FIXME\n");
}

ULONG
NTAPI
OHCI_Get32BitFrameNumber(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_HCCA HcHCCA;
    ULONG fm;
    ULONG hp;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    HcHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;

    hp = OhciExtension->FrameHighPart;
    fm = HcHCCA->FrameNumber;

    DPRINT_OHCI("OHCI_Get32BitFrameNumber: hp - %x, fm - %p\n", hp, fm);

    return ((fm & 0x7FFF) | hp) + ((fm ^ hp) & 0x8000);
}

VOID
NTAPI
OHCI_InterruptNextSOF(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = (POHCI_EXTENSION)ohciExtension;
    DPRINT_OHCI("OHCI_InterruptNextSOF: OhciExtension - %p\n",
                OhciExtension);

    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptEnable.AsULONG,
                         4);
}

VOID
NTAPI
OHCI_EnableInterrupts(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
 
    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    DPRINT_OHCI("OHCI_EnableInterrupts: OhciExtension - %p\n",
                OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;

    /* HcInterruptEnable.MIE - Master Interrupt Enable */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptEnable.AsULONG,
                         0x80000000);
}

VOID
NTAPI
OHCI_DisableInterrupts(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = (POHCI_EXTENSION)ohciExtension;
    DPRINT_OHCI("OHCI_DisableInterrupts\n");

    /* HcInterruptDisable.MIE - disables interrupt generation */
    WRITE_REGISTER_ULONG(&OhciExtension->OperationalRegs->HcInterruptDisable.AsULONG,
                         0x80000000);
}

VOID
NTAPI
OHCI_PollController(IN PVOID ohciExtension)
{
    DPRINT("OHCI_PollController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_SetEndpointDataToggle(IN PVOID ohciExtension,
                           IN PVOID ohciEndpoint,
                           IN ULONG DataToggle)
{
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_HCD_ED ED;

    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;

    DPRINT_OHCI("OHCI_SetEndpointDataToggle: Endpoint - %p, DataToggle - %x\n",
                OhciEndpoint,
                DataToggle);

    ED = OhciEndpoint->HcdED;

    if (DataToggle)
    {
        ED->HwED.HeadPointer |= 2; // toggleCarry
    }
    else
    {
        ED->HwED.HeadPointer &= ~2;
    }
}

ULONG
NTAPI
OHCI_GetEndpointStatus(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint)
{
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_HCD_ED ED;
    ULONG EndpointStatus = 0;

    DPRINT_OHCI("OHCI_GetEndpointStatus: ... \n");

    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;

    ED = OhciEndpoint->HcdED;

    if ((ED->HwED.HeadPointer & 1) && // Halted
        !(ED->Flags & OHCI_HCD_ED_FLAG_RESET_ON_HALT)) 
    {
        EndpointStatus = 1;
    }

    return EndpointStatus;
}

VOID
NTAPI
OHCI_SetEndpointStatus(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint,
                       IN ULONG EndpointStatus)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_HCD_ED ED;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;

    DPRINT_OHCI("OHCI_SetEndpointStatus: Endpoint - %p, Status - %p\n",
                OhciEndpoint,
                EndpointStatus);

    if (EndpointStatus)
    {
        if (EndpointStatus == 1)
        {
            ASSERT(FALSE);
        }
    }
    else
    {
        ED = OhciEndpoint->HcdED;
        ED->HwED.HeadPointer &= ~1; // ~Halted

        OHCI_EnableList(OhciExtension, OhciEndpoint);
    }
}

VOID
NTAPI
OHCI_ResetController(IN PVOID ohciExtension)
{
    DPRINT("OHCI_ResetController: UNIMPLEMENTED. FIXME\n");
}

MPSTATUS
NTAPI
OHCI_StartSendOnePacket(IN PVOID ohciExtension,
                        IN PVOID PacketParameters,
                        IN PVOID Data,
                        IN PULONG pDataLength,
                        IN PVOID BufferVA,
                        IN PVOID BufferPA,
                        IN ULONG BufferLength,
                        IN USBD_STATUS * pUSBDStatus)
{
    DPRINT("OHCI_StartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
OHCI_EndSendOnePacket(IN PVOID ohciExtension,
                      IN PVOID PacketParameters,
                      IN PVOID Data,
                      IN PULONG pDataLength,
                      IN PVOID BufferVA,
                      IN PVOID BufferPA,
                      IN ULONG BufferLength,
                      IN USBD_STATUS * pUSBDStatus)
{
    DPRINT("OHCI_EndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return 0;
}

MPSTATUS
NTAPI
OHCI_PassThru(IN PVOID ohciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT("OHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return 0;
}

VOID
NTAPI
OHCI_Unload(IN PVOID ohciExtension)
{
    DPRINT1("OHCI_Unload: Not supported\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    DPRINT_OHCI("DriverEntry: DriverObject - %p, RegistryPath - %wZ\n",
                DriverObject,
                RegistryPath);

    RtlZeroMemory(&RegPacket, sizeof(USBPORT_REGISTRATION_PACKET));

    RegPacket.MiniPortVersion = USB_MINIPORT_VERSION_OHCI;

    RegPacket.MiniPortFlags = USB_MINIPORT_FLAGS_INTERRUPT |
                              USB_MINIPORT_FLAGS_MEMORY_IO |
                              8;

    RegPacket.MiniPortBusBandwidth = 12000;

    RegPacket.MiniPortExtensionSize = sizeof(OHCI_EXTENSION);
    RegPacket.MiniPortEndpointSize = sizeof(OHCI_ENDPOINT);
    RegPacket.MiniPortTransferSize = sizeof(OHCI_TRANSFER);
    RegPacket.MiniPortResourcesSize = sizeof(OHCI_HC_RESOURCES);

    RegPacket.OpenEndpoint = OHCI_OpenEndpoint;
    RegPacket.ReopenEndpoint = OHCI_ReopenEndpoint;
    RegPacket.QueryEndpointRequirements = OHCI_QueryEndpointRequirements;
    RegPacket.CloseEndpoint = OHCI_CloseEndpoint;
    RegPacket.StartController = OHCI_StartController;
    RegPacket.StopController = OHCI_StopController;
    RegPacket.SuspendController = OHCI_SuspendController;
    RegPacket.ResumeController = OHCI_ResumeController;
    RegPacket.InterruptService = OHCI_InterruptService;
    RegPacket.InterruptDpc = OHCI_InterruptDpc;
    RegPacket.SubmitTransfer = OHCI_SubmitTransfer;
    RegPacket.SubmitIsoTransfer = OHCI_SubmitIsoTransfer;
    RegPacket.AbortTransfer = OHCI_AbortTransfer;
    RegPacket.GetEndpointState = OHCI_GetEndpointState;
    RegPacket.SetEndpointState = OHCI_SetEndpointState;
    RegPacket.PollEndpoint = OHCI_PollEndpoint;
    RegPacket.CheckController = OHCI_CheckController;
    RegPacket.Get32BitFrameNumber = OHCI_Get32BitFrameNumber;
    RegPacket.InterruptNextSOF = OHCI_InterruptNextSOF;
    RegPacket.EnableInterrupts = OHCI_EnableInterrupts;
    RegPacket.DisableInterrupts = OHCI_DisableInterrupts;
    RegPacket.PollController = OHCI_PollController;
    RegPacket.SetEndpointDataToggle = OHCI_SetEndpointDataToggle;
    RegPacket.GetEndpointStatus = OHCI_GetEndpointStatus;
    RegPacket.SetEndpointStatus = OHCI_SetEndpointStatus;
    RegPacket.ResetController = OHCI_ResetController;
    RegPacket.RH_GetRootHubData = OHCI_RH_GetRootHubData;
    RegPacket.RH_GetStatus = OHCI_RH_GetStatus;
    RegPacket.RH_GetPortStatus = OHCI_RH_GetPortStatus;
    RegPacket.RH_GetHubStatus = OHCI_RH_GetHubStatus;
    RegPacket.RH_SetFeaturePortReset = OHCI_RH_SetFeaturePortReset;
    RegPacket.RH_SetFeaturePortPower = OHCI_RH_SetFeaturePortPower;
    RegPacket.RH_SetFeaturePortEnable = OHCI_RH_SetFeaturePortEnable;
    RegPacket.RH_SetFeaturePortSuspend = OHCI_RH_SetFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnable = OHCI_RH_ClearFeaturePortEnable;
    RegPacket.RH_ClearFeaturePortPower = OHCI_RH_ClearFeaturePortPower;
    RegPacket.RH_ClearFeaturePortSuspend = OHCI_RH_ClearFeaturePortSuspend;
    RegPacket.RH_ClearFeaturePortEnableChange = OHCI_RH_ClearFeaturePortEnableChange;
    RegPacket.RH_ClearFeaturePortConnectChange = OHCI_RH_ClearFeaturePortConnectChange;
    RegPacket.RH_ClearFeaturePortResetChange = OHCI_RH_ClearFeaturePortResetChange;
    RegPacket.RH_ClearFeaturePortSuspendChange = OHCI_RH_ClearFeaturePortSuspendChange;
    RegPacket.RH_ClearFeaturePortOvercurrentChange = OHCI_RH_ClearFeaturePortOvercurrentChange;
    RegPacket.RH_DisableIrq = OHCI_RH_DisableIrq;
    RegPacket.RH_EnableIrq = OHCI_RH_EnableIrq;
    RegPacket.StartSendOnePacket = OHCI_StartSendOnePacket;
    RegPacket.EndSendOnePacket = OHCI_EndSendOnePacket;
    RegPacket.PassThru = OHCI_PassThru;
    RegPacket.FlushInterrupts = OHCI_Unload;

    DriverObject->DriverUnload = (PDRIVER_UNLOAD)OHCI_Unload;

    Status = USBPORT_RegisterUSBPortDriver(DriverObject, 100, &RegPacket);

    DPRINT_OHCI("DriverEntry: USBPORT_RegisterUSBPortDriver return Status - %x\n",
                Status);

    return Status;
}
