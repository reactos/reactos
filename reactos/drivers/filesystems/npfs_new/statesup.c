#include "npfs.h"

VOID
NTAPI
NpCancelListeningQueueIrp(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    FsRtlEnterFileSystem();
    ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    FsRtlExitFileSystem();
    ExReleaseResourceLite(&NpVcb->Lock);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IofCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
}

NTSTATUS
NTAPI
NpSetConnectedPipeState(IN PNP_CCB Ccb,
                        IN PFILE_OBJECT FileObject,
                        IN PLIST_ENTRY List)
{
    PLIST_ENTRY NextEntry;
    PIRP Irp;

    ASSERT(Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE);

    Ccb->ClientReadMode = FILE_PIPE_BYTE_STREAM_MODE;
    Ccb->ClientCompletionMode = FILE_PIPE_QUEUE_OPERATION;
    Ccb->NamedPipeState = FILE_PIPE_CONNECTED_STATE;
    Ccb->ClientFileObject = FileObject;

    NpSetFileObject(FileObject, Ccb, Ccb->NonPagedCcb, FALSE);

    while (!IsListEmpty(&Ccb->IrpList))
    {
        NextEntry = RemoveHeadList(&Ccb->IrpList);

        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

        if (IoSetCancelRoutine(Irp, NULL))
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            InsertTailList(List, NextEntry);
        }
        else
        {
            InitializeListHead(NextEntry);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpSetDisconnectedPipeState(IN PNP_CCB Ccb, 
                           IN PLIST_ENTRY List)
{
    PIRP Irp;
    PNP_NONPAGED_CCB NonPagedCcb;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PNP_EVENT_BUFFER EventBuffer;

    NonPagedCcb = Ccb->NonPagedCcb;

    switch (Ccb->NamedPipeState)
    {
        case FILE_PIPE_DISCONNECTED_STATE:
            Status = STATUS_PIPE_DISCONNECTED;
            break;

        case FILE_PIPE_LISTENING_STATE:
            while (!IsListEmpty(&Ccb->IrpList))
            {
                NextEntry = RemoveHeadList(&Ccb->IrpList);

                Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

                if (IoSetCancelRoutine(Irp, NULL))
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, NextEntry);
                }
                else
                {
                    InitializeListHead(NextEntry);
                }
            }

            Status = STATUS_SUCCESS;
            break;
 
        case FILE_PIPE_CONNECTED_STATE:

            EventBuffer = NonPagedCcb->EventBufferClient;
            while (Ccb->InQueue.QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->InQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            while ( Ccb->OutQueue.QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->OutQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            if ( EventBuffer ) KeSetEvent(EventBuffer->Event, 0, 0);

            Status = STATUS_SUCCESS;
            break;

        case FILE_PIPE_CLOSING_STATE:

            EventBuffer = NonPagedCcb->EventBufferClient;
            while (Ccb->InQueue.QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->InQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            ASSERT(Ccb->OutQueue.QueueState == Empty);

            NpDeleteEventTableEntry(&NpVcb->EventTable, EventBuffer);
            NonPagedCcb->EventBufferClient = NULL;

            NpSetFileObject(Ccb->ClientFileObject, NULL, NULL, FALSE);
            Ccb->ClientFileObject = NULL;

            NpUninitializeSecurity(Ccb);

            if ( Ccb->ClientSession )
            {
                ExFreePool(Ccb->ClientSession);
                Ccb->ClientSession = NULL;
            }

            Status = STATUS_SUCCESS;
            break;

        default:
            KeBugCheckEx(NPFS_FILE_SYSTEM, 0x1603DD, Ccb->NamedPipeState, 0, 0);
            break;
    }

    Ccb->NamedPipeState = FILE_PIPE_DISCONNECTED_STATE;
    return Status;
}

NTSTATUS
NTAPI
NpSetListeningPipeState(IN PNP_CCB Ccb,
                        IN PIRP Irp, 
                        IN PLIST_ENTRY List)
{
    NTSTATUS Status;

    switch (Ccb->NamedPipeState)
    {
        case FILE_PIPE_DISCONNECTED_STATE:

        Status = NpCancelWaiter(&NpVcb->WaitQueue,
                                &Ccb->Fcb->FullName,
                                STATUS_SUCCESS,
                                List);
        if (!NT_SUCCESS(Status)) return Status;

        //
        // Drop down on purpose
        //

        case FILE_PIPE_LISTENING_STATE:

            if ( Ccb->ServerCompletionMode == FILE_PIPE_COMPLETE_OPERATION)
            {
                Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
                return STATUS_PIPE_LISTENING;
            }

            IoSetCancelRoutine(Irp, NpCancelListeningQueueIrp);
            if ( Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
            {
                return STATUS_CANCELLED;
            }

            Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
            IoMarkIrpPending(Irp);
            InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
            return STATUS_PENDING;

        case FILE_PIPE_CONNECTED_STATE:
            Status = STATUS_PIPE_CONNECTED;
            break;

        case FILE_PIPE_CLOSING_STATE:
            Status = STATUS_PIPE_CLOSING;
            break;

        default:
            KeBugCheckEx(NPFS_FILE_SYSTEM, 0x160133, Ccb->NamedPipeState, 0, 0);
            break;
    }

    return Status;
}

NTSTATUS
NTAPI
NpSetClosingPipeState(IN PNP_CCB Ccb,
                      IN PIRP Irp, 
                      IN ULONG NamedPipeEnd, 
                      IN PLIST_ENTRY List)
{
    PNP_NONPAGED_CCB NonPagedCcb;
    PNP_FCB Fcb;
    PLIST_ENTRY NextEntry;
    PNP_DATA_QUEUE ReadQueue, WriteQueue, DataQueue;
    PNP_EVENT_BUFFER EventBuffer;

    NonPagedCcb = Ccb->NonPagedCcb;
    Fcb = Ccb->Fcb;

    switch (Ccb->NamedPipeState)
    {
        case FILE_PIPE_LISTENING_STATE:

            ASSERT(NamedPipeEnd == FILE_PIPE_SERVER_END);

            while (!IsListEmpty(&Ccb->IrpList))
            {
                NextEntry = RemoveHeadList(&Ccb->IrpList);

                Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

                if (IoSetCancelRoutine(Irp, NULL))
                {
                    Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, NextEntry);
                }
                else
                {
                    InitializeListHead(NextEntry);
                }
            }

            // Drop on purpose

        case FILE_PIPE_DISCONNECTED_STATE:

            ASSERT(NamedPipeEnd == FILE_PIPE_SERVER_END);

            NpSetFileObject(Ccb->ServerFileObject, NULL, NULL, TRUE);
            Ccb->ServerFileObject = NULL;

            NpSetFileObject(Ccb->ClientFileObject, NULL, NULL, FALSE);
            Ccb->ClientFileObject = NULL;

            NpDeleteCcb(Ccb, List);
            if ( !Fcb->CurrentInstances ) NpDeleteFcb(Fcb, List);
            break;

        case FILE_PIPE_CLOSING_STATE:

            if ( NamedPipeEnd == FILE_PIPE_SERVER_END)
            {
                DataQueue = &Ccb->InQueue;
            }
            else
            {
                DataQueue = &Ccb->OutQueue;
            }

            NpSetFileObject(Ccb->ServerFileObject, NULL, NULL, TRUE);
            Ccb->ServerFileObject = NULL;

            NpSetFileObject(Ccb->ClientFileObject, NULL, NULL, FALSE);
            Ccb->ClientFileObject = NULL;

            while (Ccb->InQueue.QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->InQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            NpUninitializeSecurity(Ccb);

            if ( Ccb->ClientSession )
            {
                ExFreePool(Ccb->ClientSession);
                Ccb->ClientSession = 0;
            }

            NpDeleteCcb(Ccb, List);
            if ( !Fcb->CurrentInstances ) NpDeleteFcb(Fcb, List);
            break;

        case FILE_PIPE_CONNECTED_STATE:
            if ( NamedPipeEnd == FILE_PIPE_SERVER_END)
            {
                ReadQueue = &Ccb->InQueue;
                WriteQueue = &Ccb->OutQueue;
                EventBuffer = NonPagedCcb->EventBufferServer;

                NpSetFileObject(Ccb->ServerFileObject, NULL, NULL, TRUE);
                Ccb->ServerFileObject = 0;
            }
            else
            {
                ReadQueue = &Ccb->OutQueue;
                WriteQueue = &Ccb->InQueue;
                EventBuffer = NonPagedCcb->EventBufferClient;

                NpSetFileObject(Ccb->ClientFileObject, NULL, NULL, FALSE);
                Ccb->ClientFileObject = 0;
            }

            Ccb->NamedPipeState = FILE_PIPE_CLOSING_STATE;

            while (Ccb->InQueue.QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->InQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            while (WriteQueue->QueueState == WriteEntries)
            {
                Irp = NpRemoveDataQueueEntry(WriteQueue, FALSE, List);
                if ( Irp )
                {
                    Irp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            if ( EventBuffer ) KeSetEvent(EventBuffer->Event, 0, 0);
            break;

        default:
            KeBugCheckEx(0x25u, 0x1602F9, Ccb->NamedPipeState, 0, 0);
            break;
    }
    return STATUS_SUCCESS;
}

