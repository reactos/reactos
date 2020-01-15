#include "common/fxpowerpolicystatemachine.h"

VOID
FxPowerPolicyOwnerSettings::CleanupPowerCallback(
    VOID
    )
/*++

Routine Description:
    Cleans up the power state callback registration for this object.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_PowerCallbackRegistration != NULL)
    {
        Mx::UnregisterCallback(m_PowerCallbackRegistration);
        m_PowerCallbackRegistration = NULL;
    }

    if (m_PowerCallbackObject != NULL)
    {
        Mx::MxDereferenceObject(m_PowerCallbackObject);
        m_PowerCallbackObject = NULL;
    }
}
