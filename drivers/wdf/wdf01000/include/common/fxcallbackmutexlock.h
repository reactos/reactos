#ifndef _FXCALLBACKMUTEXLOCK_H_
#define _FXCALLBACKMUTEXLOCK_H_


#include "common/fxcallbacklock.h"
#include "common/fxglobals.h"

class FxCallbackMutexLock : public FxCallbackLock {

private:
    MxPagedLock m_Lock;

public:

    FxCallbackMutexLock(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxCallbackLock(FxDriverGlobals)
    {
        m_Lock.Initialize();
    }

    virtual
    ~FxCallbackMutexLock()
    {
        if (m_Verifier)
        {
            delete m_Verifier;
        }
    }

};

#endif //_FXCALLBACKMUTEXLOCK_H_
