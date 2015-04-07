/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS kernel
 * FILE:       drivers/filesystems/msfs/create.c
 * PURPOSE:    Mailslot filesystem
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "msfs.h"

#include <ndk/iotypes.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* Creates the client side */
NTSTATUS DEFAULTAPI
MsfsCreate(PDEVICE_OBJECT DeviceObject,
           PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_DEVICE_EXTENSION DeviceExtension;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_FCB current = NULL;
    PLIST_ENTRY current_entry;
    KIRQL oldIrql;

    DPRINT("MsfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    FileObject = IoStack->FileObject;

    DPRINT("Mailslot name: %wZ\n", &FileObject->FileName);

    Ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(MSFS_CCB), 'cFsM');
    if (Ccb == NULL)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_MEMORY;
    }

    KeLockMutex(&DeviceExtension->FcbListLock);
    current_entry = DeviceExtension->FcbListHead.Flink;
    while (current_entry != &DeviceExtension->FcbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    MSFS_FCB,
                                    FcbListEntry);

        if (!RtlCompareUnicodeString(&FileObject->FileName, &current->Name, TRUE))
            break;

        current_entry = current_entry->Flink;
    }

    if (current_entry == &DeviceExtension->FcbListHead)
    {
        ExFreePoolWithTag(Ccb, 'cFsM');
        KeUnlockMutex(&DeviceExtension->FcbListLock);

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_UNSUCCESSFUL;
    }

    Fcb = current;

    KeAcquireSpinLock(&Fcb->CcbListLock, &oldIrql);
    InsertTailList(&Fcb->CcbListHead, &Ccb->CcbListEntry);
    KeReleaseSpinLock(&Fcb->CcbListLock, oldIrql);

    Fcb->ReferenceCount++;

    Ccb->Fcb = Fcb;

    KeUnlockMutex(&DeviceExtension->FcbListLock);

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;
    FileObject->Flags |= FO_MAILSLOT;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/* Creates the server side */
NTSTATUS DEFAULTAPI
MsfsCreateMailslot(PDEVICE_OBJECT DeviceObject,
                   PIRP Irp)
{
    PEXTENDED_IO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_DEVICE_EXTENSION DeviceExtension;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    KIRQL oldIrql;
    PLIST_ENTRY current_entry;
    PMSFS_FCB current = NULL;
    PMAILSLOT_CREATE_PARAMETERS Buffer;

    DPRINT("MsfsCreateMailslot(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    FileObject = IoStack->FileObject;
    Buffer = IoStack->Parameters.CreateMailslot.Parameters;

    DPRINT("Mailslot name: %wZ\n", &FileObject->FileName);

    Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(MSFS_FCB), 'fFsM');
    if (Fcb == NULL)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEMORY;
    }

    Fcb->Name.Length = FileObject->FileName.Length;
    Fcb->Name.MaximumLength = Fcb->Name.Length + sizeof(UNICODE_NULL);
    Fcb->Name.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                             Fcb->Name.MaximumLength,
                                             'NFsM');
    if (Fcb->Name.Buffer == NULL)
    {
        ExFreePoolWithTag(Fcb, 'fFsM');

        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEMORY;
    }

    RtlCopyUnicodeString(&Fcb->Name, &FileObject->FileName);

    Ccb = ExAllocatePoolWithTag(NonPagedPool, sizeof(MSFS_CCB), 'cFsM');
    if (Ccb == NULL)
    {
        ExFreePoolWithTag(Fcb->Name.Buffer, 'NFsM');
        ExFreePoolWithTag(Fcb, 'fFsM');

        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NO_MEMORY;
    }

    Fcb->ReferenceCount = 0;
    InitializeListHead(&Fcb->CcbListHead);
    KeInitializeSpinLock(&Fcb->CcbListLock);

    Fcb->MaxMessageSize = Buffer->MaximumMessageSize;
    Fcb->MessageCount = 0;
    Fcb->TimeOut = Buffer->ReadTimeout;
    KeInitializeEvent(&Fcb->MessageEvent,
                      NotificationEvent,
                      FALSE);

    InitializeListHead(&Fcb->MessageListHead);
    KeInitializeSpinLock(&Fcb->MessageListLock);

    KeLockMutex(&DeviceExtension->FcbListLock);
    current_entry = DeviceExtension->FcbListHead.Flink;
    while (current_entry != &DeviceExtension->FcbListHead)
    {
        current = CONTAINING_RECORD(current_entry,
                                    MSFS_FCB,
                                    FcbListEntry);

        if (!RtlCompareUnicodeString(&Fcb->Name, &current->Name, TRUE))
            break;

        current_entry = current_entry->Flink;
    }

    if (current_entry != &DeviceExtension->FcbListHead)
    {
        ExFreePoolWithTag(Fcb->Name.Buffer, 'NFsM');
        ExFreePoolWithTag(Fcb, 'fFsM');
        ExFreePoolWithTag(Ccb, 'cFsM');

        KeUnlockMutex(&DeviceExtension->FcbListLock);

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        InsertTailList(&DeviceExtension->FcbListHead,
                       &Fcb->FcbListEntry);
    }

    KeAcquireSpinLock(&Fcb->CcbListLock, &oldIrql);
    InsertTailList(&Fcb->CcbListHead, &Ccb->CcbListEntry);
    KeReleaseSpinLock(&Fcb->CcbListLock, oldIrql);

    Fcb->ReferenceCount++;
    Fcb->ServerCcb = Ccb;
    Ccb->Fcb = Fcb;

    KeUnlockMutex(&DeviceExtension->FcbListLock);

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;
    FileObject->Flags |= FO_MAILSLOT;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS DEFAULTAPI
MsfsClose(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PMSFS_DEVICE_EXTENSION DeviceExtension;
    PMSFS_FCB Fcb;
    PMSFS_CCB Ccb;
    PMSFS_MESSAGE Message;
    KIRQL oldIrql;

    DPRINT("MsfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;
    FileObject = IoStack->FileObject;

    KeLockMutex(&DeviceExtension->FcbListLock);

    if (DeviceExtension->FcbListHead.Flink == &DeviceExtension->FcbListHead)
    {
        KeUnlockMutex(&DeviceExtension->FcbListLock);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    Fcb = FileObject->FsContext;
    Ccb = FileObject->FsContext2;

    DPRINT("Mailslot name: %wZ\n", &Fcb->Name);

    Fcb->ReferenceCount--;
    if (Fcb->ServerCcb == Ccb)
    {
        /* delete all messages from message-list */
        KeAcquireSpinLock(&Fcb->MessageListLock, &oldIrql);

        while (Fcb->MessageListHead.Flink != &Fcb->MessageListHead)
        {
            Message = CONTAINING_RECORD(Fcb->MessageListHead.Flink,
                                        MSFS_MESSAGE,
                                        MessageListEntry);
            RemoveEntryList(Fcb->MessageListHead.Flink);
            ExFreePoolWithTag(Message, 'rFsM');
        }

        Fcb->MessageCount = 0;

        KeReleaseSpinLock(&Fcb->MessageListLock, oldIrql);
        Fcb->ServerCcb = NULL;
    }

    KeAcquireSpinLock(&Fcb->CcbListLock, &oldIrql);
    RemoveEntryList(&Ccb->CcbListEntry);
    KeReleaseSpinLock(&Fcb->CcbListLock, oldIrql);
    ExFreePoolWithTag(Ccb, 'cFsM');
    FileObject->FsContext2 = NULL;

    if (Fcb->ReferenceCount == 0)
    {
        DPRINT("ReferenceCount == 0: Deleting mailslot data\n");
        RemoveEntryList(&Fcb->FcbListEntry);
        ExFreePoolWithTag(Fcb->Name.Buffer, 'NFsM');
        ExFreePoolWithTag(Fcb, 'fFsM');
    }

    KeUnlockMutex(&DeviceExtension->FcbListLock);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
