#ifndef _MXLOCK_H_
#define _MXLOCK_H_

#include <ntddk.h>
#include "dbgmacros.h"

typedef KSPIN_LOCK MdLock;

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
MxLockNoDynam()
{
    CLEAR_DBGFLAG_INITIALIZED;

    Initialize();
}

__inline
VOID
Initialize()
{
    KeInitializeSpinLock(&m_Lock);

    SET_DBGFLAG_INITIALIZED;
}

__inline
VOID
Uninitialize()
{
    CLEAR_DBGFLAG_INITIALIZED;
}

_Acquires_lock_(this->m_Lock)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_setsIRQL(DISPATCH_LEVEL)
__inline
VOID
Acquire(
    __out __drv_deref(__drv_savesIRQL) KIRQL * OldIrql
    )
{
    ASSERT_DBGFLAG_INITIALIZED;
    
    KeAcquireSpinLock(&m_Lock, OldIrql);
}

_Releases_lock_(this->m_Lock)
__drv_requiresIRQL(DISPATCH_LEVEL)
__inline
VOID
Release(
    __drv_restoresIRQL KIRQL NewIrql
    )
{
    ASSERT_DBGFLAG_INITIALIZED;

    KeReleaseSpinLock(&m_Lock, NewIrql);    
}

};

class MxLock : public MxLockNoDynam
{
public:    
    __inline
    MxLock();
    
    __inline
    ~MxLock();
};


#endif //_MXLOCK_H_