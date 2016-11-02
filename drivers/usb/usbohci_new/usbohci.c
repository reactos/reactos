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

ULONG
NTAPI 
OHCI_MapTransferToTD(IN POHCI_EXTENSION OhciExtension,
                     IN ULONG MaxPacketSize,
                     IN OUT ULONG TransferedLen,
                     IN POHCI_TRANSFER OhciTransfer,
                     IN POHCI_HCD_TD TD,
                     IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    PUSBPORT_SCATTER_GATHER_ELEMENT SgElement;
    ULONG SgIdx = 0;
    ULONG SgRemain;
    ULONG LengthThisTd;
    ULONG_PTR BufferEnd;

    DPRINT_OHCI("OHCI_MapTransferToTD: TransferedLen - %x\n", TransferedLen);

    if (SGList->SgElementCount > 0)
    {
        SgElement = &SGList->SgElement[0];

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
        while (SgIdx < SGList->SgElementCount);
    }

    DPRINT_OHCI("OHCI_MapTransferToTD: SgIdx - %x, SgCount - %x\n",
                SgIdx,
                SGList->SgElementCount);

    ASSERT(SgIdx < SGList->SgElementCount);

    if (SGList->Flags & 1)
    {
        ASSERT(FALSE);
    }

    ASSERT(SGList->SgElement[SgIdx].SgOffset == TransferedLen);

    SgRemain = SGList->SgElementCount - SgIdx;

    if (SgRemain == 1)
    {
        LengthThisTd = OhciTransfer->TransferParameters->TransferBufferLength -
                       TransferedLen;

        BufferEnd = SGList->SgElement[SgIdx].SgPhysicalAddress.LowPart +
                    LengthThisTd;
    }
    else if (SgRemain == 2)
    {
        LengthThisTd = OhciTransfer->TransferParameters->TransferBufferLength - 
                       TransferedLen;

        BufferEnd = SGList->SgElement[SgIdx + 1].SgPhysicalAddress.LowPart +
                    SGList->SgElement[SgIdx + 1].SgTransferLength;
    }
    else
    {
        LengthThisTd = SGList->SgElement[SgIdx].SgTransferLength +
                       SGList->SgElement[SgIdx + 1].SgTransferLength;

        BufferEnd = SGList->SgElement[SgIdx + 1].SgPhysicalAddress.LowPart +
                    SGList->SgElement[SgIdx + 1].SgTransferLength;
    }

    TD->HwTD.gTD.CurrentBuffer = (PVOID)SGList->SgElement[SgIdx].SgPhysicalAddress.LowPart;
    TD->HwTD.gTD.BufferEnd = (PVOID)(BufferEnd - 1);

    TD->TransferLen = LengthThisTd;

    return TransferedLen + LengthThisTd;
}

POHCI_HCD_TD
NTAPI 
OHCI_AllocateTD(IN POHCI_EXTENSION OhciExtension,
                IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_TD TD;
    ULONG MaxTDs;
    ULONG ix;

    DPRINT_OHCI("OHCI_AllocateTD: ... \n");

    MaxTDs = OhciEndpoint->MaxTransferDescriptors;

    if (MaxTDs == 0)
    {
        return (POHCI_HCD_TD)-1;
    }

    TD = OhciEndpoint->FirstTD;

    ix = 0;

    while (TD->Flags & OHCI_HCD_TD_FLAG_ALLOCATED)
    {
        ++ix;
        TD += 1;

        if (ix >= MaxTDs)
        {
            return (POHCI_HCD_TD)-1;
        }
    }

    TD->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;

    return TD;
}

ULONG
NTAPI 
OHCI_RemainTDs(IN POHCI_EXTENSION OhciExtension,
               IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_TD TD;
    ULONG MaxTDs;
    ULONG Result;

    DPRINT_OHCI("OHCI_RemainTDs: ... \n");

    MaxTDs = OhciEndpoint->MaxTransferDescriptors;

    if (MaxTDs == 0)
    {
        return 0;
    }

    TD = (POHCI_HCD_TD)OhciEndpoint->FirstTD;

    Result = 0;

    do
    {
        if (!(TD->Flags & OHCI_HCD_TD_FLAG_ALLOCATED))
        {
            ++Result;
        }

        TD += 1;
        --MaxTDs;
    }
    while (MaxTDs > 0);

    return Result;
}

ULONG
NTAPI
OHCI_ControlTransfer(IN POHCI_EXTENSION OhciExtension,
                     IN POHCI_ENDPOINT OhciEndpoint,
                     IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                     IN POHCI_TRANSFER OhciTransfer,
                     IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    POHCI_HCD_TD FirstTD;
    POHCI_HCD_TD FirstTdPA;
    POHCI_HCD_TD TD;
    POHCI_HCD_TD NextTD;
    POHCI_HCD_TD LastTd;
    ULONG_PTR SetupPacket;
    ULONG MaxTDs;
    ULONG TransferedLen;
    UCHAR DataToggle;
    ULONG MaxPacketSize;

    DPRINT_OHCI("OHCI_ControlTransfer: ... \n");

    MaxTDs = OHCI_RemainTDs(OhciExtension, OhciEndpoint);

    if (SGList->SgElementCount + 2 > MaxTDs)
    {
        return 1;
    }

    FirstTD = OhciEndpoint->HcdTailP;

    FirstTD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
    FirstTD->HwTD.gTD.CurrentBuffer = NULL;
    FirstTD->HwTD.gTD.BufferEnd = NULL;
    FirstTD->HwTD.gTD.NextTD = 0;

    RtlCopyMemory(&FirstTD->HwTD.SetupPacket,
                  &TransferParameters->SetupPacket,
                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    FirstTD->HwTD.Padded[0] = 0;
    FirstTD->HwTD.Padded[1] = 0;

    FirstTD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
    FirstTD->OhciTransfer = OhciTransfer;
    FirstTD->NextHcdTD = NULL;

    FirstTD->HwTD.gTD.Control.AsULONG = 0;
    FirstTD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
    FirstTD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;
    FirstTD->HwTD.gTD.Control.DataToggle = 2;

    FirstTdPA = (POHCI_HCD_TD)FirstTD->PhysicalAddress;
    SetupPacket = (ULONG_PTR)&FirstTdPA->HwTD.SetupPacket;

    FirstTD->HwTD.gTD.CurrentBuffer = (PVOID)SetupPacket;
    FirstTD->HwTD.gTD.BufferEnd = (PVOID)(SetupPacket +
                                          sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) -
                                          1);

    TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
    ++OhciTransfer->PendingTDs;

    TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
    TD->HwTD.gTD.CurrentBuffer = NULL;
    TD->HwTD.gTD.BufferEnd = NULL;
    TD->HwTD.gTD.NextTD = NULL;

    RtlZeroMemory(&TD->HwTD.SetupPacket,
                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    TD->HwTD.Padded[0] = 0;
    TD->HwTD.Padded[1] = 0;

    TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
    TD->OhciTransfer = OhciTransfer;
    TD->NextHcdTD = NULL;

    FirstTD->HwTD.gTD.NextTD = (PULONG)(TD->PhysicalAddress);
    FirstTD->NextHcdTD = TD;

    LastTd = FirstTD;

    if (TransferParameters->TransferBufferLength > 0)
    {
        DataToggle = 3;
        TransferedLen = 0;

        do
        {
            if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            {
                TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_IN;
            }
            else
            {
                TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_OUT;
            }

            TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
            TD->HwTD.gTD.Control.DataToggle = DataToggle;
            TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;

            DataToggle = 0;

            MaxPacketSize = OhciEndpoint->EndpointProperties.TotalMaxPacketSize;

            TransferedLen = OHCI_MapTransferToTD(OhciExtension,
                                                 MaxPacketSize,
                                                 TransferedLen,
                                                 OhciTransfer,
                                                 TD,
                                                 SGList);

            LastTd = TD;

            TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
            ++OhciTransfer->PendingTDs;

            TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
            TD->HwTD.gTD.CurrentBuffer = NULL;
            TD->HwTD.gTD.BufferEnd = NULL;
            TD->HwTD.gTD.NextTD = NULL;

            RtlZeroMemory(&TD->HwTD.SetupPacket,
                          sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

            TD->HwTD.Padded[0] = 0;
            TD->HwTD.Padded[1] = 0;

            TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
            TD->OhciTransfer = OhciTransfer;
            TD->NextHcdTD = NULL;

            LastTd->HwTD.gTD.NextTD = (PVOID)TD->PhysicalAddress;
            LastTd->NextHcdTD = TD;
        }
        while (TransferedLen < TransferParameters->TransferBufferLength);
    }

    if (TransferParameters->TransferFlags & USBD_SHORT_TRANSFER_OK)
    {
        LastTd->HwTD.gTD.Control.DelayInterrupt |= 4;
        OhciTransfer->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;
    }

    TD->HwTD.gTD.Control.AsULONG = 0;
    TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;
    TD->HwTD.gTD.Control.DataToggle = 3;
    TD->HwTD.gTD.CurrentBuffer = NULL;
    TD->HwTD.gTD.BufferEnd = NULL;

    TD->Flags |= OHCI_HCD_TD_FLAG_CONTROLL;
    TD->TransferLen = 0;

    if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        TD->HwTD.gTD.Control.BufferRounding = FALSE;
        TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_OUT;
    }
    else
    {
        TD->HwTD.gTD.Control.BufferRounding = TRUE;
        TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_IN;
    }

    NextTD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
    ++OhciTransfer->PendingTDs;

    TD->HwTD.gTD.NextTD = (PULONG)(NextTD->PhysicalAddress);
    TD->NextHcdTD = NextTD;

    NextTD->HwTD.gTD.NextTD = NULL;
    NextTD->NextHcdTD = NULL;

    OhciTransfer->NextTD = NextTD;
    OhciEndpoint->HcdTailP = NextTD;
    OhciEndpoint->HcdED->HwED.TailPointer = (ULONG_PTR)NextTD->PhysicalAddress;

    OHCI_EnableList(OhciExtension, OhciEndpoint);

    return 0;
}

ULONG
NTAPI 
OHCI_BulkOrInterruptTransfer(IN POHCI_EXTENSION OhciExtension,
                             IN POHCI_ENDPOINT OhciEndpoint,
                             IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                             IN POHCI_TRANSFER OhciTransfer,
                             IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    POHCI_HCD_TD TD;
    POHCI_HCD_TD PrevTD;
    ULONG TransferedLen;
    ULONG MaxTDs;
    ULONG MaxPacketSize;

    DPRINT_OHCI("OHCI_BulkOrInterruptTransfer: ... \n");

    MaxTDs = OHCI_RemainTDs(OhciExtension, OhciEndpoint);

    if (SGList->SgElementCount > MaxTDs)
    {
        return 1;
    }

    TD = OhciEndpoint->HcdTailP;

    TransferedLen = 0;

    do
    {
        TD->HwTD.gTD.Control.AsULONG = 0;
        TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
        TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;

        if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
        {
            TD->HwTD.gTD.Control.BufferRounding = FALSE;
            TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_IN;
        }
        else
        {
            TD->HwTD.gTD.Control.BufferRounding = TRUE;
            TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_OUT;
        }

        TD->HwTD.gTD.CurrentBuffer = NULL;
        TD->HwTD.gTD.NextTD = NULL;
        TD->HwTD.gTD.BufferEnd = NULL;

        RtlZeroMemory(&TD->HwTD.SetupPacket,
                      sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

        TD->HwTD.Padded[0] = 0;
        TD->HwTD.Padded[1] = 0;

        TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
        TD->OhciTransfer = OhciTransfer;
        TD->NextHcdTD = NULL;

        if (TransferParameters->TransferBufferLength)
        {
            MaxPacketSize = OhciEndpoint->EndpointProperties.TotalMaxPacketSize;

            TransferedLen = OHCI_MapTransferToTD(OhciExtension,
                                                 MaxPacketSize,
                                                 TransferedLen,
                                                 OhciTransfer,
                                                 TD,
                                                 SGList);
        }
        else
        {
            ASSERT(SGList->SgElementCount == 0);

            TD->HwTD.gTD.CurrentBuffer = NULL;
            TD->HwTD.gTD.BufferEnd = NULL;

            TD->TransferLen = 0;
        }

        PrevTD = TD;

        TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
        ++OhciTransfer->PendingTDs;

        PrevTD->HwTD.gTD.NextTD = (PULONG)TD->PhysicalAddress;
        PrevTD->NextHcdTD = TD;
    }
    while (TransferedLen < TransferParameters->TransferBufferLength);

    if (TransferParameters->TransferFlags & USBD_SHORT_TRANSFER_OK)
    {
        PrevTD->HwTD.gTD.Control.BufferRounding = TRUE;
        OhciTransfer->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;
    }

    PrevTD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_IMMEDIATE;
    PrevTD->HwTD.gTD.NextTD = (PULONG)TD->PhysicalAddress;
    PrevTD->NextHcdTD = TD;

    TD->HwTD.gTD.NextTD = NULL;
    TD->NextHcdTD = NULL;

    OhciTransfer->NextTD = TD;
    OhciEndpoint->HcdTailP = TD;
    OhciEndpoint->HcdED->HwED.TailPointer = (ULONG_PTR)TD->PhysicalAddress;

    OHCI_EnableList(OhciExtension, OhciEndpoint);

    return 0;
}

MPSTATUS
NTAPI
OHCI_SubmitTransfer(IN PVOID ohciExtension,
                    IN PVOID ohciEndpoint,
                    IN PVOID transferParameters,
                    IN PVOID ohciTransfer,
                    IN PVOID sgList)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    POHCI_TRANSFER OhciTransfer;
    PUSBPORT_SCATTER_GATHER_LIST SGList;
    ULONG TransferType;

    DPRINT_OHCI("OHCI_SubmitTransfer: ... \n");

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;
    TransferParameters = (PUSBPORT_TRANSFER_PARAMETERS)transferParameters;
    OhciTransfer = (POHCI_TRANSFER)ohciTransfer;
    SGList = (PUSBPORT_SCATTER_GATHER_LIST)sgList;

    RtlZeroMemory(OhciTransfer, sizeof(OHCI_TRANSFER));

    OhciTransfer->TransferParameters = TransferParameters;
    OhciTransfer->OhciEndpoint = OhciEndpoint;

    TransferType = OhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        return OHCI_ControlTransfer(OhciExtension,
                                    OhciEndpoint,
                                    TransferParameters,
                                    OhciTransfer,
                                    SGList);
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        return OHCI_BulkOrInterruptTransfer(OhciExtension,
                                            OhciEndpoint,
                                            TransferParameters,
                                            OhciTransfer,
                                            SGList);
    }

    return 1;
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
OHCI_ProcessDoneTD(IN POHCI_EXTENSION OhciExtension,
                   IN POHCI_HCD_TD TD,
                   IN BOOLEAN IsPortComplete)
{
    POHCI_TRANSFER OhciTransfer;
    POHCI_ENDPOINT OhciEndpoint;
    ULONG_PTR Buffer;
    ULONG_PTR BufferEnd;
    ULONG Length;

    DPRINT_OHCI("OHCI_ProcessDoneTD: ... \n");

    OhciTransfer = TD->OhciTransfer;
    OhciEndpoint = OhciTransfer->OhciEndpoint;

    --OhciTransfer->PendingTDs;

    Buffer = (ULONG_PTR)TD->HwTD.gTD.CurrentBuffer;
    BufferEnd = (ULONG_PTR)TD->HwTD.gTD.BufferEnd;

    if (TD->Flags & 0x10)
    {
        TD->HwTD.gTD.Control.ConditionCode = 0;
    }
    else
    {
        if (TD->HwTD.gTD.CurrentBuffer)
        {
            if (TD->TransferLen)
            {
                Length = (((Buffer ^ BufferEnd) & 0xFFFFF000) != 0 ? 0x1000 : 0) +
                         (BufferEnd & 0xFFF) + 1 - (Buffer & 0xFFF);

                TD->TransferLen -= Length;
            }
        }

        if (TD->HwTD.gTD.Control.DirectionPID != OHCI_TD_DIRECTION_PID_SETUP)
        {
            OhciTransfer->TransferLen += TD->TransferLen;
        }

        if (TD->HwTD.gTD.Control.ConditionCode)
        {
            OhciTransfer->USBDStatus = USBD_STATUS_HALTED |
                                       TD->HwTD.gTD.Control.ConditionCode;
        }
    }

    TD->Flags = 0;
    TD->HwTD.gTD.NextTD = NULL;
    TD->OhciTransfer = NULL;

    TD->DoneLink.Flink = NULL;
    TD->DoneLink.Blink = NULL;

    if (IsPortComplete && (OhciTransfer->PendingTDs == 0))
    {
        RegPacket.UsbPortCompleteTransfer(OhciExtension,
                                          OhciEndpoint,
                                          OhciTransfer->TransferParameters,
                                          OhciTransfer->USBDStatus,
                                          OhciTransfer->TransferLen);
    }
}

VOID
NTAPI
OHCI_ProcessDoneIsoTD(IN POHCI_EXTENSION OhciExtension,
                      IN POHCI_HCD_TD TD,
                      IN BOOLEAN IsPortComplete)
{
    DPRINT1("OHCI_ProcessDoneIsoTD: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_AbortTransfer(IN PVOID ohciExtension,
                   IN PVOID ohciEndpoint,
                   IN PVOID ohciTransfer,
                   IN OUT PULONG CompletedLength)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_TRANSFER OhciTransfer;
    POHCI_HCD_ED ED;
    ULONG_PTR NextTdPA;
    POHCI_HCD_TD NextTD;
    POHCI_HCD_TD TD;
    POHCI_HCD_TD PrevTD;
    POHCI_HCD_TD LastTD;
    POHCI_HCD_TD td = 0;
    ULONG ix;
    BOOLEAN IsIsoEndpoint = FALSE;
    BOOLEAN IsProcessed = FALSE;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;
    OhciTransfer = (POHCI_TRANSFER)ohciTransfer;

    DPRINT("OHCI_AbortTransfer: ohciEndpoint - %p, ohciTransfer - %p\n",
           OhciEndpoint,
           OhciTransfer);

    if (OhciEndpoint->EndpointProperties.TransferType ==
        USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        IsIsoEndpoint = TRUE;
    }

    ED = OhciEndpoint->HcdED;

    NextTdPA = ED->HwED.HeadPointer & 0xFFFFFFF0; // physical pointer to the next TD

    NextTD = RegPacket.UsbPortGetMappedVirtualAddress((PVOID)NextTdPA,
                                                       OhciExtension,
                                                       OhciEndpoint);

    if (NextTD->OhciTransfer == OhciTransfer)
    {
        LastTD = OhciTransfer->NextTD;

        ED->HwED.HeadPointer = LastTD->PhysicalAddress |
                               (ED->HwED.HeadPointer & 2);

        OhciEndpoint->HcdHeadP = LastTD;

        if (OhciEndpoint->MaxTransferDescriptors != 0)
        {
            TD = OhciEndpoint->FirstTD;
            ix = 0;

            do
            {
                if (TD->OhciTransfer == OhciTransfer)
                {
                    if (IsIsoEndpoint)
                    {
                        OHCI_ProcessDoneIsoTD(OhciExtension, TD, FALSE);
                    }
                    else
                    {
                        OHCI_ProcessDoneTD(OhciExtension, TD, FALSE);
                    }
                }

                TD += 1;
                ++ix;
            }
            while (ix < OhciEndpoint->MaxTransferDescriptors);
        }

        *CompletedLength = OhciTransfer->TransferLen;

        return;
    }

    TD = OhciEndpoint->HcdHeadP;

    if (TD != NextTD)
    {
        do
        {
            if (TD->OhciTransfer == ohciTransfer)
            {
                PrevTD = TD;
                TD = TD->NextHcdTD;

                if (PrevTD == OhciEndpoint->HcdHeadP)
                {
                    OhciEndpoint->HcdHeadP = TD;
                }

                if (IsIsoEndpoint)
                {
                    OHCI_ProcessDoneIsoTD(OhciExtension, PrevTD, FALSE);
                }
                else
                {
                    OHCI_ProcessDoneTD(OhciExtension, PrevTD, FALSE);
                }

                IsProcessed = TRUE;
            }
            else
            {
                TD = TD->NextHcdTD;
            }
        }
        while (TD != NextTD);

        if (!IsProcessed)
        {
            TD = OhciEndpoint->HcdHeadP;

            LastTD = TD;
            td = NULL;

            while (TD != OhciEndpoint->HcdTailP)
            {
                if (TD->OhciTransfer == OhciTransfer)
                {
                    td = TD;
                    break;
                }

                LastTD = TD;

                TD = TD->NextHcdTD;
            }

            TD = td;

            do
            {
                if (TD == OhciEndpoint->HcdTailP)
                {
                    break;
                }

                PrevTD = TD;
                TD = TD->NextHcdTD;

                if (IsIsoEndpoint)
                {
                    OHCI_ProcessDoneIsoTD(OhciExtension, PrevTD, FALSE);
                }
                else
                {
                    OHCI_ProcessDoneTD(OhciExtension, PrevTD, FALSE);
                }
            }
            while (TD->OhciTransfer == OhciTransfer);

            LastTD->OhciTransfer->NextTD = TD;
            LastTD->NextHcdTD = TD;
            LastTD->HwTD.gTD.NextTD = (PULONG)TD->PhysicalAddress;
        }
    }

    *CompletedLength = OhciTransfer->TransferLen;

    if (OhciTransfer->TransferLen)
    {
        DPRINT("OHCI_AbortTransfer: *CompletedLength - %x\n", *CompletedLength);
    }
}

ULONG
NTAPI
OHCI_GetEndpointState(IN PVOID ohciExtension,
                      IN PVOID ohciEndpoint)
{
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_GetEndpointState: ... \n");

    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;
    ED = OhciEndpoint->HcdED;

    if (ED->Flags & 0x10)
    {
        return USBPORT_ENDPOINT_CLOSED;
    }

    if (ED->HwED.EndpointControl.sKip)
    {
        return USBPORT_ENDPOINT_PAUSED;
    }

    return USBPORT_ENDPOINT_ACTIVE;
}

VOID
NTAPI
OHCI_RemoveEndpointFromSchedule(IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;
    POHCI_HCD_ED PreviousED;
    POHCI_STATIC_ED HeadED;

    DPRINT("OHCI_RemoveEndpointFromSchedule \n");

    ED = OhciEndpoint->HcdED;
    HeadED = OhciEndpoint->HeadED;

    if (&HeadED->Link == ED->HcdEDLink.Blink)
    {
        if (ED->Flags & 0x20)
        {
            WRITE_REGISTER_ULONG((PULONG)HeadED->pNextED, ED->HwED.NextED);
        }
        else
        {
            *HeadED->pNextED = ED->HwED.NextED;
        }
    }
    else
    {
        PreviousED = CONTAINING_RECORD(ED->HcdEDLink.Blink,
                                       OHCI_HCD_ED,
                                       HcdEDLink);

        PreviousED->HwED.NextED = ED->HwED.NextED;
    }

    RemoveEntryList(&ED->HcdEDLink);

    OhciEndpoint->HeadED = NULL;
}

VOID
NTAPI
OHCI_SetEndpointState(IN PVOID ohciExtension,
                      IN PVOID ohciEndpoint,
                      IN ULONG EndpointState)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_SetEndpointState: EndpointState - %x\n",
                EndpointState);

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;

    ED = OhciEndpoint->HcdED;

    switch (EndpointState)
    {
        case USBPORT_ENDPOINT_PAUSED:
            ED->HwED.EndpointControl.sKip = 1;
            break;

        case USBPORT_ENDPOINT_ACTIVE:
            ED->HwED.EndpointControl.sKip = 0;
            OHCI_EnableList(OhciExtension, OhciEndpoint);
            break;

        case USBPORT_ENDPOINT_CLOSED:
            ED->HwED.EndpointControl.sKip = 1;
            ED->Flags |= 0x10;
            OHCI_RemoveEndpointFromSchedule(OhciEndpoint);
            break;

        default:
            ASSERT(FALSE);
            break;
    }
}

VOID
NTAPI 
OHCI_PollAsyncEndpoint(IN POHCI_EXTENSION OhciExtension,
                       IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;
    ULONG_PTR NextTdPA;
    POHCI_HCD_TD NextTD;
    POHCI_HCD_TD TD;
    PLIST_ENTRY DoneList;
    POHCI_TRANSFER OhciTransfer;
    UCHAR ConditionCode;
    BOOLEAN IsResetOnHalt = FALSE;

    DPRINT_OHCI("OHCI_PollAsyncEndpoint: OhciEndpoint - %p\n", OhciEndpoint);

    ED = OhciEndpoint->HcdED;
    NextTdPA = ED->HwED.HeadPointer & 0xFFFFFFF0; // physical pointer to the next TD

    if (!NextTdPA)
    {
        DPRINT("OHCI_PollAsyncEndpoint: ED                       - %p\n", ED);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.EndpointControl - %p\n", ED->HwED.EndpointControl.AsULONG);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.TailPointer     - %p\n", ED->HwED.TailPointer);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.HeadPointer     - %p\n", ED->HwED.HeadPointer);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.NextED          - %p\n", ED->HwED.NextED);
        DbgBreakPoint();
    }

    NextTD = (POHCI_HCD_TD)RegPacket.UsbPortGetMappedVirtualAddress((PVOID)NextTdPA,
                                                                    OhciExtension,
                                                                    OhciEndpoint);

    if (ED->HwED.HeadPointer & 1) //Halted FIXME
    {
        DPRINT("OHCI_PollAsyncEndpoint: ED                       - %p\n", ED);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.EndpointControl - %p\n", ED->HwED.EndpointControl.AsULONG);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.TailPointer     - %p\n", ED->HwED.TailPointer);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.HeadPointer     - %p\n", ED->HwED.HeadPointer);
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.NextED          - %p\n", ED->HwED.NextED);

        IsResetOnHalt = (ED->Flags & OHCI_HCD_ED_FLAG_RESET_ON_HALT) != 0;
        DPRINT("OHCI_PollAsyncEndpoint: IsResetOnHalt - %x\n", IsResetOnHalt);

        TD = OhciEndpoint->HcdHeadP;

        while (TRUE)
        {
            DPRINT("OHCI_PollAsyncEndpoint: TD - %p, NextTD - %p\n", TD, NextTD);

            if (!TD)
            {
                DPRINT("OHCI_PollAsyncEndpoint: ED                       - %p\n", ED);
                DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.EndpointControl - %p\n", ED->HwED.EndpointControl.AsULONG);
                DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.TailPointer     - %p\n", ED->HwED.TailPointer);
                DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.HeadPointer     - %p\n", ED->HwED.HeadPointer);
                DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.NextED          - %p\n", ED->HwED.NextED);
                DbgBreakPoint();
            }

            if (TD == NextTD)
            {
                goto HandleDoneList;
            }

            OhciTransfer = TD->OhciTransfer;
            ConditionCode = TD->HwTD.gTD.Control.ConditionCode;

            DPRINT("OHCI_PollAsyncEndpoint: TD - %p, ConditionCode - %x\n",
                   TD,
                   ConditionCode);

            switch (ConditionCode)
            {
                case OHCI_TD_CONDITION_NO_ERROR:
                    TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                    InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                    break;

                case OHCI_TD_CONDITION_NOT_ACCESSED:
                    TD->Flags |= (OHCI_HCD_TD_FLAG_DONE | 0x10);
                    InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                    break;

                case OHCI_TD_CONDITION_DATA_UNDERRUN:
                    DPRINT1("OHCI_TD_CONDITION_DATA_UNDERRUN. Transfer->Flags - %x\n",
                            OhciTransfer->Flags);

                    if (OhciTransfer->Flags & 1)
                    {
                        IsResetOnHalt = 1;
                        TD->HwTD.gTD.Control.ConditionCode = 0;

                        if (TD->Flags & OHCI_HCD_TD_FLAG_CONTROLL)
                        {
                            ASSERT(FALSE);
                        }
                        else
                        {
                            ASSERT(FALSE);
                        }

                        TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                        InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                        break;
                    }

                    /* fall through */

                default:
                    TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                    InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);

                    ED->HwED.HeadPointer = OhciTransfer->NextTD->PhysicalAddress |
                                           (ED->HwED.HeadPointer & 0xF);

                    NextTD = OhciTransfer->NextTD;
                    break;
            }

            TD = TD->NextHcdTD;
        }
    }

    TD = OhciEndpoint->HcdHeadP;

    while (TD != NextTD)
    {
        TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
        InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
        TD = TD->NextHcdTD;
    }

HandleDoneList:

    TD = NextTD;
    OhciEndpoint->HcdHeadP = NextTD;

    DoneList = &OhciEndpoint->TDList;

    while (!IsListEmpty(DoneList))
    {
        TD = CONTAINING_RECORD(DoneList->Flink,
                               OHCI_HCD_TD,
                               DoneLink);

        RemoveHeadList(DoneList);

        if (TD->Flags & OHCI_HCD_TD_FLAG_DONE &&
            TD->Flags & OHCI_HCD_TD_FLAG_PROCESSED)
        {
            OHCI_ProcessDoneTD(OhciExtension, TD, TRUE);
        }
    }

    if (IsResetOnHalt)
    {
        ED->HwED.HeadPointer &= ~1;
        DPRINT("OHCI_PollAsyncEndpoint: ED->HwED.HeadPointer - %p\n",
               ED->HwED.HeadPointer);
    }
}

VOID
NTAPI 
OHCI_PollIsoEndpoint(IN POHCI_EXTENSION OhciExtension,
                     IN POHCI_ENDPOINT OhciEndpoint)
{
    DPRINT1("OHCI_PollAsyncEndpoint: UNIMPLEMENTED. FIXME \n");
    ASSERT(FALSE);
}

VOID
NTAPI
OHCI_PollEndpoint(IN PVOID ohciExtension,
                  IN PVOID ohciEndpoint)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_ENDPOINT OhciEndpoint;
    ULONG TransferType;

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OhciEndpoint = (POHCI_ENDPOINT)ohciEndpoint;

    DPRINT_OHCI("OHCI_PollEndpoint: OhciExtension - %p, Endpoint - %p\n",
                OhciExtension,
                OhciEndpoint);

    TransferType = OhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        OHCI_PollIsoEndpoint(OhciExtension, OhciEndpoint);
    }
    else
    {
        if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
            TransferType == USBPORT_TRANSFER_TYPE_BULK ||
            TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
        {
            OHCI_PollAsyncEndpoint(OhciExtension, OhciEndpoint);
        }
    }
}

VOID
NTAPI
OHCI_CheckController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_CONTROL HcControl;
    ULONG FmNumber;
    USHORT FmDiff;
    POHCI_HCCA HcHCCA;

    //DPRINT_OHCI("OHCI_CheckController: ...\n");

    OhciExtension = (POHCI_EXTENSION)ohciExtension;
    OperationalRegs = OhciExtension->OperationalRegs;

    if (!OHCI_HardwarePresent(OhciExtension, TRUE))
    {
        return;
    }

    HcControl.AsULONG = READ_REGISTER_ULONG(&OperationalRegs->HcControl.AsULONG);

    if (HcControl.HostControllerFunctionalState != OHCI_HC_STATE_OPERATIONAL)
    {
        return;
    }

    FmNumber = READ_REGISTER_ULONG(&OperationalRegs->HcFmNumber);
    FmDiff = (USHORT)(FmNumber - OhciExtension->HcdFmNumber);

    if (FmNumber == 0 || FmDiff < 5)
    {
        return;
    }

    HcHCCA = (POHCI_HCCA)OhciExtension->HcResourcesVA;
    OhciExtension->HcdFmNumber = FmNumber;


    if (HcHCCA->Pad1)
    {
        DPRINT1("OHCI_CheckController: HcHCCA->Pad1 - %x\n",
                HcHCCA->Pad1);

        if (HcHCCA->Pad1 == 0xBAD1)
        {
            HcHCCA->Pad1 = 0xBAD2;
        }
        else if (HcHCCA->Pad1 == 0xBAD2)
        {
            HcHCCA->Pad1 = 0xBAD3;
            RegPacket.UsbPortInvalidateController(OhciExtension, 1);
        }
    }
    else
    {
        HcHCCA->Pad1 = 0xBAD1;
    }
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
