/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgGeneral.hpp

Abstract:

    This module implements a package to handle general dispath entry points
    such as IRP_MJ_CREATE and IRP_MJ_CLOSE.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPKGGENERAL_H_
#define _FXPKGGENERAL_H_

#include  "FxFileObjectCallbacks.hpp"

class FxShutDown :  public FxCallback {

public:
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION  m_Method;

    FxShutDown(
        VOID
        ) :
        FxCallback(),
        m_Method(NULL)
    {
    }

    VOID
    Invoke(
        __in WDFDEVICE Device
        )
    {
        if (m_Method != NULL) {
            CallbackStart();
            m_Method(Device);
            CallbackEnd();
        }
    }
};

#define FX_PKG_GENERAL_FLAG_CX_INFO         0x00000001
#define FX_PKG_GENERAL_FLAG_CLIENT_INFO     0x00000002
#define FX_PKG_GENERAL_FLAG_CX_CREATE       0x00000004
#define FX_PKG_GENERAL_FLAG_CLIENT_CREATE   0x00000008

#define FX_PKG_GENERAL_FLAG_CREATE \
    (FX_PKG_GENERAL_FLAG_CX_CREATE | FX_PKG_GENERAL_FLAG_CLIENT_CREATE)


class FxPkgGeneral : public FxPackage {

private:

    //
    // FileObject attributes
    //
    LONG                    m_OpenHandleCount;

    //
    // List of file objects info (driver and cx).
    //
    LIST_ENTRY              m_FileObjectInfoHeadList;

    FxIoQueue*              m_DefaultQueueForCreates;
    FxIoQueue*              m_DriverCreatedQueue;

    //
    // Generic file object flags.
    //
    ULONG                   m_Flags;

    //
    // Execution and synchronization for cx and client driver.
    //
    WDF_EXECUTION_LEVEL     m_ExecutionLevel;
    WDF_SYNCHRONIZATION_SCOPE m_SynchronizationScope;

    //
    // This pointer allows the proper lock to be acquired
    // based on the configuration with a minimal of runtime
    // checks. This is configured by ConfigureConstraints().
    // We basically inherit device's lock.
    //
    FxCallbackLock*         m_CallbackLockPtr;
    FxObject*               m_CallbackLockObjectPtr;

    FxShutDown              m_EvtDeviceShutdown;

private:
    _Must_inspect_result_
    NTSTATUS
    OnCreate(
        __inout FxIrp* FxIrp
        );

    _Must_inspect_result_
    NTSTATUS
    OnClose(
        __inout FxIrp* FxIrp
        );

    BOOLEAN
    AcquireRemoveLockForClose(
        __inout FxIrp* FxIrp
        );

    _Must_inspect_result_
    NTSTATUS
    OnCleanup(
        __inout FxIrp* FxIrp
        );

    _Must_inspect_result_
    NTSTATUS
    OnShutdown(
        __inout FxIrp* FxIrp
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureConstraints(
        __in PLIST_ENTRY FileObjInfoList
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureFileObjectClass(
        __in PLIST_ENTRY FileObjInfoList
        );

    VOID
    DecrementOpenHandleCount(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    ForwardCreateRequest(
        __in FxIrp* FxIrp,
        __in MdCompletionRoutine CompletionRoutine,
        __in PVOID  Context
        );

    static
    MdCompletionRoutineType _CreateCompletionRoutine;

    static
    MdCompletionRoutineType _CreateCompletionRoutine2;

public:

    FxPkgGeneral(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device
        );

    ~FxPkgGeneral();

    _Must_inspect_result_
    virtual
    NTSTATUS
    Dispatch(
        __inout MdIrp Irp
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    PostCreateDeviceInitialize(
        __in PWDFDEVICE_INIT Init
        );

    VOID
    CreateCompleted(
        __in FxIrp *Irp
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureForwarding(
        __in FxIoQueue *FxQueue
        );

    BOOLEAN
    CanDestroyControlDevice(
        VOID
        );

    VOID
    GetConstraintsHelper(
        __out_opt WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out_opt WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) ;

    FxCallbackLock*
    GetCallbackLockPtrHelper(
        __deref_out_opt FxObject** LockObject
        );

    __inline
    FxIoQueue*
    GetDeafultInternalCreateQueue(
        )
    {
        return m_DefaultQueueForCreates;
    }
};

#endif // _FXPKGGENERAL_H_
