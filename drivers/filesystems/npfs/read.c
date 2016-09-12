/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/read.c
 * PURPOSE:     Pipes Reading
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_READ)

/* GLOBALS ********************************************************************/

LONG NpSlowReadCalls;
ULONG NpFastReadTrue;
ULONG NpFastReadFalse;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
NpCommonRead(IN PFILE_OBJECT FileObject,
             IN PVOID Buffer,
             IN ULONG BufferSize,
             OUT PIO_STATUS_BLOCK IoStatus,
             IN PIRP Irp,
             IN PLIST_ENTRY List)
{
    NODE_TYPE_CODE NodeType;
    PNP_DATA_QUEUE ReadQueue;
    PNP_EVENT_BUFFER EventBuffer;
    NTSTATUS Status;
    ULONG NamedPipeEnd;
    PNP_CCB Ccb;
    PNP_NONPAGED_CCB NonPagedCcb;
    BOOLEAN ReadOk;
    PAGED_CODE();

    IoStatus->Information = 0;
    NodeType = NpDecodeFileObject(FileObject, NULL, &Ccb, &NamedPipeEnd);

    if (!NodeType)
    {
        IoStatus->Status = STATUS_PIPE_DISCONNECTED;
        return TRUE;
    }

    if (NodeType != NPFS_NTC_CCB)
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        return TRUE;
    }

    NonPagedCcb = Ccb->NonPagedCcb;
    ExAcquireResourceExclusiveLite(&NonPagedCcb->Lock, TRUE);

    if (Ccb->NamedPipeState == FILE_PIPE_DISCONNECTED_STATE || Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE)
    {
        IoStatus->Status = Ccb->NamedPipeState != FILE_PIPE_DISCONNECTED_STATE ? STATUS_PIPE_LISTENING : STATUS_PIPE_DISCONNECTED;
        ReadOk = TRUE;
        goto Quickie;
    }

    ASSERT((Ccb->NamedPipeState == FILE_PIPE_CONNECTED_STATE) || (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE));

    if ((NamedPipeEnd == FILE_PIPE_SERVER_END && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_OUTBOUND) ||
        (NamedPipeEnd == FILE_PIPE_CLIENT_END && Ccb->Fcb->NamedPipeConfiguration == FILE_PIPE_INBOUND))
    {
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        ReadOk = TRUE;
        goto Quickie;
    }

    if (NamedPipeEnd == FILE_PIPE_SERVER_END)
    {
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_INBOUND];
    }
    else
    {
        ReadQueue = &Ccb->DataQueue[FILE_PIPE_OUTBOUND];
    }

    EventBuffer = NonPagedCcb->EventBuffer[NamedPipeEnd];

    if (ReadQueue->QueueState == WriteEntries)
    {
        *IoStatus = NpReadDataQueue(ReadQueue,
                                    FALSE,
                                    FALSE,
                                    Buffer,
                                    BufferSize,
                                    Ccb->ReadMode[NamedPipeEnd],
                                    Ccb,
                                    List);
        if (!NT_SUCCESS(IoStatus->Status))
        {
            ReadOk = TRUE;
            goto Quickie;
        }

        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (Ccb->NamedPipeState == FILE_PIPE_CLOSING_STATE)
    {
        IoStatus->Status = STATUS_PIPE_BROKEN;
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (Ccb->CompletionMode[NamedPipeEnd] == FILE_PIPE_COMPLETE_OPERATION)
    {
        IoStatus->Status = STATUS_PIPE_EMPTY;
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
        goto Quickie;
    }

    if (!Irp)
    {
        ReadOk = FALSE;
        goto Quickie;
    }

    Status = NpAddDataQueueEntry(NamedPipeEnd,
                                 Ccb,
                                 ReadQueue,
                                 ReadEntries,
                                 Buffered,
                                 BufferSize,
                                 Irp,
                                 NULL,
                                 0);
    IoStatus->Status = Status;
    if (!NT_SUCCESS(Status))
    {
        ReadOk = FALSE;
    }
    else
    {
        ReadOk = TRUE;
        if (EventBuffer) KeSetEvent(EventBuffer->Event, IO_NO_INCREMENT, FALSE);
    }

Quickie:
    ExReleaseResourceLite(&Ccb->NonPagedCcb->Lock);
    return ReadOk;
}

NTSTATUS
NTAPI
NpFsdRead(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK IoStatus;
    LIST_ENTRY DeferredList;
    PAGED_CODE();
    NpSlowReadCalls++;

    InitializeListHead(&DeferredList);
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    FsRtlEnterFileSystem();
    NpAcquireSharedVcb();

    NpCommonRead(IoStack->FileObject,
                 Irp->UserBuffer,
                 IoStack->Parameters.Read.Length,
                 &IoStatus,
                 Irp,
                 &DeferredList);

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    if (IoStatus.Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = IoStatus.Information;
        Irp->IoStatus.Status = IoStatus.Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return IoStatus.Status;
}


_Function_class_(FAST_IO_READ)
_IRQL_requires_same_
BOOLEAN
NTAPI
NpFastRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    LIST_ENTRY DeferredList;
    BOOLEAN Result;
    PAGED_CODE();

    InitializeListHead(&DeferredList);

    FsRtlEnterFileSystem();
    NpAcquireSharedVcb();

    Result = NpCommonRead(FileObject,
                          Buffer,
                          Length,
                          IoStatus,
                          NULL,
                          &DeferredList);
    if (Result)
        ++NpFastReadTrue;
    else
        ++NpFastReadFalse;

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);
    FsRtlExitFileSystem();

    return Result;
}

/* EOF */
