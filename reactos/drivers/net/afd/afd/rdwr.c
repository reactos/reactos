/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/rdwr.c
 * PURPOSE:     File object read/write functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <afd.h>

NTSTATUS AfdReadFile(
    PDEVICE_EXTENSION DeviceExt,
    PFILE_OBJECT FileObject,
	PVOID Buffer,
    ULONG Length,
    ULONG Offset)
/*
 * FUNCTION: Reads data from a file
 */
{
    UNIMPLEMENTED

    return STATUS_UNSUCCESSFUL;
}


NTSTATUS AfdRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
#if 1
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    return STATUS_UNSUCCESSFUL;
#else
    PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IoSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IoSp->FileObject;
    NTSTATUS Status;
    ULONG Length;
    PVOID Buffer;
    ULONG Offset;

    Length = IoSp->Parameters.Read.Length;
    Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    Offset = IoSp->Parameters.Read.ByteOffset.u.LowPart;
   
    Status = AfdReadFile(DeviceExt, FileObject, Buffer, Length, Offset);
   
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
#endif
}


NTSTATUS AfdWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IoSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IoSp->FileObject;
    NTSTATUS Status;
    ULONG Length;
    PVOID Buffer;
    ULONG Offset;
    PAFDFCB FCB;
    PAFDCCB CCB;

    FCB = FileObject->FsContext;
    CCB = FileObject->FsContext2;

    Length = IoSp->Parameters.Write.Length;
    Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    Offset = IoSp->Parameters.Write.ByteOffset.u.LowPart;

    AFD_DbgPrint(MIN_TRACE, ("Called. Length (%d)  Buffer (0x%X)  Offset (0x%X)\n",
        Length, Buffer, Offset));

    /* FIXME: Connectionless communication only */
    //Status = TdiSendDatagram(FCB->TdiAddressObject, WH2N(2000), 0x7F000001, Buffer, Length);
    //if (!NT_SUCCESS(Status))
        Length = 0;

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    AFD_DbgPrint(MIN_TRACE, ("Leaving.\n"));

    return Status;
}

/* EOF */
