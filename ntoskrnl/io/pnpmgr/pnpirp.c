/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shortcuts for sending different IRP_MJ_PNP requests
 * COPYRIGHT:   Copyright 2010 Sir Richard <sir_richard@svn.reactos.org>
 *              Copyright 2020 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

NTSTATUS
IopSynchronousCall(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_STACK_LOCATION IoStackLocation,
    _Out_ PVOID *Information)
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_OBJECT TopDeviceObject;
    PAGED_CODE();

    /* Call the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize to failure */
    Irp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = IoStatusBlock.Information = 0;

    /* Special case for IRP_MN_FILTER_RESOURCE_REQUIREMENTS */
    if ((IoStackLocation->MajorFunction == IRP_MJ_PNP) &&
        (IoStackLocation->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS))
    {
        /* Copy the resource requirements list into the IOSB */
        Irp->IoStatus.Information =
        IoStatusBlock.Information = (ULONG_PTR)IoStackLocation->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    /* Initialize the event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Set them up */
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = &Event;

    /* Queue the IRP */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp(Irp);

    /* Copy-in the stack */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    *IrpStack = *IoStackLocation;

    /* Call the driver */
    Status = IoCallDriver(TopDeviceObject, Irp);
    /* Otherwise we may get stuck here or have IoStatusBlock not populated */
    ASSERT(!KeAreAllApcsDisabled());
    if (Status == STATUS_PENDING)
    {
        /* Wait for it */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Remove the reference */
    ObDereferenceObject(TopDeviceObject);

    /* Return the information */
    *Information = (PVOID)IoStatusBlock.Information;
    return Status;
}

// IRP_MN_START_DEVICE (0x00)
NTSTATUS
PiIrpStartDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeResourcesAssigned);

    PVOID info;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_START_DEVICE,
        .Parameters.StartDevice.AllocatedResources = DeviceNode->ResourceList,
        .Parameters.StartDevice.AllocatedResourcesTranslated = DeviceNode->ResourceListTranslated
    };

    // Vista+ does an asynchronous call
    NTSTATUS status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, &info);
    DeviceNode->CompletionStatus = status;
    return status;
}

// IRP_MN_STOP_DEVICE (0x04)
NTSTATUS
PiIrpStopDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeQueryStopped);

    PVOID info;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_STOP_DEVICE
    };

    // Drivers should never fail a IRP_MN_STOP_DEVICE request
    NTSTATUS status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, &info);
    ASSERT(NT_SUCCESS(status));
    return status;
}

// IRP_MN_QUERY_STOP_DEVICE (0x05)
NTSTATUS
PiIrpQueryStopDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeStarted);

    PVOID info;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_STOP_DEVICE
    };

    NTSTATUS status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, &info);
    DeviceNode->CompletionStatus = status;
    return status;
}

// IRP_MN_CANCEL_STOP_DEVICE (0x06)
NTSTATUS
PiIrpCancelStopDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeQueryStopped);

    PVOID info;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_CANCEL_STOP_DEVICE
    };

    // in fact we don't care which status is returned here
    NTSTATUS status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, &info);
    ASSERT(NT_SUCCESS(status));
    return status;
}

// IRP_MN_QUERY_DEVICE_RELATIONS (0x07)
NTSTATUS
PiIrpQueryDeviceRelations(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ DEVICE_RELATION_TYPE Type)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeStarted);

    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS,
        .Parameters.QueryDeviceRelations.Type = Type
    };

    // Vista+ does an asynchronous call
    NTSTATUS status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject,
                                         &stack,
                                         (PVOID)&DeviceNode->OverUsed1.PendingDeviceRelations);
    DeviceNode->CompletionStatus = status;
    return status;
}

// IRP_MN_QUERY_RESOURCES (0x0A)
NTSTATUS
PiIrpQueryResources(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PCM_RESOURCE_LIST *Resources)
{
    PAGED_CODE();

    ASSERT(DeviceNode);

    ULONG_PTR longRes;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_RESOURCES
    };

    NTSTATUS status;
    status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, (PVOID)&longRes);
    if (NT_SUCCESS(status))
    {
        *Resources = (PVOID)longRes;
    }

    return status;
}

// IRP_MN_QUERY_RESOURCE_REQUIREMENTS (0x0B)
NTSTATUS
PiIrpQueryResourceRequirements(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Resources)
{
    PAGED_CODE();

    ASSERT(DeviceNode);

    ULONG_PTR longRes;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    };

    NTSTATUS status;
    status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, (PVOID)&longRes);
    if (NT_SUCCESS(status))
    {
        *Resources = (PVOID)longRes;
    }

    return status;
}

// IRP_MN_QUERY_DEVICE_TEXT (0x0C)
NTSTATUS
PiIrpQueryDeviceText(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ LCID LocaleId,
    _In_ DEVICE_TEXT_TYPE Type,
    _Out_ PWSTR *DeviceText)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeUninitialized);

    ULONG_PTR longText;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_DEVICE_TEXT,
        .Parameters.QueryDeviceText.DeviceTextType = Type,
        .Parameters.QueryDeviceText.LocaleId = LocaleId
    };

    NTSTATUS status;
    status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, (PVOID)&longText);
    if (NT_SUCCESS(status))
    {
        *DeviceText = (PVOID)longText;
    }

    return status;
}

// IRP_MN_QUERY_PNP_DEVICE_STATE (0x14)
NTSTATUS
PiIrpQueryPnPDeviceState(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PPNP_DEVICE_STATE DeviceState)
{
    PAGED_CODE();

    ASSERT(DeviceNode);
    ASSERT(DeviceNode->State == DeviceNodeResourcesAssigned ||
           DeviceNode->State == DeviceNodeStartPostWork ||
           DeviceNode->State == DeviceNodeStarted);

    ULONG_PTR longState;
    IO_STACK_LOCATION stack = {
        .MajorFunction = IRP_MJ_PNP,
        .MinorFunction = IRP_MN_QUERY_PNP_DEVICE_STATE
    };

    NTSTATUS status;
    status = IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &stack, (PVOID)&longState);
    if (NT_SUCCESS(status))
    {
        *DeviceState = longState;
    }

    return status;
}
