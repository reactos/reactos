/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        misc.c
 * PURPOSE:     Helper functions
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

NDIS_STATUS
AllocateAndChainBuffer(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                       PNDIS_PACKET Packet,
                       PVOID Buffer,
                       ULONG BufferSize,
                       BOOLEAN Front)
{
    NDIS_STATUS Status;
    PNDIS_BUFFER NdisBuffer;

    /* Allocate the NDIS buffer mapping the pool */
    NdisAllocateBuffer(&Status,
                       &NdisBuffer,
                       AdapterContext->BufferPoolHandle,
                       Buffer,
                       BufferSize);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("No free buffer descriptors\n");
        return Status;
    }

    if (Front)
    {
        /* Chain the buffer to front */
        NdisChainBufferAtFront(Packet, NdisBuffer);
    }
    else
    {
        /* Chain the buffer to back */
        NdisChainBufferAtBack(Packet, NdisBuffer);
    }

    /* Return success */
    return NDIS_STATUS_SUCCESS;
}

PNDIS_PACKET
CreatePacketFromPoolBuffer(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                           PVOID Buffer,
                           ULONG BufferSize)
{
    PNDIS_PACKET Packet;
    NDIS_STATUS Status;

    /* Allocate a packet descriptor */
    NdisAllocatePacket(&Status,
                       &Packet,
                       AdapterContext->PacketPoolHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("No free packet descriptors\n");
        return NULL;
    }

    /* Use the helper to chain the buffer */
    Status = AllocateAndChainBuffer(AdapterContext, Packet,
                                    Buffer, BufferSize, TRUE);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisFreePacket(Packet);
        return NULL;
    }

    /* Return the packet */
    return Packet;
}

VOID
CleanupAndFreePacket(PNDIS_PACKET Packet, BOOLEAN FreePool)
{
    PNDIS_BUFFER Buffer;
    PVOID Data;
    ULONG Length;

    /* Free each buffer and its backing pool memory */
    while (TRUE)
    {
        /* Unchain each buffer */
        NdisUnchainBufferAtFront(Packet, &Buffer);
        if (!Buffer)
            break;

        /* Get the backing memory */
        NdisQueryBuffer(Buffer, &Data, &Length);

        /* Free the buffer */
        NdisFreeBuffer(Buffer);

        if (FreePool)
        {
            /* Free the backing memory */
            ExFreePool(Data);
        }
    }

    /* Free the packet descriptor */
    NdisFreePacket(Packet);
}

PNDISUIO_ADAPTER_CONTEXT
FindAdapterContextByName(PNDIS_STRING DeviceName)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;

    KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
    CurrentEntry = GlobalAdapterList.Flink;
    while (CurrentEntry != &GlobalAdapterList)
    {
        AdapterContext = CONTAINING_RECORD(CurrentEntry, NDISUIO_ADAPTER_CONTEXT, ListEntry);

        /* Check if the device name matches */
        if (RtlEqualUnicodeString(&AdapterContext->DeviceName, DeviceName, TRUE))
        {
            KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
            return AdapterContext;
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);

    return NULL;
}

VOID
ReferenceAdapterContext(PNDISUIO_ADAPTER_CONTEXT AdapterContext)
{
    /* Increment the open count */
    AdapterContext->OpenCount++;
}

VOID
DereferenceAdapterContextWithOpenEntry(PNDISUIO_ADAPTER_CONTEXT AdapterContext,
                                       PNDISUIO_OPEN_ENTRY OpenEntry)
{
    KIRQL OldIrql;

    /* Lock the adapter context */
    KeAcquireSpinLock(&AdapterContext->Spinlock, &OldIrql);

    /* Decrement the open count */
    AdapterContext->OpenCount--;

    /* Cleanup the open entry if we were given one */
    if (OpenEntry != NULL)
    {
        /* Remove the open entry */
        RemoveEntryList(&OpenEntry->ListEntry);

        /* Invalidate the FO */
        OpenEntry->FileObject->FsContext = NULL;
        OpenEntry->FileObject->FsContext2 = NULL;

        /* Free the open entry */
        ExFreePool(OpenEntry);
    }

    /* Release the adapter context lock */
    KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
}
