/*
* COPYRIGHT:  See COPYING in the top level directory
* PROJECT:    ReactOS kernel
* FILE:       drivers/fs/np/fsctrl.c
* PURPOSE:    Named pipe filesystem
* PROGRAMMER: David Welch <welch@cwcom.net>
*             Eric Kohl
*             Michael Martin
*/

/* INCLUDES ******************************************************************/

#include "npfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static DRIVER_CANCEL NpfsListeningCancelRoutine;
static VOID NTAPI
NpfsListeningCancelRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PNPFS_WAITER_ENTRY Waiter;

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

    IoMarkIrpPending(Irp);
    InsertTailList(&Ccb->Fcb->WaiterListHead, &Entry->Entry);

    IoAcquireCancelSpinLock(&oldIrql);
    if (!Irp->Cancel)
    {
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

    DPRINT("NpfsConnectPipe()\n");

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

    Ccb->PipeState = FILE_PIPE_LISTENING_STATE;

    Status = NpfsAddListeningServerInstance(Irp, Ccb);

    KeUnlockMutex(&Fcb->CcbListLock);

    if (Flags & FO_SYNCHRONOUS_IO)
    {
        KeWaitForSingleObject(&Ccb->ConnectEvent,
            UserRequest,
            Irp->RequestorMode,
            FALSE,
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
                Complete = (NULL == IoSetCancelRoutine(Irp, NULL));
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
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;

    DPRINT("NpfsWaitPipe\n");

    WaitPipe = (PFILE_PIPE_WAIT_FOR_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    Fcb = Ccb->Fcb;

    if (Ccb->PipeState != 0)
    {
        DPRINT("Pipe is not in passive (waiting) state!\n");
        return STATUS_UNSUCCESSFUL;
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

    /* Wait for one */
    Status = KeWaitForSingleObject(&Ccb->ConnectEvent,
        UserRequest,
        KernelMode,
        FALSE,
        &TimeOut);

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
    PNPFS_FCB Fcb;
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
    if (OutputBufferLength < sizeof(FILE_PIPE_PEEK_BUFFER))
    {
        DPRINT1("Buffer too small\n");
        return STATUS_INVALID_PARAMETER;
    }

    Ccb = IoStack->FileObject->FsContext2;
    Reply = (PFILE_PIPE_PEEK_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    Fcb = Ccb->Fcb;


    Reply->NamedPipeState = Ccb->PipeState;

    Reply->ReadDataAvailable = Ccb->ReadDataAvailable;
    DPRINT("ReadDataAvailable: %lu\n", Ccb->ReadDataAvailable);

    ExAcquireFastMutex(&Ccb->DataListLock);
    BufferPtr = Ccb->ReadPtr;
    DPRINT("BufferPtr = %x\n", BufferPtr);
    if (Ccb->Fcb->PipeType == FILE_PIPE_BYTE_STREAM_TYPE)
    {
        DPRINT("Byte Stream Mode\n");
        Reply->MessageLength = Ccb->ReadDataAvailable;
        DPRINT("Reply->MessageLength  %lu\n",Reply->MessageLength );
        MessageCount = 1;

        if (Reply->Data[0] && (OutputBufferLength >= Ccb->ReadDataAvailable + FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0])))
        {
            ReturnLength = Ccb->ReadDataAvailable;
            memcpy(&Reply->Data[0], (PVOID)BufferPtr, Ccb->ReadDataAvailable);
        }
    }
    else
    {
        DPRINT("Message Mode\n");
        ReadDataAvailable=Ccb->ReadDataAvailable;

        if (ReadDataAvailable > 0)
        {
            memcpy(&Reply->MessageLength, BufferPtr, sizeof(ULONG));

            while ((ReadDataAvailable > 0) && (BufferPtr < Ccb->WritePtr))
            {
                memcpy(&MessageLength, BufferPtr, sizeof(MessageLength));

                ASSERT(MessageLength > 0);

                DPRINT("MessageLength = %lu\n",MessageLength);
                ReadDataAvailable -= MessageLength;
                MessageCount++;

                /* If its the first message, copy the Message if the size of buffer is large enough */
                if (MessageCount==1)
                {
                    if ((Reply->Data[0])
                        && (OutputBufferLength >= (MessageLength + FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]))))
                    {
                        memcpy(&Reply->Data[0], (PVOID)((ULONG_PTR)BufferPtr + sizeof(MessageLength)), MessageLength);
                        ReturnLength = MessageLength;
                    }
                }

                BufferPtr =(PVOID)((ULONG_PTR)BufferPtr + MessageLength + sizeof(MessageLength));
                DPRINT("BufferPtr = %x\n", BufferPtr);
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

    Irp->IoStatus.Information = ReturnLength + FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]);
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
    PNPFS_DEVICE_EXTENSION DeviceExt;
    PNPFS_FCB Fcb;
    PNPFS_CCB Ccb;

    DPRINT("NpfsFileSystemContol(DeviceObject %p Irp %p)\n", DeviceObject, Irp);

    DeviceExt = (PNPFS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
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

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* EOF */
