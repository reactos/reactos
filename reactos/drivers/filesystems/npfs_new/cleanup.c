#include "npfs.h"

NTSTATUS
NTAPI
NpCommonCleanup(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NODE_TYPE_CODE NodeTypeCode;
    LIST_ENTRY List;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
    PLIST_ENTRY ThisEntry, NextEntry;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InitializeListHead(&List);

    ExAcquireResourceExclusiveLite(&NpVcb->Lock, TRUE);
    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (NodeTypeCode == NPFS_NTC_CCB)
    {
        if (NamedPipeEnd == FILE_PIPE_SERVER_END)
        {
            ASSERT(Ccb->Fcb->ServerOpenCount != 0);
            --Ccb->Fcb->ServerOpenCount;
        }

        NpSetClosingPipeState(Ccb, Irp, NamedPipeEnd, &List);
    }

    ExReleaseResourceLite(&NpVcb->Lock);

    NextEntry = List.Flink;
    while (NextEntry != &List)
    {
        ThisEntry = NextEntry;
        NextEntry = NextEntry->Flink;

        Irp = CONTAINING_RECORD(ThisEntry, IRP, Tail.Overlay.ListEntry);
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpFsdCleanup(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();

    Status = NpCommonCleanup(DeviceObject, Irp);

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

