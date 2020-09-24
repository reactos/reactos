/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriverApi.cpp

Abstract:

    This module contains the "C" interface for the FxDriver object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include <ntverp.h>
#include "FxDriverApi.tmh"
}

#include "FxTelemetry.hpp"

//
// extern the whole file
//
extern "C" {

//
// Driver Pool Allocations
//


__drv_maxIRQL(PASSIVE_LEVEL)
PWSTR
WDFEXPORT(WdfDriverGetRegistryPath)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxDriver *pDriver;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Driver,
                                   FX_TYPE_DRIVER,
                                   (PVOID *)&pDriver,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    return pDriver->GetRegistryPathUnicodeString()->Buffer;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    MdDriverObject DriverObject,
    __in
    PCUNICODE_STRING RegistryPath,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    __in
    PWDF_DRIVER_CONFIG DriverConfig,
    __out_opt
    WDFDRIVER* Driver
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver *pDriver;
    NTSTATUS status;
    WDFDRIVER hDriver;
    const LONG validFlags = WdfDriverInitNonPnpDriver |
                            WdfDriverInitNoDispatchOverride;

    hDriver = NULL;
    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DriverObject);
    FxPointerNotNull(pFxDriverGlobals, RegistryPath);
    FxPointerNotNull(pFxDriverGlobals, DriverConfig);

    //
    // Validate the size of the input Driver Config structure.  The size changed
    // after v1.1.  The size is the same for v1.1 and v1.0, verify that.
    //
    WDFCASSERT(sizeof(WDF_DRIVER_CONFIG_V1_0) == sizeof(WDF_DRIVER_CONFIG_V1_1));

    if (DriverConfig->Size != sizeof(WDF_DRIVER_CONFIG)
        &&
        DriverConfig->Size != sizeof(WDF_DRIVER_CONFIG_V1_1)) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "WDF_DRIVER_CONFIG got Size %d, expected v1.1 size %d or cur ver size %d, %!STATUS!",
            DriverConfig->Size,
            sizeof(WDF_DRIVER_CONFIG_V1_1), sizeof(WDF_DRIVER_CONFIG), status);

        return status;
    }

    //
    // Validate the DriverInitFlags value in the Driver Config.
    //
    if ((DriverConfig->DriverInitFlags & ~validFlags) != 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "DriverInitFlags 0x%x invalid, valid flags are 0x%x, %!STATUS!",
            DriverConfig->DriverInitFlags, validFlags, status);
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, RegistryPath);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Driver and Public.Driver are set once WdfDriverCreate returns successfully.
    // If they are set, that means this DDI has already been called for this
    // client.  Return an error if this occurrs.
    //
    if (pFxDriverGlobals->Driver != NULL ||
        pFxDriverGlobals->Public.Driver != NULL) {

        status = STATUS_DRIVER_INTERNAL_ERROR;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
                            "WdfDriverCreate can only be called one time per "
                            "WDM PDRIVER_OBJECT %p, %!STATUS!",
                            DriverObject, status);

        return status;
    }

    if (Driver != NULL) {
        *Driver = NULL;
    }

    //
    // Initializing the tag requires initializing the driver name first.
    //
    FxDriver::_InitializeDriverName(pFxDriverGlobals, RegistryPath);

    //
    // Initialize the tag before any allocation so that we can use the correct
    // tag value when allocating on behalf of the driver (for all allocations,
    // including FxDriver).
    //
    // Use the client's driver tag value if they specified one.  First check
    // to make sure the size of the structure is the new size (vs the old size
    // in v1.1).
    //
    // ' kdD' - was the default tag in many DDKs, don't allow its use.
    //
    if (DriverConfig->Size == sizeof(WDF_DRIVER_CONFIG) &&
        DriverConfig->DriverPoolTag != 0x0 &&
        DriverConfig->DriverPoolTag != ' kdD') {
        //
        // Copy directly using the driver's value
        //
        pFxDriverGlobals->Tag = DriverConfig->DriverPoolTag;
        pFxDriverGlobals->Public.DriverTag = DriverConfig->DriverPoolTag;
    }
    else {
        //
        // Derive the value from the driver's service name
        //
        FxDriver::_InitializeTag(pFxDriverGlobals, DriverConfig);
    }

    //
    // Check to see if this is an NT4 style device driver.  If so, fail if they
    // specified an AddDevice routine.  If no dispatch override is set,
    // do not do any checking at all.
    //
    if (DriverConfig->DriverInitFlags & WdfDriverInitNoDispatchOverride) {
        DO_NOTHING();
    }
    else if ((DriverConfig->DriverInitFlags & WdfDriverInitNonPnpDriver) &&
             DriverConfig->EvtDriverDeviceAdd != NULL) {

         DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
                            "Invalid Driver flags or EvtDriverDeviceAdd callback already added"
                              "STATUS_INVALID_PARAMETER");

        return STATUS_INVALID_PARAMETER;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, DriverAttributes,
                                        (FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED |
                                        FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
                                        FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED)
                                        );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // FxDriver stores the driver wide configuration
    //
    FxInitialize(pFxDriverGlobals, DriverObject, RegistryPath, DriverConfig);

    //
    // FxDriver stores the driver wide configuration
    //
    pDriver = new(pFxDriverGlobals, DriverAttributes)
        FxDriver(DriverObject, DriverConfig, pFxDriverGlobals);

    if (pDriver != NULL) {

        if (NT_SUCCESS(status)) {

            status = pDriver->Initialize(RegistryPath, DriverConfig, DriverAttributes);

            if (NT_SUCCESS(status)) {
                status = pDriver->Commit(DriverAttributes, (WDFOBJECT*)&hDriver, FALSE);
            }
        }
    }
    else {
        status =  STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Only return a valid handle on success.  Upon error, release any memory
    // and do not rely on any other function (like the driver unload routine) to
    // be called.
    //
    if (NT_SUCCESS(status)) {
        //
        // **** Note ****
        // Do not introduce failures after this point without ensuring
        // FxObject::DeleteFromFailedCreate has a chance to clear out any
        // assigned callbacks on the object.
        //

        //
        // Store the WDFDRIVER and FxDriver* globally so the driver can retrieve
        // it anytime.
        //
        pFxDriverGlobals->Driver = pDriver;
        pFxDriverGlobals->Public.Driver = hDriver;

        //
        // Record the flags so that diagnostics knows what type of driver it is.
        //
        pFxDriverGlobals->Public.DriverFlags |= DriverConfig->DriverInitFlags;

        if (DriverConfig->DriverInitFlags &
                (WdfDriverInitNoDispatchOverride | WdfDriverInitNonPnpDriver)) {
            //
            // If there is no dispatch override or if it is an NT4 legacy style
            // driver then we will not displace unload in the stub if one was not
            // specified.
            //
            if (DriverConfig->EvtDriverUnload != NULL) {
                pFxDriverGlobals->Public.DisplaceDriverUnload = TRUE;
            }
            else {
                pFxDriverGlobals->Public.DisplaceDriverUnload = FALSE;
            }
        }
        else {
            pFxDriverGlobals->Public.DisplaceDriverUnload = TRUE;
        }

        if (Driver != NULL) {
            *Driver = hDriver;
        }

        if (FX_TELEMETRY_ENABLED(g_TelemetryProvider, pFxDriverGlobals)) {
            FxAutoString imageName;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        const PWCHAR pVersionStr =
            FX_MAKE_WSTR(__WDF_MAJOR_VERSION_STRING) L"."
            FX_MAKE_WSTR(__WDF_MINOR_VERSION_STRING) L"."
            FX_MAKE_WSTR(__WDF_BUILD_NUMBER) ;
#else // USER_MODE
        const PWCHAR pVersionStr =
            FX_MAKE_WSTR(__WUDF_MAJOR_VERSION_STRING) L"."
            FX_MAKE_WSTR(__WUDF_MINOR_VERSION_STRING) L"."
            FX_MAKE_WSTR(__WUDF_SERVICE_VERSION) ;
#endif

            //
            // GetImageName can fail if the registry cannot be accessed. This
            // can happen during system shutdown. Since the image name is only
            // used for telemetry the failure can be ignored.
            //
            (VOID) GetImageName(pFxDriverGlobals, &imageName.m_UnicodeString);

            WDF_CENSUS_EVT_WRITE_DRIVER_LOAD(g_TelemetryProvider,
                                    pFxDriverGlobals,
                                    imageName.m_UnicodeString.Buffer,
                                    pVersionStr);
        }
    }
    else {
        if (pDriver != NULL) {
            pDriver->DeleteFromFailedCreate();
        }

        FxDestroy(pFxDriverGlobals);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverRegisterTraceInfo)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDRIVER_OBJECT DriverObject,
    __in
    PFN_WDF_TRACE_CALLBACK EvtTraceCallback,
    __in
    PVOID ControlBlock
    )
{
    DDI_ENTRY();

    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(EvtTraceCallback);
    UNREFERENCED_PARAMETER(ControlBlock);

    return STATUS_NOT_SUPPORTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverRetrieveVersionString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver* pDriver;
    FxString* pString;
    NTSTATUS status;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    const PWCHAR pVersionStr =
        L"Kernel Mode Driver Framework version "
        FX_MAKE_WSTR(__WDF_MAJOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WDF_MINOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WDF_BUILD_NUMBER) ;

    const PWCHAR pVersionStrVerifier =
        L"Kernel Mode Driver Framework (verifier on) version "
        FX_MAKE_WSTR(__WDF_MAJOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WDF_MINOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WDF_BUILD_NUMBER);
#else // USER_MODE
    const PWCHAR pVersionStr =
        L"User Mode Driver Framework version "
        FX_MAKE_WSTR(__WUDF_MAJOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WUDF_MINOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WUDF_SERVICE_VERSION) ;

    const PWCHAR pVersionStrVerifier =
        L"User Mode Driver Framework (verifier on) version "
        FX_MAKE_WSTR(__WUDF_MAJOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WUDF_MINOR_VERSION_STRING) L"."
        FX_MAKE_WSTR(__WUDF_SERVICE_VERSION);
#endif

    //
    // Even though it is unused, still convert it to make sure a valid handle is
    // being passed in.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Driver,
                                   FX_TYPE_DRIVER,
                                   (PVOID *)&pDriver,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, String);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         String,
                         FX_TYPE_STRING,
                         (PVOID *)&pString);

    status = pString->Assign(
        pFxDriverGlobals->FxVerifierOn ? pVersionStrVerifier : pVersionStr
        );

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
WDFEXPORT(WdfDriverIsVersionAvailable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver* pDriver;
    NTSTATUS status;
    ULONG major, minor;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    major = __WDF_MAJOR_VERSION;
    minor = __WDF_MINOR_VERSION;
#else
    major = __WUDF_MAJOR_VERSION;
    minor = __WUDF_MINOR_VERSION;
#endif

    //
    // Even though it is unused, still convert it to make sure a valid handle is
    // being passed in.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Driver,
                                   FX_TYPE_DRIVER,
                                   (PVOID *)&pDriver,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, VersionAvailableParams);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (VersionAvailableParams->Size != sizeof(WDF_DRIVER_VERSION_AVAILABLE_PARAMS)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "VersionAvailableParams Size 0x%x, expected 0x%x, %!STATUS!",
            VersionAvailableParams->Size, sizeof(WDF_DRIVER_VERSION_AVAILABLE_PARAMS),
            status);

        return FALSE;
    }

    //
    // We log at TRACE_LEVEL_INFORMATION so that we know it gets into the IFR at
    // all times.  This will make it easier to debug drivers which fail to load
    // when a new minor version of WDF is installed b/c they are failing
    // version checks.
    //
    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
        "IsVersionAvailable, current WDF ver major %d, minor %d, caller asking "
        "about major %d, minor %d", major, minor,
        VersionAvailableParams->MajorVersion, VersionAvailableParams->MinorVersion);

    //
    // Currently we only support one major version per KMDF binary and we support
    // all minor versions of that major version down to 0x0.
    //
    if (VersionAvailableParams->MajorVersion == major &&
        VersionAvailableParams->MinorVersion <= minor) {
        return TRUE;
    }

    return FALSE;
}

} // extern "C"
