
/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    IFxHasCallbacks.hpp

Abstract:

    Interface that objects with device driver callbacks and
    constraints implement

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef __IFX_HASCALLBACKS_HPP
#define __IFX_HASCALLBACKS_HPP

//
// Objects that have callbacks into the device driver
// (implemented by deriving a delegate from FxCallback), may
// have constraints specified by the device driver when the object
// or driver was created.
//
// These constraints control synchronization, and execution (IRQL) level
// on callbacks to the device driver.
//
// To provide synchronization an FxCallbackLock may be implemented, at
// either DISPATCH (spinlock) or PASSIVE (mutex lock) level depending
// on whether a passive level constraint is in effect.
//
// This interface allows the constraints, and any associated locks
// to be retrieved in a generic fashion from the object, such as
// for implementing the WdfObjectAcquireLock/WdfObjectReleaseLock APIs.
//

class IFxHasCallbacks {

public:

    //
    // Returns the constraints in effect for the object.
    //
    virtual
    VOID
    GetConstraints(
        __out WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) = 0;

    //
    // This returns the callback lock in effect for the object that
    // will serialize with its event callbacks to the device driver.
    //
    // If no callback locks are in effect, the return value is NULL.
    //
    // The type of FxCallbackLock returned may be a spinlock, or mutex
    // type depending on whether the object has a passive level callback
    // constraint in effect.
    //
    // In addition, optionally returns the object that contains the lock
    // providing any serialization constraint. This allows reference counting
    // the object who owns the lock, which may not be the target
    // object. An example would be a child object sharing its parent
    // FxDevice's synchronziation lock.
    //
    _Must_inspect_result_
    virtual
    FxCallbackLock*
    GetCallbackLockPtr(
        __out_opt FxObject** LockObject
        ) = 0;
};

#endif // __IFX_HASCALLBACKS_HPP


