/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/rw.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 *             Nikita Pechenkin (n.pechenkin@mail.ru)
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
    LARGE_INTEGER Timeout;
    PKTIMER Timer;
    PMSFS_DPC_CTX Context;
    PKDPC Dpc;
    PLIST_ENTRY Entry;

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
        Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    else
        Buffer = Irp->UserBuffer;


    if (Fcb->MessageCount > 0)
    {
        KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
        Entry = RemoveHeadList(&Fcb->MessageListHead);
        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

        /* copy current message into buffer */
        Message = CONTAINING_RECORD(Entry, MSFS_MESSAGE, MessageListEntry);
        memcpy(Buffer, &Message->Buffer, min(Message->Size,Length));
        LengthRead = Message->Size;

        ExFreePoolWithTag(Message, 'rFsM');
        Fcb->MessageCount--;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = LengthRead;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    Timeout = Fcb->TimeOut;
    if (Timeout.HighPart == 0 && Timeout.LowPart == 0)
    {
        Irp->IoStatus.Status = STATUS_IO_TIMEOUT;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_IO_TIMEOUT;
    }

    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(MSFS_DPC_CTX), 'NFsM');
    if (Context == NULL)
    {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoCsqInsertIrp(&Fcb->CancelSafeQueue, Irp, &Context->CsqContext);
    Timer = &Context->Timer;
    Dpc = &Context->Dpc;
    Context->Csq = &Fcb->CancelSafeQueue;

    /* No timer for INFINITY_WAIT */
    if (Timeout.QuadPart != -1)
    {
        KeInitializeTimer(Timer);
        KeInitializeDpc(Dpc, MsfsTimeout, (PVOID)Context);
        KeSetTimer(Timer, Timeout, Dpc);
    }

    Fcb->WaitCount++;
    IoMarkIrpPending(Irp);

    return STATUS_PENDING;
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
    PIRP CsqIrp;

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
        Buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    else
        Buffer = Irp->UserBuffer;

    DPRINT("Length: %lu Message: %s\n", Length, (PUCHAR)Buffer);

    /* Allocate new message */
    Message = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(MSFS_MESSAGE) + Length,
                                    'rFsM');
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

    if (Fcb->WaitCount > 0)
    {
        CsqIrp = IoCsqRemoveNextIrp(&Fcb->CancelSafeQueue, NULL);
        /* FIXME: It is necessary to reset the timers. */
        MsfsRead(DeviceObject, CsqIrp);
        Fcb->WaitCount--;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
