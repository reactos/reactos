/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/wdm/audio/backpln/portcls/irp.cpp
 * PURPOSE:         Port Class driver / IRP Handling
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

typedef struct
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
}QUERY_POWER_CONTEXT, *PQUERY_POWER_CONTEXT;


NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("PortClsCreate called\n");

    return KsDispatchIrp(DeviceObject, Irp);
}


NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION IoStack;
    POWER_STATE PowerState;
    IResourceList* resource_list = NULL;
    //ULONG Index;
    //PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor, UnPartialDescriptor;

    DeviceExt = (PPCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);


    DPRINT("PortClsPnp called %u\n", IoStack->MinorFunction);

    //PC_ASSERT(DeviceExt);

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DPRINT("IRP_MN_START_DEVICE\n");

            // Create the resource list
            Status = PcNewResourceList(
                        &resource_list,
                        NULL,
                        PagedPool,
                        IoStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                        IoStack->Parameters.StartDevice.AllocatedResources);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("PcNewResourceList failed [0x%8x]\n", Status);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            // forward irp to lower device object
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);

            if (!NT_SUCCESS(Status))
            {
                // lower device object failed to start
                resource_list->Release();
                // complete the request
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                // return result
                return Status;
            }

            // sanity check
            //PC_ASSERT(DeviceExt->StartDevice);
            // Call the StartDevice routine
            DPRINT("Calling StartDevice at 0x%8p\n", DeviceExt->StartDevice);
            Status = DeviceExt->StartDevice(DeviceObject, Irp, resource_list);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("StartDevice returned a failure code [0x%8x]\n", Status);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            // Assign the resource list to our extension
            DeviceExt->resources = resource_list;

            // store device power state
            DeviceExt->DevicePowerState = PowerDeviceD0;
            DeviceExt->SystemPowerState = PowerSystemWorking;

            // notify power manager of current state
            PowerState = *((POWER_STATE*)&DeviceExt->DevicePowerState);
            PoSetPowerState(DeviceObject, DevicePowerState, PowerState);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_REMOVE_DEVICE:
            // Clean up
            DPRINT("IRP_MN_REMOVE_DEVICE\n");

            // sanity check
            PC_ASSERT(DeviceExt);

            // FIXME more cleanup */
            if (DeviceExt->resources)
            {
                // free resource list */
                DeviceExt->resources->Release();

                // set to null
                DeviceExt->resources = NULL;
            }

            // Forward request
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);

            return PcCompleteIrp(DeviceObject, Irp, Status);

        case IRP_MN_QUERY_INTERFACE:
            DPRINT("IRP_MN_QUERY_INTERFACE\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return PcCompleteIrp(DeviceObject, Irp, Status);
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return PcCompleteIrp(DeviceObject, Irp, Status);
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return PcCompleteIrp(DeviceObject, Irp, Status);
       case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return PcCompleteIrp(DeviceObject, Irp, Status);
       case IRP_MN_READ_CONFIG:
            DPRINT("IRP_MN_READ_CONFIG\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return PcCompleteIrp(DeviceObject, Irp, Status);
    }

    DPRINT("unhandled function %u\n", IoStack->MinorFunction);
    Status = Irp->IoStatus.Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

VOID
CALLBACK
PwrCompletionFunction(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus)
{
    NTSTATUS Status;
    PQUERY_POWER_CONTEXT PwrContext = (PQUERY_POWER_CONTEXT)Context;

    if (NT_SUCCESS(IoStatus->Status))
    {
        // forward request to lower device object
        Status = PcForwardIrpSynchronous(PwrContext->DeviceObject, PwrContext->Irp);
    }
    else
    {
        // failed
        Status = IoStatus->Status;
    }

    // start next power irp
    PoStartNextPowerIrp(PwrContext->Irp);

    // complete request
    PwrContext->Irp->IoStatus.Status = Status;
    IoCompleteRequest(PwrContext->Irp, IO_NO_INCREMENT);

    // free context
    FreeItem(PwrContext, TAG_PORTCLASS);
}


NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPCLASS_DEVICE_EXTENSION DeviceExtension;
    PQUERY_POWER_CONTEXT PwrContext;
    POWER_STATE PowerState;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("PortClsPower called\n");

    // get currrent stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->MinorFunction != IRP_MN_SET_POWER && IoStack->MinorFunction != IRP_MN_QUERY_POWER)
    {
        // just forward the request
        Status = PcForwardIrpSynchronous(DeviceObject, Irp);

        // start next power irp
        PoStartNextPowerIrp(Irp);

        // complete request
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        // done
        return Status;
    }


    // get device extension
    DeviceExtension = (PPCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // get current request type
    if (IoStack->Parameters.Power.Type == DevicePowerState)
    {
        // request for device power state
        if (DeviceExtension->DevicePowerState == IoStack->Parameters.Power.State.DeviceState)
        {
            // nothing has changed
            if (IoStack->MinorFunction == IRP_MN_QUERY_POWER)
            {
                // only forward query requests
                Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            }

            // start next power irp
            PoStartNextPowerIrp(Irp);

            // complete request
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            // done
            return Status;
        }

        if (IoStack->MinorFunction == IRP_MN_QUERY_POWER)
        {
            // check if there is a registered adapter power management
            if (DeviceExtension->AdapterPowerManagement)
            {
                // it is query if the change can be changed
                PowerState = *((POWER_STATE*)&IoStack->Parameters.Power.State.DeviceState);
                Status = DeviceExtension->AdapterPowerManagement->QueryPowerChangeState(PowerState);

                // sanity check
                PC_ASSERT(Status == STATUS_SUCCESS);
            }

            // only forward query requests
            PcForwardIrpSynchronous(DeviceObject, Irp);

            // start next power irp
            PoStartNextPowerIrp(Irp);

            // complete request
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            // done
            return Status;
        }
        else
        {
            // set power state
            PowerState = *((POWER_STATE*)&IoStack->Parameters.Power.State.DeviceState);
            PoSetPowerState(DeviceObject, DevicePowerState, PowerState);

            // check if there is a registered adapter power management
            if (DeviceExtension->AdapterPowerManagement)
            {
                // notify of a power change state
                DeviceExtension->AdapterPowerManagement->PowerChangeState(PowerState);
            }

            // FIXME call all registered IPowerNotify interfaces via ISubdevice interface

            // store new power state
           DeviceExtension->DevicePowerState = IoStack->Parameters.Power.State.DeviceState;

            // complete request
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            // done
            return Status;
        }
    }
    else
    {
        // sanity check
        PC_ASSERT(IoStack->Parameters.Power.Type == SystemPowerState);

        if (IoStack->MinorFunction == IRP_MN_QUERY_POWER)
        {
            // mark irp as pending
            IoMarkIrpPending(Irp);

            // allocate power completion context
            PwrContext = (PQUERY_POWER_CONTEXT)AllocateItem(NonPagedPool, sizeof(QUERY_POWER_CONTEXT), TAG_PORTCLASS);

            if (!PwrContext)
            {
                // no memory
                PoStartNextPowerIrp(Irp);

                // complete and forget
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                // done
                return Status;
            }

            // setup power context
            PwrContext->Irp = Irp;
            PwrContext->DeviceObject = DeviceObject;

            // pass the irp down
            PowerState = *((POWER_STATE*)IoStack->Parameters.Power.State.SystemState);
            Status = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject, IoStack->MinorFunction, PowerState, PwrCompletionFunction, (PVOID)PwrContext, NULL);

            // check for success
            if (!NT_SUCCESS(Status))
            {
                // failed
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);

                // done
                return Status;
            }

            // done
            return STATUS_PENDING;
        }
        else
        {
            // set power request
            DeviceExtension->SystemPowerState = IoStack->Parameters.Power.State.SystemState;

            // only forward query requests
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);

            // start next power irp
            PoStartNextPowerIrp(Irp);

            // complete request
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            // done
            return Status;
        }
    }
}

NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("PortClsSysControl called\n");

    // TODO

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PortClsShutdown(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PPCLASS_DEVICE_EXTENSION DeviceExtension;
    DPRINT("PortClsShutdown called\n");

    // get device extension
    DeviceExtension = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->AdapterPowerManagement)
    {
        // release adapter power management
        DPRINT("Power %u\n", DeviceExtension->AdapterPowerManagement->Release());
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("PcDispatchIrp called - handling IRP in PortCls MajorFunction %x MinorFunction %x\n", IoStack->MajorFunction, IoStack->MinorFunction);

    switch ( IoStack->MajorFunction )
    {
        // PortCls
        case IRP_MJ_CREATE :
            return PortClsCreate(DeviceObject, Irp);

        case IRP_MJ_PNP :
            return PortClsPnp(DeviceObject, Irp);

        case IRP_MJ_POWER :
            return PortClsPower(DeviceObject, Irp);

        case IRP_MJ_DEVICE_CONTROL:
            return KsDispatchIrp(DeviceObject, Irp);

        case IRP_MJ_CLOSE:
            return KsDispatchIrp(DeviceObject, Irp);

        case IRP_MJ_SYSTEM_CONTROL :
            return PortClsSysControl(DeviceObject, Irp);

        case IRP_MJ_SHUTDOWN:
            return PortClsShutdown(DeviceObject, Irp);

        default:
            DPRINT("Unhandled function %x\n", IoStack->MajorFunction);
            break;
    };

    // If we reach here, we just complete the IRP
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PcCompleteIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  NTSTATUS Status)
{
#if 0
    PC_ASSERT(DeviceObject);
    PC_ASSERT(Irp);
    PC_ASSERT(Status != STATUS_PENDING);
#endif

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
CompletionRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

#undef IoSetCompletionRoutine
#define IoSetCompletionRoutine(_Irp, \
                               _CompletionRoutine, \
                               _Context, \
                               _InvokeOnSuccess, \
                               _InvokeOnError, \
                               _InvokeOnCancel) \
{ \
  PIO_STACK_LOCATION _IrpSp; \
  _IrpSp = IoGetNextIrpStackLocation(_Irp); \
  _IrpSp->CompletionRoutine = (PIO_COMPLETION_ROUTINE)(_CompletionRoutine); \
  _IrpSp->Context = (_Context); \
  _IrpSp->Control = 0; \
  if (_InvokeOnSuccess) _IrpSp->Control = SL_INVOKE_ON_SUCCESS; \
  if (_InvokeOnError) _IrpSp->Control |= SL_INVOKE_ON_ERROR; \
  if (_InvokeOnCancel) _IrpSp->Control |= SL_INVOKE_ON_CANCEL; \
}



NTSTATUS
NTAPI
PcForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    KEVENT Event;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // initialize the notification event
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    // are there enough irp stack locations
    if (Irp->CurrentLocation < Irp->StackCount + 1)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp, CompletionRoutine, (PVOID)&Event, TRUE, TRUE, TRUE);

    // now call the driver
    Status = IoCallDriver(DeviceExt->PrevDeviceObject, Irp);
    // did the request complete yet
    if (Status == STATUS_PENDING)
    {
        // not yet, lets wait a bit
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return Status;
}
