/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/dispatcher.c
 * PURPOSE:         portcls generic dispatcher
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    // let IrpTarget handle request
    return DispatchContext->Target->DeviceIoControl(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return DispatchContext->Target->Read(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return DispatchContext->Target->Write(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return DispatchContext->Target->Flush(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;
    NTSTATUS Status;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    // let IrpTarget handle request
    Status = DispatchContext->Target->Close(DeviceObject, Irp);

    if (NT_SUCCESS(Status))
    {
       KsFreeObjectHeader(DispatchContext->ObjectHeader);
       FreeItem(DispatchContext, TAG_PORTCLASS);
    }
    // done
    return Status;
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;


    // let IrpTarget handle request
    return DispatchContext->Target->QuerySecurity(DeviceObject, Irp);
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)IoStack->FileObject->FsContext;

    // let IrpTarget handle request
    return DispatchContext->Target->SetSecurity(DeviceObject, Irp);
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
    PDISPATCH_CONTEXT DispatchContext;

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)FileObject->FsContext;

    // let IrpTarget handle request
    return DispatchContext->Target->FastDeviceIoControl(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, IoStatus, DeviceObject);
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
    PDISPATCH_CONTEXT DispatchContext;

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)FileObject->FsContext;

    // let IrpTarget handle request
    return DispatchContext->Target->FastRead(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
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
    PDISPATCH_CONTEXT DispatchContext;

    // get dispatch context
    DispatchContext = (PDISPATCH_CONTEXT)FileObject->FsContext;
    // let IrpTarget handle request
    return DispatchContext->Target->FastWrite(FileObject, FileOffset, Length, Wait, LockKey, Buffer, IoStatus, DeviceObject);
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
    PDISPATCH_CONTEXT DispatchContext;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DispatchContext = (PDISPATCH_CONTEXT)AllocateItem(NonPagedPool, sizeof(DISPATCH_CONTEXT), TAG_PORTCLASS);
    if (!DispatchContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    // allocate object header
    Status = KsAllocateObjectHeader(&ObjectHeader, CreateItemCount, CreateItem, Irp, &DispatchTable);

    if (!NT_SUCCESS(Status))
    {
        // free dispatch context
        FreeItem(DispatchContext, TAG_PORTCLASS);
        // done
        return Status;
    }

    // initialize dispatch context
    DispatchContext->ObjectHeader = ObjectHeader;
    DispatchContext->Target = Target;
    DispatchContext->CreateItem = CreateItem;

    // store dispatch context
    IoStack->FileObject->FsContext = DispatchContext;

    DPRINT("KsAllocateObjectHeader result %x Target %p Context %p\n", Status, Target, DispatchContext);
    return Status;
}

