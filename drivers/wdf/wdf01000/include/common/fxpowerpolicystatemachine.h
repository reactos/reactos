#ifndef _FXPOWERPOLICYSTATEMACHINE_H_
#define _FXPOWERPOLICYSTATEMACHINE_H_


#include "common/fxeventqueue.h"
#include "common/fxpoxinterface.h"


struct FxPowerPolicyOwnerSettings : public FxStump {

    FxPoxInterface m_PoxInterface;

public:
    FxPowerPolicyOwnerSettings(
        __in FxPkgPnp* PkgPnp
        );

    ~FxPowerPolicyOwnerSettings(
        VOID
        );

public:
    VOID
    CleanupPowerCallback(
        VOID
        );

protected:
    PCALLBACK_OBJECT m_PowerCallbackObject;

    PVOID m_PowerCallbackRegistration;

    LONG m_WaitWakeCancelCompletionOwnership;
};

struct FxPowerPolicyMachine : public FxThreadedEventQueue {

public:
    FxPowerPolicyOwnerSettings* m_Owner;
};

#endif //_FXPOWERPOLICYSTATEMACHINE_H_
