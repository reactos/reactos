/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTimerApi.cpp

Abstract:

    This implements the WDFTIMER API's

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "fxtimer.hpp"

extern "C" {
// #include "FxTimerApi.tmh"
}

//
// extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfTimerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_TIMER_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFTIMER * Timer
    )

/*++

Routine Description:

    Create a TIMER object that will call the supplied function
    when it fires. It returns a handle to the WDFTIMER object.

Arguments:

    Config     - WDF_TIMER_CONFIG structure.

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object, to request
                 a context memory allocation and a DestroyCallback.

    Timer - Pointer to location to return the resulting WDFTIMER handle.

Returns:

    STATUS_SUCCESS - A WDFTIMER handle has been created.

Notes:

    The WDFTIMER object is deleted either when the DEVICE or QUEUE it is
    associated with is deleted, or WdfObjectDelete is called.

--*/

{
    DDI_ENTRY();

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
    FxPointerNotNull(pFxDriverGlobals, Timer);

    if (Config->Size != sizeof(WDF_TIMER_CONFIG) &&
        Config->Size != sizeof(WDF_TIMER_CONFIG_V1_7) &&
        Config->Size != sizeof(WDF_TIMER_CONFIG_V1_11)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PWDF_TIMER_CONFIG Size %d, expected %d, %!STATUS!",
            Config->Size, sizeof(WDF_TIMER_CONFIG), status);

        return status;
    }

    if (Config->Period > MAXLONG) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Period value %u for a periodic timer cannot be greater than "
            "MAXLONG, %!STATUS!", Config->Period, status);

        return status;
    }

    //
    // For version 1.13 and higher, the tolerable delay could
    // go upto MAXULONG
    //
    if (Config->Size > sizeof(WDF_TIMER_CONFIG_V1_7) &&
        (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,13) == FALSE)) {
        if (Config->TolerableDelay > MAXLONG) {

            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "TolerableDelay value %u cannot be greater than MAXLONG, "
                "%!STATUS!", Config->TolerableDelay, status);

            return status;
        }
    }

    if (Config->Size > sizeof(WDF_TIMER_CONFIG_V1_11)) {

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        if (Config->UseHighResolutionTimer) {

            status = STATUS_NOT_IMPLEMENTED;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "UseHighResolutionTimer option is not supported for UMDF "
                "%!STATUS!", status);
            return status;
        }
#endif

        if ((Config->TolerableDelay > 0) &&
            (Config->UseHighResolutionTimer)) {

            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "UseHighResolutionTimer option sepcified with non zero tolerable delay %u "
                "%!STATUS!", Config->TolerableDelay, status);

            return status;
        }
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                Attributes,
                                FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Config->Period  > 0 &&
        Attributes->ExecutionLevel == WdfExecutionLevelPassive) {
        status = STATUS_NOT_SUPPORTED;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Passive level periodic timer is not supported. "
            "Use one shot timer and queue the next timer from the callback "
            "or use a dedicated thread, %!STATUS!",
            status);

        return status;
    }

    return FxTimer::_Create(pFxDriverGlobals, Config, Attributes, pParent, Timer);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
WDFEXPORT(WdfTimerStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    LONGLONG DueTime
    )

/*++

Routine Description:

    Enqueue the TIMER to run at the specified time.

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

    DueTime - Time to execute

Returns:

    TRUE if the timer object was in the system's timer queue

--*/

{
    DDI_ENTRY();

    FxTimer* pFxTimer;
    LARGE_INTEGER li;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Timer,
                         FX_TYPE_TIMER,
                         (PVOID*)&pFxTimer);

    li.QuadPart = DueTime;

    return pFxTimer->Start(li);
}

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(DISPATCH_LEVEL))
BOOLEAN
STDCALL
WDFEXPORT(WdfTimerStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    BOOLEAN  Wait
    )

/*++

Routine Description:

    Stop the TIMER

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

Returns:

    TRUE if the timer object was in the system's timer queue

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxTimer* pFxTimer;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Timer,
                                   FX_TYPE_TIMER,
                                   (PVOID*)&pFxTimer,
                                   &pFxDriverGlobals);

    if (Wait) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return FALSE;
        }
    }

    return pFxTimer->Stop(Wait);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
STDCALL
WDFEXPORT(WdfTimerGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer
    )

/*++

Routine Description:

    Return the Parent Object handle supplied to WdfTimerCreate

Arguments:

    WDFTIMER - Handle to WDFTIMER object created with WdfTimerCreate.

Returns:

    Handle to the framework object that is the specified timer object's
    parent object

--*/

{
    DDI_ENTRY();

    FxTimer* pFxTimer;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Timer,
                         FX_TYPE_TIMER,
                         (PVOID*)&pFxTimer);

    return pFxTimer->GetObject();
}

} // extern "C"
