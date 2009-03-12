#include "private.h"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    //DPRINT1("Dispatch_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    return IrpTarget->lpVtbl->DeviceIoControl(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnRead called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    return IrpTarget->lpVtbl->Read(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnWrite called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    return IrpTarget->lpVtbl->Write(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnFlush called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    return IrpTarget->lpVtbl->Flush(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnClose called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    ASSERT(CreateItem != NULL);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack != NULL);
    ASSERT(IoStack->FileObject != NULL);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    //DPRINT1("IrpTarget %p\n", IrpTarget);

    return IrpTarget->lpVtbl->Close(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

    return IrpTarget->lpVtbl->QuerySecurity(IrpTarget, DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT1("Dispatch_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    IrpTarget = (IIrpTarget*)CreateItem->Context;

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
    DPRINT1("Dispatch_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


    return FALSE;
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
    DPRINT1("Dispatch_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

    return FALSE;

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
    //DPRINT1("Dispatch_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

    IrpTarget = (IIrpTarget *)FileObject->FsContext2;

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

