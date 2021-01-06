/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriverKm.cpp

Abstract:

    This is the main driver framework.

Author:



Environment:

    Kernel mode only

Revision History:



--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"

// Tracing support
extern "C" {
// #include "FxDriverKm.tmh"
}

_Must_inspect_result_
NTSTATUS
FxDriver::AddDevice(
    __in MdDriverObject DriverObject,
    __in MdDeviceObject PhysicalDeviceObject
    )
{
    FxDriver *pDriver;

    pDriver = FxDriver::GetFxDriver(DriverObject);

    if (pDriver != NULL) {
        return pDriver->AddDevice(PhysicalDeviceObject);
    }

    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
FxDriver::AddDevice(
    _In_ MdDeviceObject PhysicalDeviceObject
    )
{
    WDFDEVICE_INIT init(this);
    FxDevice* pDevice;
    NTSTATUS status;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter AddDevice PDO %p", PhysicalDeviceObject);

    pDevice = NULL;
    init.CreatedOnStack = TRUE;

    init.InitType = FxDeviceInitTypeFdo;
    init.Fdo.PhysicalDevice = PhysicalDeviceObject;

    status = m_DriverDeviceAdd.Invoke(GetHandle(), &init);

    //
    // Caller returned w/out creating a device, we are done.  Returning
    // STATUS_SUCCESS w/out creating a device and attaching to the stack is OK,
    // especially for filter drivers which selectively attach to devices.
    //
    if (init.CreatedDevice == NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Driver did not create a device in "
                            "EvtDriverAddDevice, status %!STATUS!", status);

        //
        // We do not let filters affect the building of the rest of the stack.
        // If they return error, we convert it to STATUS_SUCCESS.
        //
        if (init.Fdo.Filter && !NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "Filter returned %!STATUS! without creating a WDFDEVICE, "
                "converting to STATUS_SUCCESS", status);
            status = STATUS_SUCCESS;
        }

        return status;
    }

    pDevice = init.CreatedDevice;

    if (NT_SUCCESS(status)) {
        //
        // Make sure that DO_DEVICE_INITIALIZING is cleared.
        // FxDevice::FdoInitialize does not do this b/c the driver writer may
        // want the bit set until sometime after WdfDeviceCreate returns
        //
        pDevice->FinishInitializing();
    }
    else {
        //
        // Created a device, but returned error.
        //
        ASSERT(pDevice->IsPnp());
        ASSERT(pDevice->m_CurrentPnpState == WdfDevStatePnpInit);

        status = pDevice->DeleteDeviceFromFailedCreate(status, TRUE);
        pDevice = NULL;
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit, status %!STATUS!", status);

    return status;
}


_Must_inspect_result_
NTSTATUS
FxDriver::AllocateDriverObjectExtensionAndStoreFxDriver(
    VOID
    )
{
    NTSTATUS status;
    FxDriver** ppDriver;

    //
    // Prefast is much happier if we take the size of the type rather then
    // the size of the variable.
    //
    status = Mx::MxAllocateDriverObjectExtension( m_DriverObject.GetObject(),
                                                  FX_DRIVER_ID,
                                                  sizeof(FxDriver**),
                                                  (PVOID*)&ppDriver);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If we succeeded in creating the driver object extension,
    // then store our FxDriver pointer in the DriverObjectExtension.
    //
    *ppDriver = this;

    return STATUS_SUCCESS;
}

FxDriver*
FxDriver::GetFxDriver(
    __in MdDriverObject DriverObject
    )
{
    FxDriver* objExt;
    objExt = *(FxDriver **)Mx::MxGetDriverObjectExtension(DriverObject,
                                                       FX_DRIVER_ID);
    ASSERT(objExt != NULL);

    return objExt;
}


