/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriverApiKm.cpp

Abstract:

    This module contains the "C" interface for the FxDriver object.

Author:



Environment:

    User mode only

Revision History:

--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include <ntverp.h>
#include "FxDriverApiUm.tmh"
}

//
// extern the whole file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverOpenParametersRegistryKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    NTSTATUS status;
    LONG result;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver* pDriver;
    FxRegKey* pKey;
    HKEY hKey = NULL;
    WDFKEY keyHandle;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Driver,
                         FX_TYPE_DRIVER,
                         (PVOID*) &pDriver);

    pKey = new(pFxDriverGlobals, KeyAttributes) FxRegKey(pFxDriverGlobals);
    if (pKey == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pKey->Commit(KeyAttributes, (WDFOBJECT*)&keyHandle);
    if (NT_SUCCESS(status)) {
        if (DesiredAccess & (GENERIC_WRITE | KEY_CREATE_SUB_KEY | WRITE_DAC)) {
            //
            // These access rights are not allowed. This restriction is
            // imposed by the host process and the reflector driver.
            //
            // Even though the maximum-permissions handle is already opened,
            // we fail so that the caller knows not to assume it has the
            // GENERIC_WRITE, KEY_CREATE_SUB_KEY, or WRITE_DAC permissions.
            //
            status = STATUS_ACCESS_DENIED;
            DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
                    "Could not open '%s' service parameters key "
                    "with access rights 0x%x, %!STATUS!",
                    pFxDriverGlobals->Public.DriverName,
                    DesiredAccess, status);
        } else if ((DesiredAccess & ~(KEY_READ | GENERIC_READ)) == 0) {
            //
            // If caller requested read-only access, open a new handle
            // to the parameters key, no reason to give more privileges
            // than needed.
            //
            result = RegOpenKeyEx(pDriver->GetDriverParametersKey(),
                                  L"",
                                  0,
                                  DesiredAccess,
                                  &hKey);
            status = WinErrorToNtStatus(result);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
                    "Could not open '%s' service parameters key "
                    "with access rights 0x%x, %!STATUS!",
                    pFxDriverGlobals->Public.DriverName,
                    DesiredAccess, status);
            }
        } else {
            //
            // If caller requested write access, give it the pre-opened
            // handle, since we do not have permission to open this key
            // with write access rights from user mode.
            //
            hKey = pDriver->GetDriverParametersKey();

            //
            // Mark the registry key handle such that it won't be closed
            // when this FxRegKey is deleted. We might need the handle again
            // for future calls to WdfDriverOpenParametersRegistryKey.
            //
            pKey->SetCanCloseHandle(FALSE);
        }

        if (NT_SUCCESS(status)) {
            pKey->SetHandle((HANDLE)hKey);
            *Key = keyHandle;
        }
    }

    if (!NT_SUCCESS(status)) {
        pKey->DeleteFromFailedCreate();
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDRIVER_OBJECT
WDFEXPORT(WdfDriverWdmGetDriverObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Driver);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDRIVER
WDFEXPORT(WdfWdmDriverGetWdfDriverHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDRIVER_OBJECT DriverObject
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DriverObject);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return NULL;
}

VOID
WDFEXPORT(WdfDriverMiniportUnload)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Driver);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceMiniportCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    PDEVICE_OBJECT DeviceObject,
    __in_opt
    PDEVICE_OBJECT AttachedDeviceObject,
    __in_opt
    PDEVICE_OBJECT Pdo,
    __out
    WDFDEVICE* Device
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Driver);
    UNREFERENCED_PARAMETER(Attributes);
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(AttachedDeviceObject);
    UNREFERENCED_PARAMETER(Pdo);
    UNREFERENCED_PARAMETER(Device);

    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
