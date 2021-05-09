/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    vfpriv.hpp

Abstract:

    common header file for verifier

Author:


Environment:

    User mode only

Revision History:

--*/

#pragma once

#include "fxmin.hpp"

FORCEINLINE
VOID
PerformanceAnalysisIOProcess(
    __in PFX_DRIVER_GLOBALS pFxDriverGlobals,
    __in WDFREQUEST Handle,
    __inout FxRequest** ppReq,
    __inout GUID* pActivityId
    )
{
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Handle,
                         FX_TYPE_REQUEST,
                         (PVOID *) ppReq);

    if ((*ppReq)->GetFxIrp()->GetIoIrp()->IsActivityIdSet() == FALSE) {
        EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
        (*ppReq)->GetFxIrp()->GetIoIrp()->SetActivityId(pActivityId);
    }
    else {
        *pActivityId = *(*ppReq)->GetFxIrp()->GetIoIrp()->GetActivityId();
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

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        PerformanceAnalysisIOProcess(pFxDriverGlobals, Handle, &pReq,
                &activityId);

        UCHAR Type = pReq->GetFxIrp()->GetMajorFunction();
        WDFDEVICE Device = pReq->GetCurrentQueue()->GetDevice()->GetHandle();
        EVENT_DATA_DESCRIPTOR EventData[3];
        EventDataDescCreate(&EventData[0], &Type, sizeof(const UCHAR));
        EventDataDescCreate(&EventData[1], &pDriverDeviceAdd, sizeof(PVOID));
        EventDataDescCreate(&EventData[2], &Device, sizeof(PVOID));

        EventWriteTransfer(Microsoft_Windows_DriverFrameworks_UserMode_PerformanceHandle,
                 &FX_REQUEST_START,
                 &activityId,
                 NULL,
                 3,
                 &EventData[0]);
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

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();
        PerformanceAnalysisIOProcess(pFxDriverGlobals, Handle, &pReq,
                &activityId);

        UCHAR Type = pReq->GetFxIrp()->GetMajorFunction();
        WDFDEVICE Device = pReq->GetCurrentQueue()->GetDevice()->GetHandle();
        EVENT_DATA_DESCRIPTOR EventData[3];
        EventDataDescCreate(&EventData[0], &Type, sizeof(const UCHAR));
        EventDataDescCreate(&EventData[1], &pDriverDeviceAdd, sizeof(PVOID));
        EventDataDescCreate(&EventData[2], &Device, sizeof(PVOID));

        EventWriteTransfer(Microsoft_Windows_DriverFrameworks_UserMode_PerformanceHandle,
                 &FX_REQUEST_COMPLETE,
                 &activityId,
                 NULL,
                 3,
                 &EventData[0]);
    }
    return status;
}

__inline
BOOLEAN
PerformanceAnalysisPowerProcess(
   __in PCEVENT_DESCRIPTOR EventDescriptor,
   __in GUID* pActivityId,
   __in WDFDEVICE Handle
)
{
    WDFOBJECT_OFFSET offset = 0;
    FxObject *pObject = FxObject::_GetObjectFromHandle(Handle, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();
    BOOLEAN status = IsFxPerformanceAnalysis(pFxDriverGlobals);

    if(status) {
        PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();

        EVENT_DATA_DESCRIPTOR EventData[2];
        EventDataDescCreate(&EventData[0], &pDriverDeviceAdd, sizeof(PVOID));
        EventDataDescCreate(&EventData[1], &Handle, sizeof(PVOID));

        EventWriteTransfer(Microsoft_Windows_DriverFrameworks_UserMode_PerformanceHandle,
                 EventDescriptor,
                 pActivityId,
                 NULL,
                 2,
                 &EventData[0]);
    }

    return status;
}

__inline
BOOLEAN
PerfEvtDeviceD0EntryStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
    return PerformanceAnalysisPowerProcess(&FX_POWER_D0_ENTRY_START, pActivityId, Handle);
}

__inline
VOID
PerfEvtDeviceD0EntryStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    PerformanceAnalysisPowerProcess(&FX_POWER_D0_ENTRY_STOP, pActivityId, Handle);
}

__inline
BOOLEAN
PerfEvtDeviceD0ExitStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
    return PerformanceAnalysisPowerProcess(&FX_POWER_D0_EXIT_START, pActivityId, Handle);
}

__inline
VOID
PerfEvtDeviceD0ExitStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    PerformanceAnalysisPowerProcess(&FX_POWER_D0_EXIT_STOP, pActivityId, Handle);
}

__inline
BOOLEAN
PerfEvtDevicePrepareHardwareStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
    return PerformanceAnalysisPowerProcess(&FX_POWER_HW_PREPARE_START, pActivityId, Handle);
}

__inline
VOID
PerfEvtDevicePrepareHardwareStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    PerformanceAnalysisPowerProcess(&FX_POWER_HW_PREPARE_STOP, pActivityId, Handle);
}

__inline
BOOLEAN
PerfEvtDeviceReleaseHardwareStart(
    __in WDFDEVICE Handle,
    __inout GUID* pActivityId
)
{
    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
    return PerformanceAnalysisPowerProcess(&FX_POWER_HW_RELEASE_START, pActivityId, Handle);
}

__inline
VOID
PerfEvtDeviceReleaseHardwareStop(
    __in WDFDEVICE Handle,
    __in GUID* pActivityId
)
{
    PerformanceAnalysisPowerProcess(&FX_POWER_HW_RELEASE_STOP, pActivityId, Handle);
}

// EvtIoStop callback started.
__inline
BOOLEAN
PerfEvtIoStopStart(
    __in WDFQUEUE Queue,
    __inout GUID* pActivityId
)
{
    FxIoQueue* pQueue;
    WDFOBJECT_OFFSET offset = 0;
    WDFDEVICE device;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Queue, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();

    FxObjectHandleGetPtr(pFxDriverGlobals,
                          Queue,
                          FX_TYPE_QUEUE,
                          (PVOID*) &pQueue);
    device = (WDFDEVICE) pQueue->GetDevice()->GetHandle();

    EventActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, pActivityId);
    return PerformanceAnalysisPowerProcess(&FX_EVTIOSTOP_START, pActivityId, device);
}

// EvtIoStop callback returned.
__inline
VOID
PerfEvtIoStopStop(
    __in WDFQUEUE Queue,
    __in GUID* pActivityId
)
{
    FxIoQueue* pQueue;
    WDFOBJECT_OFFSET offset = 0;
    WDFDEVICE device;

    FxObject *pObject = FxObject::_GetObjectFromHandle(Queue, &offset);
    PFX_DRIVER_GLOBALS pFxDriverGlobals = pObject->GetDriverGlobals();

    FxObjectHandleGetPtr(pFxDriverGlobals,
                          Queue,
                          FX_TYPE_QUEUE,
                          (PVOID*) &pQueue);
    device = (WDFDEVICE) pQueue->GetDevice()->GetHandle();

    PerformanceAnalysisPowerProcess(&FX_EVTIOSTOP_STOP, pActivityId, device);
}

__inline
VOID
VerifyIrqlEntry(
    __out KIRQL *Irql
    )
{
    DO_NOTHING();
}

__inline
VOID
VerifyIrqlExit(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in KIRQL PrevIrql
    )
{
    DO_NOTHING();
}

__inline
VOID
VerifyCriticalRegionEntry(
    __out BOOLEAN *CritRegion
    )
{
    DO_NOTHING();
}

__inline
VOID
VerifyCriticalRegionExit(
    __in PWDF_DRIVER_GLOBALS DriverGlobals,
    __in BOOLEAN OldCritRegion,
    __in PVOID Pfn
    )
{
    DO_NOTHING();
}

