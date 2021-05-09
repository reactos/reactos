/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    vfpriv.hpp

Abstract:

    common header file for verifier

Author:



Environment:

    kernel mode only

Revision History:

--*/

#pragma once
extern "C" {
#include <ntddk.h>
}

#include "fx.hpp"

extern "C" {

__inline
VOID
VerifyIrqlEntry(
    __out KIRQL *Irql
    )
{
    *Irql = KeGetCurrentIrql();
}

__inline
VOID
VerifyIrqlExit(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in KIRQL PrevIrql
    )
{
    KIRQL currIrql;

    currIrql = KeGetCurrentIrql();
    if (PrevIrql != currIrql) {
        KdPrint(("WDF VERIFIER: Irql at entry of event is not same as at exit.\n"));
        FxVerifierBugCheck(GetFxDriverGlobals(DriverGlobals),
                           WDF_VERIFIER_IRQL_MISMATCH,
                           PrevIrql,
                           currIrql
                           );
    }
}

__inline
VOID
VerifyCriticalRegionEntry(
    __out BOOLEAN *CritRegion
    )
{
    if (KeGetCurrentIrql() <= APC_LEVEL) {
        *CritRegion = KeAreApcsDisabled();
    }
}

__inline
VOID
VerifyCriticalRegionExit(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in BOOLEAN OldCritRegion,
    __in PVOID Pfn
    )
{
    if (KeGetCurrentIrql() <= APC_LEVEL) {
        if (OldCritRegion != KeAreApcsDisabled()) {
            KdPrint(("WDF VERIFIER: Critical section entry and exit around event callback incorrect\n"));
            FxVerifierBugCheck(GetFxDriverGlobals(DriverGlobals),
                               WDF_VERIFIER_CRITICAL_REGION_MISMATCH,
                               (ULONG_PTR)Pfn,
                               0
                               );
        }
    }
}
} // extern "C"

FORCEINLINE
VOID
PerformanceAnalysisIOProcess(
    __in PFX_DRIVER_GLOBALS pFxDriverGlobals,
    __in WDFREQUEST Handle,
    __out FxRequest** ppReq,
    __inout GUID* pActivityId
    )
{

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Handle,
                         FX_TYPE_REQUEST,
                         (PVOID *) ppReq);

    if (IoGetActivityIdIrp((*ppReq)->GetFxIrp()->GetIrp(), pActivityId) == STATUS_NOT_FOUND)
    {
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        IoSetActivityIdIrp((*ppReq)->GetFxIrp()->GetIrp(), pActivityId);
    }
}

FORCEINLINE
BOOLEAN
PerfIoStart(
    __in WDFREQUEST Handle
)
{
    FxRequest* pReq;
    GUID activityId = { 0 };
    WDFOBJECT_OFFSET offset = 0;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status)
    {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        PerformanceAnalysisIOProcess(pFxDriverGlobals, Handle, &pReq,
                &activityId);

        EventWriteFX_REQUEST_START(&activityId, pReq->GetFxIrp()->GetMajorFunction(),
                    pDriverDeviceAdd, pReq->GetCurrentQueue()->GetDevice()->GetHandle());
    }
    return status;
}

FORCEINLINE
BOOLEAN
PerfIoComplete(
    __in WDFREQUEST Handle
)
{
    FxRequest* pReq;
    GUID activityId = { 0 };
    WDFOBJECT_OFFSET offset = 0;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status)
    {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        PerformanceAnalysisIOProcess(pFxDriverGlobals, Handle, &pReq,
                &activityId);

        EventWriteFX_REQUEST_COMPLETE(&activityId, pReq->GetFxIrp()->GetMajorFunction(),
                    pDriverDeviceAdd, pReq->GetCurrentQueue()->GetDevice()->GetHandle());
    }
    return status;
}

FORCEINLINE
PFN_WDF_DRIVER_DEVICE_ADD
PerformanceGetDriverDeviceAdd(
    __in WDFOBJECT Handle
)
{
    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();

    return pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
}

__inline
BOOLEAN
PerfEvtDeviceD0EntryStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        EventWriteFX_POWER_D0_ENTRY_START(pActivityId, pDriverDeviceAdd, Handle);
    }

    return status;
}

__inline
VOID
PerfEvtDeviceD0EntryStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    EventWriteFX_POWER_D0_ENTRY_STOP(pActivityId, PerformanceGetDriverDeviceAdd(Handle), Handle);
}

__inline
BOOLEAN
PerfEvtDeviceD0ExitStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        EventWriteFX_POWER_D0_EXIT_START(pActivityId, pDriverDeviceAdd, Handle);
    }
    return status;
}

__inline
VOID
PerfEvtDeviceD0ExitStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    EventWriteFX_POWER_D0_EXIT_STOP(pActivityId, PerformanceGetDriverDeviceAdd(Handle), Handle);
}

__inline
BOOLEAN
PerfEvtDevicePrepareHardwareStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{

    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        EventWriteFX_POWER_HW_PREPARE_START(pActivityId, pDriverDeviceAdd, Handle);
    }
    return status;
}

__inline
VOID
PerfEvtDevicePrepareHardwareStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    EventWriteFX_POWER_HW_PREPARE_STOP(pActivityId, PerformanceGetDriverDeviceAdd(Handle), Handle);
}

__inline
BOOLEAN
PerfEvtDeviceReleaseHardwareStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        EventWriteFX_POWER_HW_RELEASE_START(pActivityId, pDriverDeviceAdd, Handle);
    }
    return status;
}

__inline
VOID
PerfEvtDeviceReleaseHardwareStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    EventWriteFX_POWER_HW_RELEASE_STOP(pActivityId, PerformanceGetDriverDeviceAdd(Handle), Handle);
}

// EvtIoStop callback started.
__inline
BOOLEAN
PerfEvtIoStopStart(
    __in WDFQUEUE Queue,
    __inout GUID* pActivityId
)
{
    WDFOBJECT_OFFSET offset = 0;
    WDFDEVICE device;
    FxIoQueue* pQueue;
    PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Queue, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        FxObjectHandleGetPtr(pFxDriverGlobals,
                          Queue,
                          FX_TYPE_QUEUE,
                          (PVOID*) &pQueue);
        device = (WDFDEVICE) pQueue->GetDevice()->GetHandle();
        pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        EventWriteFX_EVTIOSTOP_START(pActivityId, pDriverDeviceAdd, device);
    }
    return status;
}

// EvtIoStop callback returned.
__inline
VOID
PerfEvtIoStopStop(
    __in WDFQUEUE Queue,
    __in GUID* pActivityId
)
{
    WDFOBJECT_OFFSET offset = 0;
    WDFDEVICE device;
    FxIoQueue* pQueue;
    PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Queue, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    FxObjectHandleGetPtr(pFxDriverGlobals,
                      Queue,
                      FX_TYPE_QUEUE,
                      (PVOID*) &pQueue);
    device = (WDFDEVICE) pQueue->GetDevice()->GetHandle();
    pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();

    EventWriteFX_EVTIOSTOP_STOP(pActivityId, pDriverDeviceAdd, device);
}
