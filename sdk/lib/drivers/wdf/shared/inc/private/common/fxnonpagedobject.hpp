/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxNonPagedObject.hpp

Abstract:

    This module defines the abstract FxNonPagedObject class.

Author:



Environment:

    Both kernel and user mode

Revision History:


        Made mode agnostic

        IMPORTANT: Common code must call Initialize method of
        FxNonPagedObject before using it

        Cannot put CreateAndInitialize method on this class as it
        cannot be instantiated

--*/

#ifndef _FXNONPAGEDOBJECT_H_
#define _FXNONPAGEDOBJECT_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxNonPagedObject.hpp.tmh"
#endif

}

class FxNonPagedObject : public FxObject
{
private:

    MxLock  m_NPLock;

public:
    FxNonPagedObject(
        __in WDFTYPE Type,
        __in USHORT Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxObject(Type, Size, FxDriverGlobals)
    {
        if (IsDebug()) {
            if (GetDriverGlobals()->FxVerifierLock) {
                //
                // VerifierLock CreateAndInitialize failure is not fatal,
                // we just won't track anything
                //
                FxVerifierLock * verifierLock = NULL;
                (void) FxVerifierLock::CreateAndInitialize(&verifierLock,
                                                    GetDriverGlobals(),
                                                    this);
                GetDebugExtension()->VerifierLock = verifierLock;
            }
        }
    }

    FxNonPagedObject(
        __in WDFTYPE Type,
        __in USHORT Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxObjectType ObjectType
        ) :
        FxObject(Type, Size, FxDriverGlobals, ObjectType)
    {
        if (IsDebug()) {
            if (GetDriverGlobals()->FxVerifierLock) {
                //
                // VerifierLock CreateAndInitialize failure is not fatal,
                // we just won't track anything
                //
                FxVerifierLock * verifierLock = NULL;
                (void) FxVerifierLock::CreateAndInitialize(&verifierLock,
                                                    GetDriverGlobals(),
                                                    this);
                GetDebugExtension()->VerifierLock = verifierLock;
            }
        }
    }

    virtual
    ~FxNonPagedObject(
        VOID
        )
    {
        if (IsDebug()) {
            FxObjectDebugExtension* pExtension;

            pExtension = GetDebugExtension();

            if (pExtension->VerifierLock != NULL) {
                delete pExtension->VerifierLock;
                pExtension->VerifierLock = NULL;
            }
        }
    }

    _Acquires_lock_(this->m_NPLock.m_Lock)
    __drv_maxIRQL(DISPATCH_LEVEL)
    __drv_setsIRQL(DISPATCH_LEVEL)
    VOID
    Lock(
        __out __drv_deref(__drv_savesIRQL) PKIRQL PreviousIrql
        )
    {
        if (IsDebug()) {
            FxObjectDebugExtension* pExtension;

            pExtension = GetDebugExtension();

            if (pExtension->VerifierLock != NULL) {
                pExtension->VerifierLock->Lock(PreviousIrql, FALSE);
                //
                // return here so that we don't acquire the non verified lock
                // below
                //
                return;
            }
        }

        m_NPLock.Acquire(PreviousIrql);
    }

    _Releases_lock_(this->m_NPLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    Unlock(
        __in __drv_restoresIRQL KIRQL PreviousIrql
        )
    {
        if (IsDebug()) {
            FxObjectDebugExtension* pExtension;

            pExtension = GetDebugExtension();

            if (pExtension->VerifierLock != NULL) {
                pExtension->VerifierLock->Unlock(PreviousIrql, FALSE);

                //
                // return here so that we don't release the non verified lock
                // below
                //
                return;
            }
        }

        m_NPLock.Release(PreviousIrql);
    }

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE

    _Acquires_lock_(this->m_NPLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    VOID
    LockAtDispatch(
        VOID
        )
    {
        if (IsDebug()) {
            FxObjectDebugExtension* pExtension;

            pExtension = GetDebugExtension();

            if (pExtension->VerifierLock != NULL) {
                KIRQL previousIrql;

                pExtension->VerifierLock->Lock(&previousIrql, TRUE);

                ASSERT(previousIrql == DISPATCH_LEVEL);
                //
                // return here so that we don't acquire the non verified lock
                // below
                //
                return;
            }
        }

        m_NPLock.AcquireAtDpcLevel();
    }

    _Requires_lock_held_(this->m_NPLock.m_Lock)
    _Releases_lock_(this->m_NPLock.m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    UnlockFromDispatch(
        VOID
        )
    {
        if (IsDebug()) {
            FxObjectDebugExtension* pExtension;

            pExtension = GetDebugExtension();

            if (pExtension->VerifierLock != NULL) {
                pExtension->VerifierLock->Unlock(DISPATCH_LEVEL, TRUE);

                //
                // return here so that we don't acquire the non verified lock
                // below
                //
                return;
            }
        }

        m_NPLock.ReleaseFromDpcLevel();
    }

#endif   //FX_CORE_MODE==FX_CORE_KERNEL_MODE
};

#endif // _FXNONPAGEDOBJECT_H_
