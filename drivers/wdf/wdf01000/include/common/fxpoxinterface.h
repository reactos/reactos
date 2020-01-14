#ifndef _FXPOXINTERFACE_H_
#define _FXPOXINTERFACE_H_

#include "common/fxdevicepwrreqstatemachine.h"

class FxPoxInterface {

public:
    //
    // Device power requirement state machine
    //
    FxDevicePwrRequirementMachine * m_DevicePowerRequirementMachine;
};

#endif //_FXPOXINTERFACE_H_
