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
    /* FIXME: Implement send/receive */
}

VOID
NTAPI
NduTransferDataComplete(NDIS_HANDLE ProtocolBindingContext,
                        PNDIS_PACKET Packet,
                        NDIS_STATUS Status,
                        UINT BytesTransferred)
{
    /* FIXME: Implement send/receive */
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
    /* FIXME: Implement send/receive */
    return NDIS_STATUS_NOT_ACCEPTED;
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

    /* Remove the adapter context from the global list */
    KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
    RemoveEntryList(&AdapterContext->ListEntry);
    KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
    
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
    KeInitializeSpinLock(&AdapterContext->Spinlock);
    InitializeListHead(&AdapterContext->PacketList);
    InitializeListHead(&AdapterContext->OpenEntryList);
    AdapterContext->OpenCount = 1;

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
    /* We don't bind like this */
    *Status = NDIS_STATUS_SUCCESS;
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

