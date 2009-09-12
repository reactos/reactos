/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/dispatcher.c
 * PURPOSE:         portcls generic dispatcher
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.hpp"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;

    // let IrpTarget handle request
    return IrpTarget->DeviceIoControl(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->Read(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->Write(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->Flush(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->Close(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->QuerySecurity(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IIrpTarget * IrpTarget;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // access IrpTarget
    IrpTarget = (IIrpTarget *)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return IrpTarget->SetSecurity(DeviceObject, Irp);
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

    // access IrpTarget
    IrpTarget = (IIrpTarget *)FileObject->FsContext;

    // let IrpTarget handle request
    return IrpTarget->FastDeviceIoControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject);
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

    // access IrpTarget
    IrpTarget = (IIrpTarget *)FileObject->FsContext;

    // let IrpTarget handle request
    return IrpTarget->FastRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
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

    // access IrpTarget
    IrpTarget = (IIrpTarget *)FileObject->FsContext;
    // let IrpTarget handle request
    return IrpTarget->FastWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
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
    IN IIrpTarget * Target,
    IN ULONG CreateItemCount,
    IN PKSOBJECT_CREATE_ITEM CreateItem)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    IoStack->FileObject->FsContext = (PVOID)Target;

    Status = KsAllocateObjectHeader(&ObjectHeader, CreateItemCount, CreateItem, Irp, &DispatchTable);
    DPRINT("KsAllocateObjectHeader result %x\n", Status);
    return Status;
}

