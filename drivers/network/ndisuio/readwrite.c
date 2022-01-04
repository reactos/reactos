/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        readwrite.c
 * PURPOSE:     Handles IRP_MJ_READ and IRP_MJ_WRITE
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

static
VOID
NTAPI
ReadIrpCancel(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_PACKET_ENTRY PacketEntry;

    /* Release the cancel spin lock */
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /* Indicate a 0-byte packet on the queue to cancel the read */
    PacketEntry = ExAllocatePool(NonPagedPool, sizeof(NDISUIO_PACKET_ENTRY));
    if (PacketEntry)
    {
        PacketEntry->PacketLength = 0;

        ExInterlockedInsertHeadList(&AdapterContext->PacketList,
                                    &PacketEntry->ListEntry,
                                    &AdapterContext->Spinlock);

        KeSetEvent(&AdapterContext->PacketReadEvent, IO_NO_INCREMENT, FALSE);
    }
}

NTSTATUS
NTAPI
NduDispatchRead(PDEVICE_OBJECT DeviceObject,
                PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_OPEN_ENTRY OpenEntry = IrpSp->FileObject->FsContext2;
    KIRQL OldIrql, OldCancelIrql;
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PNDISUIO_PACKET_ENTRY PacketEntry = NULL;
    ULONG BytesCopied = 0;

    ASSERT(DeviceObject == GlobalDeviceObject);

    if (OpenEntry->WriteOnly)
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_PARAMETER;
    }

    /* Make the read cancellable */
    IoAcquireCancelSpinLock(&OldCancelIrql);
    IoSetCancelRoutine(Irp, ReadIrpCancel);
    if (Irp->Cancel)
    {
        IoReleaseCancelSpinLock(OldCancelIrql);

        /* Indicate a 0 byte read */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }
    IoReleaseCancelSpinLock(OldCancelIrql);

    while (TRUE)
    {
        KeAcquireSpinLock(&AdapterContext->Spinlock, &OldIrql);

        /* Check if we have a packet */
        if (IsListEmpty(&AdapterContext->PacketList))
        {
            KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);

            /* Wait for a packet (in the context of the calling user thread) */
            Status = KeWaitForSingleObject(&AdapterContext->PacketReadEvent,
                                           UserRequest,
                                           UserMode,
                                           TRUE,
                                           NULL);
            if (Status != STATUS_SUCCESS)
            {
                /* Remove the cancel routine */
                IoAcquireCancelSpinLock(&OldCancelIrql);
                IoSetCancelRoutine(Irp, NULL);
                IoReleaseCancelSpinLock(OldCancelIrql);

                break;
            }
        }
        else
        {
            /* Remove the cancel routine */
            IoAcquireCancelSpinLock(&OldCancelIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(OldCancelIrql);

            /* Remove the first packet in the list */
            ListEntry = RemoveHeadList(&AdapterContext->PacketList);
            PacketEntry = CONTAINING_RECORD(ListEntry, NDISUIO_PACKET_ENTRY, ListEntry);

            /* Release the adapter lock */
            KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);

            /* And we're done with this loop */
            Status = STATUS_SUCCESS;
            break;
        }
    }

    /* Check if we got a packet */
    if (PacketEntry != NULL)
    {
        /* Find the right amount of bytes to copy */
        BytesCopied = PacketEntry->PacketLength;
        if (BytesCopied > IrpSp->Parameters.Read.Length)
            BytesCopied = IrpSp->Parameters.Read.Length;

        /* Copy the packet */
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      &PacketEntry->PacketData[0],
                      BytesCopied);

        /* Free the packet entry */
        ExFreePool(PacketEntry);
    }
    else
    {
        /* Something failed */
        BytesCopied = 0;
    }

    /* Complete the IRP */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesCopied;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
NduDispatchWrite(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDIS_PACKET Packet;
    NDIS_STATUS Status;
    ULONG BytesCopied = 0;

    ASSERT(DeviceObject == GlobalDeviceObject);

    /* Create a packet and buffer descriptor for this user buffer */
    Packet = CreatePacketFromPoolBuffer(AdapterContext,
                                        Irp->AssociatedIrp.SystemBuffer,
                                        IrpSp->Parameters.Write.Length);
    if (Packet)
    {
        /* Send it via NDIS */
        NdisSend(&Status,
                 AdapterContext->BindingHandle,
                 Packet);

        /* Wait for the send */
        if (Status == NDIS_STATUS_PENDING)
        {
            KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = AdapterContext->AsyncStatus;
        }

        /* Check if it succeeded */
        if (Status == NDIS_STATUS_SUCCESS)
            BytesCopied = IrpSp->Parameters.Write.Length;

        CleanupAndFreePacket(Packet, FALSE);
    }
    else
    {
        /* No memory */
        Status = STATUS_NO_MEMORY;
    }

    /* Complete the IRP */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesCopied;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    return Status;
}
