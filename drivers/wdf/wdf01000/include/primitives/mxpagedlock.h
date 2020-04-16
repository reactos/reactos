#ifndef _MXPAGEDLOCK_H_
#define _MXPAGEDLOCK_H_

#include "common/dbgmacros.h"

typedef FAST_MUTEX MdPagedLock;

class MxPagedLockNoDynam
{
protected:
    MdPagedLock m_Lock;

public:

    _Must_inspect_result_
    __inline
    NTSTATUS
    Initialize(
        )
    {
        ExInitializeFastMutex(&m_Lock);

        SET_DBGFLAG_INITIALIZED;
    
        return STATUS_SUCCESS;
    }

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

    __inline
    VOID
    AcquireUnsafe(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;
    
        ExAcquireFastMutexUnsafe(&m_Lock);
    }

    __inline
    VOID
    ReleaseUnsafe(
        )
    {
        ASSERT_DBGFLAG_INITIALIZED;

        ExReleaseFastMutexUnsafe(&m_Lock);
    }

};

class MxPagedLock : public MxPagedLockNoDynam
{
public:    
    __inline
    MxPagedLock()
    {
        CLEAR_DBGFLAG_INITIALIZED;

        //
        // Temporarily call initialize from c'tor
        // so that we don't have to churn all of the KMDF code
        //
    #ifndef MERGE_COMPLETE
        (VOID) MxPagedLock::Initialize();
    #endif
    }
    
    __inline
    ~MxPagedLock()
    {
        CLEAR_DBGFLAG_INITIALIZED;
    }
};

#endif //_MXPAGEDLOCK_H_