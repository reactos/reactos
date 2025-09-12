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


    KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);
    if (Fcb->MessageCount > 0)
    {
        Entry = RemoveHeadList(&Fcb->MessageListHead);
        Fcb->MessageCount--;
        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

        /* copy current message into buffer */
        Message = CONTAINING_RECORD(Entry, MSFS_MESSAGE, MessageListEntry);
        memcpy(Buffer, &Message->Buffer, min(Message->Size,Length));
        LengthRead = Message->Size;

        ExFreePoolWithTag(Message, 'rFsM');

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = LengthRead;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }
    else
    {
        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);
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

    KeInitializeEvent(&Context->Event, SynchronizationEvent, FALSE);
    IoCsqInsertIrp(&Fcb->CancelSafeQueue, Irp, &Context->CsqContext);
    Timer = &Context->Timer;
    Dpc = &Context->Dpc;
    Context->Csq = &Fcb->CancelSafeQueue;
    Irp->Tail.Overlay.DriverContext[0] = Context;

    /* No timer for INFINITY_WAIT */
    if (Timeout.QuadPart != -1)
    {
        KeInitializeTimer(Timer);
        KeInitializeDpc(Dpc, MsfsTimeout, (PVOID)Context);
        KeSetTimer(Timer, Timeout, Dpc);
    }

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
    PMSFS_DPC_CTX Context;

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
    Fcb->MessageCount++;
    KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);

    CsqIrp = IoCsqRemoveNextIrp(&Fcb->CancelSafeQueue, NULL);
    if (CsqIrp != NULL)
    {
        /* Get the context */
        Context = CsqIrp->Tail.Overlay.DriverContext[0];
        /* DPC was queued, wait for it to fail (IRP is ours) */
        if (Fcb->TimeOut.QuadPart != -1 && !KeCancelTimer(&Context->Timer))
        {
            KeWaitForSingleObject(&Context->Event, Executive, KernelMode, FALSE, NULL);
        }

        /* Free context & attempt read */
        ExFreePoolWithTag(Context, 'NFsM');
        MsfsRead(DeviceObject, CsqIrp);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
