#ifndef _FXDRIVER_H_
#define _FXDRIVER_H_

#include "common/fxnonpagedobject.h"
#include "common/mxgeneral.h"
#include "common/fxcallbacklock.h"
#include "common/fxdisposelist.h"
#include "common/mxgeneral.h"
#include "common/mxdriverobject.h"
#include "common/fxcallbackmutexlock.h"
#include "common/fxdrivercallbacks.h"
#include "wdf.h"


// forward definitions
typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;

//
// Unique value to retrieve the FxDriver* from the MdDriverObject.  Use a value
// that is not exposed to the driver writer through the dispatch table or WDM.
//
#define FX_DRIVER_ID (FxDriver::GetFxDriver)

//
// The following are support classes for FxDriver
//
class FxDriver : public FxNonPagedObject {

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

    static
    MdDriverAddDeviceType AddDevice;

public:

    // This is public to allow the C function FxCoreDriverUnload to call it
    FxDriverUnload              m_DriverUnload;

    static
    MdDriverUnloadType Unload;

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

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PCUNICODE_STRING RegistryPath,
        __in PWDF_DRIVER_CONFIG Config,
        __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
        );

    __inline
    FxDisposeList*
    GetDisposeList(
        )
    {
        return m_DisposeList;
    }

    //
    // The following methods support the callback constraints
    // and handle locking and deferral
    //
    VOID
    ConfigureConstraints(
        __in_opt PWDF_OBJECT_ATTRIBUTES DriverAttributes
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateDriverObjectExtensionAndStoreFxDriver(
        VOID
        );

    static
    FxDriver*
    GetFxDriver(
        __in MdDriverObject DriverObject
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
    AddDevice(
        _In_  IWudfDeviceStack *        DevStack,
        _In_  LPCWSTR                   KernelDeviceName,
        _In_opt_ HKEY                   PdoKey,
        _In_  LPCWSTR                   ServiceName,
        _In_  LPCWSTR                   DevInstanceID,
        _In_  ULONG                     DriverID
        );

    #endif

    __inline
    MdDriverObject
    GetDriverObject(
        VOID
        )
    {
        return m_DriverObject.GetObject();
    }

    virtual
    VOID
    GetConstraints(
        __out WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) {

        if (ExecutionLevel != NULL)
        {
            *ExecutionLevel = m_ExecutionLevel;
        }

        if (SynchronizationScope != NULL)
        {
            *SynchronizationScope = m_SynchronizationScope;
        }
    }

    __inline
    PFN_WDF_DRIVER_DEVICE_ADD
    GetDriverDeviceAddMethod(
        )
    {
        return m_DriverDeviceAdd.Method;
    }
    
};


#endif //_FXDRIVER_H_