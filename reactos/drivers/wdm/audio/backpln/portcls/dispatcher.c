#include "private.h"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->DeviceIoControl(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->Read(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->Write(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->Flush(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->Close(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->QuerySecurity(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    /* get the IrpTarget */
    IrpTarget = (IIrpTarget*)CreateItem->Context;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->SetSecurity(IrpTarget, DeviceObject, Irp);
}

BOOLEAN
NTAPI
Dispatch_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    IIrpTarget * IrpTarget;

    /* access IrpTarget */
    IrpTarget = (IIrpTarget *)FileObject->FsContext2;

    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->FastDeviceIoControl(IrpTarget, FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject);
}


BOOLEAN
NTAPI
Dispatch_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    IIrpTarget * IrpTarget;

    /* access IrpTarget */
    IrpTarget = (IIrpTarget *)FileObject->FsContext2;

    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->FastRead(IrpTarget, FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
}

BOOLEAN
NTAPI
Dispatch_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    IIrpTarget * IrpTarget;

    /* access IrpTarget */
    IrpTarget = (IIrpTarget *)FileObject->FsContext2;
    /* let IrpTarget handle request */
    return IrpTarget->lpVtbl->FastWrite(IrpTarget, FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
}

static KSDISPATCH_TABLE DispatchTable =
{
    Dispatch_fnDeviceIoControl,
    Dispatch_fnRead,
    Dispatch_fnWrite,
    Dispatch_fnFlush,
    Dispatch_fnClose,
    Dispatch_fnQuerySecurity,
    Dispatch_fnSetSecurity,
    Dispatch_fnFastDeviceIoControl,
    Dispatch_fnFastRead,
    Dispatch_fnFastWrite,
};


NTSTATUS
NTAPI
NewDispatchObject(
    IN PIRP Irp,
    IN IIrpTarget * Target)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;

    CreateItem = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM), TAG_PORTCLASS);
    if (!CreateItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    CreateItem->Context = (PVOID)Target;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IoStack->FileObject->FsContext2 = (PVOID)Target;

    Status = KsAllocateObjectHeader(&ObjectHeader, 1, CreateItem, Irp, &DispatchTable);
    DPRINT1("KsAllocateObjectHeader result %x\n", Status);
    return Status;
}

