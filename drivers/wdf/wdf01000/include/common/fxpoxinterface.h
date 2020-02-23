#ifndef _FXPOXINTERFACE_H_
#define _FXPOXINTERFACE_H_

#include "common/fxdevicepwrreqstatemachine.h"
#include "wdf.h"

class FxPoxInterface {

public:

    public:
    FxPoxInterface(
        __in FxPkgPnp* PkgPnp
        );

    ~FxPoxInterface(
        VOID
        );

    //
    // Device power requirement state machine
    //
    FxDevicePwrRequirementMachine * m_DevicePowerRequirementMachine;

    //
    // Idle timeout hint to be provided to power framework at the next 
    // opportunity, i.e. a pending update to the idle timeout hint.
    //
    ULONG m_NextIdleTimeoutHint;  

private:
    FxPkgPnp* m_PkgPnp;

    //
    // Handle obtained upon registration with power manager
    //
    //POHANDLE m_PoHandle;

    //
    // Variable that tracks whether device power is required and the lock that
    // protects this variable
    //
    BOOLEAN m_DevicePowerRequired;
    MxLock m_DevicePowerRequiredLock;

    //
    // Idle timeout hint currently provided to power framework.
    //
    ULONG m_CurrentIdleTimeoutHint;
};

#endif //_FXPOXINTERFACE_H_
