#ifndef _MXPAGEDLOCK_H_
#define _MXPAGEDLOCK_H_

#include "common/dbgmacros.h"

typedef FAST_MUTEX MdPagedLock;

class MxPagedLockNoDynam
{
protected:
    MdPagedLock m_Lock;

public:

    __inline
    VOID
    Uninitialize()
    {
        CLEAR_DBGFLAG_INITIALIZED;
    }

    __drv_maxIRQL(APC_LEVEL) 
    __drv_setsIRQL(APC_LEVEL)
    //__drv_savesIRQLGlobal(FastMutexObject, this->m_Lock) 
    _Acquires_lock_(this->m_Lock)
    __inline
    VOID
    Acquire()
    {
        ASSERT_DBGFLAG_INITIALIZED;
    
        ExAcquireFastMutex(&m_Lock);
    }

    __drv_requiresIRQL(APC_LEVEL) 
    //__drv_restoresIRQLGlobal(FastMutexObject, this->m_Lock)
    _Releases_lock_(this->m_Lock)
    __inline
    VOID
    Release()
    {
        ASSERT_DBGFLAG_INITIALIZED;

        ExReleaseFastMutex(&m_Lock);
    }

};

#endif //_MXPAGEDLOCK_H_