/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    VfEventHooks.cpp

Abstract:
    Generated implementation of verifier event callback hooks

Environment:
    User and Kernel

    ** Warning ** : manual changes to this file will be lost.

--*/

#include "vfpriv.hpp"


extern "C" {
extern WDFVERSION WdfVersion;
}

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(FX_ENHANCED_VERIFIER_SECTION_NAME,  \
    VfEvtDeviceD0Entry,    \
    VfEvtDeviceD0EntryPostInterruptsEnabled,    \
    VfEvtDeviceD0Exit,    \
    VfEvtDeviceD0ExitPreInterruptsDisabled,    \
    VfEvtDevicePrepareHardware,    \
    VfEvtDeviceReleaseHardware,    \
    VfEvtDeviceSelfManagedIoCleanup,    \
    VfEvtDeviceSelfManagedIoFlush,    \
    VfEvtDeviceSelfManagedIoInit,    \
    VfEvtDeviceSelfManagedIoSuspend,    \
    VfEvtDeviceSelfManagedIoRestart,    \
    VfEvtDeviceQueryStop,    \
    VfEvtDeviceQueryRemove,    \
    VfEvtDeviceSurpriseRemoval,    \
    VfEvtDeviceUsageNotification,    \
    VfEvtDeviceUsageNotificationEx,    \
    VfEvtDeviceRelationsQuery,    \
    VfEvtIoDefault,    \
    VfEvtIoStop,    \
    VfEvtIoResume,    \
    VfEvtIoRead,    \
    VfEvtIoWrite,    \
    VfEvtIoDeviceControl,    \
    VfEvtIoInternalDeviceControl,    \
    VfEvtIoCanceledOnQueue    \
)
#endif

NTSTATUS
VfEvtDeviceD0Entry(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_D0_ENTRY pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceD0Entry;
    if (pfn != NULL) {
        GUID activityId = { 0 };
        if (PerfEvtDeviceD0EntryStart(Device, &activityId)) {
            returnVal = (pfn)(
               Device,
               PreviousState
               );

            PerfEvtDeviceD0EntryStop(Device, &activityId);
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            returnVal = (pfn)(
               Device,
               PreviousState
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceD0EntryPostInterruptsEnabled(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceD0EntryPostInterruptsEnabled;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device,
           PreviousState
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceD0Exit(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE TargetState
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_D0_EXIT pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceD0Exit;
    if (pfn != NULL) {
        GUID activityId = { 0 };
        if (PerfEvtDeviceD0ExitStart(Device, &activityId)) {
            returnVal = (pfn)(
               Device,
               TargetState
               );

            PerfEvtDeviceD0ExitStop(Device, &activityId);
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            returnVal = (pfn)(
               Device,
               TargetState
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceD0ExitPreInterruptsDisabled(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE TargetState
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceD0ExitPreInterruptsDisabled;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device,
           TargetState
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_PREPARE_HARDWARE pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDevicePrepareHardware;
    if (pfn != NULL) {
        GUID activityId = { 0 };
        if (PerfEvtDevicePrepareHardwareStart(Device, &activityId)) {
            returnVal = (pfn)(
               Device,
               ResourcesRaw,
               ResourcesTranslated
               );

            PerfEvtDevicePrepareHardwareStop(Device, &activityId);
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            returnVal = (pfn)(
               Device,
               ResourcesRaw,
               ResourcesTranslated
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_RELEASE_HARDWARE pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceReleaseHardware;
    if (pfn != NULL) {
        GUID activityId = { 0 };
        if (PerfEvtDeviceReleaseHardwareStart(Device, &activityId)) {
            returnVal = (pfn)(
               Device,
               ResourcesTranslated
               );

            PerfEvtDeviceReleaseHardwareStop(Device, &activityId);
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            returnVal = (pfn)(
               Device,
               ResourcesTranslated
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return returnVal;
}

VOID
VfEvtDeviceSelfManagedIoCleanup(
    WDFDEVICE Device
)
{
    PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSelfManagedIoCleanup;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

VOID
VfEvtDeviceSelfManagedIoFlush(
    WDFDEVICE Device
)
{
    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSelfManagedIoFlush;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

NTSTATUS
VfEvtDeviceSelfManagedIoInit(
    WDFDEVICE Device
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSelfManagedIoInit;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceSelfManagedIoSuspend(
    WDFDEVICE Device
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSelfManagedIoSuspend;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceSelfManagedIoRestart(
    WDFDEVICE Device
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSelfManagedIoRestart;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceQueryStop(
    WDFDEVICE Device
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_QUERY_STOP pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceQueryStop;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

NTSTATUS
VfEvtDeviceQueryRemove(
    WDFDEVICE Device
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_QUERY_REMOVE pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceQueryRemove;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

VOID
VfEvtDeviceSurpriseRemoval(
    WDFDEVICE Device
)
{
    PFN_WDF_DEVICE_SURPRISE_REMOVAL pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceSurpriseRemoval;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Device
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

VOID
VfEvtDeviceUsageNotification(
    WDFDEVICE Device,
    WDF_SPECIAL_FILE_TYPE NotificationType,
    BOOLEAN IsInNotificationPath
)
{
    PFN_WDF_DEVICE_USAGE_NOTIFICATION pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceUsageNotification;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Device,
           NotificationType,
           IsInNotificationPath
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

NTSTATUS
VfEvtDeviceUsageNotificationEx(
    WDFDEVICE Device,
    WDF_SPECIAL_FILE_TYPE NotificationType,
    BOOLEAN IsInNotificationPath
)
{
    NTSTATUS  returnVal = STATUS_SUCCESS;
    PFN_WDF_DEVICE_USAGE_NOTIFICATION_EX pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceUsageNotificationEx;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        returnVal = (pfn)(
           Device,
           NotificationType,
           IsInNotificationPath
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return returnVal;
}

VOID
VfEvtDeviceRelationsQuery(
    WDFDEVICE Device,
    DEVICE_RELATION_TYPE RelationType
)
{
    PFN_WDF_DEVICE_RELATIONS_QUERY pfn = NULL;
    PVF_WDFDEVICECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Device, VF_WDFDEVICECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->PnpPowerEventCallbacksOriginal.EvtDeviceRelationsQuery;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Device,
           RelationType
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

VOID
VfEvtIoDefault(
    WDFQUEUE Queue,
    WDFREQUEST Request
)
{
    PFN_WDF_IO_QUEUE_IO_DEFAULT pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoDefault;
    if (pfn != NULL) {
        if (PerfIoStart(Request)) {
            (pfn)(
               Queue,
               Request
               );
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoStop(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    ULONG ActionFlags
)
{
    PFN_WDF_IO_QUEUE_IO_STOP pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoStop;
    if (pfn != NULL) {
        GUID activityId = { 0 };
        if (PerfEvtIoStopStart(Queue, &activityId)) {
            (pfn)(
               Queue,
               Request,
               ActionFlags
               );

            PerfEvtIoStopStop(Queue, &activityId);
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request,
               ActionFlags
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoResume(
    WDFQUEUE Queue,
    WDFREQUEST Request
)
{
    PFN_WDF_IO_QUEUE_IO_RESUME pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoResume;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Queue,
           Request
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

VOID
VfEvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
)
{
    PFN_WDF_IO_QUEUE_IO_READ pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoRead;
    if (pfn != NULL) {
        if (PerfIoStart(Request)) {
            (pfn)(
               Queue,
               Request,
               Length
               );
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request,
               Length
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoWrite(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
)
{
    PFN_WDF_IO_QUEUE_IO_WRITE pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoWrite;
    if (pfn != NULL) {
        if (PerfIoStart(Request)) {
            (pfn)(
               Queue,
               Request,
               Length
               );
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request,
               Length
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoDeviceControl;
    if (pfn != NULL) {
        if (PerfIoStart(Request)) {
            (pfn)(
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode
               );
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoInternalDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
)
{
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoInternalDeviceControl;
    if (pfn != NULL) {
        if (PerfIoStart(Request)) {
            (pfn)(
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode
               );
        } else {
            KIRQL irql = PASSIVE_LEVEL;
            BOOLEAN critRegion = FALSE;

            VerifyIrqlEntry(&irql);
            VerifyCriticalRegionEntry(&critRegion);

            (pfn)(
               Queue,
               Request,
               OutputBufferLength,
               InputBufferLength,
               IoControlCode
               );

            VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
            VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
        }
    }

    return;
}

VOID
VfEvtIoCanceledOnQueue(
    WDFQUEUE Queue,
    WDFREQUEST Request
)
{
    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE pfn = NULL;
    PVF_WDFIOQUEUECREATE_CONTEXT  context = NULL;

    PAGED_CODE_LOCKED();

    context = GET_CONTEXT(Queue, VF_WDFIOQUEUECREATE_CONTEXT);
    ASSERT(context != NULL);

    pfn = context->IoQueueConfigOriginal.EvtIoCanceledOnQueue;
    if (pfn != NULL) {
        KIRQL irql = PASSIVE_LEVEL;
        BOOLEAN critRegion = FALSE;

        VerifyIrqlEntry(&irql);
        VerifyCriticalRegionEntry(&critRegion);

        (pfn)(
           Queue,
           Request
           );

        VerifyIrqlExit(context->CommonHeader.DriverGlobals, irql);
        VerifyCriticalRegionExit(context->CommonHeader.DriverGlobals, critRegion, (PVOID)pfn);
    }

    return;
}

