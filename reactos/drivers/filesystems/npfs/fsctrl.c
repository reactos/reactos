/*
* COPYRIGHT:  See COPYING in the top level directory
* PROJECT:    ReactOS kernel
* FILE:       drivers/filesystems/npfs/fsctrl.c
* PURPOSE:    Named pipe filesystem
* PROGRAMMER: David Welch <welch@cwcom.net>
*             Eric Kohl
*             Michael Martin
*/

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

//#define USING_PROPER_NPFS_WAIT_SEMANTICS

/* FUNCTIONS *****************************************************************/

static DRIVER_CANCEL NpfsListeningCancelRoutine;
static VOID NTAPI
NpfsListeningCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PNPFS_WAITER_ENTRY Waiter;

    UNREFERENCED_PARAMETER(DeviceObject);

    Waiter = (PNPFS_WAITER_ENTRY)&Irp->Tail.Overlay.DriverContext;

    DPRINT("NpfsListeningCancelRoutine() called for <%wZ>\n",
        &Waiter->Ccb->Fcb->PipeName);

    IoReleaseCancelSpinLock(Irp->CancelIrql);


    KeLockMutex(&Waiter->Ccb->Fcb->CcbListLock);
    RemoveEntryList(&Waiter->Entry);
    KeUnlockMutex(&Waiter->Ccb->Fcb->CcbListLock);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


static NTSTATUS
NpfsAddListeningServerInstance(PIRP Irp,
                               PNPFS_CCB Ccb)
{
    PNPFS_WAITER_ENTRY Entry;
    KIRQL oldIrql;

    Entry = (PNPFS_WAITER_ENTRY)&Irp->Tail.Overlay.DriverContext;

    Entry->Ccb = Ccb;

    KeLockMutex(&Ccb->Fcb->CcbListLock);

    IoAcquireCancelSpinLock(&oldIrql);
    if (!Irp->Cancel)
    {
        Ccb->PipeState = FILE_PIPE_LISTENING_STATE;
        IoMarkIrpPending(Irp);
        InsertTailList(&Ccb->Fcb->WaiterListHead, &Entry->Entry);
        (void)IoSetCancelRoutine(Irp, NpfsListeningCancelRoutine);
        IoReleaseCancelSpinLock(oldIrql);
        KeUnlockMutex(&Ccb->Fcb->CcbListLock);
        return STATUS_PENDING;
    }
    IoReleaseCancelSpinLock(oldIrql);

    RemoveEntryList(&Entry->Entry);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    KeUnlockMutex(&Ccb->Fcb->CcbListLock);

    return STATUS_CANCELLED;
}


static NTSTATUS
NpfsConnectPipe(PIRP Irp,
                PNPFS_CCB Ccb)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    ULONG Flags;
    PLIST_ENTRY current_entry;
    PNPFS_FCB Fcb;
    PNPFS_CCB ClientCcb;
    NTSTATUS Status;
    KPROCESSOR_MODE WaitMode;

    DPRINT("NpfsConnectPipe()\n");

    /* Fail, if the CCB is not a pipe CCB */
    if (Ccb->Type != CCB_PIPE)
    {
        DPRINT("Not a pipe\n");
        return STATUS_ILLEGAL_FUNCTION;
    }

    /* Fail, if the CCB is not a server end CCB */
    if (Ccb->PipeEnd != FILE_PIPE_SERVER_END)
    {
        DPRINT("Not the server end\n");
        return STATUS_ILLEGAL_FUNCTION;
    }

    if (Ccb->PipeState == FILE_PIPE_CONNECTED_STATE)
    {
        KeResetEvent(&Ccb->ConnectEvent);
        return STATUS_PIPE_CONNECTED;
    }

    if (Ccb->PipeState == FILE_PIPE_CLOSING_STATE)
        return STATUS_PIPE_CLOSING;

    DPRINT("Waiting for connection...\n");

    Fcb = Ccb->Fcb;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = IoStack->FileObject;
    Flags = FileObject->Flags;
    WaitMode = Irp->RequestorMode;

    /* search for a listening client fcb */
    KeLockMutex(&Fcb->CcbListLock);

    current_entry = Fcb->ClientCcbListHead.Flink;
    while (current_entry != &Fcb->ClientCcbListHead)
    {
        ClientCcb = CONTAINING_RECORD(current_entry,
            NPFS_CCB,
            CcbListEntry);

        if (ClientCcb->PipeState == 0)
        {
            /* found a passive (waiting) client CCB */
            DPRINT("Passive (waiting) client CCB found -- wake the client\n");
            KeSetEvent(&ClientCcb->ConnectEvent, IO_NO_INCREMENT, FALSE);
            break;
        }

#if 0
        if (ClientCcb->PipeState == FILE_PIPE_LISTENING_STATE)
        {
            /* found a listening client CCB */
            DPRINT("Listening client CCB found -- connecting\n");

            /* connect client and server CCBs */
            Ccb->OtherSide = ClientCcb;
            ClientCcb->OtherSide = Ccb;

            /* set connected state */
            Ccb->PipeState = FILE_PIPE_CONNECTED_STATE;
            ClientCcb->PipeState = FILE_PIPE_CONNECTED_STATE;

            KeUnlockMutex(&Fcb->CcbListLock);

            /* FIXME: create and initialize data queues */

            /* signal client's connect event */
            DPRINT("Setting the ConnectEvent for %x\n", ClientCcb);
            KeSetEvent(&ClientCcb->ConnectEvent, IO_NO_INCREMENT, FALSE);

            return STATUS_PIPE_CONNECTED;
        }
#endif

        current_entry = current_entry->Flink;
    }

    /* no listening client fcb found */
    DPRINT("No listening client fcb found -- waiting for client\n");

    Status = NpfsAddListeningServerInstance(Irp, Ccb);

    KeUnlockMutex(&Fcb->CcbListLock);

    if ((Status == STATUS_PENDING) && (Flags & FO_SYNCHRONOUS_IO))
    {
        KeWaitForSingleObject(&Ccb->ConnectEvent,
            UserRequest,
            WaitMode,
            (Flags & FO_ALERTABLE_IO) != 0,
            NULL);
    }

    DPRINT("NpfsConnectPipe() done (Status %lx)\n", Status);

    return Status;
}


static NTSTATUS
NpfsDisconnectPipe(PNPFS_CCB Ccb)
{
    NTSTATUS Status;
    PNPFS_FCB Fcb;
    PNPFS_CCB OtherSide;
    BOOLEAN Server;

    DPRINT("NpfsDisconnectPipe()\n");

    /* Fail, if the CCB is not a pipe CCB */
    if (Ccb->Type != CCB_PIPE)
    {
        DPRINT("Not a pipe\n");
        return STATUS_ILLEGAL_FUNCTION;
    }

    /* Fail, if the CCB is not a server end CCB */
    if (Ccb->PipeEnd != FILE_PIPE_SERVER_END)
    {
        DPRINT("Not the server end\n");
        return STATUS_ILLEGAL_FUNCTION;
    }

    Fcb = Ccb->Fcb;
    KeLockMutex(&Fcb->CcbListLock);

    if (Ccb->PipeState == FILE_PIPE_DISCONNECTED_STATE)
    {
        DPRINT("Pipe is already disconnected\n");
        Status = STATUS_PIPE_DISCONNECTED;
    }
    else if ((!Ccb->OtherSide) && (Ccb->PipeState == FILE_PIPE_CONNECTED_STATE))
    {
        ExAcquireFastMutex(&Ccb->DataListLock);
        Ccb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
        ExReleaseFastMutex(&Ccb->DataListLock);
        Status = STATUS_SUCCESS;
    }
    else if (Ccb->PipeState == FILE_PIPE_CONNECTED_STATE)
    {
        Server = (Ccb->PipeEnd == FILE_PIPE_SERVER_END);
        OtherSide = Ccb->OtherSide;
        //Ccb->OtherSide = NULL;
        Ccb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
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
        OtherSide->PipeState = FILE_PIPE_DISCONNECTED_STATE;
        //OtherSide->OtherSide = NULL;
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
        Status = STATUS_SUCCESS;
    }
    else if (Ccb->PipeState == FILE_PIPE_LISTENING_STATE)
    {
        PLIST_ENTRY Entry;
        PNPFS_WAITER_ENTRY WaitEntry = NULL;
        BOOLEAN Complete = FALSE;
        PIRP Irp = NULL;

        Entry = Ccb->Fcb->WaiterListHead.Flink;
        while (Entry != &Ccb->Fcb->WaiterListHead)
        {
            WaitEntry = CONTAINING_RECORD(Entry, NPFS_WAITER_ENTRY, Entry);
            if (WaitEntry->Ccb == Ccb)
            {
                RemoveEntryList(Entry);
                Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DriverContext);
                Complete = (NULL != IoSetCancelRoutine(Irp, NULL));
                break;
            }
            Entry = Entry->Flink;
        }

        if (Irp)
        {
            if (Complete)
            {
                Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
        }
        Ccb->PipeState = FILE_PIPE_DISCONNECTED_STATE;
        Status = STATUS_SUCCESS;
    }
    else if (Ccb->PipeState == FILE_PIPE_CLOSING_STATE)
    {
        Status = STATUS_PIPE_CLOSING;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    KeUnlockMutex(&Fcb->CcbListLock);
    return Status;
}

static NTSTATUS
NpfsWaitPipe(PIRP Irp,
             PNPFS_CCB Ccb)
{
    PLIST_ENTRY current_entry;
    PNPFS_FCB Fcb;
    PNPFS_CCB ServerCcb;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
    PLARGE_INTEGER TimeOut;
    NTSTATUS Status;
    PEXTENDED_IO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    PNPFS_VCB Vcb;

    IoStack = (PEXTENDED_IO_STACK_LOCATION)IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack);
    FileObject = IoStack->FileObject;
    ASSERT(FileObject);

    DPRINT("Waiting on Pipe %wZ\n", &FileObject->FileName);

    WaitPipe = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    ASSERT(Ccb->Fcb);
    ASSERT(Ccb->Fcb->Vcb);

    /* Get the VCB */
    Vcb = Ccb->Fcb->Vcb;

    /* Lock the pipe list */
    KeLockMutex(&Vcb->PipeListLock);

    /* File a pipe with the given name */
    Fcb = NpfsFindPipe(Vcb,
                       &FileObject->FileName);

    /* Unlock the pipe list */
    KeUnlockMutex(&Vcb->PipeListLock);

    /* Fail if not pipe was found */
    if (Fcb == NULL)
    {
        DPRINT("No pipe found!\n");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* search for listening server */
    current_entry = Fcb->ServerCcbListHead.Flink;
    while (current_entry != &Fcb->ServerCcbListHead)
    {
        ServerCcb = CONTAINING_RECORD(current_entry,
                                      NPFS_CCB,
                                      CcbListEntry);

        if (ServerCcb->PipeState == FILE_PIPE_LISTENING_STATE)
        {
            /* found a listening server CCB */
            DPRINT("Listening server CCB found -- connecting\n");
            NpfsDereferenceFcb(Fcb);
            return STATUS_SUCCESS;
        }

        current_entry = current_entry->Flink;
    }

    /* No listening server fcb found, so wait for one */

    /* If a timeout specified */
    if (WaitPipe->TimeoutSpecified)
    {
        /* NMPWAIT_USE_DEFAULT_WAIT = 0 */
        if (WaitPipe->Timeout.QuadPart == 0)
        {
            TimeOut = &Fcb->TimeOut;
        }
        else
        {
            TimeOut = &WaitPipe->Timeout;
        }
    }
    else
    {
        /* Wait forever */
        TimeOut = NULL;
    }
    NpfsDereferenceFcb(Fcb);

    Status = KeWaitForSingleObject(&Ccb->ConnectEvent,
                                   UserRequest,
                                   Irp->RequestorMode,
                                   (Ccb->FileObject->Flags & FO_ALERTABLE_IO) != 0,
                                   TimeOut);
    if ((Status == STATUS_USER_APC) || (Status == STATUS_KERNEL_APC) || (Status == STATUS_ALERTED))
        Status = STATUS_CANCELLED;

    DPRINT("KeWaitForSingleObject() returned (Status %lx)\n", Status);

    return Status;
}

NTSTATUS
NpfsWaitPipe2(PIRP Irp,
             PNPFS_CCB Ccb)
{
    PLIST_ENTRY current_entry;
    PNPFS_FCB Fcb;
    PNPFS_CCB ServerCcb;
    PFILE_PIPE_WAIT_FOR_BUFFER WaitPipe;
    LARGE_INTEGER TimeOut;
    NTSTATUS Status;
#ifdef USING_PROPER_NPFS_WAIT_SEMANTICS
    PNPFS_VCB Vcb;
    UNICODE_STRING PipeName;
#endif

    DPRINT("NpfsWaitPipe\n");

    WaitPipe = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;

#ifdef USING_PROPER_NPFS_WAIT_SEMANTICS
    /* Fail, if the CCB does not represent the root directory */
    if (Ccb->Type != CCB_DIRECTORY)
        return STATUS_ILLEGAL_FUNCTION;

    /* Calculate the pipe name length and allocate the buffer */
    PipeName.Length = WaitPipe->NameLength + sizeof(WCHAR);
    PipeName.MaximumLength = PipeName.Length + sizeof(WCHAR);
    PipeName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                            PipeName.MaximumLength,
                                            TAG_NPFS_NAMEBLOCK);
    if (PipeName.Buffer == NULL)
    {
        DPRINT1("Could not allocate memory for the pipe name!\n");
        return STATUS_NO_MEMORY;
    }

    /* Copy the pipe name into the buffer, prepend a backslash and append a 0 character */
    PipeName.Buffer[0] = L'\\';
    RtlCopyMemory(&PipeName.Buffer[1],
                  &WaitPipe->Name[0],
                  WaitPipe->NameLength);
    PipeName.Buffer[PipeName.Length / sizeof(WCHAR)] = 0;

    DPRINT("Waiting for Pipe %wZ\n", &PipeName);

    /* Get the VCB */
    Vcb = Ccb->Fcb->Vcb;

    /* Lock the pipe list */
    KeLockMutex(&Vcb->PipeListLock);

    /* File a pipe with the given name */
    Fcb = NpfsFindPipe(Vcb,
                       &PipeName);

    /* Unlock the pipe list */
    KeUnlockMutex(&Vcb->PipeListLock);

    /* Release the pipe name buffer */
    ExFreePoolWithTag(PipeName.Buffer, TAG_NPFS_NAMEBLOCK);

    /* Fail if not pipe was found */
    if (Fcb == NULL)
    {
        DPRINT("No pipe found!\n");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    DPRINT("Fcb %p\n", Fcb);
#else
    Fcb = Ccb->Fcb;

    if (Ccb->PipeState != 0)
    {
        DPRINT("Pipe is not in passive (waiting) state!\n");
        return STATUS_UNSUCCESSFUL;
    }
#endif

    /* search for listening server */
    current_entry = Fcb->ServerCcbListHead.Flink;
    while (current_entry != &Fcb->ServerCcbListHead)
    {
        ServerCcb = CONTAINING_RECORD(current_entry,
            NPFS_CCB,
            CcbListEntry);

        if (ServerCcb->PipeState == FILE_PIPE_LISTENING_STATE)
        {
            /* found a listening server CCB */
            DPRINT("Listening server CCB found -- connecting\n");
#ifdef USING_PROPER_NPFS_WAIT_SEMANTICS
            NpfsDereferenceFcb(Fcb);
#endif
            return STATUS_SUCCESS;
        }

        current_entry = current_entry->Flink;
    }

    /* No listening server fcb found */

    /* If no timeout specified, use the default one */
    if (WaitPipe->TimeoutSpecified)
        TimeOut = WaitPipe->Timeout;
    else
        TimeOut = Fcb->TimeOut;
#ifdef USING_PROPER_NPFS_WAIT_SEMANTICS
    NpfsDereferenceFcb(Fcb);
#endif

    /* Wait for one */
    Status = KeWaitForSingleObject(&Ccb->ConnectEvent,
        UserRequest,
        Irp->RequestorMode,
        (Ccb->FileObject->Flags & FO_ALERTABLE_IO) != 0,
        &TimeOut);
    if ((Status == STATUS_USER_APC) || (Status == STATUS_KERNEL_APC) || (Status == STATUS_ALERTED))
        Status = STATUS_CANCELLED;

    DPRINT("KeWaitForSingleObject() returned (Status %lx)\n", Status);

    return Status;
}


/*
* FUNCTION: Return current state of a pipe
* ARGUMENTS:
*     Irp   = Pointer to I/O request packet
*     IrpSp = Pointer to current stack location of Irp
* RETURNS:
*     Status of operation
*/

/*
* FUNCTION: Peek at a pipe (get information about messages)
* ARGUMENTS:
*     Irp = Pointer to I/O request packet
*     IoStack = Pointer to current stack location of Irp
* RETURNS:
*     Status of operation
*/
static NTSTATUS
NpfsPeekPipe(PIRP Irp,
             PIO_STACK_LOCATION IoStack)
{
    ULONG OutputBufferLength;
    ULONG ReturnLength = 0;
    PFILE_PIPE_PEEK_BUFFER Reply;
    //PNPFS_FCB Fcb;
    PNPFS_CCB Ccb;
    NTSTATUS Status;
    ULONG MessageCount = 0;
    ULONG MessageLength;
    ULONG ReadDataAvailable;
    PVOID BufferPtr;

    DPRINT("NpfsPeekPipe\n");

    OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    DPRINT("OutputBufferLength: %lu\n", OutputBufferLength);

    /* Validate parameters */
    if (OutputBufferLength < (ULONG)FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]))
    {
        DPRINT1("Buffer too small\n");
        return STATUS_INVALID_PARAMETER;
    }

    Ccb = IoStack->FileObject->FsContext2;
    Reply = Irp->AssociatedIrp.SystemBuffer;
    //Fcb = Ccb->Fcb;


    Reply->NamedPipeState = Ccb->PipeState;

    Reply->ReadDataAvailable = Ccb->ReadDataAvailable;
    DPRINT("ReadDataAvailable: %lu\n", Ccb->ReadDataAvailable);

    ExAcquireFastMutex(&Ccb->DataListLock);
    BufferPtr = Ccb->ReadPtr;
    DPRINT("BufferPtr = %p\n", BufferPtr);
    if (Ccb->Fcb->PipeType == FILE_PIPE_BYTE_STREAM_TYPE)
    {
        DPRINT("Byte Stream Mode\n");
        Reply->MessageLength = Ccb->ReadDataAvailable;
        DPRINT("Reply->MessageLength  %lu\n", Reply->MessageLength);
        MessageCount = 1;

        if (OutputBufferLength >= (ULONG)FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[Ccb->ReadDataAvailable]))
        {
            RtlCopyMemory(Reply->Data, BufferPtr, Ccb->ReadDataAvailable);
            ReturnLength = Ccb->ReadDataAvailable;
        }
    }
    else
    {
        DPRINT("Message Mode\n");
        ReadDataAvailable = Ccb->ReadDataAvailable;

        if (ReadDataAvailable > 0)
        {
            RtlCopyMemory(&Reply->MessageLength,
                          BufferPtr,
                          sizeof(Reply->MessageLength));

            while ((ReadDataAvailable > 0) && (BufferPtr < Ccb->WritePtr))
            {
                RtlCopyMemory(&MessageLength, BufferPtr, sizeof(MessageLength));

                ASSERT(MessageLength > 0);

                DPRINT("MessageLength = %lu\n", MessageLength);
                ReadDataAvailable -= MessageLength;
                MessageCount++;

                /* If its the first message, copy the Message if the size of buffer is large enough */
                if (MessageCount == 1)
                {
                    if (OutputBufferLength >= (ULONG)FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[MessageLength]))
                    {
                        RtlCopyMemory(Reply->Data,
                                      (PVOID)((ULONG_PTR)BufferPtr + sizeof(MessageLength)),
                                      MessageLength);
                        ReturnLength = MessageLength;
                    }
                }

                BufferPtr = (PVOID)((ULONG_PTR)BufferPtr + sizeof(MessageLength) + MessageLength);
                DPRINT("BufferPtr = %p\n", BufferPtr);
                DPRINT("ReadDataAvailable: %lu\n", ReadDataAvailable);
            }

            if (ReadDataAvailable != 0)
            {
                DPRINT1("Possible memory corruption.\n");
                ASSERT(FALSE);
            }
        }
    }
    ExReleaseFastMutex(&Ccb->DataListLock);

    Reply->NumberOfMessages = MessageCount;

    Irp->IoStatus.Information = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[ReturnLength]);
    Irp->IoStatus.Status = STATUS_SUCCESS;

    Status = STATUS_SUCCESS;

    DPRINT("NpfsPeekPipe done\n");

    return Status;
}


NTSTATUS NTAPI
NpfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
                      PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    //PNPFS_VCB Vcb;
    PNPFS_FCB Fcb;
    PNPFS_CCB Ccb;

    DPRINT("NpfsFileSystemContol(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    //Vcb = (PNPFS_VCB)DeviceObject->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("IoStack: %p\n", IoStack);
    FileObject = IoStack->FileObject;
    DPRINT("FileObject: %p\n", FileObject);
    Ccb = FileObject->FsContext2;
    DPRINT("CCB: %p\n", Ccb);
    Fcb = Ccb->Fcb;
    DPRINT("Pipe: %p\n", Fcb);
    DPRINT("PipeName: %wZ\n", &Fcb->PipeName);

    Irp->IoStatus.Information = 0;

    switch (IoStack->Parameters.FileSystemControl.FsControlCode)
    {
    case FSCTL_PIPE_ASSIGN_EVENT:
        DPRINT1("Assign event not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_DISCONNECT:
        DPRINT("Disconnecting pipe %wZ\n", &Fcb->PipeName);
        Status = NpfsDisconnectPipe(Ccb);
        break;

    case FSCTL_PIPE_LISTEN:
        DPRINT("Connecting pipe %wZ\n", &Fcb->PipeName);
        Status = NpfsConnectPipe(Irp, Ccb);
        break;

    case FSCTL_PIPE_PEEK:
        DPRINT("Peeking pipe %wZ\n", &Fcb->PipeName);
        Status = NpfsPeekPipe(Irp, (PIO_STACK_LOCATION)IoStack);
        break;

    case FSCTL_PIPE_QUERY_EVENT:
        DPRINT1("Query event not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_TRANSCEIVE:
        /* If you implement this, please remove the workaround in
        lib/kernel32/file/npipe.c function TransactNamedPipe() */
        DPRINT1("Transceive not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_WAIT:
        DPRINT("Waiting for pipe %wZ\n", &Fcb->PipeName);
        Status = NpfsWaitPipe(Irp, Ccb);
        break;

    case FSCTL_PIPE_IMPERSONATE:
        DPRINT1("Impersonate not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_SET_CLIENT_PROCESS:
        DPRINT1("Set client process not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_QUERY_CLIENT_PROCESS:
        DPRINT1("Query client process not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_INTERNAL_READ:
        DPRINT1("Internal read not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_INTERNAL_WRITE:
        DPRINT1("Internal write not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_INTERNAL_TRANSCEIVE:
        DPRINT1("Internal transceive not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    case FSCTL_PIPE_INTERNAL_READ_OVFLOW:
        DPRINT1("Internal read overflow not implemented\n");
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        DPRINT1("Unrecognized IoControlCode: %x\n",
            IoStack->Parameters.FileSystemControl.FsControlCode);
        Status = STATUS_UNSUCCESSFUL;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}


NTSTATUS NTAPI
NpfsFlushBuffers(PDEVICE_OBJECT DeviceObject,
                 PIRP Irp)
{
    /* FIXME: Implement */
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
