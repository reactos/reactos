#include "pnppriv.h"
#include "common/fxirp.h"
#include "common/dbgtrace.h"


_Must_inspect_result_
NTSTATUS
GetStackCapabilities(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in MxDeviceObject* DeviceInStack,
    __out PDEVICE_CAPABILITIES Capabilities
    )
{
    //ULONG i;
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
    if (DeviceInStack->GetObject() == NULL)
    {
        goto Done;
    }

    irp.SetIrp(FxIrp::AllocateIrp(DeviceInStack->GetStackSize()));
    if (irp.GetIrp() == NULL)
    {
        goto Done;
    }

    //
    // Initialize device capabilities.
    //
    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));
    Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities->Version  =  1;
    Capabilities->Address  = (ULONG) -1;
    Capabilities->UINumber = (ULONG) -1;

    //
    // Initialize the Irp.
    //
    irp.SetStatus(STATUS_NOT_SUPPORTED);

    irp.ClearNextStack();
    irp.SetMajorFunction(IRP_MJ_PNP);
    irp.SetMinorFunction(IRP_MN_QUERY_CAPABILITIES);
    irp.SetParameterDeviceCapabilities(Capabilities);

    status = irp.SendIrpSynchronously(DeviceInStack->GetObject());

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Failed to get DEVICE_CAPABILITIES from !devobj %p, %!STATUS!", 
            DeviceInStack->GetObject(), status);
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:
    if (DeviceInStack->GetObject() != NULL)
    {
        DeviceInStack->DereferenceObject();
    }

    return status;
}