/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriver.hpp

Abstract:

    This is the definition of the FxDriver object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDRIVER_H_
#define _FXDRIVER_H_

#include "FxDriverCallbacks.hpp"


//
// Unique value to retrieve the FxDriver* from the PDEVICE_OBJECT.
//
#define FX_TRACE_INFO_ID   (FxDriver::_GetTraceInfoExtension)

//
// Structure to hold WMI Callback info
//
struct FxTraceInfo {
    MdDriverObject           DriverObject;
    PFN_WDF_TRACE_CALLBACK   Callback;
    PVOID                    Context;
};

//
// Unique value to retrieve the FxDriver* from the MdDriverObject.  Use a value
// that is not exposed to the driver writer through the dispatch table or WDM.
//
#define FX_DRIVER_ID (FxDriver::GetFxDriver)

//
// The following are support classes for FxDriver
//
class FxDriver : public FxNonPagedObject, public IFxHasCallbacks
{
friend class FxDevice;
friend class FxPackage;
friend class FxWmiIrpHandler;

private:

    MxDriverObject m_DriverObject;
    UNICODE_STRING m_RegistryPath;

    BOOLEAN m_DebuggerConnected;

    //
    // Callbacks to device driver
    //
    FxDriverDeviceAdd m_DriverDeviceAdd;

    //
    // This represents any constraints on callbacks
    // to the device driver that may be inherited
    // by child devices and their objects
    //
    WDF_EXECUTION_LEVEL       m_ExecutionLevel;
    WDF_SYNCHRONIZATION_SCOPE m_SynchronizationScope;

    //
    // Frameworks objects that raise event callbacks into the device
    // driver provide spinlock and mutex based callback locks
    // to allow proper synchronization between the driver and
    // these callbacks.
    //
    // Some events must be passive level, while others at dispatch
    // level, thus the need for two locks.
    //
    // The objects internal state is protected by the FxNonPagedObject
    // lock inherited by the object, and is different from the callback
    // locks.
    //
    FxCallbackMutexLock m_CallbackMutexLock;

    //
    // These pointers allow the proper lock to be acquired
    // based on the configuration with a minimal of runtime
    // checks. This is configured by ConfigureConstraints()
    //
    FxCallbackLock* m_CallbackLockPtr;
    FxObject*       m_CallbackLockObjectPtr;

    //
    // This is the Driver-wide configuration
    //
    WDF_DRIVER_CONFIG m_Config;

    //
    // Deferred Disposal List
    //
    FxDisposeList*    m_DisposeList;

#if FX_IS_USER_MODE
    //
    // A handle to the driver service parameters key.
    // The framework does not have permission to open it with
    // write access from user mode, so we keep a pre-opened one.
    //
    HKEY m_DriverParametersKey;
#endif

private:

    static
    MdDriverAddDeviceType AddDevice;

public:

    // This is public to allow the C function FxCoreDriverUnload to call it
    FxDriverUnload              m_DriverUnload;

    FxDriver(
        __in MdDriverObject DriverObject,
        __in PWDF_DRIVER_CONFIG DriverConfig,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxDriver();

    static
    VOID
    _InitializeDriverName(
        __in PFX_DRIVER_GLOBALS Globals,
        __in PCUNICODE_STRING RegistryPath
        );

    static
    VOID
    _InitializeTag(
        __in PFX_DRIVER_GLOBALS Globals,
        __in PWDF_DRIVER_CONFIG Config
        );

    static
    FxDriver*
    GetFxDriver(
        __in MdDriverObject DriverObject
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateDriverObjectExtensionAndStoreFxDriver(
        VOID
        );



































    __inline
    WDFDRIVER
    GetHandle(
        VOID
        )
    {
        return (WDFDRIVER) GetObjectHandle();
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)

    _Must_inspect_result_
    NTSTATUS
    AddDevice(
        __in MdDeviceObject PhysicalDeviceObject
        );

#else

    _Must_inspect_result_
    NTSTATUS
    FxDriver::AddDevice(
        _In_  IWudfDeviceStack *        DevStack,
        _In_  LPCWSTR                   KernelDeviceName,
        _In_opt_ HKEY                   PdoKey,
        _In_  LPCWSTR                   ServiceName,
        _In_  LPCWSTR                   DevInstanceID,
        _In_  ULONG                     DriverID
        );
#endif

    VOID
    InitializeInternal(
        VOID
        );

    _Must_inspect_result_
    FxString *
    GetRegistryPath(
        VOID
        );

    PUNICODE_STRING
    GetRegistryPathUnicodeString(
        VOID
        )
    {
        return &m_RegistryPath;
    }

    __inline
    MdDriverObject
    GetDriverObject(
        VOID
        )
    {
        return m_DriverObject.GetObject();
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PCUNICODE_STRING RegistryPath,
        __in PWDF_DRIVER_CONFIG Config,
        __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
        );

    //
    // The following methods support the callback constraints
    // and handle locking and deferral
    //
    VOID
    ConfigureConstraints(
        __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
        );

    //
    // IFxHasCallbacks Support
    //

    virtual
    VOID
    GetConstraints(
        __out WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) {

        if (ExecutionLevel != NULL) {
            *ExecutionLevel = m_ExecutionLevel;
        }

        if (SynchronizationScope != NULL) {
            *SynchronizationScope = m_SynchronizationScope;
        }
    }

    virtual
    FxCallbackLock*
    GetCallbackLockPtr(
        __deref_out FxObject** LockObject
        ) {

        if (LockObject != NULL) {
            *LockObject = m_CallbackLockObjectPtr;
        }

        return m_CallbackLockPtr;
    }

    //
    // IFxAssociation Support
    //
    virtual
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        )
    {
        switch (Params->Type) {
        case FX_TYPE_DRIVER:
            *Params->Object = (FxDriver*) this;
            break;

        default:
            return __super::QueryInterface(Params);
        }

        return STATUS_SUCCESS;
    }

    virtual
    VOID
    DeleteObject(
        VOID
        )
    {
        //
        // If diposed at > PASSIVE, we will cause a deadlock in FxDriver::Dispose
        // when we call into  the dispose list to wait for empty when we are in
        // the context of the dispose list's work item.
        //
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        __super::DeleteObject();
    }

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    __inline
    FxDisposeList*
    GetDisposeList(
        )
    {
        return m_DisposeList;
    }

    __inline
    PFN_WDF_DRIVER_DEVICE_ADD
    GetDriverDeviceAddMethod(
        )
    {
        return m_DriverDeviceAdd.Method;
    }

    static
    MdDriverUnloadType Unload;

#if FX_IS_USER_MODE
private:

    //
    // Open the handle to the driver service parameters key
    // that we keep opened for use from user mode.
    //
    NTSTATUS
    OpenParametersKey(
        VOID
        );

    VOID
    ClearDriverObjectFxDriver(
        VOID
        );

public:

    __inline
    HKEY
    GetDriverParametersKey(
        VOID
        )
    {
        return m_DriverParametersKey;
    }
#endif

#if (FX_CORE_MODE == FX_CORE_USER_MODE)

    VOID
    SetDriverObjectFlag(
        _In_ FxDriverObjectUmFlags Flag
        )
    {
        m_DriverObject.SetDriverObjectFlag(Flag);
    }

    BOOLEAN
    IsDriverObjectFlagSet(
        _In_ FxDriverObjectUmFlags Flag
        )
    {
        return m_DriverObject.IsDriverObjectFlagSet(Flag);
    }

#endif

};

#endif // _FXDRIVER_H_
