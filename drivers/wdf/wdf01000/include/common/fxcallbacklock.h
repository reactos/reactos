#ifndef _FXCALLBACKLOCK_H_
#define _FXCALLBACKLOCK_H_


#include "common/fxstump.h"
#include "common/mxgeneral.h"

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

};

#endif //_FXCALLBACKLOCK_H_