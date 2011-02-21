/*
* COPYRIGHT:  See COPYING in the top level directory
* PROJECT:    ReactOS kernel
* FILE:       drivers/fs/np/create.c
* PURPOSE:    Named pipe filesystem
* PROGRAMMER: David Welch <welch@cwcom.net>
*/

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

//#define USING_PROPER_NPFS_WAIT_SEMANTICS

/* FUNCTIONS *****************************************************************/

PNPFS_FCB
NpfsFindPipe(PNPFS_VCB Vcb,
             PUNICODE_STRING PipeName)
{
    PLIST_ENTRY CurrentEntry;
    PNPFS_FCB Fcb;

    CurrentEntry = Vcb->PipeListHead.Flink;
    while (CurrentEntry != &Vcb->PipeListHead)
    {
        Fcb = CONTAINING_RECORD(CurrentEntry, NPFS_FCB, PipeListEntry);
        if (RtlCompareUnicodeString(PipeName,
            &Fcb->PipeName,
            TRUE) == 0)
        {
            DPRINT("<%wZ> = <%wZ>\n", PipeName, &Fcb->PipeName);
            return Fcb;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    return NULL;
}


static PNPFS_CCB
NpfsFindListeningServerInstance(PNPFS_FCB Fcb)
{
    PLIST_ENTRY CurrentEntry;
    PNPFS_WAITER_ENTRY Waiter;
    KIRQL oldIrql;
    PIRP Irp;

    CurrentEntry = Fcb->WaiterListHead.Flink;
    while (CurrentEntry != &Fcb->WaiterListHead)
    {
        Waiter = CONTAINING_RECORD(CurrentEntry, NPFS_WAITER_ENTRY, Entry);
        Irp = CONTAINING_RECORD(Waiter, IRP, Tail.Overlay.DriverContext);
        if (Waiter->Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
        {
            DPRINT("Server found! CCB %p\n", Waiter->Ccb);

            IoAcquireCancelSpinLock(&oldIrql);
            if (!Irp->Cancel)
            {
                (void)IoSetCancelRoutine(Irp, NULL);
                IoReleaseCancelSpinLock(oldIrql);
                return Waiter->Ccb;
            }
            IoReleaseCancelSpinLock(oldIrql);
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    return NULL;
}


static VOID
NpfsSignalAndRemoveListeningServerInstance(PNPFS_FCB Fcb,
                                           PNPFS_CCB Ccb)
{
    PLIST_ENTRY CurrentEntry;
    PNPFS_WAITER_ENTRY Waiter;
    PIRP Irp;

    CurrentEntry = Fcb->WaiterListHead.Flink;
    while (CurrentEntry != &Fcb->WaiterListHead)
    {
        Waiter = CONTAINING_RECORD(CurrentEntry, NPFS_WAITER_ENTRY, Entry);
        if (Waiter->Ccb == Ccb)
        {
            DPRINT("Server found! CCB %p\n", Waiter->Ccb);

            RemoveEntryList(&Waiter->Entry);
            Irp = CONTAINING_RECORD(Waiter, IRP, Tail.Overlay.DriverContext);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
        CurrentEntry = CurrentEntry->Flink;
    }
}


static VOID
NpfsOpenFileSystem(PNPFS_FCB Fcb,
                   PFILE_OBJECT FileObject,
                   PIO_STATUS_BLOCK IoStatus)
{
    PNPFS_CCB Ccb;

    DPRINT("NpfsOpenFileSystem()\n");

    Ccb = ExAllocatePool(NonPagedPool, sizeof(NPFS_CCB));
    if (Ccb == NULL)
    {
        IoStatus->Status = STATUS_NO_MEMORY;
        return;
    }

    RtlZeroMemory(Ccb, sizeof(NPFS_CCB));

    Ccb->Type = CCB_DEVICE;
    Ccb->Fcb = Fcb;

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;

    IoStatus->Information = FILE_OPENED;
    IoStatus->Status = STATUS_SUCCESS;

    return;
}


static VOID
NpfsOpenRootDirectory(PNPFS_FCB Fcb,
                      PFILE_OBJECT FileObject,
                      PIO_STATUS_BLOCK IoStatus)
{
    PNPFS_CCB Ccb;

    DPRINT("NpfsOpenRootDirectory()\n");

    Ccb = ExAllocatePool(NonPagedPool, sizeof(NPFS_CCB));
    if (Ccb == NULL)
    {
        IoStatus->Status = STATUS_NO_MEMORY;
        return;
    }

    RtlZeroMemory(Ccb, sizeof(NPFS_CCB));

    Ccb->Type = CCB_DIRECTORY;
    Ccb->Fcb = Fcb;

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;

    IoStatus->Information = FILE_OPENED;
    IoStatus->Status = STATUS_SUCCESS;

    return;
}


NTSTATUS NTAPI
NpfsCreate(PDEVICE_OBJECT DeviceObject,
           PIRP Irp)
{
    PEXTENDED_IO_STACK_LOCATION IoStack;
    PUNICODE_STRING FileName;
    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    PNPFS_FCB Fcb;
    PNPFS_CCB ClientCcb;
    PNPFS_CCB ServerCcb = NULL;
    PNPFS_VCB Vcb;
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;
#ifndef USING_PROPER_NPFS_WAIT_SEMANTICS
    BOOLEAN SpecialAccess;
#endif

    DPRINT("NpfsCreate(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    RelatedFileObject = FileObject->RelatedFileObject;
    FileName = &FileObject->FileName;
    DesiredAccess = IoStack->Parameters.CreatePipe.SecurityContext->DesiredAccess;

    DPRINT("FileObject %p\n", FileObject);
    DPRINT("FileName %wZ\n", &FileObject->FileName);

    Irp->IoStatus.Information = 0;

#ifndef USING_PROPER_NPFS_WAIT_SEMANTICS
    SpecialAccess = ((DesiredAccess & SPECIFIC_RIGHTS_ALL) == FILE_READ_ATTRIBUTES);
    if (SpecialAccess)
    {
        DPRINT("NpfsCreate() open client end for special use!\n");
    }
#endif

    DPRINT("FileName->Length: %hu  RelatedFileObject: %p\n", FileName->Length, RelatedFileObject);

    /* Open the file system */
    if (FileName->Length == 0 &&
        (RelatedFileObject == NULL || ((PNPFS_CCB)RelatedFileObject->FsContext2)->Type == CCB_DEVICE))
    {
        DPRINT("Open the file system\n");

        NpfsOpenFileSystem(Vcb->DeviceFcb,
                           FileObject,
                           &Irp->IoStatus);

        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Open the root directory */
    if ((FileName->Length == 2 && FileName->Buffer[0] == L'\\' && RelatedFileObject == NULL) ||
        (FileName->Length == 0 && ((PNPFS_CCB)RelatedFileObject->FsContext2)->Type == CCB_DIRECTORY))
    {
        DPRINT("Open the root directory\n");

        NpfsOpenRootDirectory(Vcb->RootFcb,
                              FileObject,
                              &Irp->IoStatus);

        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }


    /*
    * Step 1. Find the pipe we're trying to open.
    */
    KeLockMutex(&Vcb->PipeListLock);
    Fcb = NpfsFindPipe(Vcb,
        &FileObject->FileName);
    if (Fcb == NULL)
    {
        /* Not found, bail out with error. */
        DPRINT("No pipe found!\n");
        KeUnlockMutex(&Vcb->PipeListLock);
        Irp->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    KeUnlockMutex(&Vcb->PipeListLock);

    /*
    * Acquire the lock for CCB lists. From now on no modifications to the
    * CCB lists are allowed, because it can cause various misconsistencies.
    */
    KeLockMutex(&Fcb->CcbListLock);

    /*
    * Step 2. Create the client CCB.
    */
    ClientCcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_CCB));
    if (ClientCcb == NULL)
    {
        DPRINT("No memory!\n");
        KeUnlockMutex(&Fcb->CcbListLock);
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_MEMORY;
    }

    ClientCcb->Type = CCB_PIPE;
    ClientCcb->Thread = (struct ETHREAD *)Irp->Tail.Overlay.Thread;
    ClientCcb->Fcb = Fcb;
    ClientCcb->PipeEnd = FILE_PIPE_CLIENT_END;
    ClientCcb->OtherSide = NULL;
#ifndef USING_PROPER_NPFS_WAIT_SEMANTICS
    ClientCcb->PipeState = SpecialAccess ? 0 : FILE_PIPE_DISCONNECTED_STATE;
#else
    ClientCcb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
#endif
    InitializeListHead(&ClientCcb->ReadRequestListHead);

    DPRINT("CCB: %p\n", ClientCcb);

    /* Initialize data list. */
    if (Fcb->OutboundQuota)
    {
        ClientCcb->Data = ExAllocatePool(PagedPool, Fcb->OutboundQuota);
        if (ClientCcb->Data == NULL)
        {
            DPRINT("No memory!\n");
            ExFreePool(ClientCcb);
            KeUnlockMutex(&Fcb->CcbListLock);
            Irp->IoStatus.Status = STATUS_NO_MEMORY;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        ClientCcb->Data = NULL;
    }

    ClientCcb->ReadPtr = ClientCcb->Data;
    ClientCcb->WritePtr = ClientCcb->Data;
    ClientCcb->ReadDataAvailable = 0;
    ClientCcb->WriteQuotaAvailable = Fcb->OutboundQuota;
    ClientCcb->MaxDataLength = Fcb->OutboundQuota;
    ExInitializeFastMutex(&ClientCcb->DataListLock);
    KeInitializeEvent(&ClientCcb->ConnectEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&ClientCcb->ReadEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ClientCcb->WriteEvent, NotificationEvent, FALSE);


    /*
    * Step 3. Search for listening server CCB.
    */
#ifndef USING_PROPER_NPFS_WAIT_SEMANTICS
    if (!SpecialAccess)
    {
#endif
        /*
        * WARNING: Point of no return! Once we get the server CCB it's
        * possible that we completed a wait request and so we have to
        * complete even this request.
        */

        ServerCcb = NpfsFindListeningServerInstance(Fcb);
        if (ServerCcb == NULL)
        {
            PLIST_ENTRY CurrentEntry;
            PNPFS_CCB Ccb;

            /*
            * If no waiting server CCB was found then try to pick
            * one of the listing server CCB on the pipe.
            */

            CurrentEntry = Fcb->ServerCcbListHead.Flink;
            while (CurrentEntry != &Fcb->ServerCcbListHead)
            {
                Ccb = CONTAINING_RECORD(CurrentEntry, NPFS_CCB, CcbListEntry);
                if (Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
                {
                    ServerCcb = Ccb;
                    break;
                }
                CurrentEntry = CurrentEntry->Flink;
            }

            /*
            * No one is listening to me?! I'm so lonely... :(
            */

            if (ServerCcb == NULL)
            {
                /* Not found, bail out with error for FILE_OPEN requests. */
                DPRINT("No listening server CCB found!\n");
                if (ClientCcb->Data)
                {
                    ExFreePool(ClientCcb->Data);
                }

                ExFreePool(ClientCcb);
                KeUnlockMutex(&Fcb->CcbListLock);
                Irp->IoStatus.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_OBJECT_PATH_NOT_FOUND;
            }
        }
        else
        {
            /* Signal the server thread and remove it from the waiter list */
            /* FIXME: Merge this with the NpfsFindListeningServerInstance routine. */
            NpfsSignalAndRemoveListeningServerInstance(Fcb, ServerCcb);
        }
#ifndef USING_PROPER_NPFS_WAIT_SEMANTICS
    }
    else if (IsListEmpty(&Fcb->ServerCcbListHead))
    {
        DPRINT("No server fcb found!\n");

        if (ClientCcb->Data)
        {
            ExFreePool(ClientCcb->Data);
        }

        ExFreePool(ClientCcb);

        KeUnlockMutex(&Fcb->CcbListLock);
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }
#endif

    /*
    * Step 4. Add the client CCB to a list and connect it if possible.
    */

    /* Add the client CCB to the pipe CCB list. */
    InsertTailList(&Fcb->ClientCcbListHead, &ClientCcb->CcbListEntry);

    /* Connect to listening server side */
    if (ServerCcb)
    {
        ClientCcb->OtherSide = ServerCcb;
        ServerCcb->OtherSide = ClientCcb;
        ClientCcb->PipeState = FILE_PIPE_CONNECTED_STATE;
        ServerCcb->PipeState = FILE_PIPE_CONNECTED_STATE;
        KeSetEvent(&ServerCcb->ConnectEvent, IO_NO_INCREMENT, FALSE);
    }

    KeUnlockMutex(&Fcb->CcbListLock);

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = ClientCcb;
    FileObject->Flags |= FO_NAMED_PIPE;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT("Success!\n");

    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
NpfsCreateNamedPipe(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
    PEXTENDED_IO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PNPFS_VCB Vcb;
    PNPFS_FCB Fcb;
    PNPFS_CCB Ccb;
    PNAMED_PIPE_CREATE_PARAMETERS Buffer;
    BOOLEAN NewPipe = FALSE;

    DPRINT("NpfsCreateNamedPipe(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    DPRINT("FileObject %p\n", FileObject);
    DPRINT("Pipe name %wZ\n", &FileObject->FileName);

    Buffer = IoStack->Parameters.CreatePipe.Parameters;

    Irp->IoStatus.Information = 0;

    if (!(IoStack->Parameters.CreatePipe.ShareAccess & (FILE_SHARE_READ|FILE_SHARE_WRITE)) ||
        (IoStack->Parameters.CreatePipe.ShareAccess & ~(FILE_SHARE_READ|FILE_SHARE_WRITE)))
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    Ccb = ExAllocatePool(NonPagedPool, sizeof(NPFS_CCB));
    if (Ccb == NULL)
    {
        Irp->IoStatus.Status = STATUS_NO_MEMORY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_MEMORY;
    }

    Ccb->Type = CCB_PIPE;
    Ccb->Thread = (struct ETHREAD *)Irp->Tail.Overlay.Thread;
    KeLockMutex(&Vcb->PipeListLock);

    /*
    * First search for existing Pipe with the same name.
    */
    Fcb = NpfsFindPipe(Vcb,
        &FileObject->FileName);
    if (Fcb != NULL)
    {
        /*
        * Found Pipe with the same name. Check if we are
        * allowed to use it.
        */
        KeUnlockMutex(&Vcb->PipeListLock);

        if (Fcb->CurrentInstances >= Fcb->MaximumInstances)
        {
            DPRINT("Out of instances.\n");
            ExFreePool(Ccb);
            Irp->IoStatus.Status = STATUS_INSTANCE_NOT_AVAILABLE;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSTANCE_NOT_AVAILABLE;
        }

        if (Fcb->MaximumInstances != Buffer->MaximumInstances ||
            Fcb->TimeOut.QuadPart != Buffer->DefaultTimeout.QuadPart ||
            Fcb->PipeType != Buffer->NamedPipeType)
        {
            DPRINT("Asked for invalid pipe mode.\n");
            ExFreePool(Ccb);
            Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_ACCESS_DENIED;
        }
    }
    else
    {
        NewPipe = TRUE;
        Fcb = ExAllocatePool(NonPagedPool, sizeof(NPFS_FCB));
        if (Fcb == NULL)
        {
            KeUnlockMutex(&Vcb->PipeListLock);
            ExFreePool(Ccb);
            Irp->IoStatus.Status = STATUS_NO_MEMORY;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NO_MEMORY;
        }

        Fcb->Type = FCB_PIPE;
        Fcb->Vcb = Vcb;
        Fcb->PipeName.Length = FileObject->FileName.Length;
        Fcb->PipeName.MaximumLength = Fcb->PipeName.Length + sizeof(UNICODE_NULL);
        Fcb->PipeName.Buffer = ExAllocatePool(NonPagedPool, Fcb->PipeName.MaximumLength);
        if (Fcb->PipeName.Buffer == NULL)
        {
            KeUnlockMutex(&Vcb->PipeListLock);
            ExFreePool(Fcb);
            ExFreePool(Ccb);
            Irp->IoStatus.Status = STATUS_NO_MEMORY;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NO_MEMORY;
        }

        RtlCopyUnicodeString(&Fcb->PipeName, &FileObject->FileName);

        InitializeListHead(&Fcb->ServerCcbListHead);
        InitializeListHead(&Fcb->ClientCcbListHead);
        InitializeListHead(&Fcb->WaiterListHead);
        KeInitializeMutex(&Fcb->CcbListLock, 0);

        Fcb->PipeType = Buffer->NamedPipeType;
        Fcb->ServerReadMode = Buffer->ReadMode;
        /* MSDN documentation reads that clients always start off in byte mode */
        Fcb->ClientReadMode = FILE_PIPE_BYTE_STREAM_MODE;

        Fcb->CompletionMode = Buffer->CompletionMode;
        switch (IoStack->Parameters.CreatePipe.ShareAccess & (FILE_SHARE_READ|FILE_SHARE_WRITE))
        {
        case FILE_SHARE_READ:
            Fcb->PipeConfiguration = FILE_PIPE_OUTBOUND;
            break;
        case FILE_SHARE_WRITE:
            Fcb->PipeConfiguration = FILE_PIPE_INBOUND;
            break;
        case FILE_SHARE_READ|FILE_SHARE_WRITE:
            Fcb->PipeConfiguration = FILE_PIPE_FULL_DUPLEX;
            break;
        }
        Fcb->MaximumInstances = Buffer->MaximumInstances;
        Fcb->CurrentInstances = 0;
        Fcb->TimeOut = Buffer->DefaultTimeout;
        if (!(Fcb->PipeConfiguration & FILE_PIPE_OUTBOUND) ||
            Fcb->PipeConfiguration & FILE_PIPE_FULL_DUPLEX)
        {
            if (Buffer->InboundQuota == 0)
            {
                Fcb->InboundQuota = Vcb->DefaultQuota;
            }
            else
            {
                Fcb->InboundQuota = PAGE_ROUND_UP(Buffer->InboundQuota);
                if (Fcb->InboundQuota < Vcb->MinQuota)
                {
                    Fcb->InboundQuota = Vcb->MinQuota;
                }
                else if (Fcb->InboundQuota > Vcb->MaxQuota)
                {
                    Fcb->InboundQuota = Vcb->MaxQuota;
                }
            }
        }
        else
        {
            Fcb->InboundQuota = 0;
        }

        if (Fcb->PipeConfiguration & (FILE_PIPE_FULL_DUPLEX|FILE_PIPE_OUTBOUND))
        {
            if (Buffer->OutboundQuota == 0)
            {
                Fcb->OutboundQuota = Vcb->DefaultQuota;
            }
            else
            {
                Fcb->OutboundQuota = PAGE_ROUND_UP(Buffer->OutboundQuota);
                if (Fcb->OutboundQuota < Vcb->MinQuota)
                {
                    Fcb->OutboundQuota = Vcb->MinQuota;
                }
                else if (Fcb->OutboundQuota > Vcb->MaxQuota)
                {
                    Fcb->OutboundQuota = Vcb->MaxQuota;
                }
            }
        }
        else
        {
            Fcb->OutboundQuota = 0;
        }

        InsertTailList(&Vcb->PipeListHead, &Fcb->PipeListEntry);
        KeUnlockMutex(&Vcb->PipeListLock);
    }

    if (Fcb->InboundQuota)
    {
        Ccb->Data = ExAllocatePool(PagedPool, Fcb->InboundQuota);
        if (Ccb->Data == NULL)
        {
            ExFreePool(Ccb);

            if (NewPipe)
            {
                KeLockMutex(&Vcb->PipeListLock);
                RemoveEntryList(&Fcb->PipeListEntry);
                KeUnlockMutex(&Vcb->PipeListLock);
                RtlFreeUnicodeString(&Fcb->PipeName);
                ExFreePool(Fcb);
            }

            Irp->IoStatus.Status = STATUS_NO_MEMORY;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        Ccb->Data = NULL;
    }

    Ccb->ReadPtr = Ccb->Data;
    Ccb->WritePtr = Ccb->Data;
    Ccb->ReadDataAvailable = 0;
    Ccb->WriteQuotaAvailable = Fcb->InboundQuota;
    Ccb->MaxDataLength = Fcb->InboundQuota;
    InitializeListHead(&Ccb->ReadRequestListHead);
    ExInitializeFastMutex(&Ccb->DataListLock);

    Fcb->CurrentInstances++;

    Ccb->Fcb = Fcb;
    Ccb->PipeEnd = FILE_PIPE_SERVER_END;
    Ccb->PipeState = FILE_PIPE_LISTENING_STATE;
    Ccb->OtherSide = NULL;

    DPRINT("CCB: %p\n", Ccb);

    KeInitializeEvent(&Ccb->ConnectEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&Ccb->ReadEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Ccb->WriteEvent, NotificationEvent, FALSE);

    KeLockMutex(&Fcb->CcbListLock);
    InsertTailList(&Fcb->ServerCcbListHead, &Ccb->CcbListEntry);
    KeUnlockMutex(&Fcb->CcbListLock);

    FileObject->FsContext = Fcb;
    FileObject->FsContext2 = Ccb;
    FileObject->Flags |= FO_NAMED_PIPE;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT("Success!\n");

    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
NpfsCleanup(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
    PNPFS_VCB Vcb;
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PNPFS_CCB Ccb, OtherSide;
    PNPFS_FCB Fcb;
    BOOLEAN Server;

    DPRINT("NpfsCleanup(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    FileObject = IoStack->FileObject;
    Ccb = FileObject->FsContext2;

    if (Ccb == NULL)
    {
        DPRINT("Success!\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    if (Ccb->Type == CCB_DEVICE)
    {
        DPRINT("Cleanup the file system!\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    if (Ccb->Type == CCB_DIRECTORY)
    {
        DPRINT("Cleanup the root directory!\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    DPRINT("CCB %p\n", Ccb);
    Fcb = Ccb->Fcb;

    DPRINT("Cleaning pipe %wZ\n", &Fcb->PipeName);

    KeLockMutex(&Fcb->CcbListLock);

    Server = (Ccb->PipeEnd == FILE_PIPE_SERVER_END);

    if (Server)
    {
        /* FIXME: Clean up existing connections here ?? */
        DPRINT("Server\n");
    }
    else
    {
        DPRINT("Client\n");
    }
    if ((Ccb->PipeState == FILE_PIPE_CONNECTED_STATE) && (Ccb->OtherSide))
    {
        OtherSide = Ccb->OtherSide;
        /* Lock the server first */
        if (Server)
        {
            ExAcquireFastMutex(&Ccb->DataListLock);
            ExAcquireFastMutex(&OtherSide->DataListLock);
        }
        else
        {
            ExAcquireFastMutex(&OtherSide->DataListLock);
            ExAcquireFastMutex(&Ccb->DataListLock);
        }
        OtherSide->OtherSide = NULL;
        /*
        * Signaling the write event. If is possible that an other
        * thread waits for an empty buffer.
        */
        KeSetEvent(&OtherSide->ReadEvent, IO_NO_INCREMENT, FALSE);
        KeSetEvent(&OtherSide->WriteEvent, IO_NO_INCREMENT, FALSE);
        if (Server)
        {
            ExReleaseFastMutex(&OtherSide->DataListLock);
            ExReleaseFastMutex(&Ccb->DataListLock);
        }
        else
        {
            ExReleaseFastMutex(&Ccb->DataListLock);
            ExReleaseFastMutex(&OtherSide->DataListLock);
        }
    }
    else if (Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
    {
        PLIST_ENTRY Entry;
        PNPFS_WAITER_ENTRY WaitEntry = NULL;
        BOOLEAN Complete = FALSE;
        KIRQL oldIrql;
        PIRP tmpIrp;

        Entry = Ccb->Fcb->WaiterListHead.Flink;
        while (Entry != &Ccb->Fcb->WaiterListHead)
        {
            WaitEntry = CONTAINING_RECORD(Entry, NPFS_WAITER_ENTRY, Entry);
            if (WaitEntry->Ccb == Ccb)
            {
                RemoveEntryList(Entry);
                tmpIrp = CONTAINING_RECORD(WaitEntry, IRP, Tail.Overlay.DriverContext);
                IoAcquireCancelSpinLock(&oldIrql);
                if (!tmpIrp->Cancel)
                {
                    (void)IoSetCancelRoutine(tmpIrp, NULL);
                    Complete = TRUE;
                }
                IoReleaseCancelSpinLock(oldIrql);
                if (Complete)
                {
                    tmpIrp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    tmpIrp->IoStatus.Information = 0;
                    IoCompleteRequest(tmpIrp, IO_NO_INCREMENT);
                }
                break;
            }
            Entry = Entry->Flink;
        }

    }
    Ccb->PipeState = FILE_PIPE_CLOSING_STATE;

    KeUnlockMutex(&Fcb->CcbListLock);

    ExAcquireFastMutex(&Ccb->DataListLock);
    if (Ccb->Data)
    {
        ExFreePool(Ccb->Data);
        Ccb->Data = NULL;
        Ccb->ReadPtr = NULL;
        Ccb->WritePtr = NULL;
    }
    ExReleaseFastMutex(&Ccb->DataListLock);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT("Success!\n");

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
NpfsClose(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PNPFS_VCB Vcb;
    PNPFS_FCB Fcb;
    PNPFS_CCB Ccb;
    BOOLEAN Server;

    DPRINT("NpfsClose(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    FileObject = IoStack->FileObject;
    Ccb = FileObject->FsContext2;

    if (Ccb == NULL)
    {
        DPRINT("Success!\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    if (Ccb->Type == CCB_DEVICE)
    {
        DPRINT("Closing the file system!\n");

        ExFreePool(Ccb);
        FileObject->FsContext = NULL;
        FileObject->FsContext2 = NULL;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    if (Ccb->Type == CCB_DIRECTORY)
    {
        DPRINT("Closing the root directory!\n");

        if (Ccb->u.Directory.SearchPattern.Buffer != NULL)
            ExFreePool(Ccb->u.Directory.SearchPattern.Buffer);

        ExFreePool(Ccb);
        FileObject->FsContext = NULL;
        FileObject->FsContext2 = NULL;

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    DPRINT("CCB %p\n", Ccb);
    Fcb = Ccb->Fcb;

    DPRINT("Closing pipe %wZ\n", &Fcb->PipeName);

    KeLockMutex(&Fcb->CcbListLock);

    Server = (Ccb->PipeEnd == FILE_PIPE_SERVER_END);

    if (Server)
    {
        DPRINT("Server\n");
        Fcb->CurrentInstances--;
    }
    else
    {
        DPRINT("Client\n");
    }

    /* Disconnect the pipes */
    if (Ccb->OtherSide)
    {
        Ccb->OtherSide->OtherSide = NULL;
        Ccb->OtherSide = NULL;
    }

    ASSERT(Ccb->PipeState == FILE_PIPE_CLOSING_STATE);

    FileObject->FsContext2 = NULL;

    RemoveEntryList(&Ccb->CcbListEntry);

    ExFreePool(Ccb);

    KeUnlockMutex(&Fcb->CcbListLock);

    if (IsListEmpty(&Fcb->ServerCcbListHead) &&
        IsListEmpty(&Fcb->ClientCcbListHead))
    {
        RtlFreeUnicodeString(&Fcb->PipeName);
        KeLockMutex(&Vcb->PipeListLock);
        RemoveEntryList(&Fcb->PipeListEntry);
        KeUnlockMutex(&Vcb->PipeListLock);
        ExFreePool(Fcb);
        FileObject->FsContext = NULL;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT("Success!\n");

    return STATUS_SUCCESS;
}

/* EOF */
