/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetRemoteKm.cpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "../../fxtargetsshared.hpp"

extern "C" {
// #include "FxIoTargetRemoteKm.tmh"
}

#include <initguid.h>
#include "wdmguid.h"

_Must_inspect_result_
NTSTATUS
STDCALL
FxIoTargetRemote::_PlugPlayNotification(
    __in PVOID NotificationStructure,
    __inout_opt PVOID Context
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PTARGET_DEVICE_REMOVAL_NOTIFICATION pNotification;
    FxIoTargetRemote* pThis;
    NTSTATUS status;

    ASSERT(Mx::MxGetCurrentIrql() < DISPATCH_LEVEL);
    pNotification = (PTARGET_DEVICE_REMOVAL_NOTIFICATION) NotificationStructure;
    pThis = (FxIoTargetRemote*) Context;

    //
    // In one of these callbacks, the driver may decide to delete the target.
    // If that is the case, we need to be able to return and deref the object until
    // we are done.
    //
    pThis->ADDREF((PVOID)_PlugPlayNotification);

    pFxDriverGlobals = pThis->GetDriverGlobals();

    status = STATUS_SUCCESS;

    if (FxIsEqualGuid(&pNotification->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFIOTARGET %p: query remove notification", pThis->GetObjectHandle());

        //
        // Device is gracefully being removed.  PnP is asking us to close down
        // the target.  If there is a driver callback, there is *no* default
        // behavior.  This is because we don't know what the callback is going
        // to do.  For instance, the driver could reopen the target to a
        // different device in a multi-path scenario.
        //
        if (pThis->m_EvtQueryRemove.m_Method != NULL) {
            status = pThis->m_EvtQueryRemove.Invoke(pThis->GetHandle());
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "WDFIOTARGET %p: query remove, default action (close for QR)",
                pThis->GetObjectHandle());

            //
            // No callback, close it down conditionally.
            //
            pThis->Close(FxIoTargetRemoteCloseReasonQueryRemove);
        }
    }
    else if (FxIsEqualGuid(&pNotification->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFIOTARGET %p: remove complete notification", pThis->GetObjectHandle());

        //
        // The device was surprise removed, close it for good if the driver has
        // no override.
        //
        if (pThis->m_EvtRemoveComplete.m_Method != NULL) {
            pThis->m_EvtRemoveComplete.Invoke(pThis->GetHandle());
        }
        else {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "WDFIOTARGET %p: remove complete, default action (close)",
                pThis->GetObjectHandle());

            //
            // The device is now gone for good.  Close down the target for good.
            //
            pThis->Close(FxIoTargetRemoteCloseReasonPlainClose);
        }
    }
    else if (FxIsEqualGuid(&pNotification->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFIOTARGET %p: remove canceled notification", pThis->GetObjectHandle());

        if (pThis->m_EvtRemoveCanceled.m_Method != NULL) {
            pThis->m_EvtRemoveCanceled.Invoke(pThis->GetHandle());
        }
        else {
            WDF_IO_TARGET_OPEN_PARAMS params;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "WDFIOTARGET %p: remove canceled, default action (reopen)",
                pThis->GetObjectHandle());

            WDF_IO_TARGET_OPEN_PARAMS_INIT_REOPEN(&params);

            //
            // Attempt to reopen the target with stored settings
            //
            status = pThis->Open(&params);
        }
    }

    pThis->RELEASE((PVOID)_PlugPlayNotification);

    return status;
}

NTSTATUS
FxIoTargetRemote::RegisterForPnpNotification(
    )
{
    NTSTATUS status;

    //
    // Register for PNP notifications on the handle we just opened.
    // This will notify us of pnp state changes on the handle.
    //
    status = IoRegisterPlugPlayNotification(
        EventCategoryTargetDeviceChange,
        0,
        m_TargetFileObject,
        m_Driver->GetDriverObject(),
        _PlugPlayNotification,
        this,
        &m_TargetNotifyHandle);

    return status;
}

VOID
FxIoTargetRemote::UnregisterForPnpNotification(
    _In_ MdTargetNotifyHandle Handle
    )
{
    if (Handle != NULL) {























        if (FxLibraryGlobals.IoUnregisterPlugPlayNotificationEx != NULL) {
            FxLibraryGlobals.IoUnregisterPlugPlayNotificationEx(Handle);
        }
        else {
            IoUnregisterPlugPlayNotification(Handle);
        }
    }
}

NTSTATUS
FxIoTargetRemote::OpenTargetHandle(
    _In_ PWDF_IO_TARGET_OPEN_PARAMS OpenParams,
    _Inout_ FxIoTargetRemoveOpenParams* pParams
    )
{
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    InitializeObjectAttributes(&oa,
                               &pParams->TargetDeviceName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateFile(&m_TargetHandle,
                          pParams->DesiredAccess,
                          &oa,
                          &ioStatus,
                          pParams->AllocationSizePointer,
                          pParams->FileAttributes,
                          pParams->ShareAccess,
                          pParams->CreateDisposition,
                          pParams->CreateOptions,
                          pParams->EaBuffer,
                          pParams->EaBufferLength);

    OpenParams->FileInformation = (ULONG)ioStatus.Information;

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIOTARGET,
            "ZwCreateFile for WDFIOTARGET %p returned status %!STATUS!, info 0x%x",
            GetObjectHandle(), status, (ULONG) ioStatus.Information);

        //
        // The open operation was successful. Dereference the file handle and
        // obtain a pointer to the device object for the handle.
        //
        status = ObReferenceObjectByHandle(
            m_TargetHandle,
            pParams->DesiredAccess,
            *IoFileObjectType,
            KernelMode,
            (PVOID*) &m_TargetFileObject,
            NULL);

        if (NT_SUCCESS(status)) {
            m_TargetDevice = IoGetRelatedDeviceObject(m_TargetFileObject);

            if (m_TargetDevice == NULL) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFIOTARGET %p, could not convert filobj %p to devobj",
                    GetObjectHandle(), m_TargetFileObject);

                status = STATUS_NO_SUCH_DEVICE;
            }
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFIOTARGET %p, could not convert handle %p to fileobject, "
                "status %!STATUS!",
                GetObjectHandle(), m_TargetHandle, status);
        }
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "ZwCreateFile for WDFIOTARGET %p returned status %!STATUS!, info 0x%x",
            GetObjectHandle(), status, (ULONG) ioStatus.Information);
    }

    return status;
}

NTSTATUS
FxIoTargetRemote::GetTargetDeviceRelations(
    _Inout_ BOOLEAN* Close
    )
{
    PDEVICE_OBJECT pTopOfStack;
    FxAutoIrp irp(NULL);
    PIRP pIrp;
    NTSTATUS status;

    pTopOfStack = IoGetAttachedDeviceReference(m_TargetDevice);

    pIrp = IoAllocateIrp(pTopOfStack->StackSize, FALSE);

    if (pIrp != NULL) {
        PIO_STACK_LOCATION stack;

        irp.SetIrp(pIrp);

        stack = irp.GetNextIrpStackLocation();
        stack->MajorFunction = IRP_MJ_PNP;
        stack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
        stack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

        //
        // Initialize the status to error in case the bus driver decides not
        // to set it correctly.
        //
        irp.SetStatus(STATUS_NOT_SUPPORTED);

        status = irp.SendIrpSynchronously(pTopOfStack);

        if (NT_SUCCESS(status)) {
            PDEVICE_RELATIONS pRelations;

            pRelations = (PDEVICE_RELATIONS) irp.GetInformation();

            ASSERT(pRelations != NULL);

            //
            // m_TargetPdo was referenced by the bus driver, it will be
            // dereferenced when the target is closed.
            //
            m_TargetPdo = pRelations->Objects[0];

            //
            // We, as the caller, are responsible for freeing the relations
            // that the bus driver allocated.
            //
            ExFreePool(pRelations);
        }
        else {
            //
            // Could not retrieve the PDO pointer, error handled later
            //
            DO_NOTHING();
        }
    }
    else {
        //
        // Could not even allocate an irp, failure.
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
        "Unable to allocate memory for IRP WDFIOTARGET %p, %!STATUS!",
        GetObjectHandle(), status);
    }

    //
    // Only fail the open if we cannot allocate an irp or if the lower
    // driver could not allocate a relations.
    //
    if (status == STATUS_INSUFFICIENT_RESOURCES) {
        *Close = TRUE;
    }
    else {
        status = STATUS_SUCCESS;
    }

    //
    // Remove the reference taken by IoGetAttachedDeviceReference
    //
    ObDereferenceObject(pTopOfStack);

    return status;
}

