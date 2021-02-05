/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDpcApi.cpp

Abstract:

    This implements the WDFDPC API's

Author:



Environment:

    Kernel mode only

Revision History:


--*/

#include "fxcorepch.hpp"

#include "fxdpc.hpp"

extern "C" {
// #include "FxDpcApi.tmh"
}

//
// extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDpcCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_DPC_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDPC * Dpc
    )

/*++

Routine Description:

    Create a DPC object that will call the supplied function with
    context when it fires. It returns a handle to the WDFDPC object.

Arguments:

    Config     - WDF_DPC_CONFIG structure.

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object and to request
                 a context memory allocation,  and a DestroyCallback.

    Dpc - Pointer to location to returnt he resulting WDFDPC handle.

Returns:

    STATUS_SUCCESS - A WDFDPC handle has been created.

Notes:

    The WDFDPC object is deleted either when the DEVICE or QUEUE it is
    associated as its parent with is deleted, or WdfObjectDelete is called.

    If the DPC is used to access WDM objects, a Cleanup callback should
    be registered to allow references to be released.

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxObject* pParent;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    status = FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                       Attributes,
                                                       FX_VALIDATE_OPTION_PARENT_REQUIRED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                   Attributes->ParentObject,
                                   FX_TYPE_OBJECT,
                                   (PVOID*)&pParent,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Config);
    FxPointerNotNull(pFxDriverGlobals, Dpc);

    if (Config->Size != sizeof(WDF_DPC_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDF_DPC_CONFIG got Size %d, expected %d, %!STATUS!",
                            Config->Size, sizeof(WDF_DPC_CONFIG), status);

        return status;
    }

    status = FxValidateObjectAttributes(
        pFxDriverGlobals,
        Attributes
        );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxDpc::_Create(pFxDriverGlobals, Config, Attributes, pParent, Dpc);

    return status;
}


__drv_maxIRQL(HIGH_LEVEL)
KDPC*
STDCALL
WDFEXPORT(WdfDpcWdmGetDpc)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )
/*++

Routine Description:

    Return the KDPC* object pointer so that it may be linked into
    a DPC list.

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

    KDPC*

--*/

{
    FxDpc* pFxDpc;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Dpc,
                         FX_TYPE_DPC,
                         (PVOID*)&pFxDpc);

    return pFxDpc->GetDpcPtr();
}


__drv_maxIRQL(HIGH_LEVEL)
BOOLEAN
STDCALL
WDFEXPORT(WdfDpcEnqueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )

/*++

Routine Description:

    Enqueue the DPC to run at a system determined time

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

--*/

{
    FxDpc* pFxDpc;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Dpc,
                         FX_TYPE_DPC,
                         (PVOID*)&pFxDpc);

    return KeInsertQueueDpc(pFxDpc->GetDpcPtr(), NULL, NULL);
}


__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(HIGH_LEVEL))
BOOLEAN
STDCALL
WDFEXPORT(WdfDpcCancel)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc,
    __in
    BOOLEAN Wait
    )

/*++

Routine Description:

    Attempt to cancel the DPC and returns status

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

    TRUE  - DPC was cancelled, and was not run

    FALSE - DPC was not cancelled, has run, is running, or will run

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDpc* pFxDpc;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Dpc,
                                   FX_TYPE_DPC,
                                   (PVOID*)&pFxDpc,
                                   &pFxDriverGlobals);

    if (Wait) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
    }

    return pFxDpc->Cancel(Wait);
}



__drv_maxIRQL(HIGH_LEVEL)
WDFOBJECT
STDCALL
WDFEXPORT(WdfDpcGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    )

/*++

Routine Description:

    Return the parent parent object supplied to WdfDpcCreate.

Arguments:

    WDFDPC - Handle to WDFDPC object created with WdfDpcCreate.

Returns:

--*/

{
    FxDpc* pFxDpc;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Dpc,
                         FX_TYPE_DPC,
                         (PVOID*)&pFxDpc);

    return pFxDpc->GetObject();
}

} // extern "C"
