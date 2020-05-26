/*
 * PROJECT:     ReactOS USB OHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBOHCI main driver functions
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbohci.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_OHCI_TRACE
#include "dbg_ohci.h"

USBPORT_REGISTRATION_PACKET RegPacket;

static const UCHAR Index[8] =
{
    ENDPOINT_INTERRUPT_1ms - 1,
    ENDPOINT_INTERRUPT_2ms - 1,
    ENDPOINT_INTERRUPT_4ms - 1,
    ENDPOINT_INTERRUPT_8ms - 1,
    ENDPOINT_INTERRUPT_16ms - 1,
    ENDPOINT_INTERRUPT_32ms - 1,
    ENDPOINT_INTERRUPT_32ms - 1,
    ENDPOINT_INTERRUPT_32ms - 1
};

static const UCHAR Balance[OHCI_NUMBER_OF_INTERRUPTS] =
{
    0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
    1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31
};

VOID
NTAPI
OHCI_DumpHcdED(POHCI_HCD_ED ED)
{
    DPRINT("ED                - %p\n", ED);
    DPRINT("EndpointControl   - %X\n", ED->HwED.EndpointControl.AsULONG);
    DPRINT("TailPointer       - %08X\n", ED->HwED.TailPointer);
    DPRINT("HeadPointer       - %08X\n", ED->HwED.HeadPointer);
    DPRINT("NextED            - %08X\n", ED->HwED.NextED);
}

VOID
NTAPI
OHCI_DumpHcdTD(POHCI_HCD_TD TD)
{
    DPRINT("TD                - %p\n", TD);
    DPRINT("gTD.Control       - %08X\n", TD->HwTD.gTD.Control.AsULONG);
if (TD->HwTD.gTD.CurrentBuffer)
    DPRINT("gTD.CurrentBuffer - %08X\n", TD->HwTD.gTD.CurrentBuffer);
if (TD->HwTD.gTD.NextTD)
    DPRINT("gTD.NextTD        - %08X\n", TD->HwTD.gTD.NextTD);
if (TD->HwTD.gTD.BufferEnd)
    DPRINT("gTD.BufferEnd     - %08X\n", TD->HwTD.gTD.BufferEnd);

if (TD->HwTD.SetupPacket.bmRequestType.B)
    DPRINT("bmRequestType     - %02X\n", TD->HwTD.SetupPacket.bmRequestType.B);
if (TD->HwTD.SetupPacket.bRequest)
    DPRINT("bRequest          - %02X\n", TD->HwTD.SetupPacket.bRequest);
if (TD->HwTD.SetupPacket.wValue.W)
    DPRINT("wValue            - %04X\n", TD->HwTD.SetupPacket.wValue.W);
if (TD->HwTD.SetupPacket.wIndex.W)
    DPRINT("wIndex            - %04X\n", TD->HwTD.SetupPacket.wIndex.W);
if (TD->HwTD.SetupPacket.wLength)
    DPRINT("wLength           - %04X\n", TD->HwTD.SetupPacket.wLength);

    DPRINT("PhysicalAddress   - %p\n", TD->PhysicalAddress);
    DPRINT("Flags             - %X\n", TD->Flags);
    DPRINT("OhciTransfer      - %08X\n", TD->OhciTransfer);
    DPRINT("NextTDVa          - %08X\n", TD->NextTDVa);
if (TD->TransferLen)
    DPRINT("TransferLen       - %X\n", TD->TransferLen);
}

VOID
NTAPI
OHCI_EnableList(IN POHCI_EXTENSION OhciExtension,
                IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG CommandStatusReg;
    ULONG TransferType;
    OHCI_REG_COMMAND_STATUS CommandStatus;

    DPRINT_OHCI("OHCI_EnableList: ... \n");

    OperationalRegs = OhciExtension->OperationalRegs;
    CommandStatusReg = (PULONG)&OperationalRegs->HcCommandStatus;

    CommandStatus.AsULONG = 0;

    if (READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcControlHeadED))
        CommandStatus.ControlListFilled = 1;

    if (READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcBulkHeadED))
        CommandStatus.BulkListFilled = 1;

    TransferType = OhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_BULK)
        CommandStatus.BulkListFilled = 1;
    else if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
        CommandStatus.ControlListFilled = 1;

    WRITE_REGISTER_ULONG(CommandStatusReg, CommandStatus.AsULONG);
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

        if (HeadED->Type == OHCI_STATIC_ED_TYPE_CONTROL ||
            HeadED->Type == OHCI_STATIC_ED_TYPE_BULK)
        {
            ED->HwED.NextED = READ_REGISTER_ULONG(HeadED->pNextED);
            WRITE_REGISTER_ULONG(HeadED->pNextED, ED->PhysicalAddress);
        }
        else if (HeadED->Type == OHCI_STATIC_ED_TYPE_INTERRUPT)
        {
            ED->HwED.NextED = *HeadED->pNextED;
            *HeadED->pNextED = ED->PhysicalAddress;
        }
        else
        {
            DPRINT1("OHCI_InsertEndpointInSchedule: Unknown HeadED->Type - %x\n",
                    HeadED->Type);
            DbgBreakPoint();
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

    EndpointControl = ED->HwED.EndpointControl;

    EndpointControl.FunctionAddress = EndpointProperties->DeviceAddress;
    EndpointControl.EndpointNumber = EndpointProperties->EndpointAddress;
    EndpointControl.MaximumPacketSize = EndpointProperties->TotalMaxPacketSize;

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

    if (EndpointProperties->DeviceSpeed == UsbLowSpeed)
        EndpointControl.Speed = OHCI_ENDPOINT_LOW_SPEED;

    if (EndpointProperties->TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        EndpointControl.Format = OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;
    else
        EndpointControl.sKip = 1;

    ED->HwED.EndpointControl = EndpointControl;

    ED->HwED.TailPointer = FirstTD->PhysicalAddress;
    ED->HwED.HeadPointer = FirstTD->PhysicalAddress;

    FirstTD->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;

    OhciEndpoint->HcdTailP = FirstTD;
    OhciEndpoint->HcdHeadP = FirstTD;

    return ED;
}

VOID
NTAPI
OHCI_InitializeTDs(IN POHCI_ENDPOINT OhciEndpoint,
                   IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties)
{
    POHCI_HCD_TD TdVA;
    ULONG TdPA;
    ULONG TdCount;
    ULONG ix;

    ASSERT(EndpointProperties->BufferLength > sizeof(OHCI_HCD_ED));

    TdCount = (EndpointProperties->BufferLength - sizeof(OHCI_HCD_ED)) /
              sizeof(OHCI_HCD_TD);

    OhciEndpoint->MaxTransferDescriptors = TdCount;

    DPRINT_OHCI("OHCI_InitializeTDs: TdCount - %x\n", TdCount);

    ASSERT(TdCount > 0);

    TdVA = OhciEndpoint->FirstTD;

    TdPA = EndpointProperties->BufferPA + sizeof(OHCI_HCD_ED);

    for (ix = 0; ix < TdCount; ix++)
    {
        DPRINT_OHCI("OHCI_InitializeTDs: TdVA - %p, TdPA - %08X\n", TdVA, TdPA);

        RtlZeroMemory(TdVA, sizeof(OHCI_HCD_TD));

        TdVA->PhysicalAddress = TdPA;
        TdVA->Flags = 0;
        TdVA->OhciTransfer = 0;

        TdVA++;
        TdPA += sizeof(OHCI_HCD_TD);
    }
}

MPSTATUS
NTAPI
OHCI_OpenControlEndpoint(IN POHCI_EXTENSION OhciExtension,
                         IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                         IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_OpenControlEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));
    OhciEndpoint->HeadED = &OhciExtension->ControlStaticED;

    OHCI_InitializeTDs(OhciEndpoint, EndpointProperties);

    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OhciEndpoint->HcdED->Flags = OHCI_HCD_ED_FLAG_CONTROL |
                                 OHCI_HCD_ED_FLAG_RESET_ON_HALT;

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_OpenBulkEndpoint(IN POHCI_EXTENSION OhciExtension,
                      IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                      IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_OpenBulkEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));
    OhciEndpoint->HeadED = &OhciExtension->BulkStaticED;

    OHCI_InitializeTDs(OhciEndpoint, EndpointProperties);

    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_OpenInterruptEndpoint(IN POHCI_EXTENSION OhciExtension,
                           IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                           IN POHCI_ENDPOINT OhciEndpoint)
{
    UCHAR Period;
    ULONG PeriodIdx = 0;
    POHCI_HCD_ED ED;
    ULONG ScheduleOffset;

    DPRINT_OHCI("OHCI_OpenInterruptEndpoint: ... \n");

    ED = (POHCI_HCD_ED)EndpointProperties->BufferVA;

    OhciEndpoint->FirstTD = (POHCI_HCD_TD)((ULONG_PTR)ED + sizeof(OHCI_HCD_ED));

    Period = EndpointProperties->Period;

    ASSERT(Period != 0);

    while (!(Period & 1))
    {
        PeriodIdx++;
        Period >>= 1;
    }

    ASSERT(PeriodIdx < ARRAYSIZE(Index));

    ScheduleOffset = EndpointProperties->ScheduleOffset;
    DPRINT_OHCI("OHCI_OpenInterruptEndpoint: InitTD. Index[PeriodIdx] - %x, ScheduleOffset - %x\n",
                Index[PeriodIdx],
                ScheduleOffset);

    OhciEndpoint->HeadED = &OhciExtension->IntStaticED[Index[PeriodIdx] +
                                                       ScheduleOffset];

    //OhciEndpoint->HeadED->UsbBandwidth += EndpointProperties->UsbBandwidth;

    OHCI_InitializeTDs(OhciEndpoint, EndpointProperties);

    OhciEndpoint->HcdED = OHCI_InitializeED(OhciEndpoint,
                                            ED,
                                            OhciEndpoint->FirstTD,
                                            EndpointProperties->BufferPA);

    OHCI_InsertEndpointInSchedule(OhciEndpoint);

    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_OpenIsoEndpoint(IN POHCI_EXTENSION OhciExtension,
                     IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                     IN POHCI_ENDPOINT OhciEndpoint)
{
    DPRINT1("OHCI_OpenIsoEndpoint: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_NOT_SUPPORTED;
}

MPSTATUS
NTAPI
OHCI_OpenEndpoint(IN PVOID ohciExtension,
                  IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                  IN PVOID ohciEndpoint)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    ULONG TransferType;
    MPSTATUS MPStatus;

    DPRINT_OHCI("OHCI_OpenEndpoint: ... \n");

    RtlCopyMemory(&OhciEndpoint->EndpointProperties,
                  EndpointProperties,
                  sizeof(OhciEndpoint->EndpointProperties));

    InitializeListHead(&OhciEndpoint->TDList);

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            MPStatus = OHCI_OpenIsoEndpoint(OhciExtension,
                                            EndpointProperties,
                                            OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            MPStatus = OHCI_OpenControlEndpoint(OhciExtension,
                                                EndpointProperties,
                                                OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            MPStatus = OHCI_OpenBulkEndpoint(OhciExtension,
                                             EndpointProperties,
                                             OhciEndpoint);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            MPStatus = OHCI_OpenInterruptEndpoint(OhciExtension,
                                                  EndpointProperties,
                                                  OhciEndpoint);
            break;

        default:
            MPStatus = MP_STATUS_NOT_SUPPORTED;
            break;
    }

    return MPStatus;
}

MPSTATUS
NTAPI
OHCI_ReopenEndpoint(IN PVOID ohciExtension,
                    IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                    IN PVOID ohciEndpoint)
{
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_ReopenEndpoint: ... \n");

    ED = OhciEndpoint->HcdED;

    RtlCopyMemory(&OhciEndpoint->EndpointProperties,
                  EndpointProperties,
                  sizeof(OhciEndpoint->EndpointProperties));

    ED->HwED.EndpointControl.FunctionAddress =
        OhciEndpoint->EndpointProperties.DeviceAddress;

    ED->HwED.EndpointControl.MaximumPacketSize =
        OhciEndpoint->EndpointProperties.TotalMaxPacketSize;

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_QueryEndpointRequirements(IN PVOID ohciExtension,
                               IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties,
                               IN PUSBPORT_ENDPOINT_REQUIREMENTS EndpointRequirements)
{
    ULONG TransferType;

    DPRINT_OHCI("OHCI_QueryEndpointRequirements: ... \n");

    TransferType = EndpointProperties->TransferType;

    switch (TransferType)
    {
        case USBPORT_TRANSFER_TYPE_ISOCHRONOUS:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: IsoTransfer\n");
            EndpointRequirements->MaxTransferSize = OHCI_MAX_ISO_TRANSFER_SIZE;
            EndpointRequirements->HeaderBufferSize =
            sizeof(OHCI_HCD_ED) + OHCI_MAX_ISO_TD_COUNT * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_CONTROL:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: ControlTransfer\n");
            EndpointRequirements->MaxTransferSize = OHCI_MAX_CONTROL_TRANSFER_SIZE;
            EndpointRequirements->HeaderBufferSize =
            sizeof(OHCI_HCD_ED) + OHCI_MAX_CONTROL_TD_COUNT * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_BULK:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: BulkTransfer\n");
            EndpointRequirements->MaxTransferSize = OHCI_MAX_BULK_TRANSFER_SIZE;
            EndpointRequirements->HeaderBufferSize =
            sizeof(OHCI_HCD_ED) + OHCI_MAX_BULK_TD_COUNT * sizeof(OHCI_HCD_TD);
            break;

        case USBPORT_TRANSFER_TYPE_INTERRUPT:
            DPRINT_OHCI("OHCI_QueryEndpointRequirements: InterruptTransfer\n");
            EndpointRequirements->MaxTransferSize = OHCI_MAX_INTERRUPT_TRANSFER_SIZE;
            EndpointRequirements->HeaderBufferSize =
            sizeof(OHCI_HCD_ED) + OHCI_MAX_INTERRUPT_TD_COUNT * sizeof(OHCI_HCD_TD);
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
#if DBG
    DPRINT1("OHCI_CloseEndpoint: Not supported\n");
#endif
    return;
}

MPSTATUS
NTAPI
OHCI_TakeControlHC(IN POHCI_EXTENSION OhciExtension,
                   IN PUSBPORT_RESOURCES Resources)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG ControlReg;
    PULONG InterruptEnableReg;
    PULONG InterruptDisableReg;
    PULONG CommandStatusReg;
    PULONG InterruptStatusReg;
    OHCI_REG_CONTROL Control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntEnable;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntDisable;
    OHCI_REG_COMMAND_STATUS CommandStatus;
    OHCI_REG_INTERRUPT_STATUS IntStatus;
    LARGE_INTEGER StartTicks, CurrentTicks;
    UINT32 TicksDiff;

    DPRINT("OHCI_TakeControlHC: ...\n");

    OperationalRegs = OhciExtension->OperationalRegs;

    ControlReg = (PULONG)&OperationalRegs->HcControl;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;
    InterruptDisableReg = (PULONG)&OperationalRegs->HcInterruptDisable;
    CommandStatusReg = (PULONG)&OperationalRegs->HcCommandStatus;
    InterruptStatusReg = (PULONG)&OperationalRegs->HcInterruptStatus;

    /* 5.1.1.3 Take Control of Host Controller */
    Control.AsULONG = READ_REGISTER_ULONG(ControlReg);

    if (Control.InterruptRouting == 0)
        return MP_STATUS_SUCCESS;

    DPRINT1("OHCI_TakeControlHC: detected Legacy BIOS\n");

    IntEnable.AsULONG = READ_REGISTER_ULONG(InterruptEnableReg);

    DPRINT("OHCI_TakeControlHC: Control - %lX, IntEnable - %lX\n",
           Control.AsULONG,
           IntEnable.AsULONG);

    if (Control.HostControllerFunctionalState == OHCI_HC_STATE_RESET &&
        IntEnable.AsULONG == 0)
    {
        Control.AsULONG = 0;
        WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);
        return MP_STATUS_SUCCESS;
    }

    /* Enable interrupt generations */
    IntEnable.AsULONG = 0;
    IntEnable.MasterInterruptEnable = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, IntEnable.AsULONG);

    /* Request a change of control of the HC */
    CommandStatus.AsULONG = 0;
    CommandStatus.OwnershipChangeRequest = 1;

    WRITE_REGISTER_ULONG(CommandStatusReg, CommandStatus.AsULONG);

    /* Disable interrupt generation due to Root Hub Status Change */
    IntDisable.AsULONG = 0;
    IntDisable.RootHubStatusChange = 1;

    WRITE_REGISTER_ULONG(InterruptDisableReg, IntDisable.AsULONG);

    /* Monitoring the InterruptRouting bit
       to determine when the ownership change has taken effect. */

    TicksDiff = (500 * 10000) / KeQueryTimeIncrement(); // 500 ms
    KeQueryTickCount(&StartTicks);

    do
    {
        Control.AsULONG = READ_REGISTER_ULONG(ControlReg);

        if (Control.InterruptRouting == 0)
        {
            /* Clear all bits in register */
            IntStatus.AsULONG = 0xFFFFFFFF;
            WRITE_REGISTER_ULONG(InterruptStatusReg, IntStatus.AsULONG);

            /* Disable interrupt generations */
            IntDisable.AsULONG = 0;
            IntDisable.MasterInterruptEnable = 1;

            WRITE_REGISTER_ULONG(InterruptDisableReg, IntDisable.AsULONG);

            return MP_STATUS_SUCCESS;
        }

        KeQueryTickCount(&CurrentTicks);
    }
    while (CurrentTicks.QuadPart - StartTicks.QuadPart < TicksDiff);

    return MP_STATUS_HW_ERROR;
}

MPSTATUS
NTAPI
OHCI_StartController(IN PVOID ohciExtension,
                     IN PUSBPORT_RESOURCES Resources)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG CommandStatusReg;
    PULONG FmIntervalReg;
    PULONG ControlReg;
    PULONG InterruptEnableReg;
    PULONG RhStatusReg;
    OHCI_REG_COMMAND_STATUS CommandStatus;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE Interrupts;
    OHCI_REG_RH_STATUS RhStatus;
    OHCI_REG_FRAME_INTERVAL FrameInterval;
    ULONG MaxFrameIntervalAdjusting;
    OHCI_REG_CONTROL Control;
    UCHAR HeadIndex;
    POHCI_ENDPOINT_DESCRIPTOR IntED;
    ULONG_PTR IntEdPA;
    POHCI_HCCA OhciHCCA;
    LARGE_INTEGER StartTicks, CurrentTicks;
    UINT32 TicksDiff;
    ULONG ix;
    ULONG jx;
    MPSTATUS MPStatus = MP_STATUS_SUCCESS;

    DPRINT_OHCI("OHCI_StartController: ohciExtension - %p, Resources - %p\n",
                ohciExtension,
                Resources);

    /* HC on-chip operational registers */
    OperationalRegs = (POHCI_OPERATIONAL_REGISTERS)Resources->ResourceBase;
    OhciExtension->OperationalRegs = OperationalRegs;

    CommandStatusReg = (PULONG)&OperationalRegs->HcCommandStatus;
    FmIntervalReg = (PULONG)&OperationalRegs->HcFmInterval;
    ControlReg = (PULONG)&OperationalRegs->HcControl;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;
    RhStatusReg = (PULONG)&OperationalRegs->HcRhStatus;

    /* 5.1.1 Initialization */

    MPStatus = OHCI_TakeControlHC(OhciExtension, Resources);

    if (MPStatus != MP_STATUS_SUCCESS)
    {
        DPRINT1("OHCI_StartController: OHCI_TakeControlHC return MPStatus - %x\n",
                MPStatus);

        return MPStatus;
    }

    OhciExtension->HcResourcesVA = (POHCI_HC_RESOURCES)Resources->StartVA;
    OhciExtension->HcResourcesPA = Resources->StartPA;

    DPRINT_OHCI("OHCI_StartController: HcResourcesVA - %p, HcResourcesPA - %lx\n",
                OhciExtension->HcResourcesVA,
                OhciExtension->HcResourcesPA);

    /* 5.2.7.2 Interrupt */

    /* Build structure of interrupt static EDs */
    for (ix = 0; ix < INTERRUPT_ENDPOINTs; ix++)
    {
        IntED = &OhciExtension->HcResourcesVA->InterrruptHeadED[ix];
        IntEdPA = OhciExtension->HcResourcesPA + FIELD_OFFSET(OHCI_HC_RESOURCES, InterrruptHeadED[ix]);

        if (ix == (ENDPOINT_INTERRUPT_1ms - 1))
        {
            HeadIndex = ED_EOF;
            IntED->NextED = 0;
        }
        else
        {
            HeadIndex = ((ix - 1) / 2);

            ASSERT(HeadIndex >= (ENDPOINT_INTERRUPT_1ms - 1) &&
                   HeadIndex < (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms));

            IntED->NextED = OhciExtension->IntStaticED[HeadIndex].PhysicalAddress;
        }

        IntED->EndpointControl.sKip = 1;

        IntED->TailPointer = 0;
        IntED->HeadPointer = 0;

        OhciExtension->IntStaticED[ix].HwED = IntED;
        OhciExtension->IntStaticED[ix].PhysicalAddress = IntEdPA;
        OhciExtension->IntStaticED[ix].HeadIndex = HeadIndex;
        OhciExtension->IntStaticED[ix].pNextED = &IntED->NextED;
        OhciExtension->IntStaticED[ix].Type = OHCI_STATIC_ED_TYPE_INTERRUPT;

        InitializeListHead(&OhciExtension->IntStaticED[ix].Link);
    }

    OhciHCCA = &OhciExtension->HcResourcesVA->HcHCCA;
    DPRINT_OHCI("OHCI_InitializeSchedule: OhciHCCA - %p\n", OhciHCCA);

    /* Set head pointers which start from HCCA */
    for (ix = 0, jx = (INTERRUPT_ENDPOINTs - ENDPOINT_INTERRUPT_32ms);
         ix < OHCI_NUMBER_OF_INTERRUPTS;
         ix++, jx++)
    {
        OhciHCCA->InterrruptTable[Balance[ix]] =
            OhciExtension->IntStaticED[jx].PhysicalAddress;

        OhciExtension->IntStaticED[jx].pNextED =
            (PULONG)&OhciHCCA->InterrruptTable[Balance[ix]];

        OhciExtension->IntStaticED[jx].HccaIndex = Balance[ix];
    }

    DPRINT_OHCI("OHCI_InitializeSchedule: ix - %x\n", ix);

    /* Init static Control and Bulk EDs head pointers which start from HCCA */
    InitializeListHead(&OhciExtension->ControlStaticED.Link);

    OhciExtension->ControlStaticED.HeadIndex = ED_EOF;
    OhciExtension->ControlStaticED.Type = OHCI_STATIC_ED_TYPE_CONTROL;
    OhciExtension->ControlStaticED.pNextED = &OperationalRegs->HcControlHeadED;

    InitializeListHead(&OhciExtension->BulkStaticED.Link);

    OhciExtension->BulkStaticED.HeadIndex = ED_EOF;
    OhciExtension->BulkStaticED.Type = OHCI_STATIC_ED_TYPE_BULK;
    OhciExtension->BulkStaticED.pNextED = &OperationalRegs->HcBulkHeadED;

    /* 6.3.1 Frame Timing */
    FrameInterval.AsULONG = READ_REGISTER_ULONG(FmIntervalReg);

    MaxFrameIntervalAdjusting = OHCI_DEFAULT_FRAME_INTERVAL / 10; // 10%

    if ((FrameInterval.FrameInterval) < (OHCI_DEFAULT_FRAME_INTERVAL - MaxFrameIntervalAdjusting) ||
        (FrameInterval.FrameInterval) > (OHCI_DEFAULT_FRAME_INTERVAL + MaxFrameIntervalAdjusting))
    {
        FrameInterval.FrameInterval = OHCI_DEFAULT_FRAME_INTERVAL;
    }

    /* 5.4 FrameInterval Counter */
    FrameInterval.FrameIntervalToggle = 1;

    /* OHCI_MAXIMUM_OVERHEAD is the maximum overhead per frame */
    FrameInterval.FSLargestDataPacket =
        ((FrameInterval.FrameInterval - OHCI_MAXIMUM_OVERHEAD) * 6) / 7;

    OhciExtension->FrameInterval = FrameInterval;

    DPRINT_OHCI("OHCI_StartController: FrameInterval - %lX\n",
                FrameInterval.AsULONG);

    /* Reset HostController */
    CommandStatus.AsULONG = 0;
    CommandStatus.HostControllerReset = 1;

    WRITE_REGISTER_ULONG(CommandStatusReg, CommandStatus.AsULONG);

    KeStallExecutionProcessor(25);

    Control.AsULONG = READ_REGISTER_ULONG(ControlReg);
    Control.HostControllerFunctionalState = OHCI_HC_STATE_RESET;

    WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);

    TicksDiff = (500 * 10000) / KeQueryTimeIncrement(); // 500 ms
    KeQueryTickCount(&StartTicks);

    while (TRUE)
    {
        WRITE_REGISTER_ULONG(FmIntervalReg, OhciExtension->FrameInterval.AsULONG);
        FrameInterval.AsULONG = READ_REGISTER_ULONG(FmIntervalReg);

        KeQueryTickCount(&CurrentTicks);

        if (CurrentTicks.QuadPart - StartTicks.QuadPart >= TicksDiff)
        {
            MPStatus = MP_STATUS_HW_ERROR;
            break;
        }

        if (FrameInterval.AsULONG == OhciExtension->FrameInterval.AsULONG)
        {
            MPStatus = MP_STATUS_SUCCESS;
            break;
        }
    }

    if (MPStatus != MP_STATUS_SUCCESS)
    {
        DPRINT_OHCI("OHCI_StartController: frame interval not set\n");
        return MPStatus;
    }

    /* Setup HcPeriodicStart register */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcPeriodicStart,
                        (OhciExtension->FrameInterval.FrameInterval * 9) / 10); //90%

    /* Setup HcHCCA register */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcHCCA,
                         OhciExtension->HcResourcesPA + FIELD_OFFSET(OHCI_HC_RESOURCES, HcHCCA));

    /* Setup HcInterruptEnable register */
    Interrupts.AsULONG = 0;

    Interrupts.SchedulingOverrun = 1;
    Interrupts.WritebackDoneHead = 1;
    Interrupts.UnrecoverableError = 1;
    Interrupts.FrameNumberOverflow = 1;
    Interrupts.OwnershipChange = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, Interrupts.AsULONG);

    /* Setup HcControl register */
    Control.AsULONG = READ_REGISTER_ULONG(ControlReg);

    Control.ControlBulkServiceRatio = 0; // FIXME (1 : 1)
    Control.PeriodicListEnable = 1;
    Control.IsochronousEnable = 1;
    Control.ControlListEnable = 1;
    Control.BulkListEnable = 1;
    Control.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;

    WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);

    /* Setup HcRhStatus register */
    RhStatus.AsULONG = 0;
    RhStatus.SetGlobalPower = 1;

    WRITE_REGISTER_ULONG(RhStatusReg, RhStatus.AsULONG);

    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_StopController(IN PVOID ohciExtension,
                    IN BOOLEAN IsDoDisableInterrupts)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG ControlReg;
    PULONG InterruptDisableReg;
    PULONG InterruptStatusReg;
    OHCI_REG_CONTROL Control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntDisable;
    OHCI_REG_INTERRUPT_STATUS IntStatus;

    DPRINT("OHCI_StopController: ... \n");

    OperationalRegs = OhciExtension->OperationalRegs;

    ControlReg = (PULONG)&OperationalRegs->HcControl;
    InterruptDisableReg = (PULONG)&OperationalRegs->HcInterruptDisable;
    InterruptStatusReg = (PULONG)&OperationalRegs->HcInterruptStatus;

    /* Setup HcControl register */
    Control.AsULONG = READ_REGISTER_ULONG(ControlReg);

    Control.PeriodicListEnable = 0;
    Control.IsochronousEnable = 0;
    Control.ControlListEnable = 0;
    Control.BulkListEnable = 0;
    Control.HostControllerFunctionalState = OHCI_HC_STATE_SUSPEND;
    Control.RemoteWakeupEnable = 0;

    WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);

    /* Disable interrupt generations */
    IntDisable.AsULONG = 0xFFFFFFFF;
    WRITE_REGISTER_ULONG(InterruptDisableReg, IntDisable.AsULONG);

    /* Clear all bits in HcInterruptStatus register */
    IntStatus.AsULONG = 0xFFFFFFFF;
    WRITE_REGISTER_ULONG(InterruptStatusReg, IntStatus.AsULONG);
}

VOID
NTAPI
OHCI_SuspendController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG ControlReg;
    PULONG InterruptEnableReg;
    OHCI_REG_CONTROL Control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptReg;

    DPRINT("OHCI_SuspendController: ... \n");

    OperationalRegs = OhciExtension->OperationalRegs;
    ControlReg = (PULONG)&OperationalRegs->HcControl;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;

    /* Disable all interrupt generations */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptDisable.AsULONG,
                         0xFFFFFFFF);

    /* Clear all bits in HcInterruptStatus register */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcInterruptStatus.AsULONG,
                         0xFFFFFFFF);

    /* Setup HcControl register */
    Control.AsULONG = READ_REGISTER_ULONG(ControlReg);
    Control.HostControllerFunctionalState = OHCI_HC_STATE_SUSPEND;
    Control.RemoteWakeupEnable =  1;

    WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);

    /* Setup HcInterruptEnable register */
    InterruptReg.AsULONG = 0;
    InterruptReg.ResumeDetected = 1;
    InterruptReg.UnrecoverableError = 1;
    InterruptReg.RootHubStatusChange = 1;
    InterruptReg.MasterInterruptEnable = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, InterruptReg.AsULONG);
}

MPSTATUS
NTAPI
OHCI_ResumeController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG ControlReg;
    PULONG InterruptEnableReg;
    POHCI_HCCA HcHCCA;
    OHCI_REG_CONTROL control;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptReg;

    DPRINT("OHCI_ResumeController \n");

    OperationalRegs = OhciExtension->OperationalRegs;
    ControlReg = (PULONG)&OperationalRegs->HcControl;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;

    control.AsULONG = READ_REGISTER_ULONG(ControlReg);

    if (control.HostControllerFunctionalState != OHCI_HC_STATE_SUSPEND)
        return MP_STATUS_HW_ERROR;

    HcHCCA = &OhciExtension->HcResourcesVA->HcHCCA;
    HcHCCA->Pad1 = 0;

    /* Setup HcControl register */
    control.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;
    WRITE_REGISTER_ULONG(ControlReg, control.AsULONG);

    /* Setup HcInterruptEnable register */
    InterruptReg.AsULONG = 0;
    InterruptReg.SchedulingOverrun = 1;
    InterruptReg.WritebackDoneHead = 1;
    InterruptReg.UnrecoverableError = 1;
    InterruptReg.FrameNumberOverflow = 1;
    InterruptReg.OwnershipChange = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, InterruptReg.AsULONG);
    WRITE_REGISTER_ULONG(ControlReg, control.AsULONG);

    return MP_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
OHCI_HardwarePresent(IN POHCI_EXTENSION OhciExtension,
                     IN BOOLEAN IsInvalidateController)
{
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG CommandStatusReg;

    OperationalRegs = OhciExtension->OperationalRegs;
    CommandStatusReg = (PULONG)&OperationalRegs->HcCommandStatus;

    if (READ_REGISTER_ULONG(CommandStatusReg) != 0xFFFFFFFF)
        return TRUE;

    DPRINT1("OHCI_HardwarePresent: IsInvalidateController - %x\n",
            IsInvalidateController);

    if (IsInvalidateController)
    {
        RegPacket.UsbPortInvalidateController(OhciExtension,
                                              USBPORT_INVALIDATE_CONTROLLER_SURPRISE_REMOVE);
    }

    return FALSE;
}

BOOLEAN
NTAPI
OHCI_InterruptService(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    OHCI_REG_INTERRUPT_STATUS IntStatus;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntEnable;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntDisable;
    BOOLEAN HardwarePresent = FALSE;

    DPRINT_OHCI("OHCI_Interrupt: Ext %p\n", OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;

    HardwarePresent = OHCI_HardwarePresent(OhciExtension, FALSE);

    if (!HardwarePresent)
        return FALSE;

    IntEnable.AsULONG = READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcInterruptEnable);
    IntStatus.AsULONG = READ_REGISTER_ULONG((PULONG)&OperationalRegs->HcInterruptStatus) & IntEnable.AsULONG;

    if ((IntStatus.AsULONG == 0) || (IntEnable.MasterInterruptEnable == 0))
        return FALSE;

    if (IntStatus.UnrecoverableError)
        DPRINT1("OHCI_InterruptService: IntStatus.UnrecoverableError\n");

    if (IntStatus.FrameNumberOverflow)
    {
        POHCI_HCCA HcHCCA;
        ULONG fm;
        ULONG hp;

        HcHCCA = &OhciExtension->HcResourcesVA->HcHCCA;

        DPRINT("FrameNumberOverflow %lX\n", HcHCCA->FrameNumber);

        hp = OhciExtension->FrameHighPart;
        fm = HcHCCA->FrameNumber;

        /* Increment value of FrameHighPart */
        OhciExtension->FrameHighPart += 1 * (1 << 16) - ((hp ^ fm) & 0x8000);
    }

    /* Disable interrupt generation */
    IntDisable.AsULONG = 0;
    IntDisable.MasterInterruptEnable = 1;
    WRITE_REGISTER_ULONG((PULONG)&OperationalRegs->HcInterruptDisable,
                         IntDisable.AsULONG);

    return TRUE;
}

VOID
NTAPI
OHCI_InterruptDpc(IN PVOID ohciExtension,
                  IN BOOLEAN IsDoEnableInterrupts)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptDisableReg;
    PULONG InterruptEnableReg;
    PULONG InterruptStatusReg;
    OHCI_REG_INTERRUPT_STATUS IntStatus;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE InterruptBits;
    POHCI_HCCA HcHCCA;

    OperationalRegs = OhciExtension->OperationalRegs;

    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;
    InterruptDisableReg = (PULONG)&OperationalRegs->HcInterruptDisable;
    InterruptStatusReg = (PULONG)&OperationalRegs->HcInterruptStatus;

    DPRINT_OHCI("OHCI_InterruptDpc: OhciExtension - %p, IsDoEnableInterrupts - %x\n",
                OhciExtension,
                IsDoEnableInterrupts);

    IntStatus.AsULONG = READ_REGISTER_ULONG(InterruptStatusReg);

    if (IntStatus.RootHubStatusChange)
    {
        DPRINT_OHCI("OHCI_InterruptDpc: RootHubStatusChange\n");
        RegPacket.UsbPortInvalidateRootHub(OhciExtension);
    }

    if (IntStatus.WritebackDoneHead)
    {
        DPRINT_OHCI("OHCI_InterruptDpc: WritebackDoneHead\n");

        HcHCCA = &OhciExtension->HcResourcesVA->HcHCCA;
        HcHCCA->DoneHead = 0;

        RegPacket.UsbPortInvalidateEndpoint(OhciExtension, NULL);
    }

    if (IntStatus.StartofFrame)
    {
        /* Disable interrupt generation due to Start of Frame */
        InterruptBits.AsULONG = 0;
        InterruptBits.StartofFrame = 1;

        WRITE_REGISTER_ULONG(InterruptDisableReg, InterruptBits.AsULONG);
    }

    if (IntStatus.ResumeDetected)
        DPRINT1("OHCI_IntDpc: ResumeDetected\n");

    if (IntStatus.UnrecoverableError)
    {
        DPRINT1("OHCI_IntDpc: UnrecoverableError\n");
    }

    WRITE_REGISTER_ULONG(InterruptStatusReg, IntStatus.AsULONG);

    if (IsDoEnableInterrupts)
    {
        /*  Enable interrupt generation */
        InterruptBits.AsULONG = 0;
        InterruptBits.MasterInterruptEnable = 1;

        WRITE_REGISTER_ULONG(InterruptEnableReg, InterruptBits.AsULONG);
    }
}

/**
 * @brief      Forms the next General Transfer Descriptor for the current transfer
 *
 * @param[in]  OhciExtension  The ohci extension
 * @param[in]  TransferedLen  The consolidated length of all previous descriptors' buffers
 * @param[in]  OhciTransfer   The ohci transfer
 * @param[out] TD             The transfer descriptor we are forming
 * @param[in]  SGList         The scatter/gather list
 *
 * @return     The length of all previous buffers summed with the length of the current buffer
 */
static
ULONG
OHCI_MapTransferToTD(IN POHCI_EXTENSION OhciExtension,
                     IN ULONG TransferedLen,
                     IN POHCI_TRANSFER OhciTransfer,
                     OUT POHCI_HCD_TD TD,
                     IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    PUSBPORT_SCATTER_GATHER_ELEMENT SgElement;
    ULONG SgIdx, CurrentTransferLen, BufferEnd, CurrentBuffer;
    ULONG TransferDataLeft = OhciTransfer->TransferParameters->TransferBufferLength - TransferedLen;

    DPRINT_OHCI("OHCI_MapTransferToTD: TransferedLen - %x\n", TransferedLen);

    for (SgIdx = 0; SgIdx < SGList->SgElementCount; SgIdx++)
    {
        SgElement = &SGList->SgElement[SgIdx];

        if (TransferedLen >= SgElement->SgOffset &&
            TransferedLen < SgElement->SgOffset + SgElement->SgTransferLength)
        {
            break;
        }
    }

    DPRINT_OHCI("OHCI_MapTransferToTD: SgIdx - %x, SgCount - %x\n",
                SgIdx,
                SGList->SgElementCount);

    ASSERT(SgIdx < SGList->SgElementCount);
    ASSERT(TransferedLen == SgElement->SgOffset);

    /* The buffer for a TD can be 0 to 8192 bytes long,
     * and can span within mo more than two 4k pages (see OpenHCI spec 3.3.2)
     * CurrentBuffer - the (physical) address of the first byte in the buffer
     * BufferEnd - the address of the last byte in the buffer. It can be on a different physical 4k page
     * when a controller will reach the end of a page from CurrentBuffer, it will take the first 20 bits
     * of the BufferEnd as a next address (OpenHCI spec, 4.3.1.3.1)
     */

    CurrentBuffer = SgElement->SgPhysicalAddress.LowPart;

    if (TransferDataLeft <= SgElement->SgTransferLength)
    {
        CurrentTransferLen = TransferDataLeft;
        BufferEnd = SgElement->SgPhysicalAddress.LowPart + CurrentTransferLen - 1;
    }
    else
    {
        PUSBPORT_SCATTER_GATHER_ELEMENT SgNextElement;
        ASSERT(SGList->SgElementCount - SgIdx > 1);

        SgNextElement = &SGList->SgElement[SgIdx + 1];

        TransferDataLeft -= SgElement->SgTransferLength;
        CurrentTransferLen = SgElement->SgTransferLength;

        if (TransferDataLeft <= SgNextElement->SgTransferLength)
        {
            CurrentTransferLen += TransferDataLeft;
            BufferEnd = SgNextElement->SgPhysicalAddress.LowPart + TransferDataLeft - 1;
        }
        else
        {
            CurrentTransferLen += SgNextElement->SgTransferLength;
            BufferEnd = SgNextElement->SgPhysicalAddress.LowPart + SgNextElement->SgTransferLength - 1;
        }
    }

    TD->HwTD.gTD.CurrentBuffer = CurrentBuffer;
    TD->HwTD.gTD.BufferEnd = BufferEnd;
    TD->TransferLen = CurrentTransferLen;

    return TransferedLen + CurrentTransferLen;
}

POHCI_HCD_TD
NTAPI
OHCI_AllocateTD(IN POHCI_EXTENSION OhciExtension,
                IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_TD TD;

    DPRINT_OHCI("OHCI_AllocateTD: ... \n");

    TD = OhciEndpoint->FirstTD;

    while (TD->Flags & OHCI_HCD_TD_FLAG_ALLOCATED)
    {
        TD += 1;
    }

    TD->Flags |= OHCI_HCD_TD_FLAG_ALLOCATED;

    RtlSecureZeroMemory(&TD->HwTD, sizeof(TD->HwTD));

    return TD;
}

ULONG
NTAPI
OHCI_RemainTDs(IN POHCI_EXTENSION OhciExtension,
               IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_TD TD;
    ULONG MaxTDs;
    ULONG RemainTDs;
    ULONG ix;

    DPRINT_OHCI("OHCI_RemainTDs: ... \n");

    MaxTDs = OhciEndpoint->MaxTransferDescriptors;
    TD = OhciEndpoint->FirstTD;

    RemainTDs = 0;

    for (ix = 0; ix < MaxTDs; ix++)
    {
        if (!(TD->Flags & OHCI_HCD_TD_FLAG_ALLOCATED))
            RemainTDs++;

        TD += 1;
    }

    return RemainTDs;
}

static
MPSTATUS
OHCI_ControlTransfer(IN POHCI_EXTENSION OhciExtension,
                     IN POHCI_ENDPOINT OhciEndpoint,
                     IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                     IN POHCI_TRANSFER OhciTransfer,
                     IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    POHCI_HCD_TD SetupTD;
    POHCI_HCD_TD TD;
    POHCI_HCD_TD PrevTD;
    ULONG MaxTDs;
    ULONG TransferedLen;
    UCHAR DataToggle;

    DPRINT_OHCI("OHCI_ControlTransfer: Ext %p, Endpoint %p\n",
                OhciExtension,
                OhciEndpoint);

    MaxTDs = OHCI_RemainTDs(OhciExtension, OhciEndpoint);

    if ((SGList->SgElementCount + OHCI_NON_DATA_CONTROL_TDS) > MaxTDs)
        return MP_STATUS_FAILURE;

    /* Form a setup packet first */
    SetupTD = OhciEndpoint->HcdTailP;
    RtlSecureZeroMemory(&SetupTD->HwTD, sizeof(SetupTD->HwTD));

    SetupTD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
    SetupTD->OhciTransfer = OhciTransfer;

    OhciTransfer->PendingTDs++;

    RtlCopyMemory(&SetupTD->HwTD.SetupPacket,
                  &TransferParameters->SetupPacket,
                  sizeof(SetupTD->HwTD.SetupPacket));

    SetupTD->HwTD.gTD.CurrentBuffer = SetupTD->PhysicalAddress + FIELD_OFFSET(OHCI_HCD_TD, HwTD.SetupPacket);
    SetupTD->HwTD.gTD.BufferEnd = SetupTD->PhysicalAddress + FIELD_OFFSET(OHCI_HCD_TD, HwTD.SetupPacket) +
                                  sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) - 1;
    SetupTD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
    SetupTD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;
    SetupTD->HwTD.gTD.Control.DataToggle = OHCI_TD_DATA_TOGGLE_DATA0;

    PrevTD = SetupTD;

    /* Data packets follow a setup packet (if any) */
    TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
    TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
    TD->OhciTransfer = OhciTransfer;

    PrevTD->HwTD.gTD.NextTD = TD->PhysicalAddress;
    PrevTD->NextTDVa = TD;

    /* The first data packet should use DATA1, subsequent ones use DATA0 (OpenHCI spec, 4.3.1.3.4) */
    DataToggle = OHCI_TD_DATA_TOGGLE_DATA1;
    TransferedLen = 0;

    while (TransferedLen < TransferParameters->TransferBufferLength)
    {
        OhciTransfer->PendingTDs++;

        if (TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
            TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_IN;
        else
            TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_OUT;

        TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
        TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;
        TD->HwTD.gTD.Control.DataToggle = DataToggle;

        TransferedLen = OHCI_MapTransferToTD(OhciExtension,
                                             TransferedLen,
                                             OhciTransfer,
                                             TD,
                                             SGList);

        PrevTD = TD;

        TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
        TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_NONE;
        TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
        TD->OhciTransfer = OhciTransfer;

        PrevTD->HwTD.gTD.NextTD = TD->PhysicalAddress;
        PrevTD->NextTDVa = TD;

        DataToggle = OHCI_TD_DATA_TOGGLE_DATA0;
    }

    if (TransferParameters->TransferFlags & USBD_SHORT_TRANSFER_OK)
    {
        PrevTD->HwTD.gTD.Control.BufferRounding = TRUE;
        OhciTransfer->Flags |= OHCI_TRANSFER_FLAGS_SHORT_TRANSFER_OK;
    }

    /* After data packets, goes a status packet */

    TD->Flags |= OHCI_HCD_TD_FLAG_CONTROL_STATUS;
    TD->TransferLen = 0;

    if ((TransferParameters->TransferFlags & USBD_TRANSFER_DIRECTION_IN) != 0)
    {
        TD->HwTD.gTD.Control.BufferRounding = FALSE;
        TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_OUT;
    }
    else
    {
        TD->HwTD.gTD.Control.BufferRounding = TRUE;
        TD->HwTD.gTD.Control.DirectionPID = OHCI_TD_DIRECTION_PID_IN;
    }

    /* OpenHCI spec, 4.3.1.3.4 */
    TD->HwTD.gTD.Control.DataToggle = OHCI_TD_DATA_TOGGLE_DATA1;
    TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NOT_ACCESSED;
    TD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_IMMEDIATE;

    OhciTransfer->PendingTDs++;
    OhciTransfer->ControlStatusTD = TD;

    PrevTD = TD;

    /* And the last descriptor, which is not used in the current transfer (OpenHCI spec, 4.6) */
    TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);

    PrevTD->HwTD.gTD.NextTD = TD->PhysicalAddress;
    PrevTD->NextTDVa = TD;

    TD->NextTDVa = NULL;
    /* TD->HwTD.gTD.NextTD = 0; */

    OhciTransfer->NextTD = TD;
    OhciEndpoint->HcdTailP = TD;

    OhciEndpoint->HcdED->HwED.TailPointer = TD->PhysicalAddress;

    OHCI_EnableList(OhciExtension, OhciEndpoint);

    return MP_STATUS_SUCCESS;
}

static
MPSTATUS
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

    DPRINT_OHCI("OHCI_BulkOrInterruptTransfer: ... \n");

    MaxTDs = OHCI_RemainTDs(OhciExtension, OhciEndpoint);

    if (SGList->SgElementCount > MaxTDs)
        return MP_STATUS_FAILURE;

    TD = OhciEndpoint->HcdTailP;

    TransferedLen = 0;

    do
    {
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

        TD->Flags |= OHCI_HCD_TD_FLAG_PROCESSED;
        TD->OhciTransfer = OhciTransfer;

        if (TransferParameters->TransferBufferLength)
        {
            TransferedLen = OHCI_MapTransferToTD(OhciExtension,
                                                 TransferedLen,
                                                 OhciTransfer,
                                                 TD,
                                                 SGList);
        }
        else
        {
            ASSERT(SGList->SgElementCount == 0);
            TD->TransferLen = 0;
        }

        PrevTD = TD;

        TD = OHCI_AllocateTD(OhciExtension, OhciEndpoint);
        OhciTransfer->PendingTDs++;

        PrevTD->HwTD.gTD.NextTD = TD->PhysicalAddress;
        PrevTD->NextTDVa = TD;
    }
    while (TransferedLen < TransferParameters->TransferBufferLength);

    if (TransferParameters->TransferFlags & USBD_SHORT_TRANSFER_OK)
    {
        PrevTD->HwTD.gTD.Control.BufferRounding = TRUE;
        OhciTransfer->Flags |= OHCI_TRANSFER_FLAGS_SHORT_TRANSFER_OK;
    }

    PrevTD->HwTD.gTD.Control.DelayInterrupt = OHCI_TD_INTERRUPT_IMMEDIATE;

    /* The last TD in a chain is not used in a transfer. The controller does not access it
     * so it will be used for chaining a next transfer to it (OpenHCI spec, 4.6)
     */
    /* TD->HwTD.gTD.NextTD = 0; */
    TD->NextTDVa = NULL;

    OhciTransfer->NextTD = TD;
    OhciEndpoint->HcdTailP = TD;

    OhciEndpoint->HcdED->HwED.TailPointer = TD->PhysicalAddress;

    OHCI_EnableList(OhciExtension, OhciEndpoint);

    return MP_STATUS_SUCCESS;
}

/**
 * @brief      Creates the transfer descriptor chain for the given transfer's buffer
 *             and attaches it to a given endpoint (for control, bulk or interrupt transfers)
 *
 * @param[in]  OhciExtension       The ohci extension
 * @param[in]  OhciEndpoint        The ohci endpoint
 * @param[in]  TransferParameters  The transfer parameters
 * @param[in]  OhciTransfer        The ohci transfer
 * @param[in]  SGList              The scatter/gather list
 *
 * @return     MP_STATUS_SUCCESS or MP_STATUS_FAILURE if there are not enough TDs left
 *             or wrong transfer type given
 */
MPSTATUS
NTAPI
OHCI_SubmitTransfer(IN PVOID ohciExtension,
                    IN PVOID ohciEndpoint,
                    IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                    IN PVOID ohciTransfer,
                    IN PUSBPORT_SCATTER_GATHER_LIST SGList)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_TRANSFER OhciTransfer = ohciTransfer;
    ULONG TransferType;

    DPRINT_OHCI("OHCI_SubmitTransfer: ... \n");

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

    return MP_STATUS_FAILURE;
}

MPSTATUS
NTAPI
OHCI_SubmitIsoTransfer(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint,
                       IN PUSBPORT_TRANSFER_PARAMETERS TransferParameters,
                       IN PVOID ohciTransfer,
                       IN PVOID isoParameters)
{
    DPRINT1("OHCI_SubmitIsoTransfer: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_ProcessDoneTD(IN POHCI_EXTENSION OhciExtension,
                   IN POHCI_HCD_TD TD,
                   IN BOOLEAN IsPortComplete)
{
    POHCI_TRANSFER OhciTransfer;
    POHCI_ENDPOINT OhciEndpoint;
    ULONG Buffer;
    ULONG BufferEnd;
    ULONG Length;

    DPRINT_OHCI("OHCI_ProcessDoneTD: ... \n");

    OhciTransfer = TD->OhciTransfer;
    OhciEndpoint = OhciTransfer->OhciEndpoint;

    OhciTransfer->PendingTDs--;

    Buffer = TD->HwTD.gTD.CurrentBuffer;
    BufferEnd = TD->HwTD.gTD.BufferEnd;

    if (TD->Flags & OHCI_HCD_TD_FLAG_NOT_ACCESSED)
    {
        TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NO_ERROR;
    }
    else
    {
        if (TD->HwTD.gTD.CurrentBuffer)
        {
            if (TD->TransferLen)
            {
                Length = (BufferEnd & (PAGE_SIZE - 1)) -
                         (Buffer & (PAGE_SIZE - 1));

                Length++;

                if (Buffer >> PAGE_SHIFT != BufferEnd >> PAGE_SHIFT)
                    Length += PAGE_SIZE;

                TD->TransferLen -= Length;
            }
        }

        if (TD->HwTD.gTD.Control.DirectionPID != OHCI_TD_DIRECTION_PID_SETUP)
            OhciTransfer->TransferLen += TD->TransferLen;

        if (TD->HwTD.gTD.Control.ConditionCode)
        {
            OhciTransfer->USBDStatus = USBD_STATUS_HALTED |
                                       TD->HwTD.gTD.Control.ConditionCode;
        }
    }

    TD->Flags = 0;
    TD->HwTD.gTD.NextTD = 0;
    TD->OhciTransfer = 0;

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

/**
 * @brief      Aborts the transfer descriptor chain in a given endpoint
 *
 * @param[in]  ohciExtension   The ohci extension
 * @param[in]  ohciEndpoint    The ohci endpoint
 * @param[in]  ohciTransfer    The ohci transfer
 * @param[out] CompletedLength
 */
VOID
NTAPI
OHCI_AbortTransfer(IN PVOID ohciExtension,
                   IN PVOID ohciEndpoint,
                   IN PVOID ohciTransfer,
                   IN OUT PULONG CompletedLength)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_TRANSFER OhciTransfer = ohciTransfer;
    POHCI_HCD_ED ED = OhciEndpoint->HcdED;
    POHCI_HCD_TD TD, NextTD, LastTD;
    ULONG ix;
    BOOLEAN IsIsoEndpoint;
    BOOLEAN IsProcessed = FALSE;

    DPRINT("OHCI_AbortTransfer: ohciEndpoint - %p, ohciTransfer - %p\n",
           OhciEndpoint,
           OhciTransfer);

    IsIsoEndpoint = (OhciEndpoint->EndpointProperties.TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS);
    NextTD = RegPacket.UsbPortGetMappedVirtualAddress(ED->HwED.HeadPointer & OHCI_ED_HEAD_POINTER_MASK,
                                                      OhciExtension,
                                                      OhciEndpoint);

    if (NextTD->OhciTransfer == OhciTransfer)
    {
        LastTD = OhciTransfer->NextTD;

        /* Keeping the carry bit from previous pointer value */
        ED->HwED.HeadPointer = LastTD->PhysicalAddress |
                               (ED->HwED.HeadPointer & OHCI_ED_HEAD_POINTER_CARRY);

        OhciEndpoint->HcdHeadP = LastTD;

        for (ix = 0; ix < OhciEndpoint->MaxTransferDescriptors; ix++)
        {
            TD = &OhciEndpoint->FirstTD[ix];

            if (TD->OhciTransfer != OhciTransfer)
                continue;

            if (IsIsoEndpoint)
                OHCI_ProcessDoneIsoTD(OhciExtension, TD, FALSE);
            else
                OHCI_ProcessDoneTD(OhciExtension, TD, FALSE);
        }

        *CompletedLength = OhciTransfer->TransferLen;
        return;
    }

    if (NextTD == OhciEndpoint->HcdHeadP)
        IsProcessed = TRUE;

    for (TD = OhciEndpoint->HcdHeadP; TD != NextTD; TD = TD->NextTDVa)
    {
        if (TD->OhciTransfer != OhciTransfer)
            continue;

        if (OhciEndpoint->HcdHeadP == TD)
            OhciEndpoint->HcdHeadP = TD->NextTDVa;

        if (IsIsoEndpoint)
            OHCI_ProcessDoneIsoTD(OhciExtension, TD, FALSE);
        else
            OHCI_ProcessDoneTD(OhciExtension, TD, FALSE);

        IsProcessed = TRUE;
    }

    if (!IsProcessed)
    {
        for (TD = OhciEndpoint->HcdHeadP; TD->OhciTransfer != OhciTransfer; TD = TD->NextTDVa)
        {
            if (TD == OhciEndpoint->HcdTailP)
            {
                TD = NULL;
                break;
            }
            LastTD = TD;
        }

        for (; TD->OhciTransfer == OhciTransfer; TD = TD->NextTDVa)
        {
            if (TD == OhciEndpoint->HcdTailP)
                break;

            if (IsIsoEndpoint)
                OHCI_ProcessDoneIsoTD(OhciExtension, TD, FALSE);
            else
                OHCI_ProcessDoneTD(OhciExtension, TD, FALSE);
        }

        LastTD->OhciTransfer->NextTD = TD;

        LastTD->NextTDVa = TD;
        LastTD->HwTD.gTD.NextTD = TD->PhysicalAddress;
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
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_GetEndpointState: ... \n");

    ED = OhciEndpoint->HcdED;

    if (ED->Flags & OHCI_HCD_TD_FLAG_NOT_ACCESSED)
        return USBPORT_ENDPOINT_REMOVE;

    if (ED->HwED.EndpointControl.sKip)
        return USBPORT_ENDPOINT_PAUSED;

    return USBPORT_ENDPOINT_ACTIVE;
}

VOID
NTAPI
OHCI_RemoveEndpointFromSchedule(IN POHCI_ENDPOINT OhciEndpoint)
{
    POHCI_HCD_ED ED;
    POHCI_HCD_ED PreviousED;
    POHCI_STATIC_ED HeadED;

    DPRINT_OHCI("OHCI_RemoveEndpointFromSchedule \n");

    ED = OhciEndpoint->HcdED;
    HeadED = OhciEndpoint->HeadED;

    if (&HeadED->Link == ED->HcdEDLink.Blink)
    {
        if (HeadED->Type == OHCI_STATIC_ED_TYPE_CONTROL ||
            HeadED->Type == OHCI_STATIC_ED_TYPE_BULK)
        {
            WRITE_REGISTER_ULONG(HeadED->pNextED, ED->HwED.NextED);
        }
        else if (HeadED->Type == OHCI_STATIC_ED_TYPE_INTERRUPT)
        {
            *HeadED->pNextED = ED->HwED.NextED;
        }
        else
        {
            DPRINT1("OHCI_RemoveEndpointFromSchedule: Unknown HeadED->Type - %x\n",
                    HeadED->Type);
            DbgBreakPoint();
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
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_SetEndpointState: EndpointState - %x\n",
                EndpointState);

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

        case USBPORT_ENDPOINT_REMOVE:
            ED->HwED.EndpointControl.sKip = 1;
            ED->Flags |= OHCI_HCD_ED_FLAG_NOT_ACCESSED;
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
    PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
    POHCI_HCD_ED ED;
    ULONG_PTR NextTdPA;
    POHCI_HCD_TD NextTD;
    POHCI_HCD_TD TD;
    PLIST_ENTRY DoneList;
    POHCI_TRANSFER OhciTransfer;
    POHCI_HCD_TD ControlStatusTD;
    ULONG_PTR PhysicalAddress;
    ULONG TransferNumber;
    POHCI_TRANSFER transfer;
    UCHAR ConditionCode;
    BOOLEAN IsResetOnHalt = FALSE;

    //DPRINT_OHCI("OHCI_PollAsyncEndpoint: Endpoint - %p\n", OhciEndpoint);

    ED = OhciEndpoint->HcdED;
    NextTdPA = ED->HwED.HeadPointer & OHCI_ED_HEAD_POINTER_MASK;

    if (!NextTdPA)
    {
        OHCI_DumpHcdED(ED);
        DbgBreakPoint();
    }

    NextTD = RegPacket.UsbPortGetMappedVirtualAddress(NextTdPA,
                                                      OhciExtension,
                                                      OhciEndpoint);
    DPRINT_OHCI("NextTD - %p\n", NextTD);

    if ((ED->HwED.HeadPointer & OHCI_ED_HEAD_POINTER_HALT) == 0)
        goto ProcessListTDs;

    OHCI_DumpHcdED(ED);

    IsResetOnHalt = (ED->Flags & OHCI_HCD_ED_FLAG_RESET_ON_HALT) != 0;
    DPRINT1("PollAsyncEndpoint: IsResetOnHalt %x\n", IsResetOnHalt);

    for (TD = OhciEndpoint->HcdHeadP; ; TD = TD->NextTDVa)
    {
        if (!TD)
        {
            OHCI_DumpHcdED(ED);
            DbgBreakPoint();
        }

        if (TD == NextTD)
        {
            DPRINT("TD == NextTD - %p\n", TD);
            goto HandleDoneList;
        }

        OhciTransfer = TD->OhciTransfer;
        ConditionCode = TD->HwTD.gTD.Control.ConditionCode;

        DPRINT("TD - %p, ConditionCode - %X\n", TD, ConditionCode);
        OHCI_DumpHcdTD(TD);

        switch (ConditionCode)
        {
            case OHCI_TD_CONDITION_NO_ERROR:
                TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                continue;

            case OHCI_TD_CONDITION_NOT_ACCESSED:
                TD->Flags |= (OHCI_HCD_TD_FLAG_DONE | OHCI_HCD_TD_FLAG_NOT_ACCESSED);
                InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                continue;

            case OHCI_TD_CONDITION_DATA_UNDERRUN:
                DPRINT1("DATA_UNDERRUN. Transfer->Flags - %X\n", OhciTransfer->Flags);

                if (OhciTransfer->Flags & OHCI_TRANSFER_FLAGS_SHORT_TRANSFER_OK)
                {
                    IsResetOnHalt = TRUE;
                    TD->HwTD.gTD.Control.ConditionCode = OHCI_TD_CONDITION_NO_ERROR;

                    ControlStatusTD = OhciTransfer->ControlStatusTD;

                    if ((TD->Flags & OHCI_HCD_TD_FLAG_CONTROL_STATUS) == 0 &&
                        ControlStatusTD)
                    {
                        PhysicalAddress = ControlStatusTD->PhysicalAddress;
                        PhysicalAddress |= (ED->HwED.HeadPointer &
                                            OHCI_ED_HEAD_POINTER_FLAGS_MASK);

                        ED->HwED.HeadPointer = PhysicalAddress;

                        NextTD = OhciTransfer->ControlStatusTD;
                        DPRINT("PhysicalAddress - %p, NextTD - %p\n", PhysicalAddress, NextTD);
                    }
                    else
                    {
                        TransferParameters = OhciTransfer->TransferParameters;

                        if (TransferParameters->IsTransferSplited)
                        {
                            TransferNumber = TransferParameters->TransferCounter;
                            transfer = OhciTransfer;

                            do
                            {
                                transfer = transfer->NextTD->OhciTransfer;
                                NextTD = transfer->NextTD;
                            }
                            while (transfer && TransferNumber ==
                                   transfer->TransferParameters->TransferCounter);

                            PhysicalAddress = NextTD->PhysicalAddress;
                            PhysicalAddress |= (ED->HwED.HeadPointer &
                                                OHCI_ED_HEAD_POINTER_FLAGS_MASK);

                            ED->HwED.HeadPointer = PhysicalAddress;
                            DPRINT("PhysicalAddress - %p, NextTD - %p\n", PhysicalAddress, NextTD);
                        }
                        else
                        {
                            PhysicalAddress = OhciTransfer->NextTD->PhysicalAddress;
                            PhysicalAddress |= (ED->HwED.HeadPointer &
                                                OHCI_ED_HEAD_POINTER_FLAGS_MASK);

                            ED->HwED.HeadPointer = PhysicalAddress;

                            NextTD = OhciTransfer->NextTD;
                            DPRINT("PhysicalAddress - %p, NextTD - %p\n", PhysicalAddress, NextTD);
                        }
                    }

                    TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                    InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
                    continue;
                }

                /* fall through */

            default:
                TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
                InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);

                ED->HwED.HeadPointer = OhciTransfer->NextTD->PhysicalAddress |
                                       (ED->HwED.HeadPointer &
                                        OHCI_ED_HEAD_POINTER_FLAGS_MASK);

                NextTD = OhciTransfer->NextTD;
                break;
        }
    }

ProcessListTDs:

    TD = OhciEndpoint->HcdHeadP;

    while (TD != NextTD)
    {
        OHCI_DumpHcdTD(TD);
        TD->Flags |= OHCI_HCD_TD_FLAG_DONE;
        InsertTailList(&OhciEndpoint->TDList, &TD->DoneLink);
        TD = TD->NextTDVa;
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
        ED->HwED.HeadPointer &= ~OHCI_ED_HEAD_POINTER_HALT;
        DPRINT("ED->HwED.HeadPointer - %p\n", ED->HwED.HeadPointer);
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
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    ULONG TransferType;

    DPRINT_OHCI("OHCI_PollEndpoint: OhciExtension - %p, Endpoint - %p\n",
                OhciExtension,
                OhciEndpoint);

    TransferType = OhciEndpoint->EndpointProperties.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        OHCI_PollIsoEndpoint(OhciExtension, OhciEndpoint);
        return;
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_BULK ||
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        OHCI_PollAsyncEndpoint(OhciExtension, OhciEndpoint);
    }
}

VOID
NTAPI
OHCI_CheckController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG HcControlReg;
    OHCI_REG_CONTROL HcControl;
    ULONG FmNumber;
    USHORT FmDiff;
    POHCI_HCCA HcHCCA;

    //DPRINT_OHCI("OHCI_CheckController: ...\n");

    OperationalRegs = OhciExtension->OperationalRegs;

    if (!OHCI_HardwarePresent(OhciExtension, TRUE))
        return;

    HcControlReg = (PULONG)&OperationalRegs->HcControl;
    HcControl.AsULONG = READ_REGISTER_ULONG(HcControlReg);

    if (HcControl.HostControllerFunctionalState != OHCI_HC_STATE_OPERATIONAL)
        return;

    FmNumber = READ_REGISTER_ULONG(&OperationalRegs->HcFmNumber);
    FmDiff = (USHORT)(FmNumber - OhciExtension->HcdFmNumber);

    if (FmNumber == 0 || FmDiff < 5)
        return;

    HcHCCA = &OhciExtension->HcResourcesVA->HcHCCA;
    OhciExtension->HcdFmNumber = FmNumber;

    if (HcHCCA->Pad1 == 0)
    {
        HcHCCA->Pad1 = 0xBAD1;
        return;
    }

    DPRINT1("OHCI_CheckController: HcHCCA->Pad1 - %x\n", HcHCCA->Pad1);

    if (HcHCCA->Pad1 == 0xBAD1)
    {
        HcHCCA->Pad1 = 0xBAD2;
    }
    else if (HcHCCA->Pad1 == 0xBAD2)
    {
        HcHCCA->Pad1 = 0xBAD3;

        RegPacket.UsbPortInvalidateController(OhciExtension,
                                              USBPORT_INVALIDATE_CONTROLLER_RESET);
    }
}

ULONG
NTAPI
OHCI_Get32BitFrameNumber(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_HCCA HcHCCA;
    ULONG fm;
    ULONG hp;

    HcHCCA = &OhciExtension->HcResourcesVA->HcHCCA;

    /* 5.4 FrameInterval Counter: Get32BitFrameNumber() */

    hp = OhciExtension->FrameHighPart;
    fm = HcHCCA->FrameNumber;

    DPRINT_OHCI("OHCI_Get32BitFrameNumber: hp - %lX, fm - %lX\n", hp, fm);

    return ((fm & 0x7FFF) | hp) + ((fm ^ hp) & 0x8000);
}

VOID
NTAPI
OHCI_InterruptNextSOF(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptEnableReg;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntEnable;

    DPRINT_OHCI("OHCI_InterruptNextSOF: OhciExtension - %p\n",
                OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;

    /* Enable interrupt generation due to Start of Frame */
    IntEnable.AsULONG = 0;
    IntEnable.StartofFrame = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, IntEnable.AsULONG);
}

VOID
NTAPI
OHCI_EnableInterrupts(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptEnableReg;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntEnable;

    DPRINT_OHCI("OHCI_EnableInterrupts: OhciExtension - %p\n",
                OhciExtension);

    OperationalRegs = OhciExtension->OperationalRegs;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;

    /*  Enable interrupt generation */
    IntEnable.AsULONG = 0;
    IntEnable.MasterInterruptEnable = 1;

    WRITE_REGISTER_ULONG(InterruptEnableReg, IntEnable.AsULONG);
}

VOID
NTAPI
OHCI_DisableInterrupts(IN PVOID ohciExtension)
{
    POHCI_EXTENSION  OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    PULONG InterruptDisableReg;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntDisable;

    DPRINT_OHCI("OHCI_DisableInterrupts\n");

    OperationalRegs = OhciExtension->OperationalRegs;
    InterruptDisableReg = (PULONG)&OperationalRegs->HcInterruptDisable;

    /*  Disable interrupt generation */
    IntDisable.AsULONG = 0;
    IntDisable.MasterInterruptEnable = 1;

    WRITE_REGISTER_ULONG(InterruptDisableReg, IntDisable.AsULONG);
}

VOID
NTAPI
OHCI_PollController(IN PVOID ohciExtension)
{
    DPRINT1("OHCI_PollController: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
OHCI_SetEndpointDataToggle(IN PVOID ohciExtension,
                           IN PVOID ohciEndpoint,
                           IN ULONG DataToggle)
{
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_SetEndpointDataToggle: Endpoint - %p, DataToggle - %x\n",
                OhciEndpoint,
                DataToggle);

    ED = OhciEndpoint->HcdED;

    if (DataToggle)
        ED->HwED.HeadPointer |= OHCI_ED_HEAD_POINTER_CARRY;
    else
        ED->HwED.HeadPointer &= ~OHCI_ED_HEAD_POINTER_CARRY;
}

ULONG
NTAPI
OHCI_GetEndpointStatus(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint)
{
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;
    ULONG EndpointStatus = USBPORT_ENDPOINT_RUN;

    DPRINT_OHCI("OHCI_GetEndpointStatus: ... \n");

    ED = OhciEndpoint->HcdED;

    if ((ED->HwED.HeadPointer & OHCI_ED_HEAD_POINTER_HALT) &&
        !(ED->Flags & OHCI_HCD_ED_FLAG_RESET_ON_HALT))
    {
        EndpointStatus = USBPORT_ENDPOINT_HALT;
    }

    return EndpointStatus;
}

VOID
NTAPI
OHCI_SetEndpointStatus(IN PVOID ohciExtension,
                       IN PVOID ohciEndpoint,
                       IN ULONG EndpointStatus)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_ENDPOINT OhciEndpoint = ohciEndpoint;
    POHCI_HCD_ED ED;

    DPRINT_OHCI("OHCI_SetEndpointStatus: Endpoint - %p, EndpointStatus - %lX\n",
                OhciEndpoint,
                EndpointStatus);

    if (EndpointStatus == USBPORT_ENDPOINT_RUN)
    {
        ED = OhciEndpoint->HcdED;
        ED->HwED.HeadPointer &= ~OHCI_ED_HEAD_POINTER_HALT;

        OHCI_EnableList(OhciExtension, OhciEndpoint);
    }
    else if (EndpointStatus == USBPORT_ENDPOINT_HALT)
    {
        ASSERT(FALSE);
    }
}

VOID
NTAPI
OHCI_ResetController(IN PVOID ohciExtension)
{
    POHCI_EXTENSION OhciExtension = ohciExtension;
    POHCI_OPERATIONAL_REGISTERS OperationalRegs;
    ULONG FrameNumber;
    PULONG ControlReg;
    PULONG CommandStatusReg;
    PULONG InterruptEnableReg;
    PULONG FmIntervalReg;
    PULONG RhStatusReg;
    PULONG PortStatusReg;
    OHCI_REG_CONTROL ControlBak;
    OHCI_REG_CONTROL Control;
    OHCI_REG_COMMAND_STATUS CommandStatus;
    OHCI_REG_INTERRUPT_ENABLE_DISABLE IntEnable;
    ULONG_PTR HCCA;
    ULONG_PTR ControlHeadED;
    ULONG_PTR BulkHeadED;
    OHCI_REG_FRAME_INTERVAL FrameInterval;
    ULONG_PTR PeriodicStart;
    ULONG_PTR LSThreshold;
    OHCI_REG_RH_STATUS RhStatus;
    OHCI_REG_RH_DESCRIPTORA RhDescriptorA;
    OHCI_REG_RH_PORT_STATUS PortStatus;
    ULONG NumPorts;
    ULONG ix;

    DPRINT("OHCI_ResetController: ... \n");

    OperationalRegs = OhciExtension->OperationalRegs;

    ControlReg = (PULONG)&OperationalRegs->HcControl;
    CommandStatusReg = (PULONG)&OperationalRegs->HcCommandStatus;
    InterruptEnableReg = (PULONG)&OperationalRegs->HcInterruptEnable;
    FmIntervalReg = (PULONG)&OperationalRegs->HcFmInterval;
    RhStatusReg = (PULONG)&OperationalRegs->HcRhStatus;

    /* Backup FrameNumber from HcHCCA */
    FrameNumber = OhciExtension->HcResourcesVA->HcHCCA.FrameNumber;

    /* Backup registers */
    ControlBak.AsULONG = READ_REGISTER_ULONG(ControlReg);
    HCCA = READ_REGISTER_ULONG(&OperationalRegs->HcHCCA);
    ControlHeadED = READ_REGISTER_ULONG(&OperationalRegs->HcControlHeadED);
    BulkHeadED = READ_REGISTER_ULONG(&OperationalRegs->HcBulkHeadED);
    FrameInterval.AsULONG = READ_REGISTER_ULONG(FmIntervalReg);
    PeriodicStart = READ_REGISTER_ULONG(&OperationalRegs->HcPeriodicStart);
    LSThreshold = READ_REGISTER_ULONG(&OperationalRegs->HcLSThreshold);

    /* Reset HostController */
    CommandStatus.AsULONG = 0;
    CommandStatus.HostControllerReset = 1;
    WRITE_REGISTER_ULONG(CommandStatusReg, CommandStatus.AsULONG);

    KeStallExecutionProcessor(10);

    /* Restore registers */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcHCCA, HCCA);
    WRITE_REGISTER_ULONG(&OperationalRegs->HcControlHeadED, ControlHeadED);
    WRITE_REGISTER_ULONG(&OperationalRegs->HcBulkHeadED, BulkHeadED);

    /* Set OPERATIONAL state for HC */
    Control.AsULONG = 0;
    Control.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;
    WRITE_REGISTER_ULONG(ControlReg, Control.AsULONG);

    /* Set Toggle bit for FmInterval register */
    FrameInterval.FrameIntervalToggle = 1;
    WRITE_REGISTER_ULONG(FmIntervalReg, FrameInterval.AsULONG);

    /* Restore registers */
    WRITE_REGISTER_ULONG(&OperationalRegs->HcFmNumber, FrameNumber);
    WRITE_REGISTER_ULONG(&OperationalRegs->HcPeriodicStart, PeriodicStart);
    WRITE_REGISTER_ULONG(&OperationalRegs->HcLSThreshold, LSThreshold);

    /* Setup RhStatus register */
    RhStatus.AsULONG = 0;
    RhStatus.SetRemoteWakeupEnable = 1;
    RhStatus.SetGlobalPower = 1;
    WRITE_REGISTER_ULONG(RhStatusReg, RhStatus.AsULONG);

    /* Setup RH PortStatus registers */
    RhDescriptorA = OHCI_ReadRhDescriptorA(OhciExtension);
    NumPorts = RhDescriptorA.NumberDownstreamPorts;

    PortStatus.AsULONG = 0;
    PortStatus.SetPortPower = 1;

    for (ix = 0; ix < NumPorts; ix++)
    {
        PortStatusReg = (PULONG)&OperationalRegs->HcRhPortStatus[ix];
        WRITE_REGISTER_ULONG(PortStatusReg, PortStatus.AsULONG);
    }

    /* Restore HcControl register */
    ControlBak.HostControllerFunctionalState = OHCI_HC_STATE_OPERATIONAL;
    WRITE_REGISTER_ULONG(ControlReg, ControlBak.AsULONG);

    /* Setup HcInterruptEnable register */
    IntEnable.AsULONG = 0xFFFFFFFF;
    IntEnable.Reserved1 = 0;
    WRITE_REGISTER_ULONG(InterruptEnableReg, IntEnable.AsULONG);
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
    DPRINT1("OHCI_StartSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
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
    DPRINT1("OHCI_EndSendOnePacket: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
OHCI_PassThru(IN PVOID ohciExtension,
              IN PVOID passThruParameters,
              IN ULONG ParameterLength,
              IN PVOID pParameters)
{
    DPRINT1("OHCI_PassThru: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
OHCI_Unload(IN PDRIVER_OBJECT DriverObject)
{
#if DBG
    DPRINT1("OHCI_Unload: Not supported\n");
#endif
    return;
}

VOID
NTAPI
OHCI_FlushInterrupts(IN PVOID uhciExtension)
{
#if DBG
    DPRINT1("OHCI_FlushInterrupts: Not supported\n");
#endif
    return;
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
                              USB_MINIPORT_FLAGS_MEMORY_IO;

    RegPacket.MiniPortBusBandwidth = TOTAL_USB11_BUS_BANDWIDTH;

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
    RegPacket.FlushInterrupts = OHCI_FlushInterrupts;

    DriverObject->DriverUnload = OHCI_Unload;

    Status = USBPORT_RegisterUSBPortDriver(DriverObject,
                                           USB10_MINIPORT_INTERFACE_VERSION,
                                           &RegPacket);

    DPRINT_OHCI("DriverEntry: USBPORT_RegisterUSBPortDriver return Status - %x\n",
                Status);

    return Status;
}
