#include "common/fxdevice.h"
#include "common/fxdriver.h"
#include "common/fxcallbackmutexlock.h"
#include "common/fxcallbackspinlock.h"
#include "common/fxqueryinterface.h"


NTSTATUS
FxDeviceBase::ConfigureConstraints(
    __in_opt PWDF_OBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS            status;
    WDF_EXECUTION_LEVEL driverLevel;
    WDF_SYNCHRONIZATION_SCOPE driverScope;

    ASSERT(m_Driver != NULL);

    //
    // If WDF_OBJECT_ATTRIBUTES is specified, these override any
    // default settings.
    //
    if (ObjectAttributes != NULL)
    {
        m_ExecutionLevel = ObjectAttributes->ExecutionLevel;
        m_SynchronizationScope = ObjectAttributes->SynchronizationScope;
    }

    //
    // If no WDFDEVICE specific attributes are specified, we
    // get them from WDFDRIVER, which allows WDFDRIVER to
    // provide a default for all WDFDEVICE's created.
    //
    m_Driver->GetConstraints(&driverLevel, &driverScope);

    if (m_ExecutionLevel == WdfExecutionLevelInheritFromParent)
    {
        m_ExecutionLevel = driverLevel;
    }

    if (m_SynchronizationScope == WdfSynchronizationScopeInheritFromParent)
    {
        m_SynchronizationScope = driverScope;
    }

    //
    // Configure The Execution Level Constraint
    //
    if (m_ExecutionLevel == WdfExecutionLevelPassive)
    {
        m_CallbackLockPtr = new(GetDriverGlobals()) 
                                FxCallbackMutexLock(GetDriverGlobals());
        //
        // Currently, all event callbacks from FxDevice into the driver
        // are from PASSIVE_LEVEL threads, and there is no need to defer
        // to a system workitem like IoQueue. So we don't allocate and
        // setup an FxSystemWorkItem here.
        //
        // If the FxDevice starts raising events to the device driver
        // whose thread starts out above PASSIVE_LEVEL, then an FxSystemWorkItem
        // would need to be allocated when WdfExecutionLevelPassive is specified.
        //
        // (FDO and PDO variants of FxDevice may own their own event dispatch
        //  and deferral logic separate from FxDevice.)
        //
    }
    else
    {
        m_CallbackLockPtr  = new(GetDriverGlobals()) 
                                FxCallbackSpinLock(GetDriverGlobals());
    }

    //
    // Finish initializing the spin/mutex lock.
    //
    if (NULL == m_CallbackLockPtr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGOBJECT,
            "WDFDEVICE %p, could not allocate callback lock, %!STATUS!",
            GetHandle(), status);
        goto Done;
    }
    
    m_CallbackLockPtr->Initialize(this);
    m_CallbackLockObjectPtr = this;

    status = STATUS_SUCCESS;
    
Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxDeviceBase::AllocateTarget(
    _Out_ FxIoTarget** Target,
    _In_  BOOLEAN SelfTarget
    )
/*++

Routine Description:

    Allocates an IO Target or an Self IO target for the FxDevice.    

Arguments:

    Target - Out - returns the pointer to the allocated IO target.
 
    SelfTarget - If TRUE allocates an Self IO Target, if FALSE allocates
        a regular IO target. 

Returns:

    NTSTATUS    

--*/
{
    FxIoTarget* pTarget;
    NTSTATUS status;

    if (SelfTarget)
    {
        pTarget = (FxIoTarget*) new(GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
            FxIoTargetSelf(GetDriverGlobals(), sizeof(FxIoTargetSelf));
    }
    else
    {
        pTarget = new(GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
            FxIoTarget(GetDriverGlobals(), sizeof(FxIoTarget));
    }

    if (pTarget == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p could not allocate a WDFIOTARGET, %!STATUS!", 
            GetHandle(), status);

        goto Done;
    }
        
    status = AddIoTarget(pTarget);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p failed to initialize (add) a WDFIOTARGET, %!STATUS!", 
            GetHandle(), status);

        goto Done;
    }

    status = pTarget->Init(this);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p failed to initialize a WDFIOTARGET, %!STATUS!", 
            GetHandle(), status);

        goto Done;
    }

    status = pTarget->Commit(WDF_NO_OBJECT_ATTRIBUTES, NULL, this);
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p failed to initialize (commit) a WDFIOTARGET, %!STATUS!", 
            GetHandle(), status);
        
        goto Done;
    }

    status = STATUS_SUCCESS;
    
Done:
    if (!NT_SUCCESS(status))
    {
        if (pTarget != NULL) {
            pTarget->DeleteFromFailedCreate();
            pTarget = NULL;
        }
    }

    *Target = pTarget;

    return status;
}

VOID
FxDeviceBase::GetConstraints(
    __out_opt WDF_EXECUTION_LEVEL*       ExecutionLevel,
    __out_opt WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
    )
{
    if (ExecutionLevel != NULL)
    {
        *ExecutionLevel = m_ExecutionLevel;
    }

    if (SynchronizationScope != NULL)
    {
        *SynchronizationScope = m_SynchronizationScope;
    }
}

FxCallbackLock*
FxDeviceBase::GetCallbackLockPtr(
    __out_opt FxObject** LockObject
    )
{
    if (LockObject != NULL)
    {
        *LockObject = m_CallbackLockObjectPtr;
    }

    return m_CallbackLockPtr;
}

FxDeviceBase::FxDeviceBase(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxDriver* Driver,
    __in WDFTYPE Type,
    __in USHORT Size
    ) :
    FxNonPagedObject(Type, Size, FxDriverGlobals)
{
    m_Driver = Driver;

    m_CallbackLockPtr = NULL;
    m_CallbackLockObjectPtr = NULL;

    m_DisposeList = NULL;

    m_DmaPacketTransactionStatus = FxDmaPacketTransactionCompleted;

    m_ExecutionLevel = WdfExecutionLevelInheritFromParent;
    m_SynchronizationScope = WdfSynchronizationScopeInheritFromParent;

    MarkPassiveDispose(ObjectDoNotLock);
    SetDeviceBase(this);
}

FxDeviceBase::~FxDeviceBase(
    VOID
    )
{
    if (m_DisposeList != NULL)
    {
        m_DisposeList->DeleteObject();
        m_DisposeList = NULL;
    }

    if (m_CallbackLockPtr != NULL)
    {
        delete m_CallbackLockPtr;
        m_CallbackLockPtr = NULL;
    }
}

FxDeviceBase*
FxDeviceBase::_SearchForDevice(
    __in FxObject* Object,
    __out_opt IFxHasCallbacks** Callbacks
    )
{
    FxObject* pParent, *pOrigParent;
    FxDeviceBase* pDeviceBase;
    FxQueryInterfaceParams cbParams = { (PVOID*) Callbacks, FX_TYPE_IHASCALLBACKS, 0 };
    PVOID pTag;

    pDeviceBase = Object->GetDeviceBase();
    if (pDeviceBase == NULL)
    {
        DoTraceLevelMessage(
            Object->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGOBJECT,
            "WDFHANDLE %p does not have a WDFDEVICE as an ancestor",
            Object->GetObjectHandle());
        return NULL;
    }

    if (Callbacks == NULL)
    {
        //
        // Caller is not interested in the callbacks interface, just return
        // now without searching for it.
        //
        return pDeviceBase;
    }
    
    //
    // Init out parameter.
    //
    *Callbacks = NULL;

    pOrigParent = Object;
    pTag = pOrigParent;

    //
    // By adding a reference now, we simulate what GetParentObjectReferenced
    // does later, thus allowing simple logic on when/how to release the
    // reference on exit.
    //
    Object->ADDREF(pTag);

    do
    {
        //
        // If successful, Callbacks will be != NULL
        //
        if (NT_SUCCESS(Object->QueryInterface(&cbParams)))
        {
            ASSERT(*Callbacks != NULL);
            //
            // Release the reference previously taken by the top of the function
            // or GetParentObjectReferenced in a previous pass in the loop.
            //
            Object->RELEASE(pTag);
            return pDeviceBase;
        }

        pParent = Object->GetParentObjectReferenced(pTag);

        //
        // Release the reference previously taken by the top of the function
        // or GetParentObjectReferenced in a previous pass in the loop.
        //
        Object->RELEASE(pTag);

        Object = pParent;
    } while (Object != NULL);

    ASSERT(Object == NULL);
    
    //
    // Queue presented requests do not have parents (to increase performance).
    // Try to find the callback interface on this object's device base.
    //
    if (NT_SUCCESS(pDeviceBase->QueryInterface(&cbParams)))
    {
        ASSERT(*Callbacks != NULL);
        //
        // Success, we got a callback interface.
        //
        return pDeviceBase;
    }
    
    DoTraceLevelMessage(
        pOrigParent->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGOBJECT,
        "WDFHANDLE %p does not have a callbacks interface in its object tree"
        "(WDFDEVICE %p)", pOrigParent->GetObjectHandle(),
        pDeviceBase->GetHandle());

    return pDeviceBase;
}

_Must_inspect_result_
NTSTATUS
FxDeviceBase::QueryInterface(
    __inout FxQueryInterfaceParams* Params
    )
{
    switch (Params->Type) {
    case FX_TYPE_DEVICE_BASE:
        *Params->Object = this;
        break;

    case FX_TYPE_IHASCALLBACKS:
        *Params->Object = (IFxHasCallbacks*) this;
        break;

    default:
        return __super::QueryInterface(Params);
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxDeviceBase::QueryForInterface(
    __in const GUID* InterfaceType,
    __out PINTERFACE Interface,
    __in USHORT Size,
    __in USHORT Version,
    __in PVOID InterfaceSpecificData,
    __in_opt MdDeviceObject TargetDevice
    )
/*++

Routine Description:
    Send an IRP_MJPNP/IRP_MN_QUERY_INTERFACE irp to a device object and its
    attached stack.

Arguments:
    InterfaceType - The type of interface to query for

    Interface - The interface to fill out

    Size - Size of Interface in bytes

    Version - Version of the interface to be queried

    InterfaceSpecificData - Addtional interface data to be queried

    TargetDevice - device in the stack to send the query to.  If NULL, the top
                   of the stack will receive the query.

Return Value:
    NTSTATUS as indicated by the handler of the QI with in the device stack,
    STATUS_NOT_SUPPORTED if the QI is not handled.

  --*/
{
    PDEVICE_OBJECT pTopOfStack;
    NTSTATUS status;

    //
    // PnP rules dictate you send the QI through the entire stack and not just
    // the stack below you...but we let the caller override this.  There are
    // some stacks which are not PnP reentrant, so sending a QI from a lower
    // filter might cause the stack to stop responding if the FDO is synchronously 
    // sending a PnP irp down the stack already.
    //
    if (TargetDevice == NULL)
    {
        pTopOfStack = GetAttachedDeviceReference();
    }
    else
    {
        //
        // To make the exit logic simpler below, just add our own reference.
        //
        Mx::MxReferenceObject(TargetDevice);
        pTopOfStack = TargetDevice;
    }

    status = FxQueryInterface::_QueryForInterface(
        pTopOfStack,
        InterfaceType,
        Interface,
        Size,
        Version,
        InterfaceSpecificData
        );

    Mx::MxDereferenceObject(pTopOfStack);

    return status;
}
