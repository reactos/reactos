#include "npfs.h"

NTSTATUS
NTAPI
NpCommonQuerySecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp)
{
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (!NodeTypeCode) return STATUS_PIPE_DISCONNECTED;
    if (NodeTypeCode != NPFS_NTC_CCB) return STATUS_INVALID_PARAMETER;

    Status = SeQuerySecurityDescriptorInfo(&IoStack->Parameters.QuerySecurity.SecurityInformation,
                                           Irp->UserBuffer,
                                           &IoStack->Parameters.QuerySecurity.Length,
                                           &Fcb->SecurityDescriptor);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        Irp->IoStatus.Information = IoStack->Parameters.QuerySecurity.Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    return Status;
}

NTSTATUS
NTAPI
NpCommonSetSecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    NODE_TYPE_CODE NodeTypeCode;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PNP_FCB Fcb;
    PNP_CCB Ccb;
    ULONG NamedPipeEnd;
    PVOID CachedSecurityDescriptor, SecurityDescriptor;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (!NodeTypeCode) return STATUS_PIPE_DISCONNECTED;
    if (NodeTypeCode != NPFS_NTC_CCB) return STATUS_INVALID_PARAMETER;

    Status = SeSetSecurityDescriptorInfo(NULL,
                                         &IoStack->Parameters.SetSecurity.SecurityInformation,
                                         IoStack->Parameters.SetSecurity.SecurityDescriptor,
                                         &SecurityDescriptor,
                                         TRUE,
                                         IoGetFileObjectGenericMapping());
    if (!NT_SUCCESS(Status)) return Status;

    Status = ObLogSecurityDescriptor(SecurityDescriptor, &CachedSecurityDescriptor, TRUE);
    ExFreePool(SecurityDescriptor);

    if (!NT_SUCCESS(Status)) return Status;

    Fcb->SecurityDescriptor = CachedSecurityDescriptor;
    ObDereferenceSecurityDescriptor(SecurityDescriptor, 1);
    return Status;
}

NTSTATUS
NTAPI
NpFsdQuerySecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    Status = NpCommonQuerySecurityInfo(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
NpFsdSetSecurityInfo(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PAGED_CODE();

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    Status = NpCommonQuerySecurityInfo(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

