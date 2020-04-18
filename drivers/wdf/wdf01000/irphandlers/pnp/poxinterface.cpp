#include "common/fxpoxinterface.h"


FxPoxInterface::FxPoxInterface(
    __in FxPkgPnp* PkgPnp
    )
{
    m_PkgPnp = PkgPnp;
    //m_PoHandle = NULL;
    m_DevicePowerRequired = TRUE;
    m_DevicePowerRequirementMachine = NULL;
    m_CurrentIdleTimeoutHint = 0;
    m_NextIdleTimeoutHint = 0;
}

FxPoxInterface::~FxPoxInterface(
    VOID
    )
{
    if (NULL != m_DevicePowerRequirementMachine)
    {
        delete m_DevicePowerRequirementMachine;
    }
}