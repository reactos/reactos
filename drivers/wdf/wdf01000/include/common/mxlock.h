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

};    

#endif //_MXLOCK_H_