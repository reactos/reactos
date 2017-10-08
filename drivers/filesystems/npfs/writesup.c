/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/writesup.c
 * PURPOSE:     Pipes Writing Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_WRITESUP)

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
NpWriteDataQueue(IN PNP_DATA_QUEUE WriteQueue,
                 IN ULONG Mode,
                 IN PVOID OutBuffer,
                 IN ULONG OutBufferSize,
                 IN ULONG PipeType,
                 OUT PULONG BytesNotWritten,
                 IN PNP_CCB Ccb,
                 IN ULONG NamedPipeEnd,
                 IN PETHREAD Thread,
                 IN PLIST_ENTRY List)
{
    BOOLEAN HaveContext = FALSE, MoreProcessing, AllocatedBuffer;
    PNP_DATA_QUEUE_ENTRY DataEntry;
    ULONG DataSize, BufferSize;
    PIRP WriteIrp;
    PIO_STACK_LOCATION IoStack;
    PVOID Buffer;
    NTSTATUS Status;
    PSECURITY_CLIENT_CONTEXT ClientContext;
    PAGED_CODE();

    *BytesNotWritten = OutBufferSize;

    MoreProcessing = TRUE;
    if ((PipeType != FILE_PIPE_MESSAGE_MODE) || (OutBufferSize))
    {
        MoreProcessing = FALSE;
    }

    for (DataEntry = CONTAINING_RECORD(NpGetNextRealDataQueueEntry(WriteQueue, List),
                                       NP_DATA_QUEUE_ENTRY,
                                       QueueEntry);
         ((WriteQueue->QueueState == ReadEntries) &&
         ((*BytesNotWritten > 0) || (MoreProcessing)));
         DataEntry = CONTAINING_RECORD(NpGetNextRealDataQueueEntry(WriteQueue, List),
                                       NP_DATA_QUEUE_ENTRY,
                                       QueueEntry))
    {
        DataSize = DataEntry->DataSize;

        IoStack = IoGetCurrentIrpStackLocation(DataEntry->Irp);

        if (IoStack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
            IoStack->Parameters.FileSystemControl.FsControlCode == FSCTL_PIPE_INTERNAL_READ_OVFLOW &&
            (DataSize < OutBufferSize || MoreProcessing))
        {
            WriteIrp = NpRemoveDataQueueEntry(WriteQueue, TRUE, List);
            if (WriteIrp)
            {
                WriteIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
            }
            continue;
        }

        if (DataEntry->DataEntryType == Unbuffered)
        {
            DataEntry->Irp->Overlay.AllocationSize.QuadPart = 0;
        }

        BufferSize = *BytesNotWritten;
        if (BufferSize >= DataSize) BufferSize = DataSize;

        if (DataEntry->DataEntryType != Unbuffered && BufferSize)
        {
            Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, NPFS_DATA_ENTRY_TAG);
            if (!Buffer) return STATUS_INSUFFICIENT_RESOURCES;
            AllocatedBuffer = TRUE;
        }
        else
        {
            Buffer = DataEntry->Irp->AssociatedIrp.SystemBuffer;
            AllocatedBuffer = FALSE;
        }

        _SEH2_TRY
        {
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)OutBuffer + OutBufferSize - *BytesNotWritten),
                          BufferSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            if (AllocatedBuffer) ExFreePool(Buffer);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        if (!HaveContext)
        {
            HaveContext = TRUE;
            Status = NpGetClientSecurityContext(NamedPipeEnd, Ccb, Thread, &ClientContext);
            if (!NT_SUCCESS(Status))
            {
                if (AllocatedBuffer) ExFreePool(Buffer);
                return Status;
            }

            if (ClientContext)
            {
                NpFreeClientSecurityContext(Ccb->ClientContext);
                Ccb->ClientContext = ClientContext;
            }
        }

        WriteIrp = NpRemoveDataQueueEntry(WriteQueue, TRUE, List);
        if (WriteIrp)
        {
            *BytesNotWritten -= BufferSize;
            WriteIrp->IoStatus.Information = BufferSize;

            if (AllocatedBuffer)
            {
                WriteIrp->AssociatedIrp.SystemBuffer = Buffer;
                WriteIrp->Flags |= IRP_DEALLOCATE_BUFFER  | IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
            }

            if (!*BytesNotWritten)
            {
                MoreProcessing = FALSE;
                WriteIrp->IoStatus.Status = STATUS_SUCCESS;
                InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
                continue;
            }

            if (Mode == FILE_PIPE_MESSAGE_MODE)
            {
                WriteIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                WriteIrp->IoStatus.Status = STATUS_SUCCESS;
            }

            InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
        }
        else if (AllocatedBuffer)
        {
            ExFreePool(Buffer);
        }
    }

    if (*BytesNotWritten > 0 || MoreProcessing)
    {
        ASSERT(WriteQueue->QueueState != ReadEntries);
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/* EOF */
