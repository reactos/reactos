/*++

Copyright (c) Microsoft Corporation

Module Name:

    supportUM.cpp

Abstract:

    This module implements the pnp support routines.

Author:


Environment:

    User mode only

Revision History:

--*/

#include "..\pnppriv.hpp"

VOID
CopyQueryInterfaceToIrpStack(
    __in PPOWER_THREAD_INTERFACE PowerThreadInterface,
    __in FxIrp* Irp
    )
{
    UNREFERENCED_PARAMETER(PowerThreadInterface);
    UNREFERENCED_PARAMETER(Irp);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
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
    HRESULT hr;
    NTSTATUS status;
    MdDeviceObject deviceObject;
    IWudfDeviceStack* deviceStack;

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(D3ColdInterface);

    deviceObject = DeviceInStack->GetObject();
    deviceStack = deviceObject->GetDeviceStackInterface();

    hr = deviceStack->GetStackCapabilities(Capabilities);

    if (S_OK == hr) {
        status = STATUS_SUCCESS;
    }
    else {
        PUMDF_VERSION_DATA driverVersion = deviceStack->GetMinDriverVersion();
        BOOL preserveCompat =
             deviceStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(hr,
                                             driverVersion->MajorNumber,
                                             driverVersion->MinorNumber,
                                             preserveCompat);
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
    MdDeviceObject deviceObject;
    IWudfDeviceStack* deviceStack;

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(D3ColdInterface);

    deviceObject = DeviceInStack->GetObject();
    deviceStack = deviceObject->GetDeviceStackInterface();

    deviceStack->SetD3ColdSupport(UseD3Cold);
}

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
    //
    // In user-mode we don't need an IO manager object for work item allocation
    // so we return NULL.
    //
    return NULL;
}

BOOLEAN
IdleTimeoutManagement::_SystemManagedIdleTimeoutAvailable(
    VOID
    )
{

    return TRUE;
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
    UNREFERENCED_PARAMETER(RelatedDevice);
    UNREFERENCED_PARAMETER(RelatedIrp);
    UNREFERENCED_PARAMETER(Workitem);
    UNREFERENCED_PARAMETER(OriginalIrp);
    UNREFERENCED_PARAMETER(Revert);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}


