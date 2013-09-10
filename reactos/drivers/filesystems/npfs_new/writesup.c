#include "npfs.h"

NTSTATUS 
NTAPI
NpWriteDataQueue(IN PNP_DATA_QUEUE WriteQueue,
                 IN ULONG Mode, 
                 IN PVOID OutBuffer, 
                 IN ULONG OutBufferSize, 
                 IN ULONG PipeType, 
                 OUT PULONG BytesWritten, 
                 IN PNP_CCB Ccb, 
                 IN BOOLEAN ServerSide, 
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

    *BytesWritten = OutBufferSize;

    MoreProcessing = 1;
    if ( PipeType != FILE_PIPE_OUTBOUND || (OutBufferSize) ) MoreProcessing = 0;

    for (DataEntry = NpGetNextRealDataQueueEntry(WriteQueue, List);
         ((WriteQueue->QueueState == ReadEntries) &&
          ((*BytesWritten > 0) || (MoreProcessing)));
         DataEntry = NpGetNextRealDataQueueEntry(WriteQueue, List))
    {
        DataSize = DataEntry->DataSize;

        IoStack = IoGetCurrentIrpStackLocation( DataEntry->Irp);

        if ( IoStack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL && 
             IoStack->Parameters.FileSystemControl.FsControlCode == FSCTL_PIPE_INTERNAL_WRITE &&
             (DataSize < OutBufferSize || MoreProcessing) )
        {
            WriteIrp = NpRemoveDataQueueEntry(WriteQueue, 1, List);
            if (WriteIrp )
            {
                WriteIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
            }
            continue;
        }

        if ( DataEntry->DataEntryType == Unbuffered )
        {
             DataEntry->Irp->Overlay.AllocationSize.QuadPart = 0;
        }
        
        BufferSize = *BytesWritten;
        if ( BufferSize >= DataSize ) BufferSize = DataSize;

        if ( DataEntry->DataEntryType != Unbuffered && BufferSize )
        {
            Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, 'RFpN');
            if ( !Buffer ) return STATUS_INSUFFICIENT_RESOURCES;
            AllocatedBuffer = 1;
        }
        else
        {
            Buffer = DataEntry->Irp->AssociatedIrp.SystemBuffer;
            AllocatedBuffer = 0;
        }

        _SEH2_TRY
        {
             RtlCopyMemory(Buffer,
                           (PVOID)((ULONG_PTR)OutBuffer + OutBufferSize - *BytesWritten),
                           BufferSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ASSERT(FALSE);
        }
        _SEH2_END;

        if ( !HaveContext )
        {
            HaveContext = 1;
            Status = NpGetClientSecurityContext(ServerSide, Ccb, Thread, &ClientContext);
            if (!NT_SUCCESS(Status))
            {
                if ( AllocatedBuffer ) ExFreePool(Buffer);
                return Status;
            }

            if ( ClientContext )
            {
                NpFreeClientSecurityContext(Ccb->ClientContext);
                Ccb->ClientContext = ClientContext;
            }
        }

        WriteIrp = NpRemoveDataQueueEntry(WriteQueue, 1, List);
        if ( WriteIrp )
        {
            *BytesWritten -= BufferSize;
            WriteIrp->IoStatus.Information = BufferSize;

            if ( AllocatedBuffer )
            {
                WriteIrp->AssociatedIrp.SystemBuffer = Buffer;
                WriteIrp->Flags |= IRP_DEALLOCATE_BUFFER  | IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
            }

            if ( !*BytesWritten )
            {
                MoreProcessing = 0;
                WriteIrp->IoStatus.Status = 0;
                InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
                continue;
            }

            if ( Mode == FILE_PIPE_MESSAGE_MODE )
            {
                WriteIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                WriteIrp->IoStatus.Status = 0;
            }

            InsertTailList(List, &WriteIrp->Tail.Overlay.ListEntry);
        }
        else if ( AllocatedBuffer )
        {
            ExFreePool(Buffer);
        }
    }

    if ( *BytesWritten > 0 || MoreProcessing )
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

