/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetRemote.cpp

Abstract:

Author:

Environment:

    Both kernel and user mode

Revision History:

--*/

#include "..\FxTargetsShared.hpp"

extern "C" {
#include "FxIoTargetRemote.tmh"
}

#include <initguid.h>
#include "wdmguid.h"


FxIoTargetRemote::FxIoTargetRemote(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxIoTarget(FxDriverGlobals, sizeof(FxIoTargetRemote)),
    m_EvtQueryRemove(FxDriverGlobals),
    m_EvtRemoveCanceled(FxDriverGlobals),
    m_EvtRemoveComplete(FxDriverGlobals)
{

    //
    // No automatic state changes based on the pnp state changes of our own
    // device stack.  The one exception is remove where we need to shut
    // everything down.
    //
    m_InStack = FALSE;

    m_ClearedPointers = NULL;
    m_OpenState = FxIoTargetRemoteOpenStateClosed;

    m_TargetHandle = NULL;

    m_EvtQueryRemove.m_Method = NULL;
    m_EvtRemoveCanceled.m_Method = NULL;
    m_EvtRemoveComplete.m_Method = NULL;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_TargetNotifyHandle = NULL;
#else (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_TargetNotifyHandle = WUDF_TARGET_CONTEXT_INVALID;

    m_pIoDispatcher = NULL;
    m_pRemoteDispatcher = NULL;
    m_NotificationCallback = NULL;
#endif
}

FxIoTargetRemote::~FxIoTargetRemote()
{
}

_Must_inspect_result_
NTSTATUS
FxIoTargetRemote::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in CfxDeviceBase* Device,
    __out FxIoTargetRemote** Target
    )
{
    FxIoTargetRemote* pTarget;
    FxObject* pParent;
    WDFOBJECT hTarget;
    NTSTATUS status;

    *Target = NULL;

    if (Attributes == NULL || Attributes->ParentObject == NULL) {
        pParent = Device;
    }
    else {
        CfxDeviceBase* pSearchDevice;

        FxObjectHandleGetPtr(FxDriverGlobals,
                             Attributes->ParentObject,
                             FX_TYPE_OBJECT,
                             (PVOID*) &pParent);

        pSearchDevice = FxDeviceBase::_SearchForDevice(pParent, NULL);

        if (pSearchDevice == NULL) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Attributes->ParentObject 0x%p must have WDFDEVICE as an "
                "eventual ancestor, %!STATUS!",
                Attributes->ParentObject, status);

            return status;
        }
        else if (pSearchDevice != Device) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Attributes->ParentObject 0x%p ancestor is WDFDEVICE %p, but "
                "not the same WDFDEVICE 0x%p passed to WdfIoTargetCreate, "
                "%!STATUS!",
                Attributes->ParentObject, pSearchDevice->GetHandle(),
                Device->GetHandle(), status);

            return status;
        }
    }

    pTarget = new (FxDriverGlobals, Attributes)
        FxIoTargetRemote(FxDriverGlobals);

    if (pTarget == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate memory for target, %!STATUS!", status);
        return status;
    }

    //
    // initialize the new target
    //
    status = pTarget->InitRemote(Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Commit and apply the attributes
    //
    status = pTarget->Commit(Attributes, &hTarget, pParent);

    if (NT_SUCCESS(status)) {
        *Target = pTarget;
    }
    else {
        //
        // This will properly clean up the target's state and free it
        //
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Commit failed for target, %!STATUS!", status);
        pTarget->DeleteFromFailedCreate();
    }

    return status;
}

NTSTATUS
FxIoTargetRemote::InitRemote(
    __in FxDeviceBase* Device
    )
{
    NTSTATUS status;

    //
    // do the base class mode-specific initialization
    //
    status = __super::InitModeSpecific(Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Do mode-specific initilialization
    //
    status = InitRemoteModeSpecific(Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    m_Driver = Device->GetDriver();

    SetDeviceBase(Device);
    m_InStackDevice = Device->GetDeviceObject();

    (void) Device->AddIoTarget(this);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxIoTargetRemote::Open(
    __in PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    )
{
    FxIoTargetRemoveOpenParams params, *pParams;
    UNICODE_STRING name;
    LIST_ENTRY pended;
    WDF_IO_TARGET_OPEN_TYPE type;
    NTSTATUS status;
    BOOLEAN close, reopen;
    PVOID pEa;
    ULONG eaLength;
    KIRQL irql;

    RtlZeroMemory(&name, sizeof(name));
    close = FALSE;
    reopen = OpenParams->Type == WdfIoTargetOpenReopen ? TRUE : FALSE;

    pEa = NULL;
    eaLength = 0;

    //
    // We only support reopening using stored settings when we open by name
    //
    if (reopen && m_OpenParams.OpenType != WdfIoTargetOpenByName) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Reopen only supported if the open type is WdfIoTargetOpenByName WDFIOTARGET %p %!STATUS!",
            GetObjectHandle(), status);
        return status;
    }

    //
    // Must preallocate all settings now
    //
    if (reopen) {
        //
        // convert the type into the type used for the previous open
        //
        type = m_OpenParams.OpenType;
        pParams = &m_OpenParams;
    }
    else {
        type = OpenParams->Type;
        pParams = &params;

        if (OpenParams->Type == WdfIoTargetOpenByName) {

            status = FxDuplicateUnicodeString(GetDriverGlobals(),
                                              &OpenParams->TargetDeviceName,
                                              &name);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not allocate memory for target name for WDFIOTARGET %p",
                    GetObjectHandle());
                goto Done;
            }
            if (OpenParams->EaBuffer != NULL && OpenParams->EaBufferLength > 0) {

                pEa = FxPoolAllocate(GetDriverGlobals(),
                                     PagedPool,
                                     OpenParams->EaBufferLength);

                if (pEa == NULL) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                        "Could not allocate memory for target name "
                        "for WDFIOTARGET %p", GetObjectHandle());
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Done;
                }
                else {
                    eaLength = OpenParams->EaBufferLength;
                    RtlCopyMemory(pEa, OpenParams->EaBuffer, eaLength);
                }
            }
        }
    }

    Lock(&irql);

    if (m_State == WdfIoTargetDeleted) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Opening WDFIOTARGET %p which is removed, state %d",
            GetObjectHandle(), m_State);
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (m_OpenState != FxIoTargetRemoteOpenStateClosed) {
        //
        // We are either open or are opening
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Opening an already open WDFIOTARGET %p, open state %d",
            GetObjectHandle(), m_OpenState);
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Opening WDFIOTARGET %p", GetObjectHandle());

        //
        // Clear the event so that if something is waiting on the state
        // transition, they will block until we are done.
        //
        m_OpenedEvent.Clear();

        m_OpenState = FxIoTargetRemoteOpenStateOpening;
        status = STATUS_SUCCESS;
    }
    Unlock(irql);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    ASSERT(m_TargetFileObject == NULL);
    ASSERT(m_TargetDevice == NULL);
    ASSERT(m_TargetPdo == NULL);
    ASSERT(m_TargetHandle == NULL);

    //
    // m_TargetNotifyHandle can be a valid value if the caller has previously
    // opened the target, received a query remove, then a cancel remove, and
    // is now reopening the target.
    //
    UnregisterForPnpNotification(m_TargetNotifyHandle);
    ResetTargetNotifyHandle();

    //
    // Only clear the open parameters if we are not attempting a reopen.
    //
    if (reopen == FALSE) {
        m_OpenParams.Clear();
    }

    switch (type) {
    case WdfIoTargetOpenUseExistingDevice:
        KMDF_ONLY_CODE_PATH_ASSERT();

        //
        // OpenParams must be non NULL b/c we can't reopen a target with a
        // previous device object.
        //
        ASSERT(OpenParams->Type == WdfIoTargetOpenUseExistingDevice);

        m_TargetDevice = (MdDeviceObject) OpenParams->TargetDeviceObject;
        m_TargetFileObject = (MdFileObject) OpenParams->TargetFileObject;
        m_TargetHandle = NULL;

        //
        // By taking a manual reference here, we simplify the code in
        // FxIoTargetRemote::Close where we can assume there is an outstanding
        // reference on the PFILE_OBJECT at all times as long as we have a non
        // NULL pointer.
        //
        if (m_TargetFileObject != NULL) {
            Mx::MxReferenceObject(m_TargetFileObject);
        }

        status = STATUS_SUCCESS;

        break;

    case WdfIoTargetOpenLocalTargetByFile:
        UMDF_ONLY_CODE_PATH_ASSERT();

        status = OpenLocalTargetByFile(OpenParams);
        break;

    case WdfIoTargetOpenByName:
        //
        // Only capture the open parameters if we are not reopening.
        //
        if (reopen == FALSE) {
            pParams->Set(OpenParams, &name, pEa, eaLength);
        }

        status = OpenTargetHandle(OpenParams, pParams);
        if (NT_SUCCESS(status)) {
            if (reopen == FALSE) {
                m_OpenParams.Set(OpenParams, &name, pEa, eaLength);

                //
                // Setting pEa to NULL stops it from being freed later.
                // Zeroing out name stops it from being freed later.
                //
                pEa = NULL;
                RtlZeroMemory(&name, sizeof(name));
            }
        }
        else {
            close = TRUE;
        }
        break;
    }

    InitializeListHead(&pended);

    //
    // Get Target file object for KMDF. Noop for UMDF.
    //
    if (NT_SUCCESS(status)) {
        status = GetTargetDeviceRelations(&close);
    }

    if (NT_SUCCESS(status) && CanRegisterForPnpNotification()) {
        if (reopen == FALSE) {
            //
            // Set the values before the register so that if a notification
            // comes in before the register returns, we have a function to call.
            //
            m_EvtQueryRemove.m_Method = OpenParams->EvtIoTargetQueryRemove;
            m_EvtRemoveCanceled.m_Method = OpenParams->EvtIoTargetRemoveCanceled;
            m_EvtRemoveComplete.m_Method = OpenParams->EvtIoTargetRemoveComplete;
        }

        status = RegisterForPnpNotification();

        //
        // Even if we can't register, we still are successful in opening
        // up the device and we will proceed from there.
        //
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "WDFIOTARGET %p, could not register pnp notification, %!STATUS! not "
                "treated as an error", GetObjectHandle(), status);

            m_EvtQueryRemove.m_Method = NULL;
            m_EvtRemoveCanceled.m_Method = NULL;
            m_EvtRemoveComplete.m_Method = NULL;

            status = STATUS_SUCCESS;
        }
    }

    //
    // UMDF only. Bind handle to remote dispatcher.
    //
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    if (NT_SUCCESS(status) && type != WdfIoTargetOpenLocalTargetByFile) {
        status = BindToHandle();
        if (!NT_SUCCESS(status)) {
            close = TRUE;
        }
    }
#endif

    Lock(&irql);

    if (NT_SUCCESS(status)) {

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        m_TargetStackSize = m_TargetDevice->StackSize;
        m_TargetIoType = GetTargetIoType();
#endif

        m_OpenState = FxIoTargetRemoteOpenStateOpen;

        //
        // Set our state to started.  This will also resend any pended requests
        // due to a query remove.
        //
        status = GotoStartState(&pended, FALSE);

        //
        // We could not successfully start, close back down
        //
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "WDFIOTARGET %p could not transition to started state, %!STATUS!",
                GetObjectHandle(), status);

            close = TRUE;
        }
    }
    else {
        m_OpenState = FxIoTargetRemoteOpenStateClosed;
    }

    //
    // No matter what, indicate to any waiters that our state change has
    // completed.
    //
    m_OpenedEvent.Set();

    Unlock(irql);

Done:
    //
    // Resubmit any reads that were pended until now.
    //
    if (NT_SUCCESS(status)) {
        SubmitPendedRequests(&pended);
    }
    else if (close) {
        Close(FxIoTargetRemoteCloseReasonPlainClose);
    }

    if (name.Buffer != NULL) {
        FxPoolFree(name.Buffer);
    }

    if (pEa != NULL) {
        FxPoolFree(pEa);
    }

    return status;
}


VOID
FxIoTargetRemote::Close(
    __in FxIoTargetRemoteCloseReason Reason
    )
{
    FxIoTargetClearedPointers pointers;
    MdTargetNotifyHandle pNotifyHandle;
    SINGLE_LIST_ENTRY sent;
    LIST_ENTRY pended;
    WDF_IO_TARGET_STATE removeState;
    KIRQL irql;
    BOOLEAN wait;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "enter:  WDFIOTARGET %p, reason %d", GetObjectHandle(), Reason);

    RtlZeroMemory(&pointers, sizeof(pointers));
    pNotifyHandle = NULL;

    sent.Next = NULL;
    InitializeListHead(&pended);

    wait = FALSE;

    //
    // Pick a value that is not used anywhere in the function and make sure that
    // we have changed it, before we go to the Remove state
    //
#pragma prefast(suppress: __WARNING_UNUSED_SCALAR_ASSIGNMENT, "PFD is warning that the following assignement is unused. Suppress it to prevent changing any logic.")
    removeState = WdfIoTargetStarted;

CheckState:
    Lock(&irql);

    //
    // If we are in the process of opening the target, wait for that to finish.
    //
    if (m_OpenState == FxIoTargetRemoteOpenStateOpening) {
        Unlock(irql);

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Closing WDFIOTARGET %p which is opening, waiting on event %p",
            GetObjectHandle(), m_OpenedEvent.GetEvent());

        m_OpenedEvent.EnterCRAndWaitAndLeave();

        //
        // Jump back to the top and recheck
        //
        goto CheckState;
    }

    if (Reason == FxIoTargetRemoteCloseReasonDelete) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Closing WDFIOTARGET %p, reason:   delete", GetObjectHandle());

        removeState = WdfIoTargetDeleted;
    }
    else if (m_OpenState == FxIoTargetRemoteOpenStateOpen) {
        if (Reason == FxIoTargetRemoteCloseReasonQueryRemove) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "Closing WDFIOTARGET %p, reason:   query remove",
                GetObjectHandle());
            //
            // Not really being removed, but that is what the API name is...
            //
            removeState = WdfIoTargetClosedForQueryRemove;
        }
        else {

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "Closing WDFIOTARGET %p, reason:   close", GetObjectHandle());

            if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
                removeState = WdfIoTargetClosed;
            }
            else {
                removeState = WdfIoTargetClosedForQueryRemove;
            }
        }

        //
        // Either way, we are no longer open for business
        //
        m_OpenState = FxIoTargetRemoteOpenStateClosed;
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Closing WDFIOTARGET %p which is not open", GetObjectHandle());

        //
        // We are not opened, so treat this as a cleanup
        //
        removeState = WdfIoTargetClosed;
    }

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p:  fileobj %p, devobj %p, handle %p, notify handle %I64d",
        GetObjectHandle(), m_TargetFileObject,
        m_TargetDevice, m_TargetHandle, (UINT64)m_TargetNotifyHandle);

    if (Reason != FxIoTargetRemoteCloseReasonQueryRemove) {
        //
        // If we are closing for a query remove, we want to keep the handle
        // around so that we can be notified of the final close or if the close
        // was canceled.
        //
        pNotifyHandle = m_TargetNotifyHandle;
        ResetTargetNotifyHandle();
    }

    ASSERT(removeState != WdfIoTargetStarted);
    m_ClearedPointers =  &pointers;
    GotoRemoveState(removeState, &pended, &sent, FALSE, &wait);

    Unlock(irql);

    UnregisterForPnpNotification(pNotifyHandle);

    //
    // Complete any requests we might have pulled off of our lists
    //
    CompletePendedRequestList(&pended);
    _CancelSentRequests(&sent);

    //
    // We were just removed, wait for any I/O to complete back if necessary.
    //
    if (wait) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFIOTARGET %p, waiting for stop to complete", GetObjectHandle());

        WaitForSentIoToComplete();
    }

    switch (Reason) {
    case FxIoTargetRemoteCloseReasonQueryRemove:
        //
        // m_OpenParams is needed for reopen on canceled query remove
        //
        DO_NOTHING();
        break;

    case FxIoTargetRemoteCloseReasonDelete:
        m_OpenParams.Clear();
        break;

    default:
        //
        // If this object is not about to be deleted, we need to revert some
        // of the state that just changed.
        //
        m_SentIoEvent.Clear();
        break;
    }

    if (removeState == WdfIoTargetDeleted) {
       WaitForDisposeEvent();
    }

    //
    // Finally, close down our handle and pointers
    //
    if (pointers.TargetPdo != NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p derefing PDO %p on close",
            GetObjectHandle(), pointers.TargetPdo);

        Mx::MxDereferenceObject(pointers.TargetPdo);
    }

    if (pointers.TargetFileObject != NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p derefing FileObj %p on close",
            GetObjectHandle(), pointers.TargetFileObject);
        Mx::MxDereferenceObject(pointers.TargetFileObject);

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        CloseWdfFileObject(pointers.TargetFileObject);
#endif
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        UnbindHandle(&pointers);
#endif

    if (pointers.TargetHandle != NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p closing handle %p on close",
            GetObjectHandle(), pointers.TargetHandle);
        Mx::MxClose(pointers.TargetHandle);
    }
}

VOID
FxIoTargetRemote::ClearTargetPointers(
    VOID
    )
{
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
        "WDFIOTARGET %p cleared pointers %p state %!WDF_IO_TARGET_STATE!,"
        " open state %d, pdo %p, fileobj %p, handle %p",
        GetObjectHandle(), m_ClearedPointers, m_State, m_OpenState, m_TargetPdo,
        m_TargetFileObject, m_TargetHandle);

    //
    // Check to see if the caller who is changing state wants these pointer
    // values before they being cleared out.
    //
    if (m_ClearedPointers != NULL) {
        m_ClearedPointers->TargetPdo = m_TargetPdo;
        m_ClearedPointers->TargetFileObject = m_TargetFileObject;
        m_ClearedPointers->TargetHandle = m_TargetHandle;
        m_ClearedPointers = NULL;
    }

    //
    // m_TargetHandle is only an FxIoTargetRemote field, clear it now
    //
    m_TargetHandle = NULL;

    //
    // m_TargetPdo and m_TargetFileObject will be cleared in the following call.
    //
    // m_TargetNotifyHandle is not cleared in the following call and is left
    // valid because we want to receive the notification about query remove being
    // canceled or completing.  When we receive either of those notifications,
    // m_TargetNotifyHandle will be freed then.
    //
    __super::ClearTargetPointers();
}

VOID
FxIoTargetRemote::Remove(
    VOID
    )
{
    //
    // Close is the same as remove in this object
    //
    Close(FxIoTargetRemoteCloseReasonDelete);

    //
    // Do mode-specific work
    //
    RemoveModeSpecific();

    return ;
}

VOID
FxIoTargetRemoveOpenParams::Clear(
    VOID
    )
{
    if (EaBuffer != NULL) {
        FxPoolFree(EaBuffer);
    }

    if (TargetDeviceName.Buffer != NULL) {
        FxPoolFree(TargetDeviceName.Buffer);
    }

    RtlZeroMemory(this, sizeof(FxIoTargetRemoveOpenParams));
}

VOID
FxIoTargetRemoveOpenParams::Set(
    __in PWDF_IO_TARGET_OPEN_PARAMS OpenParams,
    __in PUNICODE_STRING Name,
    __in PVOID Ea,
    __in ULONG EaLength
    )
{
    OpenType = WdfIoTargetOpenByName;

    EaBuffer = Ea;
    EaBufferLength = EaLength;

    RtlCopyMemory(&TargetDeviceName, Name, sizeof(UNICODE_STRING));

    DesiredAccess = OpenParams->DesiredAccess;
    FileAttributes = OpenParams->FileAttributes;
    ShareAccess = OpenParams->ShareAccess;
    CreateDisposition = OpenParams->CreateDisposition;
    CreateOptions = OpenParams->CreateOptions;

    if (OpenParams->AllocationSize != NULL) {
        AllocationSize.QuadPart = *(OpenParams->AllocationSize);
        AllocationSizePointer = &AllocationSize;
    }
    else {
        AllocationSizePointer = NULL;
    }
}
