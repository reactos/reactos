/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/seinfo.c
 * PURPOSE:     Pipes Security Information
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_SEINFO)

/* FUNCTIONS ******************************************************************/

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
    PSECURITY_DESCRIPTOR OldSecurityDescriptor;
    PSECURITY_DESCRIPTOR TempSecurityDescriptor;
    PSECURITY_DESCRIPTOR NewSecurityDescriptor;
    PAGED_CODE();

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    NodeTypeCode = NpDecodeFileObject(IoStack->FileObject,
                                      (PVOID*)&Fcb,
                                      &Ccb,
                                      &NamedPipeEnd);
    if (!NodeTypeCode) return STATUS_PIPE_DISCONNECTED;
    if (NodeTypeCode != NPFS_NTC_CCB) return STATUS_INVALID_PARAMETER;

    OldSecurityDescriptor = TempSecurityDescriptor = Fcb->SecurityDescriptor;
    Status = SeSetSecurityDescriptorInfo(NULL,
                                         &IoStack->Parameters.SetSecurity.SecurityInformation,
                                         IoStack->Parameters.SetSecurity.SecurityDescriptor,
                                         &TempSecurityDescriptor,
                                         PagedPool,
                                         IoGetFileObjectGenericMapping());
    if (!NT_SUCCESS(Status)) return Status;

    Status = ObLogSecurityDescriptor(TempSecurityDescriptor, &NewSecurityDescriptor, 1);
    ASSERT(TempSecurityDescriptor != OldSecurityDescriptor);
    ExFreePoolWithTag(TempSecurityDescriptor, 0);

    if (!NT_SUCCESS(Status)) return Status;

    Fcb->SecurityDescriptor = NewSecurityDescriptor;
    ObDereferenceSecurityDescriptor(OldSecurityDescriptor, 1);
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

    Status = NpCommonSetSecurityInfo(DeviceObject, Irp);

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NAMED_PIPE_INCREMENT);
    }

    return Status;
}

/* EOF */
