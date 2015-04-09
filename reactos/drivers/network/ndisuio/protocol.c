/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        protocol.c
 * PURPOSE:     Protocol stuff
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
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
    DPRINT("NetPnPEvent\n");

    switch (NetPnPEvent->NetEvent)
    {
        case NetEventQueryRemoveDevice:
            /* Nothing to do */
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
            ExFreePool(PacketBuffer);
            CleanupAndFreePacket(Packet, TRUE);
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

static
NDIS_STATUS
BindAdapterByName(PNDIS_STRING DeviceName)
{
    NDIS_STATUS OpenErrorStatus;
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;
    NDIS_MEDIUM SupportedMedia[1] = {NdisMedium802_3};
    UINT SelectedMedium;
    NDIS_STATUS Status;
    NDIS_REQUEST Request;

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

    AdapterContext->DeviceName.Length =
    AdapterContext->DeviceName.MaximumLength = DeviceName->Length;
    AdapterContext->DeviceName.Buffer = ExAllocatePool(NonPagedPool, DeviceName->Length);
    if (!AdapterContext->DeviceName.Buffer)
    {
        ExFreePool(AdapterContext);
        return NDIS_STATUS_RESOURCES;
    }

    /* Copy the device name into the adapter context */
    RtlCopyMemory(AdapterContext->DeviceName.Buffer, DeviceName->Buffer, DeviceName->Length);
    
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

    /* Send the open request */
    NdisOpenAdapter(&Status,
                    &OpenErrorStatus,
                    &AdapterContext->BindingHandle,
                    &SelectedMedium,
                    SupportedMedia,
                    1,
                    GlobalProtocolHandle,
                    AdapterContext,
                    DeviceName,
                    0,
                    NULL);
    
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
    
    /* Get the MAC options */
    Request.RequestType = NdisRequestQueryInformation;
    Request.DATA.QUERY_INFORMATION.Oid = OID_GEN_MAC_OPTIONS;
    Request.DATA.QUERY_INFORMATION.InformationBuffer = &AdapterContext->MacOptions;
    Request.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(ULONG);
    NdisRequest(&Status,
                AdapterContext->BindingHandle,
                &Request);

    /* Wait for a pending request */
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
        NDIS_STATUS CloseStatus;

        DPRINT1("Failed to get MAC options with status 0x%x\n", Status);

        NdisCloseAdapter(&CloseStatus,
                         AdapterContext->BindingHandle);
        if (CloseStatus == NDIS_STATUS_PENDING)
        {
            KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        NdisFreePacketPool(AdapterContext->PacketPoolHandle);
        NdisFreeBufferPool(AdapterContext->BufferPoolHandle);
        RtlFreeUnicodeString(&AdapterContext->DeviceName);
        ExFreePool(AdapterContext);
        return Status;
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
    /* Use our helper function to create a context for this adapter */
    *Status = BindAdapterByName(DeviceName);
}

VOID
NTAPI
NduUnbindAdapter(PNDIS_STATUS Status,
                 NDIS_HANDLE ProtocolBindingContext,
                 NDIS_HANDLE UnbindContext)
{
    /* This is forced unbind. UnbindAdapterByContext() will take care of 
     * invalidating file handles pointer to this adapter for us */
    *Status = UnbindAdapterByContext(ProtocolBindingContext);
}
