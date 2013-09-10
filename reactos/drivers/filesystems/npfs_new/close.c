#include "npfs.h"

NTSTATUS
NTAPI
NpCommonClose(IN PDEVICE_OBJECT DeviceObject,
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
    if (NodeTypeCode == NPFS_NTC_ROOT_DCB)
    {
        --Fcb->CurrentInstances;
        NpDeleteCcb(Ccb, &List);
    }
    else if (NodeTypeCode == NPFS_NTC_VCB)
    {
        --NpVcb->ReferenceCount;
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

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NpFsdClose(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();

    Status = NpCommonClose(DeviceObject, Irp);

    FsRtlExitFileSystem();

    return Status;
}

