/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/loopback.c
 * PURPOSE:     Loopback adapter
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <loopback.h>
#include <ip.h>
#include <address.h>
#include <receive.h>
#include <transmit.h>
#include <routines.h>


WORK_QUEUE_ITEM  LoopWorkItem;
PIP_INTERFACE    Loopback = NULL;
/* Indicates wether the loopback interface is currently transmitting */
BOOLEAN          LoopBusy = FALSE;
/* Loopback transmit queue */
PNDIS_PACKET     LoopQueueHead = (PNDIS_PACKET)NULL;
PNDIS_PACKET     LoopQueueTail = (PNDIS_PACKET)NULL;
/* Spin lock for protecting loopback transmit queue */
KSPIN_LOCK       LoopLock;


VOID RealTransmit(
    PVOID Context)
/*
 * FUNCTION: Transmits one or more packet(s) in loopback queue to ourselves
 * ARGUMENTS:
 *     Context = Pointer to context information (loopback interface)
 */
{
    KIRQL OldIrql;
    PNDIS_PACKET NdisPacket;
    IP_PACKET IPPacket;
    PNDIS_BUFFER Buffer;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeAcquireSpinLockAtDpcLevel(&LoopLock);

    for (;;) {
        /* Get the next packet from the queue (if any) */
        NdisPacket = LoopQueueHead;
        if (!NdisPacket)
            break;

        LoopQueueHead = *(PNDIS_PACKET*)NdisPacket->u.s3.MacReserved;

        KeReleaseSpinLockFromDpcLevel(&LoopLock);

        IPPacket.NdisPacket = NdisPacket;

        NdisGetFirstBufferFromPacket(NdisPacket,
                                    &Buffer,
                                    &IPPacket.Header,
                                    &IPPacket.ContigSize,
                                    &IPPacket.TotalSize);

        IPReceive(Context, &IPPacket);

        AdjustPacket(NdisPacket, 0, PC(NdisPacket)->DLOffset);

        PC(NdisPacket)->DLComplete(Context, NdisPacket, NDIS_STATUS_SUCCESS);

        /* Lower IRQL for a moment to prevent starvation */
        KeLowerIrql(OldIrql);

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        KeAcquireSpinLockAtDpcLevel(&LoopLock);
    }

    LoopBusy = FALSE;

    KeReleaseSpinLockFromDpcLevel(&LoopLock);

    KeLowerIrql(OldIrql);
}


VOID LoopTransmit(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    UINT Offset,
    PVOID LinkAddress,
    USHORT Type)
/*
 * FUNCTION: Transmits a packet
 * ARGUMENTS:
 *     Context     = Pointer to context information (NULL)
 *     NdisPacket  = Pointer to NDIS packet to send
 *     Offset      = Offset in packet where packet data starts
 *     LinkAddress = Pointer to link address
 *     Type        = LAN protocol type (unused)
 */
{
    PNDIS_PACKET *pNdisPacket;
    KIRQL OldIrql;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* NDIS send routines don't have an offset argument so we
       must offset the data in upper layers and adjust the
       packet here. We save the offset in the packet context
       area so it can be undone before we release the packet */
    AdjustPacket(NdisPacket, Offset, 0);
    PC(NdisPacket)->DLOffset = Offset;

    pNdisPacket  = (PNDIS_PACKET*)NdisPacket->u.s3.MacReserved;
    *pNdisPacket = NULL;

    KeAcquireSpinLock(&LoopLock, &OldIrql);

    /* Add packet to transmit queue */
    if (LoopQueueHead) {
        /* Transmit queue is not empty */
        pNdisPacket  = (PNDIS_PACKET*)LoopQueueTail->u.s3.MacReserved;
        *pNdisPacket = NdisPacket;
    } else
        /* Transmit queue is empty */
        LoopQueueHead = NdisPacket;

    LoopQueueTail = NdisPacket;

    /* If LoopTransmit is not running (or scheduled), schedule it to run */
    if (!LoopBusy) {
        ExQueueWorkItem(&LoopWorkItem, CriticalWorkQueue);
        LoopBusy = TRUE;
    }

    KeReleaseSpinLock(&LoopLock, OldIrql);
}


NDIS_STATUS LoopRegisterAdapter(
    PNDIS_STRING AdapterName,
    PLAN_ADAPTER *Adapter)
/*
 * FUNCTION: Registers loopback adapter
 * ARGUMENTS:
 *     AdapterName = Unused
 *     Adapter     = Unused
 * RETURNS:
 *     Status of operation
 */
{
    PIP_ADDRESS Address;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    Address = AddrBuildIPv4(LOOPBACK_ADDRESS_IPv4);
    if (Address) {
        LLIP_BIND_INFO BindInfo;

        /* Bind the adapter to IP layer */
        BindInfo.Context       = NULL;
        BindInfo.HeaderSize    = 0;
        BindInfo.MinFrameSize  = 0;
        BindInfo.MTU           = 16384;
        BindInfo.Address       = NULL;
        BindInfo.AddressLength = 0;
        BindInfo.Transmit      = LoopTransmit;

        Loopback = IPCreateInterface(&BindInfo);
        if ((Loopback) && (IPCreateNTE(Loopback, Address, 8))) {
            /* Reference the interface for the NTE. The reference for
               the address is just passed on to the NTE */
            ReferenceObject(Loopback);

            IPRegisterInterface(Loopback);

            ExInitializeWorkItem(&LoopWorkItem, RealTransmit, Loopback);

            KeInitializeSpinLock(&LoopLock);
            LoopBusy = FALSE;
        } else
            Status = NDIS_STATUS_RESOURCES;
    } else
        Status = NDIS_STATUS_RESOURCES;

    if (!NT_SUCCESS(Status))
        LoopUnregisterAdapter(NULL);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

    return Status;
}


NDIS_STATUS LoopUnregisterAdapter(
    PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Unregisters loopback adapter
 * ARGUMENTS:
 *     Adapter = Unused
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Does not care wether we have registered loopback adapter
 */
{
    TI_DbgPrint(MID_TRACE, ("Called.\n"));

    if (Loopback) {
        IPUnregisterInterface(Loopback);
        IPDestroyInterface(Loopback);
        Loopback = NULL;
    }

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

    return NDIS_STATUS_SUCCESS;
}

/* EOF */
