#ifndef _FXDRIVER_H_
#define _FXDRIVER_H_

#include "common/fxnonpagedobject.h"
#include "common/mxgeneral.h"
#include "common/fxcallbacklock.h"
#include "common/fxdisposelist.h"
#include "common/mxgeneral.h"
#include "common/mxdriverobject.h"
#include "common/fxcallbackmutexlock.h"
#include "wdf.h"


// forward definitions
typedef struct _FX_DRIVER_GLOBALS *PFX_DRIVER_GLOBALS;

//
// The following are support classes for FxDriver
//
class FxDriver : public FxNonPagedObject {

private:
    
    MxDriverObject m_DriverObject;
    UNICODE_STRING m_RegistryPath;

    BOOLEAN m_DebuggerConnected;

    //
    // Callbacks to device driver
    //
    //FxDriverDeviceAdd m_DriverDeviceAdd;

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

public:

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
    
};

#endif //_FXDRIVER_H_