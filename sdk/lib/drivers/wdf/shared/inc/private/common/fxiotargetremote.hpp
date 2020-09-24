/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxIoTargetRemote.hpp

Abstract:

    Derivation of FxIoTarget specializing in targets remote to this device
    stack.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXIOTARGETREMOTE_H_
#define _FXIOTARGETREMOTE_H_

enum FxIoTargetRemoteCloseReason {
    FxIoTargetRemoteCloseReasonQueryRemove = 1,
    FxIoTargetRemoteCloseReasonPlainClose,
    FxIoTargetRemoteCloseReasonDelete
};

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
typedef PVOID               MdTargetNotifyHandle;
#else
typedef WUDF_TARGET_CONTEXT MdTargetNotifyHandle;
#endif

class FxIoTargetRemoteNotificationCallback;

struct FxIoTargetQueryRemove : public FxCallback {

    FxIoTargetQueryRemove(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    _Must_inspect_result_
    NTSTATUS
    Invoke(
        __in WDFIOTARGET IoTarget
        )
    {
        NTSTATUS status;

        CallbackStart();
        status = m_Method(IoTarget);
        CallbackEnd();

        return status;
    }

    PFN_WDF_IO_TARGET_QUERY_REMOVE  m_Method;
};

struct FxIoTargetRemoveCanceled : public FxCallback {

    FxIoTargetRemoveCanceled(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFIOTARGET Target
        )
    {
        CallbackStart();
        m_Method(Target);
        CallbackEnd();
    }

    PFN_WDF_IO_TARGET_REMOVE_CANCELED  m_Method;
};

struct FxIoTargetRemoveComplete : public FxCallback {

    FxIoTargetRemoveComplete(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallback(FxDriverGlobals),
        m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFIOTARGET Target
        )
    {
        CallbackStart();
        m_Method(Target);
        CallbackEnd();
    }

    PFN_WDF_IO_TARGET_REMOVE_COMPLETE m_Method;
};

enum FxIoTargetRemoteOpenState {
    FxIoTargetRemoteOpenStateClosed = 1,
    FxIoTargetRemoteOpenStateOpening,
    FxIoTargetRemoteOpenStateOpen,
};

struct FxIoTargetRemoveOpenParams {

    FxIoTargetRemoveOpenParams()
    {
        RtlZeroMemory(this, sizeof(FxIoTargetRemoveOpenParams));
    }

    VOID
    Set(
        __in PWDF_IO_TARGET_OPEN_PARAMS OpenParams,
        __in PUNICODE_STRING Name,
        __in PVOID Ea,
        __in ULONG EaLength
        );

    VOID
    Clear(
        VOID
        );

    UNICODE_STRING TargetDeviceName;

    WDF_IO_TARGET_OPEN_TYPE OpenType;

    ACCESS_MASK DesiredAccess;

    ULONG ShareAccess;

    ULONG FileAttributes;

    ULONG CreateDisposition;

    ULONG CreateOptions;

    __field_bcount(EaBufferLength) PVOID EaBuffer;

    ULONG EaBufferLength;

    LARGE_INTEGER AllocationSize;

    PLARGE_INTEGER AllocationSizePointer;

};

struct FxIoTargetClearedPointers {
    MdDeviceObject TargetPdo;
    MdFileObject TargetFileObject;
    HANDLE TargetHandle;
};

class FxIoTargetRemote : public FxIoTarget {

public:

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDF_OBJECT_ATTRIBUTES Attributes,
        __in FxDeviceBase* Device,
        __out FxIoTargetRemote** Target
        );

    ~FxIoTargetRemote();

    NTSTATUS
    InitRemote(
        __in FxDeviceBase* Device
        );

    NTSTATUS
    InitRemoteModeSpecific(
        __in FxDeviceBase* Device
        );

    _Must_inspect_result_
    NTSTATUS
    Open(
        __in PWDF_IO_TARGET_OPEN_PARAMS OpenParams
        );

    VOID
    Close(
        __in FxIoTargetRemoteCloseReason Reason
        );

    NTSTATUS
    GetTargetDeviceRelations(
        _Out_ BOOLEAN* Close
        );

    BOOLEAN
    CanRegisterForPnpNotification(
        VOID
        )
    {
        BOOLEAN canRegister = FALSE;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        if (m_TargetFileObject != NULL) {
            canRegister = TRUE;
        }
#else // FX_CORE_USER_MODE
        if (m_TargetHandle != NULL) {
            canRegister = TRUE;
        }
#endif
        return canRegister;
    }

    VOID
    ResetTargetNotifyHandle(
        VOID
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_TargetNotifyHandle = NULL;
#else // FX_CORE_USER_MODE
    m_TargetNotifyHandle = WUDF_TARGET_CONTEXT_INVALID;
#endif
    }

    NTSTATUS
    OpenTargetHandle(
        _In_ PWDF_IO_TARGET_OPEN_PARAMS OpenParams,
        _Inout_ FxIoTargetRemoveOpenParams* pParams
        );

    VOID
    CloseTargetHandle(
        VOID
        );

    HANDLE
    GetTargetHandle(
        VOID
        );

    NTSTATUS
    RegisterForPnpNotification(
        VOID
        );

    VOID
    UnregisterForPnpNotification(
        _In_ MdTargetNotifyHandle Handle
        );

    __inline
    WDFIOTARGET
    GetHandle(
        VOID
        )
    {
        return (WDFIOTARGET) GetObjectHandle();
    }

    virtual
    VOID
    Remove(
        VOID
        );

    VOID
    RemoveModeSpecific(
        VOID
        );

protected:
    FxIoTargetRemote(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    VOID
    ClearTargetPointers(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        )
    {
        if (Params->Type == FX_TYPE_IO_TARGET_REMOTE) {
            *Params->Object = (FxIoTargetRemote*) this;
            return STATUS_SUCCESS;
        }
        else {
            return __super::QueryInterface(Params);
        }
    }

    _Must_inspect_result_
    NTSTATUS
    FxIoTargetRemote::OpenLocalTargetByFile(
        _In_ PWDF_IO_TARGET_OPEN_PARAMS OpenParams
        );

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    static
    DRIVER_NOTIFICATION_CALLBACK_ROUTINE
    _PlugPlayNotification;

#else // FX_CORE_USER_MODE
    //
    // I/O dispatcher to be used for IRPs forwarded to this remote target. It is
    // created when the CWdfRemoteTarget is created. The win32 handle is
    // associated with it via a call to m_pRemoteDispatcher->BindToHandle()
    // right after we call CreateFile(...). We must call
    // m_pRemoteDispatcher->CloseHandle() to close the handle.
    //
    // Because of the plug-in pattern of the IoDispatcher, we need two
    // interface pointers (one to the outer object, and one to the plug-in.
    //
    IWudfIoDispatcher     * m_pIoDispatcher;
    IWudfRemoteDispatcher * m_pRemoteDispatcher;

    //
    // Implements host's callback interface for pnp notification
    //
    FxIoTargetRemoteNotificationCallback* m_NotificationCallback;

    VOID
    Forward(
        _In_ MdIrp Irp
        )
    {
        if (m_OpenParams.OpenType == WdfIoTargetOpenLocalTargetByFile) {
            //
            // Ignore the return value because once we have sent the request, we
            // want all processing to be done in the completion routine.
            //
            (void) Irp->Forward();
        }
        else {
            IWudfIoIrp* pSubmitIrp = FxIrp(Irp).GetIoIrp();

            //
            // Move the stack location to the next location
            //
            pSubmitIrp->SetNextIrpStackLocation();

            //
            // Route it using Remote dispatcher
            //
            m_pIoDispatcher->Dispatch(pSubmitIrp, NULL);
        }
    }

private:

    NTSTATUS
    BindToHandle(
        VOID
        );

    VOID
    UnbindHandle(
        _In_ FxIoTargetClearedPointers* TargetPointers
        );

    NTSTATUS
    CreateWdfFileObject(
        _In_opt_ PUNICODE_STRING  FileName,
        _Out_ MdFileObject* FileObject
        );

    VOID
    CloseWdfFileObject(
        _In_ MdFileObject FileObject
        );

#endif // FX_CORE_USER-MODE)

public:
    //
    // File handle for m_TargetHandle
    //
    HANDLE m_TargetHandle;

    //
    // Notification handle returned by IoRegisterPlugPlayNotification for KMDF,
    // or host's notification registartion interface for UMDf. Note that host
    // uses the term RegistrationId for the same (with WUDF_CONTEXT_TYPE which
    // is UINT64).
    //
    MdTargetNotifyHandle m_TargetNotifyHandle;

    //
    // Driver writer callbacks to indicate state changes
    //
    FxIoTargetQueryRemove m_EvtQueryRemove;
    FxIoTargetRemoveCanceled m_EvtRemoveCanceled;
    FxIoTargetRemoveComplete m_EvtRemoveComplete;

    FxCREvent m_OpenedEvent;

    FxIoTargetClearedPointers* m_ClearedPointers;

    //
    // Value from FxIoTargetRemoteOpenState
    //
    UCHAR m_OpenState;

protected:
    FxIoTargetRemoveOpenParams m_OpenParams;
};

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "FxIoTargetRemoteKm.hpp"
#else
#include "FxIoTargetRemoteUm.hpp"
#endif

#endif // _FXIOTARGETREMOTE_H_
