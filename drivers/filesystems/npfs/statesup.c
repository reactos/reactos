/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/statesup.c
 * PURPOSE:     Pipes State Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_STATESUP)

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
NpCancelListeningQueueIrp(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
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

    Ccb->ReadMode[FILE_PIPE_CLIENT_END] = FILE_PIPE_BYTE_STREAM_MODE;
    Ccb->CompletionMode[FILE_PIPE_CLIENT_END] = FILE_PIPE_QUEUE_OPERATION;
    Ccb->NamedPipeState = FILE_PIPE_CONNECTED_STATE;
    Ccb->FileObject[FILE_PIPE_CLIENT_END] = FileObject;

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

            EventBuffer = NonPagedCcb->EventBuffer[FILE_PIPE_CLIENT_END];

            while (Ccb->DataQueue[FILE_PIPE_INBOUND].QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->DataQueue[FILE_PIPE_INBOUND], FALSE, List);
                if (Irp)
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            while (Ccb->DataQueue[FILE_PIPE_OUTBOUND].QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->DataQueue[FILE_PIPE_OUTBOUND], FALSE, List);
                if (Irp)
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);

            // drop down on purpose... queue will be empty so flush code is nop
            ASSERT(Ccb->DataQueue[FILE_PIPE_OUTBOUND].QueueState == Empty);

        case FILE_PIPE_CLOSING_STATE:

            EventBuffer = NonPagedCcb->EventBuffer[FILE_PIPE_CLIENT_END];

            while (Ccb->DataQueue[FILE_PIPE_INBOUND].QueueState != Empty)
            {
                Irp = NpRemoveDataQueueEntry(&Ccb->DataQueue[FILE_PIPE_INBOUND], FALSE, List);
                if (Irp)
                {
                    Irp->IoStatus.Status = STATUS_PIPE_DISCONNECTED;
                    InsertTailList(List, &Irp->Tail.Overlay.ListEntry);
                }
            }

            ASSERT(Ccb->DataQueue[FILE_PIPE_OUTBOUND].QueueState == Empty);

            NpDeleteEventTableEntry(&NpVcb->EventTable, EventBuffer);
            NonPagedCcb->EventBuffer[FILE_PIPE_CLIENT_END] = NULL;

            NpSetFileObject(Ccb->FileObject[FILE_PIPE_CLIENT_END], NULL, NULL, FALSE);
            Ccb->FileObject[FILE_PIPE_CLIENT_END] = NULL;

            NpUninitializeSecurity(Ccb);

            if (Ccb->ClientSession)
            {
                ExFreePool(Ccb->ClientSession);
                Ccb->ClientSession = NULL;
            }

            Status = STATUS_SUCCESS;
            break;

        default:
            NpBugCheck(Ccb->NamedPipeState, 0, 0);
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

            if (Ccb->CompletionMode[FILE_PIPE_SERVER_END] == FILE_PIPE_COMPLETE_OPERATION)
            {
                Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
                return STATUS_PIPE_LISTENING;
            }

            IoSetCancelRoutine(Irp, NpCancelListeningQueueIrp);
            if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
            {
                return STATUS_CANCELLED;
            }

            Ccb->NamedPipeState = FILE_PIPE_LISTENING_STATE;
            IoMarkIrpPending(Irp);
            InsertTailList(&Ccb->IrpList, &Irp->Tail.Overlay.ListEntry);
            return STATUS_PENDING;

        case FILE_PIPE_CONNECTED_STATE:
            Status = STATUS_PIPE_CONNECTED;
            break;

        case FILE_PIPE_CLOSING_STATE:
            Status = STATUS_PIPE_CLOSING;
            break;

        default:
            NpBugCheck(Ccb->NamedPipeState, 0, 0);
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
    PIRP ListIrp;

    NonPagedCcb = Ccb->NonPagedCcb;
    Fcb = Ccb->Fcb;

    switch (Ccb->NamedPipeState)
    {
        case FILE_PIPE_LISTENING_STATE:

            ASSERT(NamedPipeEnd == FILE_PIPE_SERVER_END);

            while (!IsListEmpty(&Ccb->IrpList))
            {
                NextEntry = RemoveHeadList(&Ccb->IrpList);

                ListIrp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

                if (IoSetCancelRoutine(ListIrp, NULL))
                {
                    ListIrp->IoStatus.Status = STATUS_PIPE_BROKEN;
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

            NpSetFileObject(Ccb->FileObject[FILE_PIPE_SERVER_END], NULL, NULL, TRUE);
            Ccb->FileObject[FILE_PIPE_SERVER_END] = NULL;

            NpSetFileObject(Ccb->FileObject[FILE_PIPE_CLIENT_END], NULL, NULL, FALSE);
            Ccb->FileObject[FILE_PIPE_CLIENT_END] = NULL;

            NpDeleteCcb(Ccb, List);
            if (!Fcb->CurrentInstances) NpDeleteFcb(Fcb, List);
            break;

        case FILE_PIPE_CLOSING_STATE:

            if (NamedPipeEnd == FILE_PIPE_SERVER_END)
            {
                DataQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
            }
            else
            {
                DataQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
            }

            NpSetFileObject(Ccb->FileObject[FILE_PIPE_SERVER_END], NULL, NULL, TRUE);
            Ccb->FileObject[FILE_PIPE_SERVER_END] = NULL;

            NpSetFileObject(Ccb->FileObject[FILE_PIPE_CLIENT_END], NULL, NULL, FALSE);
            Ccb->FileObject[FILE_PIPE_CLIENT_END] = NULL;

            while (DataQueue->QueueState != Empty)
            {
                ListIrp = NpRemoveDataQueueEntry(DataQueue, FALSE, List);
                if (ListIrp)
                {
                    ListIrp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &ListIrp->Tail.Overlay.ListEntry);
                }
            }

            NpUninitializeSecurity(Ccb);

            if (Ccb->ClientSession)
            {
                ExFreePool(Ccb->ClientSession);
                Ccb->ClientSession = NULL;
            }

            NpDeleteCcb(Ccb, List);
            if (!Fcb->CurrentInstances) NpDeleteFcb(Fcb, List);
            break;

        case FILE_PIPE_CONNECTED_STATE:

            if (NamedPipeEnd == FILE_PIPE_SERVER_END)
            {
                ReadQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
                WriteQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];

                NpSetFileObject(Ccb->FileObject[FILE_PIPE_SERVER_END], NULL, NULL, TRUE);
                Ccb->FileObject[FILE_PIPE_SERVER_END] = NULL;
            }
            else
            {
                ReadQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
                WriteQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];

                NpSetFileObject(Ccb->FileObject[FILE_PIPE_CLIENT_END], NULL, NULL, FALSE);
                Ccb->FileObject[FILE_PIPE_CLIENT_END] = NULL;
            }

            EventBuffer = NonPagedCcb->EventBuffer[NamedPipeEnd];

            Ccb->NamedPipeState = FILE_PIPE_CLOSING_STATE;

            while (ReadQueue->QueueState != Empty)
            {
                ListIrp = NpRemoveDataQueueEntry(ReadQueue, FALSE, List);
                if (ListIrp)
                {
                    ListIrp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &ListIrp->Tail.Overlay.ListEntry);
                }
            }

            while (WriteQueue->QueueState == ReadEntries)
            {
                ListIrp = NpRemoveDataQueueEntry(WriteQueue, FALSE, List);
                if (ListIrp)
                {
                    ListIrp->IoStatus.Status = STATUS_PIPE_BROKEN;
                    InsertTailList(List, &ListIrp->Tail.Overlay.ListEntry);
                }
            }

            if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
            break;

        default:
            NpBugCheck(Ccb->NamedPipeState, 0, 0);
            break;
    }
    return STATUS_SUCCESS;
}

/* EOF */
