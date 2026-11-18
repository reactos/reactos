/*
 * COPYRIGHT:   See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@taveraos.com>
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        drivers/network/ndisuio/protocol.c
 * PURPOSE:     Protocol stuff
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 *              Ryan Wilson <ryan@taveraos.com>
 */

/*
 * NDIS 6 open uses NdisOpenAdapterEx with open-parameters header type
 * 0x87 revision 1 size 0x40 from drivers/network/lldp/src/lib.rs /
 * drivers/network/ndis/src/prot6.rs. AdapterName sits at bind+16 on x64
 * (prot6.rs). Learn:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenadapterex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-protocol_bind_adapter_ex
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
NduOpenAdapterComplete(NDIS_HANDLE ProtocolBindingContext,
                       NDIS_STATUS Status,
                       NDIS_STATUS OpenStatus)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter open completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
NduCloseAdapterComplete(NDIS_HANDLE ProtocolBindingContext,
                        NDIS_STATUS Status)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter close completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

NDIS_STATUS
NTAPI
NduNetPnPEvent(NDIS_HANDLE ProtocolBindingContext,
               PNET_PNP_EVENT NetPnPEvent)
{
    PNDIS_DEVICE_POWER_STATE PowerState;

    DPRINT("NetPnPEvent\n");

    switch (NetPnPEvent->NetEvent)
    {
        case NetEventQueryRemoveDevice:
            /* Nothing to do */
            DPRINT1("NetPnPEvent: QueryRemoveDevice\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventSetPower:
            ASSERT(NetPnPEvent->BufferLength >= sizeof(*PowerState));

            PowerState = NetPnPEvent->Buffer;
            switch (*PowerState)
            {
                case NdisDeviceStateD0:
                    DPRINT1("NetPnPEvent: SetPower D0\n");
                    return NDIS_STATUS_SUCCESS;

                default:
                    DPRINT1("NetPnPEvent: SetPower state %d not supported\n", *PowerState);
                    return NDIS_STATUS_FAILURE;
            }

        case NetEventQueryPower:
            DPRINT1("NetPnPEvent: QueryPower\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventCancelRemoveDevice:
            DPRINT1("NetPnPEvent: CancelRemoveDevice\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventReconfigure:
            DPRINT1("NetPnPEvent: Reconfigure\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventBindList:
            DPRINT1("NetPnPEvent: BindList\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventBindsComplete:
            DPRINT("NetPnPEvent: BindsComplete\n");
            return NDIS_STATUS_SUCCESS;

        case NetEventPnPCapabilities:
            DPRINT1("NetPnPEvent: PnPCapabilities\n");
            return NDIS_STATUS_SUCCESS;

        default:
            DPRINT1("NetPnPEvent unimplemented for net event 0x%x\n", NetPnPEvent->NetEvent);
            return NDIS_STATUS_FAILURE;
    }
}

VOID
NTAPI
NduSendComplete(NDIS_HANDLE ProtocolBindingContext,
                PNDIS_PACKET Packet,
                NDIS_STATUS Status)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter send completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
NduTransferDataComplete(NDIS_HANDLE ProtocolBindingContext,
                        PNDIS_PACKET Packet,
                        NDIS_STATUS Status,
                        UINT BytesTransferred)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter transfer completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
NduResetComplete(NDIS_HANDLE ProtocolBindingContext,
                 NDIS_STATUS Status)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter reset completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

VOID
NTAPI
NduRequestComplete(NDIS_HANDLE ProtocolBindingContext,
                   PNDIS_REQUEST NdisRequest,
                   NDIS_STATUS Status)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;

    DPRINT("Asynchronous adapter request completed\n");

    /* Store the final status and signal the event */
    AdapterContext->AsyncStatus = Status;
    KeSetEvent(&AdapterContext->AsyncEvent, IO_NO_INCREMENT, FALSE);
}

NDIS_STATUS
NTAPI
NduReceive(NDIS_HANDLE ProtocolBindingContext,
           NDIS_HANDLE MacReceiveContext,
           PVOID HeaderBuffer,
           UINT HeaderBufferSize,
           PVOID LookAheadBuffer,
           UINT LookaheadBufferSize,
           UINT PacketSize)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = ProtocolBindingContext;
    PNDISUIO_PACKET_ENTRY PacketEntry;
    PVOID PacketBuffer;
    PNDIS_PACKET Packet;
    NDIS_STATUS Status;
    UINT BytesTransferred;

    DPRINT("Received a %d byte packet\n", PacketSize);

    /* Discard if nobody is waiting for it */
    if (AdapterContext->OpenCount == 0)
        return NDIS_STATUS_NOT_ACCEPTED;

    /* Allocate a buffer to hold the packet data and header */
    PacketBuffer = ExAllocatePool(NonPagedPool, PacketSize + HeaderBufferSize);
    if (!PacketBuffer)
        return NDIS_STATUS_NOT_ACCEPTED;

    /* Allocate the packet descriptor and buffer */
    Packet = CreatePacketFromPoolBuffer(AdapterContext,
                                        (PUCHAR)PacketBuffer + HeaderBufferSize,
                                        PacketSize);
    if (!Packet)
    {
        ExFreePool(PacketBuffer);
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Transfer the packet data into our data buffer */
    if (LookaheadBufferSize == PacketSize)
    {
        NdisCopyLookaheadData((PVOID)((PUCHAR)PacketBuffer + HeaderBufferSize),
                              LookAheadBuffer,
                              PacketSize,
                              AdapterContext->MacOptions);
        BytesTransferred = PacketSize;
    }
    else
    {
        NdisTransferData(&Status,
                         AdapterContext->BindingHandle,
                         MacReceiveContext,
                         0,
                         PacketSize,
                         Packet,
                         &BytesTransferred);
        if (Status == NDIS_STATUS_PENDING)
        {
            KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = AdapterContext->AsyncStatus;
        }
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DPRINT1("Failed to transfer data with status 0x%x\n", Status);
            CleanupAndFreePacket(Packet, FALSE);
            ExFreePool(PacketBuffer);
            return NDIS_STATUS_NOT_ACCEPTED;
        }
    }

    /* Copy the header data */
    RtlCopyMemory(PacketBuffer, HeaderBuffer, HeaderBufferSize);

    /* Free the packet descriptor and buffers
       but not the pool because we still need it */
    CleanupAndFreePacket(Packet, FALSE);

    /* Allocate a packet entry from pool */
    PacketEntry = ExAllocatePool(NonPagedPool, sizeof(NDISUIO_PACKET_ENTRY) + BytesTransferred + HeaderBufferSize - 1);
    if (!PacketEntry)
    {
        ExFreePool(PacketBuffer);
        return NDIS_STATUS_RESOURCES;
    }

    /* Initialize the packet entry and copy in packet data */
    PacketEntry->PacketLength = BytesTransferred + HeaderBufferSize;
    RtlCopyMemory(PacketEntry->PacketData, PacketBuffer, PacketEntry->PacketLength);

    /* Free the old buffer */
    ExFreePool(PacketBuffer);

    /* Insert the packet on the adapter's packet list */
    ExInterlockedInsertTailList(&AdapterContext->PacketList,
                                &PacketEntry->ListEntry,
                                &AdapterContext->Spinlock);

    /* Signal the read event */
    KeSetEvent(&AdapterContext->PacketReadEvent,
               IO_NETWORK_INCREMENT,
               FALSE);

    return NDIS_STATUS_SUCCESS;
}

VOID
NTAPI
NduReceiveComplete(NDIS_HANDLE ProtocolBindingContext)
{
    /* No op */
}

VOID
NTAPI
NduStatus(NDIS_HANDLE ProtocolBindingContext,
          NDIS_STATUS GeneralStatus,
          PVOID StatusBuffer,
          UINT StatusBufferSize)
{
    /* FIXME: Implement status tracking */
}

VOID
NTAPI
NduStatusComplete(NDIS_HANDLE ProtocolBindingContext)
{
    /* FIXME: Implement status tracking */
}

static
NDIS_STATUS
UnbindAdapterByContext(PNDISUIO_ADAPTER_CONTEXT AdapterContext)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PNDISUIO_OPEN_ENTRY OpenEntry;
    PNDISUIO_PACKET_ENTRY PacketEntry;
    NDIS_STATUS Status;

    DPRINT("Unbinding adapter %wZ\n", &AdapterContext->DeviceName);

    /* FIXME: We don't do anything with outstanding reads */

    /* Remove the adapter context from the global list */
    KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
    RemoveEntryList(&AdapterContext->ListEntry);
    KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);

    /* Free the device name string */
    RtlFreeUnicodeString(&AdapterContext->DeviceName);

    /* Invalidate all handles to this adapter */
    CurrentEntry = AdapterContext->OpenEntryList.Flink;
    while (CurrentEntry != &AdapterContext->OpenEntryList)
    {
        OpenEntry = CONTAINING_RECORD(CurrentEntry, NDISUIO_OPEN_ENTRY, ListEntry);

        /* Make sure the entry is sane */
        ASSERT(OpenEntry->FileObject);

        /* Remove the adapter context pointer */
        ASSERT(AdapterContext == OpenEntry->FileObject->FsContext);
        OpenEntry->FileObject->FsContext = NULL;
        AdapterContext->OpenCount--;

        /* Remove the open entry pointer */
        ASSERT(OpenEntry == OpenEntry->FileObject->FsContext2);
        OpenEntry->FileObject->FsContext2 = NULL;

        /* Move to the next entry */
        CurrentEntry = CurrentEntry->Flink;

        /* Free the open entry */
        ExFreePool(OpenEntry);
    }

    /* If this fails, we have a refcount mismatch somewhere */
    ASSERT(AdapterContext->OpenCount == 0);

    /* Free all pending packet entries */
    CurrentEntry = AdapterContext->PacketList.Flink;
    while (CurrentEntry != &AdapterContext->PacketList)
    {
        PacketEntry = CONTAINING_RECORD(CurrentEntry, NDISUIO_PACKET_ENTRY, ListEntry);

        /* Move to the next entry */
        CurrentEntry = CurrentEntry->Flink;

        /* Free the packet entry */
        ExFreePool(PacketEntry);
    }

    /* Send the close request */
    NdisCloseAdapter(&Status,
                     AdapterContext->BindingHandle);

    /* Wait for a pending close */
    if (Status == NDIS_STATUS_PENDING)
    {
        KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = AdapterContext->AsyncStatus;
    }

    /* Free the context */
    ExFreePool(AdapterContext);

    return Status;
}

NDIS_STATUS
NTAPI
NdisOpenAdapterEx(
    NDIS_HANDLE NdisProtocolHandle,
    NDIS_HANDLE ProtocolBindingContext,
    PVOID OpenParameters,
    NDIS_HANDLE BindContext,
    PNDIS_HANDLE NdisBindingHandle);

#define OPEN_BYTES 0x40
#define OPEN_NAME_AT 8
#define OPEN_MEDIUM_AT 16
#define OPEN_MEDIUM_COUNT_AT 24
#define OPEN_SELECTED_AT 32

static
VOID
PutU16Local(PUCHAR Buffer, ULONG Offset, USHORT Value)
{
    Buffer[Offset] = (UCHAR)(Value & 0xFF);
    Buffer[Offset + 1] = (UCHAR)((Value >> 8) & 0xFF);
}

static
VOID
PutPtrLocal(PUCHAR Buffer, ULONG Offset, PVOID Value)
{
    ULONG_PTR Address = (ULONG_PTR)Value;
    ULONG i;

    for (i = 0; i < sizeof(PVOID); i++)
    {
        Buffer[Offset + i] = (UCHAR)((Address >> (8 * i)) & 0xFF);
    }
}

static
NDIS_STATUS
BindAdapterByName(PNDIS_STRING DeviceName,
                  NDIS_HANDLE BindContext)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;
    NDIS_MEDIUM SupportedMedia[1] = {NdisMedium802_3};
    ULONG SelectedMedium = 0;
    NDIS_STATUS Status;
    NDIS_REQUEST Request;
    ULONG NameLength;
    UCHAR OpenParams[OPEN_BYTES];
    NDIS_HANDLE BindingHandle = NULL;

    /* Allocate the adapter context */
    AdapterContext = ExAllocatePool(NonPagedPool, sizeof(*AdapterContext));
    if (!AdapterContext)
    {
        return NDIS_STATUS_RESOURCES;
    }

    /* Set up the adapter context */
    RtlZeroMemory(AdapterContext, sizeof(*AdapterContext));
    KeInitializeEvent(&AdapterContext->AsyncEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&AdapterContext->PacketReadEvent, SynchronizationEvent, FALSE);
    KeInitializeSpinLock(&AdapterContext->Spinlock);
    InitializeListHead(&AdapterContext->PacketList);
    InitializeListHead(&AdapterContext->OpenEntryList);
    AdapterContext->OpenCount = 0;

    /* Store the name without a terminator, so an open matches
     * whether or not the caller passes one */
    NameLength = DeviceName->Length;
    if (NameLength >= sizeof(WCHAR) &&
        DeviceName->Buffer &&
        DeviceName->Buffer[NameLength / sizeof(WCHAR) - 1] == UNICODE_NULL)
    {
        NameLength -= sizeof(WCHAR);
    }

    /* Zero length hits ASSERT(NumberOfBytes != 0) in
     * ntoskrnl/mm/ARM3/expool.c. Guest log
     * output/e1000-fix-20261118T113038Z.log: NDISUIO BIND6 then that
     * assert when BINDALL AdapterName length was still 0. */
    if (NameLength == 0 || !DeviceName->Buffer)
    {
        ExFreePool(AdapterContext);
        return NDIS_STATUS_RESOURCES;
    }

    AdapterContext->DeviceName.Length =
    AdapterContext->DeviceName.MaximumLength = (USHORT)NameLength;
    AdapterContext->DeviceName.Buffer = ExAllocatePool(NonPagedPool, NameLength);
    if (!AdapterContext->DeviceName.Buffer)
    {
        ExFreePool(AdapterContext);
        return NDIS_STATUS_RESOURCES;
    }

    /* Copy the device name into the adapter context */
    RtlCopyMemory(AdapterContext->DeviceName.Buffer, DeviceName->Buffer, NameLength);

    DPRINT("Binding adapter %wZ\n", &AdapterContext->DeviceName);

    /* Create the buffer pool */
    NdisAllocateBufferPool(&Status,
                           &AdapterContext->BufferPoolHandle,
                           50);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("Failed to allocate buffer pool with status 0x%x\n", Status);
        RtlFreeUnicodeString(&AdapterContext->DeviceName);
        ExFreePool(AdapterContext);
        return Status;
    }

    /* Create the packet pool */
    NdisAllocatePacketPool(&Status,
                           &AdapterContext->PacketPoolHandle,
                           25,
                           PROTOCOL_RESERVED_SIZE_IN_PACKET);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("Failed to allocate packet pool with status 0x%x\n", Status);
        NdisFreeBufferPool(AdapterContext->BufferPoolHandle);
        RtlFreeUnicodeString(&AdapterContext->DeviceName);
        ExFreePool(AdapterContext);
        return Status;
    }

    /* NDIS 6 open-parameters header type 0x87 revision 1 size 0x40 */
    RtlZeroMemory(OpenParams, OPEN_BYTES);
    OpenParams[0] = 0x87;
    OpenParams[1] = 1;
    PutU16Local(OpenParams, 2, OPEN_BYTES);
    PutPtrLocal(OpenParams, OPEN_NAME_AT, DeviceName);
    PutPtrLocal(OpenParams, OPEN_MEDIUM_AT, SupportedMedia);
    PutU16Local(OpenParams, OPEN_MEDIUM_COUNT_AT, 1);
    PutPtrLocal(OpenParams, OPEN_SELECTED_AT, &SelectedMedium);

    Status = NdisOpenAdapterEx(GlobalProtocolHandle,
                               AdapterContext,
                               OpenParams,
                               BindContext,
                               &BindingHandle);
    AdapterContext->BindingHandle = BindingHandle;

    /* Wait for a pending open */
    if (Status == NDIS_STATUS_PENDING)
    {
        KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = AdapterContext->AsyncStatus;
    }

    /* Check the final status */
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("Failed to open adapter for bind with status 0x%x\n", Status);
        NdisFreePacketPool(AdapterContext->PacketPoolHandle);
        NdisFreeBufferPool(AdapterContext->BufferPoolHandle);
        RtlFreeUnicodeString(&AdapterContext->DeviceName);
        ExFreePool(AdapterContext);
        return Status;
    }

    /* MAC options are best-effort. NdisRequest is NDIS 5 and returns
     * not-supported on the NDIS 6 binding from NdisOpenAdapterEx
     * (drivers/network/ndis/src/ndis5.rs). Leave MacOptions 0 and still
     * publish the binding so QUERY_BINDING can succeed. */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAC_OPTIONS;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &AdapterContext->MacOptions;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);
    NdisRequest(&Status,
                AdapterContext->BindingHandle,
                &Request);
    if (Status == NDIS_STATUS_PENDING)
    {
        KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = AdapterContext->AsyncStatus;
    }
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("MAC options query status 0x%x; continuing with 0\n", Status);
        AdapterContext->MacOptions = 0;
    }

    /* Add the adapter context to the global list */
    ExInterlockedInsertTailList(&GlobalAdapterList,
                                &AdapterContext->ListEntry,
                                &GlobalAdapterListLock);

    return STATUS_SUCCESS;
}

VOID
NTAPI
NduBindAdapter(PNDIS_STATUS Status,
               NDIS_HANDLE BindContext,
               PNDIS_STRING DeviceName,
               PVOID SystemSpecific1,
               PVOID SystemSpecific2)
{
    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    *Status = BindAdapterByName(DeviceName, BindContext);
}

VOID
NTAPI
NduUnbindAdapter(PNDIS_STATUS Status,
                 NDIS_HANDLE ProtocolBindingContext,
                 NDIS_HANDLE UnbindContext)
{
    UNREFERENCED_PARAMETER(UnbindContext);
    *Status = UnbindAdapterByContext(ProtocolBindingContext);
}

NDIS_STATUS
NTAPI
NduBindAdapterEx(NDIS_HANDLE ProtocolDriverContext,
                 NDIS_HANDLE BindContext,
                 PVOID BindParameters)
{
    PNDIS_STRING DeviceName;

    UNREFERENCED_PARAMETER(ProtocolDriverContext);

    if (!BindParameters)
    {
        return NDIS_STATUS_RESOURCES;
    }

    /* AdapterName UNICODE_STRING* at bind+16 (prot6.rs / lldp). */
    DeviceName = *(PNDIS_STRING *)((PUCHAR)BindParameters + 16);
    if (!DeviceName)
    {
        return NDIS_STATUS_RESOURCES;
    }

    DbgPrint("NDISUIO BIND6\n");
    return BindAdapterByName(DeviceName, BindContext);
}

NDIS_STATUS
NTAPI
NduUnbindAdapterEx(NDIS_HANDLE UnbindContext,
                   NDIS_HANDLE ProtocolBindingContext)
{
    UNREFERENCED_PARAMETER(UnbindContext);
    return UnbindAdapterByContext(ProtocolBindingContext);
}
