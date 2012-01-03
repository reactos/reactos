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

PNDIS_MEDIUM SupportedMedia = {NdisMedium802_3};

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
    PVOID PacketBuffer;
    PNDIS_PACKET Packet;
    NDIS_STATUS Status;
    ULONG BytesTransferred;
    
    /* Allocate a buffer to hold the packet data and header */
    PacketBuffer = ExAllocatePool(NonPagedPool, PacketSize);
    if (!PacketBuffer)
        return NDIS_STATUS_NOT_ACCEPTED;

    /* Allocate the packet descriptor and buffer */
    Packet = CreatePacketFromPoolBuffer((PUCHAR)PacketBuffer + HeaderBufferSize,
                                        PacketSize);
    if (!Packet)
    {
        ExFreePool(PacketBuffer);
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    /* Transfer the packet data into our data buffer */
    NdisTransferData(&Status,
                     AdapterContext->BindingHandle,
                     MacReceiveContext,
                     0,
                     PacketSize,
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
        CleanupAndFreePacket(Packet, TRUE);
        return NDIS_STATUS_NOT_ACCEPTED;
    }
    
    /* Copy the header data */
    RtlCopyMemory(PacketBuffer, HeaderBuffer, HeaderBufferSize);
    
    /* Free the packet descriptor and buffers 
       but not the pool because we still need it */
    CleanupAndFreePacket(Packet, FALSE);

    /* Allocate a packet entry from paged pool */
    PacketEntry = ExAllocatePool(PagedPool, sizeof(NDISUIO_PACKET_ENTRY) + BytesTransferred + HeaderBufferSize - 1);
    if (!PacketEntry)
    {
        ExFreePool(PacketBuffer);
        return NDIS_STATUS_RESOURCES;
    }

    /* Initialize the packet entry and copy in packet data */
    PacketEntry->PacketLength = BytesTransferred + HeaderBufferSize;
    RtlCopyMemory(&PacketEntry->PacketData[0], PacketBuffer, PacketEntry->PacketLength);
    
    /* Free the old non-paged buffer */
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

NDIS_STATUS
UnbindAdapterByContext(PNDISUIO_ADAPTER_CONTEXT AdapterContext)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentOpenEntry;
    PNDISUIO_OPEN_ENTRY OpenEntry;
    
    /* Remove the adapter context from the global list */
    KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
    RemoveEntryList(&AdapterContext->ListEntry);
    KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);

    /* Invalidate all handles to this adapter */
    CurrentOpenEntry = AdapterContext->OpenEntryList.Flink;
    while (CurrentOpenEntry != &AdapterContext->OpenEntryList)
    {
        OpenEntry = CONTAINING_RECORD(CurrentOpenEntry, NDISUIO_OPEN_ENTRY, ListEntry);

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
        CurrentOpenEntry = CurrentOpenEntry->Flink;

        /* Free the open entry */
        ExFreePool(OpenEntry);
    }

    /* If this fails, we have a refcount mismatch somewhere */
    ASSERT(AdapterContext->OpenCount == 0);
    
    /* Send the close request */
    NdisCloseAdapter(Status,
                     AdapterContext->BindingHandle);
    
    /* Wait for a pending close */
    if (*Status == NDIS_STATUS_PENDING)
    {
        KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        *Status = AdapterContext->AsyncStatus;
    }
    
    /* Free the context */
    ExFreePool(AdapterContext);
}


NDIS_STATUS
BindAdapterByName(PNDIS_STRING DeviceName, PNDISUIO_ADAPTER_CONTEXT *Context)
{
    NDIS_STATUS OpenErrorStatus;
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;
    UINT SelectedMedium;
    NDIS_STATUS Status;
    
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

    /* Send the open request */
    NdisOpenAdapter(&Status,
                    &OpenErrorStatus,
                    &AdapterContext->BindingHandle,
                    &SelectedMedium,
                    SupportedMedia,
                    sizeof(SupportedMedia),
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
        DPRINT1("Failed to open adapter for bind with status 0x%x\n", *Status);
        ExFreePool(AdapterContext);
        return;
    }
    
    /* Add the adapter context to the global list */
    ExInterlockedInsertTailList(&GlobalAdapterList,
                                &AdapterContext->ListEntry,
                                &GlobalAdapterListLock);
    
    /* Return the context */
    *Context = AdapterContext;
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

