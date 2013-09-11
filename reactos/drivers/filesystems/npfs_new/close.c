#include "npfs.h"

NTSTATUS
NTAPI
NpCommonClose(IN PDEVICE_OBJECT DeviceObject,
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
    if (NodeTypeCode == NPFS_NTC_ROOT_DCB)
    {
        --Fcb->CurrentInstances;
        NpDeleteCcb(Ccb, &DeferredList);
    }
    else if (NodeTypeCode == NPFS_NTC_VCB)
    {
        --NpVcb->ReferenceCount;
    }

    NpReleaseVcb();
    NpCompleteDeferredIrps(&DeferredList);

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

