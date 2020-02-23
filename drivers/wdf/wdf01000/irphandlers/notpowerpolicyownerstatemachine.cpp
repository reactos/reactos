#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"


VOID
FxPkgPnp::NotPowerPolicyOwnerEnterNewState(
    __in WDF_DEVICE_POWER_POLICY_STATE NewState
    )
{
    CPNOT_POWER_POLICY_OWNER_STATE_TABLE entry;
    WDF_DEVICE_POWER_POLICY_STATE currentState, newState;
    WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA data;

    currentState = m_Device->GetDevicePowerPolicyState();
    newState = NewState;

    while (newState != WdfDevStatePwrPolNull)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering not power policy owner state "
            "%!WDF_DEVICE_POWER_POLICY_STATE! from "
            "%!WDF_DEVICE_POWER_POLICY_STATE!", m_Device->GetHandle(),
            m_Device->GetDeviceObject(), newState, currentState);

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationLeaveState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_PowerPolicyMachine.m_States.History[
            m_PowerPolicyMachine.IncrementHistoryIndex()] = (USHORT) newState;

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(newState,
                                                StateNotificationEnterState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_Device->SetDevicePowerPolicyState(newState);
        currentState = newState;

        entry = GetNotPowerPolicyOwnerTableEntry(currentState);

        //
        // Call the state handler, if there is one.
        //
        if (entry->StateFunc != NULL)
        {
            newState = entry->StateFunc(this);
        }
        else
        {
            newState = WdfDevStatePwrPolNull;
        }

        if (m_PowerPolicyStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationPostProcessState,
                                                m_Device->GetHandle(),
                                                &data);
        }
    }
}
