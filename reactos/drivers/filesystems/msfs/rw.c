/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/ms/rw.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS DEFAULTAPI
MsfsRead(PDEVICE_OBJECT DeviceObject,
         PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_MESSAGE Message;
    KIRQL oldIrql;
    ULONG Length;
    ULONG LengthRead = 0;
    PVOID Buffer;
    NTSTATUS Status;

    DPRINT("MsfsRead(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation (Irp);
    FileObject = IoStack->FileObject;
    Fcb = (PMSFS_FCB)FileObject->FsContext;
    Ccb = (PMSFS_CCB)FileObject->FsContext2;

    DPRINT("MailslotName: %wZ\n", &Fcb->Name);

    /* reading is not permitted on client side */
    if (Fcb->ServerCcb != Ccb)
    {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_ACCESS_DENIED;
    }

    Length = IoStack->Parameters.Read.Length;
    if (Irp->MdlAddress)
        Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
    else
        Buffer = Irp->UserBuffer;

    Status = KeWaitForSingleObject(&Fcb->MessageEvent,
                                   UserRequest,
                                   KernelMode,
                                   FALSE,
                                   NULL); /* FIXME: handle timeout */
    if ((NT_SUCCESS(Status)) && (Fcb->MessageCount > 0))
    {
        /* copy current message into buffer */
        Message = CONTAINING_RECORD(Fcb->MessageListHead.Flink,
                                    MSFS_MESSAGE,
                                    MessageListEntry);

        memcpy(Buffer, &Message->Buffer, min(Message->Size,Length));
        LengthRead = Message->Size;

        KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
        RemoveHeadList(&Fcb->MessageListHead);
        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

        ExFreePool(Message);
        Fcb->MessageCount--;
        if (Fcb->MessageCount == 0)
        {
            KeClearEvent(&Fcb->MessageEvent);
        }
     }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = LengthRead;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS DEFAULTAPI
MsfsWrite(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_MESSAGE Message;
    KIRQL oldIrql;
    ULONG Length;
    PVOID Buffer;

    DPRINT("MsfsWrite(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation (Irp);
    FileObject = IoStack->FileObject;
    Fcb = (PMSFS_FCB)FileObject->FsContext;
    Ccb = (PMSFS_CCB)FileObject->FsContext2;

    DPRINT("MailslotName: %wZ\n", &Fcb->Name);

    /* writing is not permitted on server side */
    if (Fcb->ServerCcb == Ccb)
    {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_ACCESS_DENIED;
    }

    Length = IoStack->Parameters.Write.Length;
    if (Irp->MdlAddress)
        Buffer = MmGetSystemAddressForMdl (Irp->MdlAddress);
    else
        Buffer = Irp->UserBuffer;

    DPRINT("Length: %lu Message: %s\n", Length, (PUCHAR)Buffer);

    /* Allocate new message */
    Message = ExAllocatePool(NonPagedPool,
                             sizeof(MSFS_MESSAGE) + Length);
    if (Message == NULL)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEMORY;
    }

    Message->Size = Length;
    memcpy(&Message->Buffer, Buffer, Length);

    KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
    InsertTailList(&Fcb->MessageListHead, &Message->MessageListEntry);
    KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

    Fcb->MessageCount++;
    if (Fcb->MessageCount == 1)
    {
        KeSetEvent(&Fcb->MessageEvent,
                   0,
                   FALSE);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
