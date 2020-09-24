/*++

Copyright (c) Microsoft Corporation

Module Name:

    supportKM.cpp

Abstract:

    This module implements the pnp support routines.

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "..\pnppriv.hpp"
#include <wdmguid.h>

#if defined(EVENT_TRACING)
#include "supportKM.tmh"
#endif

VOID
CopyQueryInterfaceToIrpStack(
    __in PPOWER_THREAD_INTERFACE PowerThreadInterface,
    __in FxIrp* Irp
    )
{
    PIO_STACK_LOCATION stack;

    stack = Irp->GetCurrentIrpStackLocation();

    RtlCopyMemory(stack->Parameters.QueryInterface.Interface,
        PowerThreadInterface,
        PowerThreadInterface->Interface.Size);
}

_Must_inspect_result_
NTSTATUS
GetStackCapabilities(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in MxDeviceObject* DeviceInStack,
    __in_opt PD3COLD_SUPPORT_INTERFACE D3ColdInterface,
    __out PSTACK_DEVICE_CAPABILITIES Capabilities
    )
{
    ULONG i;
    FxAutoIrp irp;
    NTSTATUS status;

    ASSERT(Capabilities != NULL);

    status = STATUS_INSUFFICIENT_RESOURCES;

    //
    // Normally this would be assigned to a local variable on the stack.  Since
    // query caps iteratively moves down the device's tree until it hits the
    // root.  As such, stack usage is consumed quickly.  While this is only one
    // pointer value we are saving, it does eventually add up on very deep stacks.
    //
    DeviceInStack->SetObject(DeviceInStack->GetAttachedDeviceReference());
    if (DeviceInStack->GetObject() == NULL) {
        goto Done;
    }

    irp.SetIrp(FxIrp::AllocateIrp(DeviceInStack->GetStackSize()));
    if (irp.GetIrp() == NULL) {
        goto Done;
    }

    //
    // Initialize device capabilities.
    //
    RtlZeroMemory(Capabilities, sizeof(STACK_DEVICE_CAPABILITIES));
    Capabilities->DeviceCaps.Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities->DeviceCaps.Version  =  1;
    Capabilities->DeviceCaps.Address  = (ULONG) -1;
    Capabilities->DeviceCaps.UINumber = (ULONG) -1;

    //
    // Initialize the Irp.
    //
    irp.SetStatus(STATUS_NOT_SUPPORTED);

    irp.ClearNextStack();
    irp.SetMajorFunction(IRP_MJ_PNP);
    irp.SetMinorFunction(IRP_MN_QUERY_CAPABILITIES);
    irp.SetParameterDeviceCapabilities(&Capabilities->DeviceCaps);

    status = irp.SendIrpSynchronously(DeviceInStack->GetObject());

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Failed to get DEVICE_CAPABILITIES from !devobj %p, %!STATUS!",
            DeviceInStack->GetObject(), status);
        goto Done;
    }

    //
    // Invoke the D3cold support interface.  If present, it will tell
    // us specifically which D-states will work for generating wake signals
    // from specific S-states.
    //
    // Earlier versions of WDF didn't make this query, so for compatibility,
    // we only make it now if the driver was built against WDF 1.11 or
    // later.  In truth, this just shifts the failure from initialization
    // time to run time, because the information that we're presumably
    // getting from the BIOS with this interrogation is saying that the
    // code in earlier verisions of WDF would have blindly enabled a device
    // for wake which simply wasn't capable of generating its wake signal
    // from the chosen D-state.  Thus the device would have been put into
    // a low power state and then failed to resume in response to its wake
    // signal.
    //

    for (i = 0; i <= PowerSystemHibernate; i++) {
        Capabilities->DeepestWakeableDstate[i] = DeviceWakeDepthMaximum;
    }

    if (ARGUMENT_PRESENT(D3ColdInterface) &&
        (D3ColdInterface->GetIdleWakeInfo != NULL) &&
        DriverGlobals->IsVersionGreaterThanOrEqualTo(1,11)) {

        DEVICE_WAKE_DEPTH deepestWakeableDstate;

        for (i = PowerSystemWorking; i <= PowerSystemHibernate; i++) {

            //
            // Failure from D3ColdInterface->GetIdleWakeInfo just means that
            // the bus drivers didn't have any information beyond what can
            // gleaned from the older Query-Capabilities code path.
            //
            // In specific ACPI terms, ACPI will respond to this function with
            // success whenever there is an _SxW object (where x is the sleep
            // state, a value from 0 to 4.)
            //
            // PCI will respond to this interface if ACPI doesn't override its
            // answer and if a parent bus is capable of leaving D0 in S0, and
            // if the PCI device has
            status = D3ColdInterface->GetIdleWakeInfo(
                D3ColdInterface->Context,
                (SYSTEM_POWER_STATE)i,
                &deepestWakeableDstate
                );

            if (NT_SUCCESS(status)) {
                Capabilities->DeepestWakeableDstate[i] = deepestWakeableDstate;
            }
        }
    }

    status = STATUS_SUCCESS;

Done:
    if (DeviceInStack->GetObject() != NULL) {
        DeviceInStack->DereferenceObject();
    }

    return status;
}

VOID
SetD3ColdSupport(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in MxDeviceObject* DeviceInStack,
    __in PD3COLD_SUPPORT_INTERFACE D3ColdInterface,
    __in BOOLEAN UseD3Cold
    )
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DeviceInStack);

    if (D3ColdInterface->SetD3ColdSupport != NULL) {
        D3ColdInterface->SetD3ColdSupport(D3ColdInterface->Context, UseD3Cold);
    }
}

_Must_inspect_result_
PVOID
GetIoMgrObjectForWorkItemAllocation(
    VOID
    )
/*++
Routine description:
    Returns an IO manager object that can be passed in to IoAllocateWorkItem

Arguments:
    None

Return value:
    Pointer to the object that can be passed in to IoAllocateWorkItem
--*/
{
    return (PVOID) FxLibraryGlobals.DriverObject;
}

BOOLEAN
IdleTimeoutManagement::_SystemManagedIdleTimeoutAvailable(
    VOID
    )
{
    return (NULL != FxLibraryGlobals.PoxRegisterDevice);
}

_Must_inspect_result_
NTSTATUS
SendDeviceUsageNotification(
    __in MxDeviceObject* RelatedDevice,
    __inout FxIrp* RelatedIrp,
    __in MxWorkItem* Workitem,
    __in FxIrp* OriginalIrp,
    __in BOOLEAN Revert
    )
{
    NTSTATUS status;

    //
    // use workitem if available
    //
    if (Workitem->GetWorkItem() != NULL) {
        FxUsageWorkitemParameters param;

        param.RelatedDevice = RelatedDevice;
        param.RelatedIrp = RelatedIrp;
        param.OriginalIrp = OriginalIrp;
        param.Revert = Revert;

        //
        // Kick off to another thread
        //
        Workitem->Enqueue(_DeviceUsageNotificationWorkItem, &param);

        //
        // wait for the workitem to finish
        //
        param.Event.EnterCRAndWaitAndLeave();

        status = param.Status;
    }
    else {
        status = SendDeviceUsageNotificationWorker(RelatedDevice,
                                                   RelatedIrp,
                                                   OriginalIrp,
                                                   Revert);
    }

    return status;
}

NTSTATUS
SendDeviceUsageNotificationWorker(
    __in MxDeviceObject* RelatedDevice,
    __inout FxIrp* RelatedIrp,
    __in FxIrp* OriginalIrp,
    __in BOOLEAN Revert
    )
{
    MxDeviceObject relatedTopOfStack;
    NTSTATUS status;

    relatedTopOfStack.SetObject(RelatedDevice->GetAttachedDeviceReference());
    ASSERT(relatedTopOfStack.GetObject() != NULL);

    //
    // Initialize the new IRP with the stack data from the current IRP and
    // and send it to the parent stack.
    //
    RelatedIrp->InitNextStackUsingStack(OriginalIrp);

    if (Revert) {
        RelatedIrp->SetParameterUsageNotificationInPath(
            !RelatedIrp->GetNextStackParameterUsageNotificationInPath());
    }

    RelatedIrp->SetStatus(STATUS_NOT_SUPPORTED);

    status = RelatedIrp->SendIrpSynchronously(relatedTopOfStack.GetObject());

    relatedTopOfStack.DereferenceObject();

    return status;
}

VOID
_DeviceUsageNotificationWorkItem(
    __in MdDeviceObject DeviceObject,
    __in PVOID Context
    )
{
    FxUsageWorkitemParameters* param;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);

    param = (FxUsageWorkitemParameters*) Context;

    status = SendDeviceUsageNotificationWorker(param->RelatedDevice,
                                               param->RelatedIrp,
                                               param->OriginalIrp,
                                               param->Revert);

    //
    // capture status in notification object
    //
    param->Status = status;

    //
    // set event to allow the origial notifcation thread to proceed
    //
    param->Event.Set();
}

