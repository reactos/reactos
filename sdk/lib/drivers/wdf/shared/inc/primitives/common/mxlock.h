/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxLock.h

Abstract:

    Mode agnostic definition of lock

    See MxLockKm.h and MxLockUm.h for mode
    specific implementations

Author:



Revision History:



--*/

#pragma once

//
// MxLockNoDynam has no c'tor/d'tor
// so as to be usable in global structs in km
//
// MxLock dervies from it and adds c'tor/d'tor
//
class MxLockNoDynam
{

DECLARE_DBGFLAG_INITIALIZED;

protected:
    MdLock m_Lock;
public:
    MdLock &
    Get(
        )
    {
        return m_Lock;
    }

    __inline
    VOID
    Initialize(
        );

    _Acquires_lock_(this->m_Lock)
    __drv_maxIRQL(DISPATCH_LEVEL)
    __drv_setsIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    Acquire(
        __out __drv_deref(__drv_savesIRQL) KIRQL * OldIrql
        );

#if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))

    CHECK_RETURN_IF_USER_MODE
    __inline
    BOOLEAN
    TryToAcquire(
        VOID
        );
#endif

    _Acquires_lock_(this->m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    AcquireAtDpcLevel(
        );

    _Releases_lock_(this->m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    Release(
        __drv_restoresIRQL KIRQL NewIrql
        );

    _Releases_lock_(this->m_Lock)
    __drv_requiresIRQL(DISPATCH_LEVEL)
    __inline
    VOID
    ReleaseFromDpcLevel(
        );

    __inline
    VOID
    Uninitialize(
        );
};

class MxLock : public MxLockNoDynam
{
public:
    __inline
    MxLock();

    __inline
    ~MxLock();
};
