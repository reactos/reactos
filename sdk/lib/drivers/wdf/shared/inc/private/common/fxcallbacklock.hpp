/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCallbackLock.hpp

Abstract:

    This is the C++ header for the FxCallbackLock

    This represents an abstract class for implementing
    the locks used when we callback into the device driver.

Author:




Revision History:


        Made it mode agnostic

--*/

#ifndef _FXCALLBACKLOCK_H_
#define _FXCALLBACKLOCK_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxCallbackLock.hpp.tmh"
#endif

}

//
// Callback locks track current owner. This is used to determine if a callback
// event into the driver needs to be deferred to prevent recursive
// locking.
//
// A Callback lock supports recursive locking, but this is not exposed in
// the current Driver Frameworks <-> Device Driver interactions, and results
// in a verifier assert.
//

class FxCallbackLock : public FxGlobalsStump {

protected:
    MxThread         m_OwnerThread;
    ULONG            m_RecursionCount;

    // For Verifier
    FxVerifierLock*  m_Verifier;

public:







    KIRQL m_PreviousIrql;

public:

    FxCallbackLock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxGlobalsStump(FxDriverGlobals)
    {
        m_OwnerThread = NULL;
        m_RecursionCount = 0;
        m_Verifier = NULL;
    }

    virtual ~FxCallbackLock()
    {
    }

    virtual
    void
    Initialize(
        FxObject* ParentObject
        ) = 0;

    virtual
    void
    Lock(
        __out PKIRQL PreviousIrql
        ) = 0;

    virtual
    void
    Unlock(
        __in KIRQL PreviousIrql
        ) = 0;

    // TRUE if the current thread is the owner
    _Must_inspect_result_
    virtual
    BOOLEAN
    IsOwner(
        VOID
        ) = 0;

    VOID
    CheckOwnership(
        VOID
        )
    {
        PFX_DRIVER_GLOBALS pFxDriverGlobals;

        pFxDriverGlobals = GetDriverGlobals();

        //
        // If verify locks is on, catch drivers
        // returning with the lock released.
        //
        if (pFxDriverGlobals->FxVerifierLock) {
            if (IsOwner() == FALSE) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Callback: Driver released the callback lock 0x%p",
                    this);
                FxVerifierDbgBreakPoint(pFxDriverGlobals);
            }
        }
    }
};


































































































































































































































































































































































































































































































































#endif // _FXCALLBACKLOCK_H_
