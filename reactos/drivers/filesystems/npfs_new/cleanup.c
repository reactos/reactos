#include "npfs.h"

NTSTATUS
NTAPI
NpCommonCleanup(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NODE_TYPE_CODE NodeTypeCode;
    LIST_ENTRY DeferredList;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    InitializeListHead(&DeferredList);

    NpAcquireExclusiveVcb();
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

        NpSetClosingPipeState(Ccb, Irp, NamedPipeEnd, &DeferredList);
    }

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);

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

