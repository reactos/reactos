/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCxDeviceInitApi.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object
    for the class extensions.

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxCxDeviceInitApi.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

__inline
static
NTSTATUS
FxValiateCx(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_DRIVER_GLOBALS CxDriverGlobals
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (FxIsClassExtension(FxDriverGlobals, CxDriverGlobals) == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "This function can only be called by a WDF "
                            "extension driver, Driver 0x%p, %!STATUS!",
                            CxDriverGlobals->Public.Driver, status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfCxDeviceInitAssignWdmIrpPreprocessCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFCXDEVICE_INIT   CxDeviceInit,
    __in
    PFN_WDFCXDEVICE_WDM_IRP_PREPROCESS EvtCxDeviceWdmIrpPreprocess,
    __in
    UCHAR MajorFunction,
    __drv_when(NumMinorFunctions > 0, __in_bcount(NumMinorFunctions))
    __drv_when(NumMinorFunctions == 0, __in_opt)
    PUCHAR MinorFunctions,
    __in
    ULONG NumMinorFunctions
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS fxDriverGlobals;
    PFX_DRIVER_GLOBALS cxDriverGlobals;
    NTSTATUS status;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    FxPointerNotNull(cxDriverGlobals, CxDeviceInit);
    fxDriverGlobals = CxDeviceInit->ClientDriverGlobals;

    //
    // Caller must be a class extension driver.
    //
    status = FxValiateCx(fxDriverGlobals, cxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(fxDriverGlobals, EvtCxDeviceWdmIrpPreprocess);

    if (NumMinorFunctions > 0) {
        FxPointerNotNull(fxDriverGlobals, MinorFunctions);
    }

    //
    // ARRAY_SIZE(CxDeviceInit->PreprocessInfo->Dispatch) just returns a
    // constant size, it does not actually deref PreprocessInfo (which could
    // be NULL)
    //
    if (MajorFunction >= ARRAY_SIZE(CxDeviceInit->PreprocessInfo->Dispatch)) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "MajorFunction %x is invalid, %!STATUS!",
                            (ULONG)MajorFunction, status);

        goto Done;
    }

    //
    // CX must call this API multiple times if it wants to register preprocess callbacks for
    // multiple IRP major codes.
    //
    if (CxDeviceInit->PreprocessInfo == NULL) {
        CxDeviceInit->PreprocessInfo = new(fxDriverGlobals) FxIrpPreprocessInfo();
        if (CxDeviceInit->PreprocessInfo == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object PreprocessInfo, "
                                "%!STATUS!", status);

            goto Done;
        }
        CxDeviceInit->PreprocessInfo->ClassExtension = TRUE;
    }

    ASSERT(CxDeviceInit->PreprocessInfo->ClassExtension);

    if (NumMinorFunctions > 0) {
        if (CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].NumMinorFunctions != 0) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Already assigned Minorfunctions, %!STATUS!",
                                status);
            goto Done;
        }

        CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions =
            (PUCHAR) FxPoolAllocate(fxDriverGlobals,
                                    NonPagedPool,
                                    sizeof(UCHAR) * NumMinorFunctions);

        if (CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object MinorFunctions, "
                                "%!STATUS!", status);
            goto Done;
        }

        RtlCopyMemory(
            &CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions[0],
            &MinorFunctions[0],
            NumMinorFunctions
            );

        CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].NumMinorFunctions =
            NumMinorFunctions;
    }

    CxDeviceInit->PreprocessInfo->Dispatch[MajorFunction].EvtCxDevicePreprocess =
        EvtCxDeviceWdmIrpPreprocess;

    status = STATUS_SUCCESS;

Done:
    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCxDeviceInitSetIoInCallerContextCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFCXDEVICE_INIT CxDeviceInit,
    __in
    PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext
    )

/*++

Routine Description:

    Registers an I/O pre-processing callback for the class extension device.

    If registered, any I/O for the device is first presented to this
    callback function before being placed in any I/O Queue's.

    The callback is invoked in the thread and/or DPC context of the
    original WDM caller as presented to the I/O package. No framework
    threading, locking, synchronization, or queuing occurs, and
    responsibility for synchronization is up to the device driver.

    This API is intended to support METHOD_NEITHER IRP_MJ_DEVICE_CONTROL's
    which must access the user buffer in the original callers context. The
    driver would probe and lock the buffer pages from within this event
    handler using the functions supplied on the WDFREQUEST object, storing
    any required mapped buffers and/or pointers on the WDFREQUEST context
    whose size is set by the RequestContextSize of the WDF_DRIVER_CONFIG structure.

    It is the responsibility of this routine to either complete the request, or
    pass it on to the I/O package through WdfDeviceEnqueueRequest(Device, Request).

Arguments:

    CxDeviceInit - Class Extension Device initialization structure

    EvtIoInCallerContext - Pointer to driver supplied callback function

Return Value:

    None

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS fxDriverGlobals;
    PFX_DRIVER_GLOBALS cxDriverGlobals;
    NTSTATUS status;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    FxPointerNotNull(cxDriverGlobals, CxDeviceInit);
    fxDriverGlobals = CxDeviceInit->ClientDriverGlobals;

    //
    // Caller must be a class extension driver.
    //
    status = FxValiateCx(fxDriverGlobals, cxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(fxDriverGlobals, EvtIoInCallerContext);

    CxDeviceInit->IoInCallerContextCallback = EvtIoInCallerContext;

Done:
    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCxDeviceInitSetRequestAttributes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFCXDEVICE_INIT CxDeviceInit,
    __in
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS fxDriverGlobals;
    PFX_DRIVER_GLOBALS cxDriverGlobals;
    NTSTATUS status;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    FxPointerNotNull(cxDriverGlobals, CxDeviceInit);
    fxDriverGlobals = CxDeviceInit->ClientDriverGlobals;

    //
    // Caller must be a class extension driver.
    //
    status = FxValiateCx(fxDriverGlobals, cxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(fxDriverGlobals, RequestAttributes);

    //
    // Parent of all requests created from WDFDEVICE are parented by the WDFDEVICE.
    //
    status = FxValidateObjectAttributes(fxDriverGlobals,
                                        RequestAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);

    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        return;
    }

    RtlCopyMemory(&CxDeviceInit->RequestAttributes,
                  RequestAttributes,
                  sizeof(WDF_OBJECT_ATTRIBUTES));

Done:
    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCxDeviceInitSetFileObjectConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFCXDEVICE_INIT CxDeviceInit,
    __in
    PWDFCX_FILEOBJECT_CONFIG CxFileObjectConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    )

/*++

Routine Description:

    Registers file object callbacks for class extensions.

    Defaults to WdfFileObjectNotRequired if no file obj config set.

Arguments:

Returns:

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS fxDriverGlobals;
    PFX_DRIVER_GLOBALS cxDriverGlobals;
    NTSTATUS status;
    WDF_FILEOBJECT_CLASS normalizedFileClass;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    FxPointerNotNull(cxDriverGlobals, CxDeviceInit);
    fxDriverGlobals = CxDeviceInit->ClientDriverGlobals;

    //
    // Caller must be a class extension driver.
    //
    status = FxValiateCx(fxDriverGlobals, cxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(fxDriverGlobals, CxFileObjectConfig);

    if (CxFileObjectConfig->Size != sizeof(WDFCX_FILEOBJECT_CONFIG)) {
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid CxFileObjectConfig Size %d, expected %d",
            CxFileObjectConfig->Size, sizeof(WDFCX_FILEOBJECT_CONFIG));

        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    status = FxValidateObjectAttributes(
        fxDriverGlobals,
        FileObjectAttributes,
        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED |
            FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
            FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED
        );

    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Validate AutoForwardCleanupClose.
    //
    switch (CxFileObjectConfig->AutoForwardCleanupClose) {
    case WdfTrue:
    case WdfFalse:
    case WdfUseDefault:
        break;

    default:
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid CxFileObjectConfig->AutoForwardCleanupClose value 0x%x, "
            "expected WDF_TRI_STATE value",
            CxFileObjectConfig->AutoForwardCleanupClose);

        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    CxDeviceInit->FileObject.Set = TRUE;

    CxDeviceInit->FileObject.AutoForwardCleanupClose =
        CxFileObjectConfig->AutoForwardCleanupClose;

    //
    // Remove bit flags and validate file object class value.
    //
    normalizedFileClass = FxFileObjectClassNormalize(
                                CxFileObjectConfig->FileObjectClass);

    if (normalizedFileClass == WdfFileObjectInvalid ||
        normalizedFileClass > WdfFileObjectWdfCannotUseFsContexts)  {
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Out of range CxFileObjectConfig->FileObjectClass %d",
            CxFileObjectConfig->FileObjectClass);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // The optional flag can only be combined with a subset of values.
    //
    if (FxIsFileObjectOptional(CxFileObjectConfig->FileObjectClass)) {
        switch(normalizedFileClass) {
        case WdfFileObjectWdfCanUseFsContext:
        case WdfFileObjectWdfCanUseFsContext2:
        case WdfFileObjectWdfCannotUseFsContexts:
            break;

        default:
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Invalid CxFileObjectConfig->FileObjectClass %d",
                CxFileObjectConfig->FileObjectClass);
            FxVerifierDbgBreakPoint(fxDriverGlobals);
            goto Done;
            break; // just in case static verification tools complain.
        }
    }

    CxDeviceInit->FileObject.Class = CxFileObjectConfig->FileObjectClass;

    RtlCopyMemory(&CxDeviceInit->FileObject.Callbacks,
                  CxFileObjectConfig,
                  sizeof(CxDeviceInit->FileObject.Callbacks));

    if (FileObjectAttributes != NULL) {
        RtlCopyMemory(&CxDeviceInit->FileObject.Attributes,
                      FileObjectAttributes,
                      sizeof(CxDeviceInit->FileObject.Attributes));
    }

Done:
    return;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFCXDEVICE_INIT
WDFEXPORT(WdfCxDeviceInitAllocate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS cxDriverGlobals;
    PFX_DRIVER_GLOBALS fxDriverGlobals;
    PWDFCXDEVICE_INIT  cxDeviceInit;
    NTSTATUS status;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    FxPointerNotNull(cxDriverGlobals, DeviceInit);
    fxDriverGlobals = DeviceInit->DriverGlobals;
    cxDeviceInit = NULL;

    //
    // Caller must be a class extension driver.
    //
    status = FxValiateCx(fxDriverGlobals, cxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    status = FxVerifierCheckIrqlLevel(fxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    cxDeviceInit = WDFCXDEVICE_INIT::_AllocateCxDeviceInit(DeviceInit);
    if (NULL == cxDeviceInit) {
        goto Done;
    }

    cxDeviceInit->ClientDriverGlobals = fxDriverGlobals;
    cxDeviceInit->CxDriverGlobals = cxDriverGlobals;

Done:
    return cxDeviceInit;
}

} // extern "C"

