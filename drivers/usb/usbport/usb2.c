/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort USB 2.0 functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

//#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
USB2_AllocateCheck(IN OUT PULONG OutTimeUsed,
                   IN ULONG CalcBusTime,
                   IN ULONG LimitAllocation)
{
    ULONG BusTime;
    BOOLEAN Result = TRUE;

    BusTime = *OutTimeUsed + CalcBusTime;
    *OutTimeUsed += CalcBusTime;

    if (BusTime > LimitAllocation)
    {
        DPRINT("USB2_AllocateCheck: BusTime > LimitAllocation\n");
        Result = FALSE;
    }

    return Result;
}

USHORT
NTAPI
USB2_AddDataBitStuff(IN USHORT DataTime)
{
    return (DataTime + (DataTime / 16));
}

BOOLEAN
NTAPI
USB2_CheckTtEndpointInsert(IN PUSB2_TT_ENDPOINT nextTtEndpoint,
                           IN PUSB2_TT_ENDPOINT TtEndpoint)
{
    ULONG TransferType;

    DPRINT("USB2_CheckTtEndpointInsert: nextTtEndpoint - %p, TtEndpoint - %p\n",
           nextTtEndpoint,
           TtEndpoint);

    ASSERT(TtEndpoint);

    if (TtEndpoint->CalcBusTime >= (USB2_FS_MAX_PERIODIC_ALLOCATION / 2))
    {
        DPRINT1("USB2_CheckTtEndpointInsert: Result - FALSE\n");
        return FALSE;
    }

    if (!nextTtEndpoint)
    {
        DPRINT("USB2_CheckTtEndpointInsert: Result - TRUE\n");
        return TRUE;
    }

    TransferType = TtEndpoint->TtEndpointParams.TransferType;

    if (nextTtEndpoint->ActualPeriod < TtEndpoint->ActualPeriod &&
        TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        DPRINT("USB2_CheckTtEndpointInsert: Result - TRUE\n");
        return TRUE;
    }

    if ((nextTtEndpoint->ActualPeriod <= TtEndpoint->ActualPeriod &&
        TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS) ||
        nextTtEndpoint == TtEndpoint)
    {
        DPRINT("USB2_CheckTtEndpointInsert: Result - TRUE\n");
        return TRUE;
    }

    DPRINT("USB2_CheckTtEndpointInsert: Result - FALSE\n");
    return FALSE;
}

ULONG
NTAPI
USB2_GetOverhead(IN PUSB2_TT_ENDPOINT TtEndpoint)
{
    ULONG TransferType;
    ULONG Direction;
    ULONG DeviceSpeed;
    ULONG Overhead;
    ULONG HostDelay;

    TransferType = TtEndpoint->TtEndpointParams.TransferType;
    Direction = TtEndpoint->TtEndpointParams.Direction;
    DeviceSpeed = TtEndpoint->TtEndpointParams.Direction;

    HostDelay = TtEndpoint->Tt->HcExtension->HcDelayTime;

    if (DeviceSpeed == UsbHighSpeed)
    {
        if (Direction == USBPORT_TRANSFER_DIRECTION_OUT)
        {
            if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
                Overhead = HostDelay + USB2_HS_ISOCHRONOUS_OUT_OVERHEAD;
            else
                Overhead = HostDelay + USB2_HS_INTERRUPT_OUT_OVERHEAD;
        }
        else
        {
            if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
                Overhead = HostDelay + USB2_HS_ISOCHRONOUS_IN_OVERHEAD;
            else
                Overhead = HostDelay + USB2_HS_ISOCHRONOUS_OUT_OVERHEAD;
        }
    }
    else if (DeviceSpeed == UsbFullSpeed)
    {
        if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
            Overhead = HostDelay + USB2_FS_ISOCHRONOUS_OVERHEAD;
        else
            Overhead = HostDelay + USB2_FS_INTERRUPT_OVERHEAD;
    }
    else
    {
        Overhead = HostDelay + USB2_LS_INTERRUPT_OVERHEAD;
    }

    return Overhead;
}

ULONG
NTAPI
USB2_GetLastIsoTime(IN PUSB2_TT_ENDPOINT TtEndpoint,
                    IN ULONG Frame)
{
    PUSB2_TT_ENDPOINT nextTtEndpoint;
    ULONG Result;

    DPRINT("USB2_GetLastIsoTime: TtEndpoint - %p, Frame - %X\n",
           TtEndpoint,
           Frame);

    nextTtEndpoint = TtEndpoint->Tt->FrameBudget[Frame].IsoEndpoint->NextTtEndpoint;

    if (nextTtEndpoint ||
        (nextTtEndpoint = TtEndpoint->Tt->FrameBudget[Frame].AltEndpoint) != NULL)
    {
        Result = nextTtEndpoint->StartTime + nextTtEndpoint->CalcBusTime;
    }
    else
    {
        Result = USB2_FS_SOF_TIME;
    }

    return Result;
}

VOID
NTAPI
USB2_InitTtEndpoint(IN PUSB2_TT_ENDPOINT TtEndpoint,
                    IN UCHAR TransferType,
                    IN UCHAR Direction,
                    IN UCHAR DeviceSpeed,
                    IN USHORT Period,
                    IN USHORT MaxPacketSize,
                    IN PUSB2_TT Tt)
{
    RtlZeroMemory(TtEndpoint, sizeof(USB2_TT_ENDPOINT));

    TtEndpoint->TtEndpointParams.TransferType = TransferType;
    TtEndpoint->TtEndpointParams.Direction = Direction;
    TtEndpoint->TtEndpointParams.DeviceSpeed = DeviceSpeed;

    TtEndpoint->Period = Period;
    TtEndpoint->MaxPacketSize = MaxPacketSize;
    TtEndpoint->Tt = Tt;
}

BOOLEAN
NTAPI
USB2_AllocateTimeForEndpoint(IN PUSB2_TT_ENDPOINT TtEndpoint,
                             IN PUSB2_REBALANCE Rebalance,
                             IN PULONG RebalanceListEntries)
{
    DPRINT("USB2_AllocateTimeForEndpoint: UNIMPLEMENTED. FIXME\n");
    ASSERT(FALSE);
    return FALSE;
}

BOOLEAN
NTAPI
USB2_PromotePeriods(IN PUSB2_TT_ENDPOINT TtEndpoint,
                    IN PUSB2_REBALANCE Rebalance,
                    IN PULONG RebalanceListEntries)
{
    DPRINT1("USB2_PromotePeriods: UNIMPLEMENTED. FIXME\n");
    ASSERT(FALSE);
    return FALSE;
}

VOID
NTAPI
USBPORT_UpdateAllocatedBwTt(IN PUSB2_TT_EXTENSION TtExtension)
{
    ULONG BusBandwidth;
    ULONG NewBusBandwidth;
    ULONG MaxBusBandwidth = 0;
    ULONG MinBusBandwidth;
    ULONG ix;

    DPRINT("USBPORT_UpdateAllocatedBwTt: TtExtension - %p\n", TtExtension);

    BusBandwidth = TtExtension->BusBandwidth;
    MinBusBandwidth = BusBandwidth;

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        NewBusBandwidth = BusBandwidth - TtExtension->Bandwidth[ix];

        if (NewBusBandwidth > MaxBusBandwidth)
            MaxBusBandwidth = NewBusBandwidth;

        if (NewBusBandwidth < MinBusBandwidth)
            MinBusBandwidth = NewBusBandwidth;
    }

    TtExtension->MaxBandwidth = MaxBusBandwidth;

    if (MinBusBandwidth == BusBandwidth)
        TtExtension->MinBandwidth = 0;
    else
        TtExtension->MinBandwidth = MinBusBandwidth;
}

BOOLEAN
NTAPI
USBPORT_AllocateBandwidthUSB2(IN PDEVICE_OBJECT FdoDevice,
                              IN PUSBPORT_ENDPOINT Endpoint)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties;
    PUSB2_TT_EXTENSION TtExtension;
    ULONG TransferType;
    PUSB2_REBALANCE Rebalance;
    LIST_ENTRY RebalanceList;
    ULONG RebalanceListEntries;
    PUSB2_TT_ENDPOINT TtEndpoint;
    PUSB2_TT_ENDPOINT RebalanceTtEndpoint;

    PUSB2_TT Tt;
    USB_DEVICE_SPEED DeviceSpeed;
    ULONG Period;

    ULONG ix;
    BOOLEAN Direction;
    BOOLEAN Result;

    DPRINT("USBPORT_AllocateBandwidthUSB2: FdoDevice - %p, Endpoint - %p\n",
           FdoDevice,
           Endpoint);

    EndpointProperties = &Endpoint->EndpointProperties;
    EndpointProperties->ScheduleOffset = 0;

    if (Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0)
    {
        DPRINT("USBPORT_AllocateBandwidthUSB2: ENDPOINT_FLAG_ROOTHUB_EP0\n");
        return TRUE;
    }

    FdoExtension = FdoDevice->DeviceExtension;

    TransferType = EndpointProperties->TransferType;
    DPRINT("USBPORT_AllocateBandwidthUSB2: TransferType - %X\n", TransferType);

    if (TransferType == USBPORT_TRANSFER_TYPE_CONTROL ||
        TransferType == USBPORT_TRANSFER_TYPE_BULK)
    {
        return TRUE;
    }

    if (Endpoint->TtExtension)
        TtExtension = Endpoint->TtExtension;
    else
        TtExtension = NULL;

    InitializeListHead(&RebalanceList);

    Rebalance = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(USB2_REBALANCE),
                                      USB_PORT_TAG);

    DPRINT("USBPORT_AllocateBandwidthUSB2: Rebalance - %p, TtExtension - %p\n",
           Rebalance,
           TtExtension);

    if (Rebalance)
    {
        RtlZeroMemory(Rebalance, sizeof(USB2_REBALANCE));

        TtEndpoint = Endpoint->TtEndpoint;
        TtEndpoint->Endpoint = Endpoint;

        Direction = EndpointProperties->Direction == USBPORT_TRANSFER_DIRECTION_OUT;
        DeviceSpeed = EndpointProperties->DeviceSpeed;

        switch (DeviceSpeed)
        {
            case UsbLowSpeed:
            case UsbFullSpeed:
            {
                Tt = &TtExtension->Tt;
                Period = USB2_FRAMES;

                while (Period && Period > EndpointProperties->Period);
                {
                    Period >>= 1;
                }

                DPRINT("USBPORT_AllocateBandwidthUSB2: Period - %X\n", Period);
                break;
            }

            case UsbHighSpeed:
            {
                Tt = &FdoExtension->Usb2Extension->HcTt;
                Period = EndpointProperties->Period;

                if (EndpointProperties->Period > USB2_MAX_MICROFRAMES)
                    Period = USB2_MAX_MICROFRAMES;

                break;
            }

            default:
            {
                DPRINT1("USBPORT_AllocateBandwidthUSB2: DeviceSpeed - %X!\n", DeviceSpeed);
                DbgBreakPoint();
                Tt = &TtExtension->Tt;
                break;
            }
        }

        USB2_InitTtEndpoint(TtEndpoint,
                            TransferType,
                            Direction,
                            DeviceSpeed,
                            Period,
                            EndpointProperties->MaxPacketSize,
                            Tt);

        RebalanceListEntries = USB2_FRAMES - 2;

        Result = USB2_AllocateTimeForEndpoint(TtEndpoint,
                                              Rebalance,
                                              &RebalanceListEntries);

        if (Result)
        {
            Result = USB2_PromotePeriods(TtEndpoint,
                                         Rebalance,
                                         &RebalanceListEntries);
        }

        RebalanceListEntries = 0;

        for (ix = 0; Rebalance->RebalanceEndpoint[ix]; ix++)
        {
            RebalanceListEntries = ix + 1;
        }
    }
    else
    {
        RebalanceListEntries = 0;
        Result = FALSE;
    }

    DPRINT("USBPORT_AllocateBandwidthUSB2: RebalanceListEntries - %X, Result - %X\n",
           RebalanceListEntries,
           Result);

    for (ix = 0; ix < RebalanceListEntries; ix++)
    {
        DPRINT("USBPORT_AllocateBandwidthUSB2: RebalanceEndpoint[%X] - %X\n",
               ix,
               Rebalance->RebalanceEndpoint[ix]);

        RebalanceTtEndpoint = Rebalance->RebalanceEndpoint[ix];

        InsertTailList(&RebalanceList,
                       &RebalanceTtEndpoint->Endpoint->RebalanceLink);
    }

    if (Rebalance)
        ExFreePool(Rebalance);

    if (Result)
    {
        DPRINT1("USBPORT_AllocateBandwidthUSB2: UNIMPLEMENTED. FIXME. \n");
        ASSERT(FALSE);
    }

    //USB2_Rebalance(FdoDevice, &RebalanceList);

    if (!TtExtension)
    {
        DPRINT("USBPORT_AllocateBandwidthUSB2: Result - %X\n", Result);
        return Result;
    }

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        FdoExtension->Bandwidth[ix] += TtExtension->MaxBandwidth;
    }

    USBPORT_UpdateAllocatedBwTt(TtExtension);

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        FdoExtension->Bandwidth[ix] -= TtExtension->MaxBandwidth;
    }

    DPRINT("USBPORT_AllocateBandwidthUSB2: Result - %X\n", Result);

    return Result;
}

VOID
NTAPI
USBPORT_FreeBandwidthUSB2(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_ENDPOINT Endpoint)
{
    DPRINT1("USBPORT_FreeBandwidthUSB2: UNIMPLEMENTED. FIXME. \n");
}

VOID
NTAPI
USB2_InitTT(IN PUSB2_HC_EXTENSION HcExtension,
            IN PUSB2_TT Tt)
{
    ULONG ix;
    ULONG jx;

    DPRINT("USB2_InitTT: HcExtension - %p, Tt - %p\n", HcExtension, Tt);

    Tt->HcExtension = HcExtension;
    Tt->DelayTime = 1;
    Tt->MaxTime = USB2_FS_MAX_PERIODIC_ALLOCATION;

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        Tt->FrameBudget[ix].TimeUsed = USB2_MAX_MICROFRAMES;
        Tt->FrameBudget[ix].AltEndpoint = NULL;

        for (jx = 0; jx < USB2_MICROFRAMES; jx++)
        {
            Tt->TimeCS[ix][jx] = 0;
            Tt->NumStartSplits[ix][jx] = 0;
        }

        Tt->FrameBudget[ix].IsoEndpoint = &Tt->IsoEndpoint[ix];

        USB2_InitTtEndpoint(&Tt->IsoEndpoint[ix],
                            USBPORT_TRANSFER_TYPE_ISOCHRONOUS,
                            USBPORT_TRANSFER_DIRECTION_OUT,
                            UsbFullSpeed,
                            USB2_FRAMES,
                            0,
                            Tt);

        Tt->IsoEndpoint[ix].ActualPeriod = USB2_FRAMES;
        Tt->IsoEndpoint[ix].CalcBusTime = USB2_FS_SOF_TIME + USB2_HUB_DELAY;
        Tt->IsoEndpoint[ix].StartFrame = ix;
        Tt->IsoEndpoint[ix].StartMicroframe = 0xFF;

        Tt->FrameBudget[ix].IntEndpoint = &Tt->IntEndpoint[ix];

        USB2_InitTtEndpoint(&Tt->IntEndpoint[ix],
                            USBPORT_TRANSFER_TYPE_INTERRUPT,
                            USBPORT_TRANSFER_DIRECTION_OUT,
                            UsbFullSpeed,
                            USB2_FRAMES,
                            0,
                            Tt);

        Tt->IntEndpoint[ix].ActualPeriod = USB2_FRAMES;
        Tt->IntEndpoint[ix].CalcBusTime = USB2_FS_SOF_TIME + USB2_HUB_DELAY;
        Tt->IntEndpoint[ix].StartFrame = ix;
        Tt->IntEndpoint[ix].StartMicroframe = 0xFF;
    }
}

VOID
NTAPI
USB2_InitController(IN PUSB2_HC_EXTENSION HcExtension)
{
    ULONG ix;
    ULONG jx;

    DPRINT("USB2_InitController: HcExtension - %p\n", HcExtension);

    HcExtension->MaxHsBusAllocation = USB2_MAX_MICROFRAME_ALLOCATION;

    for (ix = 0; ix < USB2_FRAMES; ix++)
    {
        for (jx = 0; jx < USB2_MICROFRAMES; jx++)
        {
            HcExtension->TimeUsed[ix][jx] = 0;
        }
    }

    HcExtension->HcDelayTime = USB2_CONTROLLER_DELAY;

    USB2_InitTT(HcExtension, &HcExtension->HcTt);
}
