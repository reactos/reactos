/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetRemoteUm.cpp

Abstract:

Author:

Environment:

    user mode only

Revision History:

--*/

#include "..\..\FxTargetsShared.hpp"

extern "C" {
#include "FxIoTargetRemoteUm.tmh"
}

#include <initguid.h>
#include "wdmguid.h"

NTSTATUS
FxIoTargetRemote::InitRemoteModeSpecific(
    __in FxDeviceBase* Device
    )
{
    NTSTATUS status;
    HRESULT hr;
    IWudfDeviceStack* devStack;

    devStack = Device->GetDeviceObject()->GetDeviceStackInterface();

    //
    // Event initialization can fail in UM so initialize it now instead of in
    // constructor.
    //
    status = m_OpenedEvent.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Failed to initialize m_OpenedEvent, %!STATUS!", status);
        return status;
    }

    //
    // Create remote dispatcher.
    // This calls directly into the Host to create.
    // For most IoTargets the dispatcher is hidden from the Fx, but
    // for the RemoteTarget, we need to directly dispatch I/O to
    // a win32 handle, regardless of what dispatch method the device
    // is set to use in it's INF.
    //
    hr = devStack->CreateRemoteDispatcher(&m_pIoDispatcher,
                                          &m_pRemoteDispatcher);

    if (FAILED(hr)) {
        status = FxDevice::NtStatusFromHr(devStack, hr);
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Failed to Create RemoteDispatcher, %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
FxIoTargetRemote::RemoveModeSpecific(
    VOID
    )

{
    //
    // Delete callback object
    //
    if (m_NotificationCallback != NULL) {
        delete m_NotificationCallback;
        m_NotificationCallback = NULL;
    }

    SAFE_RELEASE(m_pIoDispatcher);
    SAFE_RELEASE(m_pRemoteDispatcher);
}

NTSTATUS
FxIoTargetRemote::OpenTargetHandle(
    _In_ PWDF_IO_TARGET_OPEN_PARAMS OpenParams,
    _Inout_ FxIoTargetRemoveOpenParams* pParams
    )
{
    NTSTATUS status;
    HRESULT hr = S_OK;
    HANDLE hTarget;
    ULONG flagsAndAttributes;

    FX_VERIFY_WITH_NAME(INTERNAL,
              VERIFY(INVALID_HANDLE_VALUE == m_pRemoteDispatcher->GetHandle()),
              GetDriverGlobals()->Public.DriverName);

    //
    // UMDF 1.11 allowed following fields to be set by caller.
    //  DWORD dwDesiredAccess
    //  typedef struct _UMDF_IO_TARGET_OPEN_PARAMS
    //  {
    //      DWORD dwShareMode;   //
    //      DWORD dwCreationDisposition;
    //      DWORD dwFlagsAndAttributes;
    //  } UMDF_IO_TARGET_OPEN_PARAMS;
    //
    //
    // We always use overlapped I/O
    //
    flagsAndAttributes = pParams->FileAttributes | FILE_FLAG_OVERLAPPED;

    hTarget = CreateFile(pParams->TargetDeviceName.Buffer,
                         pParams->DesiredAccess,         // dwDesiredAccess
                         pParams->ShareAccess,           // dwShareMode
                         NULL,                           // lpSecurityAttributes
                         pParams->CreateDisposition,     // dwCreationDisposition
                         flagsAndAttributes,             // dwFlagsAndAttributes
                         NULL);

    if (INVALID_HANDLE_VALUE == hTarget) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        status = m_Device->NtStatusFromHr(hr);

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "CreateFile for WDFIOTARGET %p returned status %!STATUS!",
            GetObjectHandle(), status);

        FX_VERIFY_WITH_NAME(INTERNAL, VERIFY(FAILED(hr)), GetDriverGlobals()->Public.DriverName);
    }
    else {
        m_TargetHandle = hTarget;
        status = STATUS_SUCCESS;
    }

    return status;
}

HANDLE
FxIoTargetRemote::GetTargetHandle(
    VOID
    )
{
    HRESULT hrQi;
    IWudfFile2* pFile;
    HANDLE handle = m_TargetHandle;

    if (m_OpenParams.OpenType == WdfIoTargetOpenLocalTargetByFile) {
        if (m_TargetFileObject == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFIOTARGET %p has no target file object, could not get handle",
                GetObjectHandle());
        }
        else {
            ASSERT(m_TargetHandle == NULL);

            hrQi = m_TargetFileObject->QueryInterface(IID_IWudfFile2, (PVOID*)&pFile);
            FX_VERIFY(INTERNAL, CHECK_QI(hrQi, pFile));
            pFile->Release();

            handle = pFile->GetWeakRefHandle();
        }
    }

    //
    // Normalize the invalid handle value returned by CreateFile in host
    // to what's expected by the WdfIoTargetWdmGetTargetFileHandle caller.
    //
    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}

NTSTATUS
FxIoTargetRemote::BindToHandle(
    VOID
    )
{
    NTSTATUS status;
    HRESULT hr;

    //
    // Tell the RemoteDispatcher to bind to the new handle.
    //
    hr = m_pRemoteDispatcher->BindToHandle(m_TargetHandle);
    if (FAILED(hr)) {
        status = m_Device->NtStatusFromHr(hr);
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFIOTARGET %p could not bind remote dispatcher to new handle, "
            "%!STATUS!", GetObjectHandle(), status);
        return status;
    }

    status = STATUS_SUCCESS;
    return status;
}

void
FxIoTargetRemote::UnbindHandle(
    _In_ FxIoTargetClearedPointers* TargetPointers
    )
{
    if (NULL != m_pRemoteDispatcher) {
        //
        // Close the handle we gave to the RemoteDispatcher
        //
        // NOTE: IWudfRemoteDispatcher::CloseHandle can be safely called even if
        // we've not previously given it a handle. In this case, it does
        // nothing.
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p Unbinding RemoteDispatcher %p to handle %p on close",
            GetObjectHandle(), m_pRemoteDispatcher, TargetPointers->TargetHandle);

        m_pRemoteDispatcher->CloseHandle();

        //
        // Host closes the handle in CloseHandle call above so set the handle
        // in TargetPointers to NULL.
        //
        TargetPointers->TargetHandle = NULL;
    }
}

NTSTATUS
FxIoTargetRemote::GetTargetDeviceRelations(
    _Inout_ BOOLEAN* Close
    )
{
    UNREFERENCED_PARAMETER(Close);

    //
    // Not needed for UMDF
    //
    DO_NOTHING();

    return STATUS_SUCCESS;
}

NTSTATUS
FxIoTargetRemote::RegisterForPnpNotification(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HRESULT hr;
    FxIoTargetRemoteNotificationCallback* callback;

    UNREFERENCED_PARAMETER(hr);

    //
    // Allocate callback object
    //
    if (m_NotificationCallback == NULL) {
        callback = new (GetDriverGlobals())
            FxIoTargetRemoteNotificationCallback(GetDriverGlobals(), this);

        if (callback == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFIOTARGET %p could not allocate resources for "
                "notification registration, %!STATUS!",
                GetObjectHandle(), status);
            return status;
        }

        m_NotificationCallback = callback;
    }

    //
    // Register for Target Device Change notifications
    // These notifications will arrive asynchronously.
    //
    IWudfDeviceStack * pDevStack = m_Device->GetDeviceStack();

    hr = pDevStack->RegisterTargetDeviceNotification(
             static_cast<IWudfTargetCallbackDeviceChange *> (m_NotificationCallback),
             m_TargetHandle,
             &m_TargetNotifyHandle);

    if (FAILED(hr)) {
        if (m_NotificationCallback != NULL) {
            delete m_NotificationCallback;
            m_NotificationCallback = NULL;
        }

        status = m_Device->NtStatusFromHr(hr);
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFIOTARGET %p failed to register for Pnp notification, %!STATUS!",
            GetObjectHandle(), status);
        return status;
    }

    status = STATUS_SUCCESS;
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p registered for Pnp notification, %!STATUS!",
        GetObjectHandle(), status);

    return status;
}

VOID
FxIoTargetRemote::UnregisterForPnpNotification(
    _In_ MdTargetNotifyHandle NotifyHandle
    )
{
    //
    // check if we previously registered
    //
    if (NotifyHandle == WUDF_TARGET_CONTEXT_INVALID) {
        return;
    }

    //
    // Unregister.
    //
    IWudfDeviceStack * pDevStack = m_Device->GetDeviceStack();
    pDevStack->UnregisterTargetDeviceNotification(NotifyHandle);

}

NTSTATUS
FxIoTargetRemote::OpenLocalTargetByFile(
    _In_ PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    )
{
    NTSTATUS status;

    //
    // OpenParams must be non NULL b/c we can't reopen a target with a
    // previous device object.
    //
    ASSERT(OpenParams->Type == WdfIoTargetOpenLocalTargetByFile);
    m_OpenParams.OpenType = OpenParams->Type;

    //
    // Create a file object. This is UM-specific feature, where host opens
    // the reflector control device (optionally supplying the reference string
    // provided by caller). If there are lower device drivers in the um stack,
    // host sends them IRP_MJ_CREATE as well. The lower drivers in kernel see
    // IRP_MJ_CREATE as well as a result of opening the reflector control
    // object.
    // Note that m_TargetDevice is already set to next lower device during init.
    //
    status = CreateWdfFileObject(&OpenParams->FileName,
                                 &m_TargetFileObject);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Failed to create WDF File Object, %!STATUS!", status);
        return status;
    }

    //
    // Target handle is not used in this type of open.
    //
    m_TargetHandle = NULL;

    //
    // By taking a manual reference here, we simplify the code in
    // FxIoTargetRemote::Close where we can assume there is an outstanding
    // reference on the WDM file object at all times as long as we have a non
    // NULL pointer.
    //
    if (m_TargetFileObject != NULL) {
        Mx::MxReferenceObject(m_TargetFileObject);
    }

    return status;
}

NTSTATUS
FxIoTargetRemote::CreateWdfFileObject(
    _In_opt_ PUNICODE_STRING  FileName,
    _Out_ MdFileObject* FileObject
    )
{
    HRESULT hr = S_OK;
    NTSTATUS status;
    MdFileObject wdmFileObject = NULL;

    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(FileObject),
        GetDriverGlobals()->Public.DriverName);

    *FileObject = NULL;

    hr = m_Device->GetDeviceStack()->CreateWdfFile(
                            m_Device->GetDeviceObject(),
                            m_Device->GetAttachedDevice(),
                            FileName->Buffer,
                            &wdmFileObject
                            );
    if (SUCCEEDED(hr)) {
        *FileObject = wdmFileObject;
        status = STATUS_SUCCESS;
    }
    else {
        status = m_Device->NtStatusFromHr(hr);
    }

    return status;
}

VOID
FxIoTargetRemote::CloseWdfFileObject(
    _In_ MdFileObject FileObject
    )
{
   m_Device->GetDeviceStack()->CloseFile(FileObject);
   SAFE_RELEASE(FileObject);
}

BOOL
__stdcall
FxIoTargetRemoteNotificationCallback::OnQueryRemove(
    _In_ WUDF_TARGET_CONTEXT RegistrationID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pThis;
    NTSTATUS status;
    BOOLEAN bStatus;

    pThis = m_RemoteTarget;

    //
    // In one of these callbacks, the driver may decide to delete the target.
    // If that is the case, we need to be able to return and deref the object until
    // we are done.
    //
    pThis->ADDREF(m_RemoteTarget);

    pFxDriverGlobals = pThis->GetDriverGlobals();

    status = STATUS_SUCCESS;
    bStatus = TRUE;

    if (GetRegistrationId() != RegistrationID) {
        //
        // By design, we can get notification callbacks even after we have
        // unregistered for notifications. This can happen if there were
        // callbacks already in flight before we unregistered. In this case, we
        // simply succeed on query-remove. Since we have already unregistered,
        // there is no reason for us to fail query-remove.
        //
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIOTARGET,
                    "QueryRemove callback was for an old registration, ignoring.");

        bStatus = TRUE;
        goto exit;
    }

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p: query remove notification",
        m_RemoteTarget->GetObjectHandle());

    //
    // Device is gracefully being removed.  PnP is asking us to close down
    // the target.  If there is a driver callback, there is *no* default
    // behavior.  This is because we don't know what the callback is going
    // to do.  For instance, the driver could reopen the target to a
    // different device in a multi-path scenario.
    //
    if (pThis->m_EvtQueryRemove.m_Method != NULL) {
        status = pThis->m_EvtQueryRemove.Invoke(
            pThis->GetHandle());
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

    if (!NT_SUCCESS(status)) {
        bStatus = FALSE;
    }

exit:

    pThis->RELEASE(m_RemoteTarget);

    return bStatus;
}

VOID
__stdcall
FxIoTargetRemoteNotificationCallback::OnRemoveComplete(
    _In_ WUDF_TARGET_CONTEXT RegistrationID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pThis;

    pThis = m_RemoteTarget;

    //
    // In one of these callbacks, the driver may decide to delete the target.
    // If that is the case, we need to be able to return and deref the object until
    // we are done.
    //
    pThis->ADDREF(m_RemoteTarget);

    pFxDriverGlobals = pThis->GetDriverGlobals();

    if (GetRegistrationId() != RegistrationID) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIOTARGET,
                    "RemoveComplete callback was for an old registration, ignoring.");

        goto exit;
    }

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

exit:

    pThis->RELEASE(m_RemoteTarget);
}

VOID
__stdcall
FxIoTargetRemoteNotificationCallback::OnRemoveCanceled(
    _In_ WUDF_TARGET_CONTEXT RegistrationID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pThis;
    NTSTATUS status;

    pThis = m_RemoteTarget;

    //
    // In one of these callbacks, the driver may decide to delete the target.
    // If that is the case, we need to be able to return and deref the object until
    // we are done.
    //
    pThis->ADDREF(m_RemoteTarget);

    pFxDriverGlobals = pThis->GetDriverGlobals();
    status = STATUS_SUCCESS;

    if (GetRegistrationId() != RegistrationID) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIOTARGET,
                    "RemoveCanceled callback was for an old registration, ignoring.");

        goto exit;
    }

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







        UNREFERENCED_PARAMETER(status);
    }

exit:

    pThis->RELEASE(m_RemoteTarget);
}

VOID
__stdcall
FxIoTargetRemoteNotificationCallback::OnCustomEvent(
    _In_ WUDF_TARGET_CONTEXT  RegistrationID,
    _In_ REFGUID Event,
    _In_reads_bytes_(DataSize) BYTE * Data,
    _In_ DWORD DataSize,
    _In_ DWORD NameBufferOffset
    )
{
    UNREFERENCED_PARAMETER(RegistrationID);
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(DataSize);
    UNREFERENCED_PARAMETER(NameBufferOffset);

    //
    // UMDF 2.0 doesn't yet support custom event. Ignore the event.
    //
    DO_NOTHING();

    return;
}

