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

VOID
NTAPI
USB2_IncMicroFrame(OUT PUCHAR frame,
                   OUT PUCHAR uframe)
{
    ++*uframe;

    if (*uframe > (USB2_MICROFRAMES - 1))
    {
        *uframe = 0;
        *frame = (*frame + 1) & (USB2_FRAMES - 1);
    }
}

VOID
NTAPI
USB2_GetPrevMicroFrame(OUT PUCHAR frame,
                       OUT PUCHAR uframe)
{
    *uframe = USB2_MICROFRAMES - 1;

    if (*frame)
        --*frame;
    else
        *frame = USB2_FRAMES - 1;
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

VOID
NTAPI
USB2_GetHsOverhead(IN PUSB2_TT_ENDPOINT TtEndpoint,
                   IN PULONG OverheadSS,
                   IN PULONG OverheadCS)
{
    ULONG TransferType;
    ULONG Direction;
    ULONG HostDelay;

    TransferType = TtEndpoint->TtEndpointParams.TransferType;
    Direction = TtEndpoint->TtEndpointParams.Direction;

    HostDelay = TtEndpoint->Tt->HcExtension->HcDelayTime;

    if (Direction == USBPORT_TRANSFER_DIRECTION_OUT)
    {
        if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            *OverheadSS = HostDelay + USB2_HS_SS_ISOCHRONOUS_OUT_OVERHEAD;
            *OverheadCS = 0;
        }
        else
        {
            *OverheadSS = HostDelay + USB2_HS_SS_INTERRUPT_OUT_OVERHEAD;
            *OverheadCS = HostDelay + USB2_HS_CS_INTERRUPT_OUT_OVERHEAD;
        }
    }
    else
    {
        if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
        {
            *OverheadSS = HostDelay + USB2_HS_SS_ISOCHRONOUS_IN_OVERHEAD;
            *OverheadCS = HostDelay + USB2_HS_CS_ISOCHRONOUS_IN_OVERHEAD;
        }
        else
        {
            *OverheadSS = HostDelay + USB2_HS_SS_INTERRUPT_IN_OVERHEAD;
            *OverheadCS = HostDelay + USB2_HS_CS_INTERRUPT_IN_OVERHEAD;
        }

        DPRINT("USB2_GetHsOverhead: *OverheadSS - %X, *OverheadCS - %X\n",
               *OverheadSS,
               *OverheadCS);
    }
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

ULONG
NTAPI
USB2_GetStartTime(IN PUSB2_TT_ENDPOINT nextTtEndpoint,
                  IN PUSB2_TT_ENDPOINT TtEndpoint,
                  IN PUSB2_TT_ENDPOINT prevTtEndpoint,
                  IN ULONG Frame)
{
    PUSB2_TT_ENDPOINT ttEndpoint;
    ULONG TransferType;

    DPRINT("USB2_GetStartTime: nextTtEndpoint - %p, TtEndpoint - %p, prevTtEndpoint - %p, Frame - %X\n",
           nextTtEndpoint,
           TtEndpoint,
           prevTtEndpoint,
           Frame);

    TransferType = TtEndpoint->TtEndpointParams.TransferType;

    if (nextTtEndpoint && TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        return nextTtEndpoint->StartTime + nextTtEndpoint->CalcBusTime;
    }

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        ttEndpoint = TtEndpoint->Tt->FrameBudget[Frame].AltEndpoint;

        if (ttEndpoint)
           return ttEndpoint->StartTime + ttEndpoint->CalcBusTime;
        else
           return USB2_FS_SOF_TIME;
    }
    else
    {
        ttEndpoint = prevTtEndpoint;

        if (ttEndpoint == TtEndpoint->Tt->FrameBudget[Frame].IntEndpoint)
            return USB2_GetLastIsoTime(TtEndpoint, Frame);
        else
            return ttEndpoint->StartTime + ttEndpoint->CalcBusTime;
    }
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
USB2_AllocateHS(IN PUSB2_TT_ENDPOINT TtEndpoint,
                IN LONG Frame)
{
    PUSB2_HC_EXTENSION HcExtension;
    PUSB2_TT Tt;
    ULONG TransferType;
    ULONG Direction;
    ULONG DataTime;
    ULONG RemainDataTime;
    ULONG OverheadCS;
    ULONG OverheadSS;
    ULONG ix;
    USHORT PktSize;
    UCHAR frame;
    UCHAR uframe;
    BOOL Result = TRUE;

    DPRINT("USB2_AllocateHS: TtEndpoint - %p, Frame - %X, TtEndpoint->StartFrame - %X\n",
           TtEndpoint,
           Frame,
           TtEndpoint->StartFrame);

    Tt = TtEndpoint->Tt;
    HcExtension = Tt->HcExtension;

    TransferType = TtEndpoint->TtEndpointParams.TransferType;
    Direction = TtEndpoint->TtEndpointParams.Direction;

    if (Frame == 0)
    {
        TtEndpoint->StartMicroframe =
        TtEndpoint->StartTime / USB2_FS_RAW_BYTES_IN_MICROFRAME - 1;

        DPRINT("USB2_AllocateHS: TtEndpoint->StartMicroframe - %X\n",
               TtEndpoint->StartMicroframe);
    }

    USB2_GetHsOverhead(TtEndpoint, &OverheadSS, &OverheadCS);

    if (TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
    {
        if (Frame == 0)
        {
            TtEndpoint->Nums.NumStarts = 1;

            if ((CHAR)TtEndpoint->StartMicroframe < 5)
            {
                TtEndpoint->Nums.NumCompletes = 3;
            }
            else
            {
                TtEndpoint->Nums.NumCompletes = 2;
            }
        }
    }
    else
    {
        if (Direction == USBPORT_TRANSFER_DIRECTION_OUT)
        {
            DPRINT("USB2_AllocateHS: ISO UNIMPLEMENTED\n");
            ASSERT(FALSE);
        }
        else
        {
            DPRINT("USB2_AllocateHS: ISO UNIMPLEMENTED\n");
            ASSERT(FALSE);
        }
    }

    frame = TtEndpoint->StartFrame + Frame;
    uframe = TtEndpoint->StartMicroframe;

    if (TtEndpoint->StartMicroframe == 0xFF)
        USB2_GetPrevMicroFrame(&frame, &uframe);

    for (ix = 0; ix < TtEndpoint->Nums.NumStarts; ix++)
    {
        if (!USB2_AllocateCheck(&HcExtension->TimeUsed[frame][uframe],
                                OverheadSS,
                                USB2_MAX_MICROFRAME_ALLOCATION))
        {
            Result = FALSE;
        }

        if (Tt->NumStartSplits[frame][uframe] > (USB2_MAX_FS_LS_TRANSACTIONS_IN_UFRAME - 1))
        {
            DPRINT1("USB2_AllocateHS: Num Start Splits - %X\n",
                    Tt->NumStartSplits[frame][uframe] + 1);

            ASSERT(FALSE);
            Result = FALSE;
        }

        ++Tt->NumStartSplits[frame][uframe];
        USB2_IncMicroFrame(&frame, &uframe);
    }

    frame = TtEndpoint->StartFrame + Frame;
    uframe = TtEndpoint->StartMicroframe + TtEndpoint->Nums.NumStarts + 1;

    for (ix = 0; ix < TtEndpoint->Nums.NumCompletes; ix++)
    {
        if (!USB2_AllocateCheck(&HcExtension->TimeUsed[frame][uframe],
                                OverheadCS,
                                USB2_MAX_MICROFRAME_ALLOCATION))
        {
            Result = FALSE;
        }

        USB2_IncMicroFrame(&frame, &uframe);
    }

    if (Direction == USBPORT_TRANSFER_DIRECTION_OUT)
    {
        DPRINT("USB2_AllocateHS: DIRECTION OUT UNIMPLEMENTED\n");
        ASSERT(FALSE);
    }
    else
    {
        frame = TtEndpoint->StartFrame + Frame;
        uframe = TtEndpoint->StartMicroframe + TtEndpoint->Nums.NumStarts + 1;

        for (ix = 0; ix < TtEndpoint->Nums.NumCompletes; ix++)
        {
            if (Tt->TimeCS[frame][uframe] < USB2_FS_RAW_BYTES_IN_MICROFRAME)
            {
                if (Tt->TimeCS[frame][uframe] < USB2_FS_RAW_BYTES_IN_MICROFRAME)
                {
                    RemainDataTime = USB2_FS_RAW_BYTES_IN_MICROFRAME -
                                     Tt->TimeCS[frame][uframe];
                }
                else
                {
                    RemainDataTime = 0;
                }

                PktSize = TtEndpoint->MaxPacketSize;

                if (RemainDataTime >= USB2_AddDataBitStuff(PktSize))
                {
                    DataTime = USB2_AddDataBitStuff(PktSize);
                }
                else
                {
                    DataTime = RemainDataTime;
                }

                if (!USB2_AllocateCheck(&HcExtension->TimeUsed[frame][uframe],
                                        DataTime,
                                        USB2_MAX_MICROFRAME_ALLOCATION))
                {
                    Result = FALSE;
                }
            }

            PktSize = TtEndpoint->MaxPacketSize;

            if (USB2_AddDataBitStuff(PktSize) < USB2_FS_RAW_BYTES_IN_MICROFRAME)
            {
                Tt->TimeCS[frame][uframe] += USB2_AddDataBitStuff(PktSize);
            }
            else
            {
                Tt->TimeCS[frame][uframe] += USB2_FS_RAW_BYTES_IN_MICROFRAME;
            }

            USB2_IncMicroFrame(&frame, &uframe);
        }
    }

    DPRINT("USB2_AllocateHS: Result - %X\n", Result);
    return Result;
}

BOOLEAN
NTAPI
USB2_DeallocateEndpointBudget(IN PUSB2_TT_ENDPOINT TtEndpoint,
                              IN PUSB2_REBALANCE Rebalance,
                              IN PULONG RebalanceListEntries,
                              IN ULONG MaxFrames)
{
    DPRINT("USB2_DeallocateEndpointBudget: UNIMPLEMENTED FIXME\n");
    ASSERT(FALSE);
    return FALSE;
}

BOOLEAN
NTAPI
USB2_AllocateTimeForEndpoint(IN PUSB2_TT_ENDPOINT TtEndpoint,
                             IN PUSB2_REBALANCE Rebalance,
                             IN PULONG RebalanceListEntries)
{
    PUSB2_TT Tt;
    PUSB2_HC_EXTENSION HcExtension;
    ULONG Speed;
    ULONG TimeUsed;
    ULONG MinTimeUsed;
    ULONG ix;
    ULONG frame;
    ULONG uframe;
    ULONG Microframe;
    ULONG TransferType;
    ULONG Overhead;
    BOOLEAN Result = TRUE;

    DPRINT("USB2_AllocateTimeForEndpoint: TtEndpoint - %p\n", TtEndpoint);

    Tt = TtEndpoint->Tt;
    HcExtension = Tt->HcExtension;

    TtEndpoint->Nums.NumStarts = 0;
    TtEndpoint->Nums.NumCompletes = 0;

    TtEndpoint->StartFrame = 0;
    TtEndpoint->StartMicroframe = 0;

    if (TtEndpoint->CalcBusTime)
    {
        DPRINT("USB2_AllocateTimeForEndpoint: TtEndpoint already allocated!\n");
        return FALSE;
    }

    Speed = TtEndpoint->TtEndpointParams.DeviceSpeed;

    if (Speed == UsbHighSpeed)
    {
        if (TtEndpoint->Period > USB2_MAX_MICROFRAMES)
            TtEndpoint->ActualPeriod = USB2_MAX_MICROFRAMES;
        else
            TtEndpoint->ActualPeriod = TtEndpoint->Period;

        MinTimeUsed = HcExtension->TimeUsed[0][0];

        for (ix = 1; ix < TtEndpoint->ActualPeriod; ix++)
        {
            frame = ix / USB2_MICROFRAMES;
            uframe = ix % (USB2_MICROFRAMES - 1);

            TimeUsed = HcExtension->TimeUsed[frame][uframe];

            if (TimeUsed < MinTimeUsed)
            {
                MinTimeUsed = TimeUsed;
                TtEndpoint->StartFrame = frame;
                TtEndpoint->StartMicroframe = uframe;
            }
        }

        TtEndpoint->CalcBusTime = USB2_GetOverhead(TtEndpoint) +
                                  USB2_AddDataBitStuff(TtEndpoint->MaxPacketSize);

        DPRINT("USB2_AllocateTimeForEndpoint: StartFrame - %X, StartMicroframe - %X, CalcBusTime - %X\n",
               TtEndpoint->StartFrame,
               TtEndpoint->StartMicroframe,
               TtEndpoint->CalcBusTime);

        Microframe = TtEndpoint->StartFrame * USB2_MICROFRAMES +
                     TtEndpoint->StartMicroframe;

        if (Microframe >= USB2_MAX_MICROFRAMES)
        {
            DPRINT("USB2_AllocateTimeForEndpoint: Microframe >= 256. Result - TRUE\n");
            return TRUE;
        }

        for (ix = Microframe;
             ix < USB2_MAX_MICROFRAMES;
             ix += TtEndpoint->ActualPeriod)
        {
            frame = ix / USB2_MICROFRAMES;
            uframe = ix % (USB2_MICROFRAMES - 1);

            DPRINT("USB2_AllocateTimeForEndpoint: frame - %X, uframe - %X, TimeUsed[f][uf] - %X\n",
                   frame,
                   uframe,
                   HcExtension->TimeUsed[frame][uframe]);

            if (!USB2_AllocateCheck(&HcExtension->TimeUsed[frame][uframe],
                                    TtEndpoint->CalcBusTime,
                                    USB2_MAX_MICROFRAME_ALLOCATION))
            {
                DPRINT("USB2_AllocateTimeForEndpoint: Result = FALSE\n");
                Result = FALSE;
            }
        }

        if (!Result)
        {
            for (ix = Microframe;
                 ix < USB2_MAX_MICROFRAMES;
                 ix += TtEndpoint->ActualPeriod)
            {
                frame = ix / USB2_MICROFRAMES;
                uframe = ix % (USB2_MICROFRAMES - 1);

                HcExtension->TimeUsed[frame][uframe] -= TtEndpoint->CalcBusTime;
            }
        }

        DPRINT("USB2_AllocateTimeForEndpoint: Result - TRUE\n");
        return TRUE;
    }

    /* Speed != UsbHighSpeed (FS/LS) */

    if (TtEndpoint->Period > USB2_FRAMES)
        TtEndpoint->ActualPeriod = USB2_FRAMES;
    else
        TtEndpoint->ActualPeriod = TtEndpoint->Period;

    MinTimeUsed = Tt->FrameBudget[0].TimeUsed;

    for (ix = 1; ix < TtEndpoint->ActualPeriod; ix++)
    {
        if ((Tt->FrameBudget[ix].TimeUsed) < MinTimeUsed)
        {
            MinTimeUsed = Tt->FrameBudget[ix].TimeUsed;
            TtEndpoint->StartFrame = ix;
        }
    }

    TransferType = TtEndpoint->TtEndpointParams.TransferType;

    if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS)
    {
        if (Speed == UsbFullSpeed)
        {
            Overhead = USB2_FS_ISOCHRONOUS_OVERHEAD + Tt->DelayTime;
        }
        else
        {
            DPRINT("USB2_AllocateTimeForEndpoint: ISO can not be on a LS bus!\n");
            return FALSE;
        }
    }
    else
    {
        if (Speed == UsbFullSpeed)
            Overhead = USB2_FS_INTERRUPT_OVERHEAD + Tt->DelayTime;
        else
            Overhead = USB2_LS_INTERRUPT_OVERHEAD + Tt->DelayTime;
    }

    if (Speed == UsbLowSpeed)
    {
        TtEndpoint->CalcBusTime = TtEndpoint->MaxPacketSize * 8 + Overhead;
    }
    else
    {
        TtEndpoint->CalcBusTime = TtEndpoint->MaxPacketSize + Overhead;
    }


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
    ULONG AllocedBusTime;
    ULONG EndpointBandwidth;
    ULONG ScheduleOffset;
    ULONG Factor;
    ULONG ix;
    ULONG n;
    BOOLEAN Direction;
    UCHAR SMask;
    UCHAR CMask;
    UCHAR ActualPeriod;
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

                while (Period > 0 && Period > EndpointProperties->Period)
                {
                    Period >>= 1;
                }

                DPRINT("USBPORT_AllocateBandwidthUSB2: Period - %X\n", Period);
                break;
            }

            case UsbHighSpeed:
            {
                Tt = &FdoExtension->Usb2Extension->HcTt;

                if (EndpointProperties->Period > USB2_MAX_MICROFRAMES)
                    Period = USB2_MAX_MICROFRAMES;
                else
                    Period = EndpointProperties->Period;

                break;
            }

            default:
            {
                DPRINT1("USBPORT_AllocateBandwidthUSB2: DeviceSpeed - %X!\n",
                        DeviceSpeed);

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
        RebalanceTtEndpoint = Rebalance->RebalanceEndpoint[ix];

        DPRINT("USBPORT_AllocateBandwidthUSB2: RebalanceTtEndpoint[%X] - %p, RebalanceTtEndpoint - %p, RebalanceLink - %p\n",
               ix,
               RebalanceTtEndpoint,
               &RebalanceTtEndpoint->Endpoint->RebalanceLink);

        InsertTailList(&RebalanceList,
                       &RebalanceTtEndpoint->Endpoint->RebalanceLink);
    }

    if (Rebalance)
        ExFreePoolWithTag(Rebalance, USB_PORT_TAG);

    if (Result)
    {
        SMask = USB2_GetSMASK(Endpoint->TtEndpoint);
        EndpointProperties->InterruptScheduleMask = SMask;

        CMask = USB2_GetCMASK(Endpoint->TtEndpoint);
        EndpointProperties->SplitCompletionMask = CMask;

        AllocedBusTime = TtEndpoint->CalcBusTime;

        EndpointBandwidth = USB2_MICROFRAMES * AllocedBusTime;
        EndpointProperties->UsbBandwidth = EndpointBandwidth;

        ActualPeriod = Endpoint->TtEndpoint->ActualPeriod;
        EndpointProperties->Period = ActualPeriod;

        ScheduleOffset = Endpoint->TtEndpoint->StartFrame;
        EndpointProperties->ScheduleOffset = ScheduleOffset;

        Factor = USB2_FRAMES / ActualPeriod;
        ASSERT(Factor);

        n = ScheduleOffset * Factor;

        if (TtExtension)
        {
            for (ix = 0; ix < Factor; ix++)
            {
                TtExtension->Bandwidth[n + ix] -= EndpointBandwidth;
            }
        }
        else
        {
            for (ix = 1; ix < Factor; ix++)
            {
                FdoExtension->Bandwidth[n + ix] -= EndpointBandwidth;
            }
        }

        USBPORT_DumpingEndpointProperties(EndpointProperties);
        USBPORT_DumpingTtEndpoint(Endpoint->TtEndpoint);

        if (AllocedBusTime >= (USB2_FS_MAX_PERIODIC_ALLOCATION / 2))
        {
            DPRINT1("USBPORT_AllocateBandwidthUSB2: AllocedBusTime >= 0.5 * MAX_ALLOCATION \n");
        }
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
